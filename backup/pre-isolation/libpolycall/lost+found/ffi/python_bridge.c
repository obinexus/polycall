/**
#include "polycall/core/ffi/python_bridge.h"

 * @file python_bridge.c
 * @brief Python language bridge implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the Python language bridge for LibPolyCall FFI, providing
 * an interface for Python code to interact with other languages through the
 * FFI system.
 */

 #include <stdlib.h>
 #include <string.h>
 #include <Python.h>  // Python C API
 
 // Define error source if not already defined
 #ifndef POLYCALL_ERROR_SOURCE_FFI
 #define POLYCALL_ERROR_SOURCE_FFI 2
 #endif
 
 // Internal structure for Python function registry
 typedef struct {
     char* name;                  // Function name in FFI system
     char* module_name;           // Python module name
     char* py_function_name;      // Python function name
     PyObject* py_module;         // Python module object (borrowed reference)
     PyObject* py_function;       // Python function object (borrowed reference)
     ffi_signature_t signature;   // Function signature
     uint32_t flags;              // Function flags
 } py_function_t;
 
 // Registry for Python functions
 typedef struct {
     py_function_t* functions;    // Array of functions
     size_t count;                // Current number of functions
     size_t capacity;             // Maximum number of functions
     pthread_mutex_t mutex;       // Thread safety mutex
 } py_function_registry_t;
 
 // Complete Python bridge structure
 struct polycall_python_bridge {
     polycall_core_context_t* core_ctx;
     polycall_ffi_context_t* ffi_ctx;
     PyThreadState* main_thread_state;
     bool owns_interpreter;       // Whether we initialized Python
     bool gil_release_enabled;    // Whether GIL release is enabled
     bool numpy_enabled;          // Whether NumPy integration is enabled
     bool pandas_enabled;         // Whether Pandas integration is enabled
     bool asyncio_enabled;        // Whether asyncio integration is enabled
     PyObject* numpy_module;      // NumPy module if enabled (borrowed reference)
     PyObject* pandas_module;     // Pandas module if enabled (borrowed reference)
     py_function_registry_t function_registry;
     void* user_data;
     
     // Language bridge interface
     language_bridge_t bridge_interface;
 };
 
 // Forward declarations for Python bridge functions
 static polycall_core_error_t py_convert_to_native(
     polycall_core_context_t* ctx,
     const ffi_value_t* src,
     void* dest,
     ffi_type_info_t* dest_type
 );
 
 static polycall_core_error_t py_convert_from_native(
     polycall_core_context_t* ctx,
     const void* src,
     ffi_type_info_t* src_type,
     ffi_value_t* dest
 );
 
 static polycall_core_error_t py_register_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     uint32_t flags
 );
 
 static polycall_core_error_t py_call_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 );
 
 static polycall_core_error_t py_acquire_memory(
     polycall_core_context_t* ctx,
     void* ptr,
     size_t size
 );
 
 static polycall_core_error_t py_release_memory(
     polycall_core_context_t* ctx,
     void* ptr
 );
 
 static polycall_core_error_t py_handle_exception(
     polycall_core_context_t* ctx,
     void* exception,
     char* message,
     size_t message_size
 );
 
 static polycall_core_error_t py_initialize(
     polycall_core_context_t* ctx
 );
 
 static void py_cleanup(
     polycall_core_context_t* ctx
 );
 
 // Initialize function registry
 static polycall_core_error_t init_function_registry(
     polycall_core_context_t* ctx,
     py_function_registry_t* registry,
     size_t capacity
 ) {
     registry->functions = polycall_core_malloc(ctx, capacity * sizeof(py_function_t));
     if (!registry->functions) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     registry->count = 0;
     registry->capacity = capacity;
     
     if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
         polycall_core_free(ctx, registry->functions);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Cleanup function registry
 static void cleanup_function_registry(
     polycall_core_context_t* ctx,
     py_function_registry_t* registry
 ) {
     if (!registry->functions) {
         return;
     }
     
     for (size_t i = 0; i < registry->count; i++) {
         py_function_t* func = &registry->functions[i];
         
         // Free strings
         if (func->name) polycall_core_free(ctx, func->name);
         if (func->module_name) polycall_core_free(ctx, func->module_name);
         if (func->py_function_name) polycall_core_free(ctx, func->py_function_name);
         
         // Clean up signature
         if (func->signature.param_types) {
             polycall_core_free(ctx, func->signature.param_types);
         }
         if (func->signature.param_type_infos) {
             polycall_core_free(ctx, func->signature.param_type_infos);
         }
         if (func->signature.param_names) {
             for (size_t j = 0; j < func->signature.param_count; j++) {
                 if (func->signature.param_names[j]) {
                     polycall_core_free(ctx, (void*)func->signature.param_names[j]);
                 }
             }
             polycall_core_free(ctx, func->signature.param_names);
         }
         if (func->signature.param_optional) {
             polycall_core_free(ctx, func->signature.param_optional);
         }
     }
     
     polycall_core_free(ctx, registry->functions);
     registry->functions = NULL;
     registry->count = 0;
     registry->capacity = 0;
     
     pthread_mutex_destroy(&registry->mutex);
 }
 
 // Find a function in the registry by name
 static py_function_t* find_function(
     py_function_registry_t* registry,
     const char* name
 ) {
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->functions[i].name, name) == 0) {
             return &registry->functions[i];
         }
     }
     return NULL;
 }
 
 // Helper to convert FFI value to Python object
 static PyObject* ffi_to_python_value(
     polycall_core_context_t* ctx,
     polycall_python_bridge_t* py_bridge,
     const ffi_value_t* ffi_value
 ) {
     if (!ffi_value) {
         Py_RETURN_NONE;
     }
     
     switch (ffi_value->type) {
         case POLYCALL_FFI_TYPE_VOID:
             Py_RETURN_NONE;
             
         case POLYCALL_FFI_TYPE_BOOL:
             return PyBool_FromLong(ffi_value->value.bool_value ? 1 : 0);
             
         case POLYCALL_FFI_TYPE_CHAR:
             // Return a single character string
             return PyUnicode_FromStringAndSize(&ffi_value->value.char_value, 1);
             
         case POLYCALL_FFI_TYPE_UINT8:
             return PyLong_FromUnsignedLong((unsigned long)ffi_value->value.uint8_value);
             
         case POLYCALL_FFI_TYPE_INT8:
             return PyLong_FromLong((long)ffi_value->value.int8_value);
             
         case POLYCALL_FFI_TYPE_UINT16:
             return PyLong_FromUnsignedLong((unsigned long)ffi_value->value.uint16_value);
             
         case POLYCALL_FFI_TYPE_INT16:
             return PyLong_FromLong((long)ffi_value->value.int16_value);
             
         case POLYCALL_FFI_TYPE_UINT32:
             return PyLong_FromUnsignedLong((unsigned long)ffi_value->value.uint32_value);
             
         case POLYCALL_FFI_TYPE_INT32:
             return PyLong_FromLong((long)ffi_value->value.int32_value);
             
         case POLYCALL_FFI_TYPE_UINT64:
             return PyLong_FromUnsignedLongLong((unsigned long long)ffi_value->value.uint64_value);
             
         case POLYCALL_FFI_TYPE_INT64:
             return PyLong_FromLongLong((long long)ffi_value->value.int64_value);
             
         case POLYCALL_FFI_TYPE_FLOAT:
             return PyFloat_FromDouble((double)ffi_value->value.float_value);
             
         case POLYCALL_FFI_TYPE_DOUBLE:
             return PyFloat_FromDouble(ffi_value->value.double_value);
             
         case POLYCALL_FFI_TYPE_STRING:
             if (ffi_value->value.string_value) {
                 return PyUnicode_FromString(ffi_value->value.string_value);
             } else {
                 Py_RETURN_NONE;
             }
             
         case POLYCALL_FFI_TYPE_POINTER:
             // Wrap pointer in PyCapsule
             return PyCapsule_New(ffi_value->value.pointer_value, "LibPolyCall.Pointer", NULL);
             
         case POLYCALL_FFI_TYPE_STRUCT:
             // Struct handling would need more elaborate conversion
             // depending on the specific struct type
             // For now, just use a simplified approach with PyCapsule
             if (ffi_value->value.struct_value) {
                 return PyCapsule_New(ffi_value->value.struct_value, "LibPolyCall.Struct", NULL);
             } else {
                 Py_RETURN_NONE;
             }
             
         case POLYCALL_FFI_TYPE_ARRAY:
             // Array handling would depend on element type and count
             // For a basic implementation, convert to Python list
             if (ffi_value->value.array_value && ffi_value->type_info) {
                 // Simple case: array of basic types
                 size_t element_count = ffi_value->type_info->details.array_info.element_count;
                 polycall_ffi_type_t element_type = ffi_value->type_info->details.array_info.element_type;
                 
                 PyObject* list = PyList_New(element_count);
                 if (!list) {
                     return NULL;  // Python exception set
                 }
                 
                 // Convert each element
                 for (size_t i = 0; i < element_count; i++) {
                     PyObject* item = NULL;
                     // This is a simplified approach - a real implementation
                     // would need more sophisticated element access based on type
                     ffi_value_t element;
                     element.type = element_type;
                     
                     // Extract element from array (simplified for example)
                     void* element_ptr = ((char*)ffi_value->value.array_value) + 
                                        (i * get_element_size(element_type));
                     
                     // Copy data based on type
                     switch (element_type) {
                         case POLYCALL_FFI_TYPE_BOOL:
                             element.value.bool_value = *((bool*)element_ptr);
                             break;
                         case POLYCALL_FFI_TYPE_INT32:
                             element.value.int32_value = *((int32_t*)element_ptr);
                             break;
                         // Handle other types as needed
                         default:
                             // For complex types, we'd need a more sophisticated approach
                             break;
                     }
                     
                     // Convert element to Python
                     item = ffi_to_python_value(ctx, py_bridge, &element);
                     if (!item) {
                         Py_DECREF(list);
                         return NULL;  // Python exception set
                     }
                     
                     PyList_SET_ITEM(list, i, item);  // Steals reference to item
                 }
                 
                 return list;
             } else {
                 Py_RETURN_NONE;
             }
             
         case POLYCALL_FFI_TYPE_CALLBACK:
             // Callback handling would require creating a Python callable
             // that forwards to the FFI callback
             // For now, just use a simplified approach with PyCapsule
             if (ffi_value->value.callback_value) {
                 return PyCapsule_New(ffi_value->value.callback_value, "LibPolyCall.Callback", NULL);
             } else {
                 Py_RETURN_NONE;
             }
             
         case POLYCALL_FFI_TYPE_OBJECT:
             // Object could be anything - for now just use PyCapsule
             if (ffi_value->value.object_value) {
                 return PyCapsule_New(ffi_value->value.object_value, "LibPolyCall.Object", NULL);
             } else {
                 Py_RETURN_NONE;
             }
             
         default:
             PyErr_Format(PyExc_TypeError, "Unsupported FFI type: %d", ffi_value->type);
             return NULL;
     }
 }
 
 // Helper to convert Python object to FFI value
 static polycall_core_error_t python_to_ffi_value(
     polycall_core_context_t* ctx,
     polycall_python_bridge_t* py_bridge,
     PyObject* py_value,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 ) {
     if (!ctx || !py_bridge || !py_value || !ffi_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize FFI value
     ffi_value->type = expected_type;
     
     // Handle None as null/void
     if (py_value == Py_None) {
         switch (expected_type) {
             case POLYCALL_FFI_TYPE_VOID:
                 // Nothing to do
                 break;
                 
             case POLYCALL_FFI_TYPE_POINTER:
             case POLYCALL_FFI_TYPE_STRING:
             case POLYCALL_FFI_TYPE_STRUCT:
             case POLYCALL_FFI_TYPE_ARRAY:
             case POLYCALL_FFI_TYPE_CALLBACK:
             case POLYCALL_FFI_TYPE_OBJECT:
                 // Set pointer to NULL
                 ffi_value->value.pointer_value = NULL;
                 break;
                 
             default:
                 // None for other types is an error
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Cannot convert None to FFI type %d", expected_type);
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
         }
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Convert based on expected type
     switch (expected_type) {
         case POLYCALL_FFI_TYPE_BOOL:
             if (PyBool_Check(py_value)) {
                 ffi_value->value.bool_value = (py_value == Py_True);
             } else {
                 // Any object can be used as boolean in Python
                 ffi_value->value.bool_value = PyObject_IsTrue(py_value) != 0;
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Cannot convert Python object to boolean");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
             }
             break;
             
         case POLYCALL_FFI_TYPE_CHAR:
             if (PyUnicode_Check(py_value)) {
                 if (PyUnicode_GetLength(py_value) != 1) {
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Expected single character string");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 
                 // Get first character
                 Py_UCS4 ch = PyUnicode_ReadChar(py_value, 0);
                 if (ch > 255) {
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Character value too large for char type");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 
                 ffi_value->value.char_value = (char)ch;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected string for char type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_UINT8:
             if (PyLong_Check(py_value)) {
                 unsigned long val = PyLong_AsUnsignedLong(py_value);
                 if (PyErr_Occurred() || val > UINT8_MAX) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for uint8");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.uint8_value = (uint8_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for uint8 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_INT8:
             if (PyLong_Check(py_value)) {
                 long val = PyLong_AsLong(py_value);
                 if (PyErr_Occurred() || val < INT8_MIN || val > INT8_MAX) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for int8");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.int8_value = (int8_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for int8 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_UINT16:
             if (PyLong_Check(py_value)) {
                 unsigned long val = PyLong_AsUnsignedLong(py_value);
                 if (PyErr_Occurred() || val > UINT16_MAX) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for uint16");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.uint16_value = (uint16_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for uint16 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_INT16:
             if (PyLong_Check(py_value)) {
                 long val = PyLong_AsLong(py_value);
                 if (PyErr_Occurred() || val < INT16_MIN || val > INT16_MAX) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for int16");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.int16_value = (int16_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for int16 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_UINT32:
             if (PyLong_Check(py_value)) {
                 unsigned long val = PyLong_AsUnsignedLong(py_value);
                 if (PyErr_Occurred() || val > UINT32_MAX) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for uint32");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.uint32_value = (uint32_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for uint32 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_INT32:
             if (PyLong_Check(py_value)) {
                 long val = PyLong_AsLong(py_value);
                 if (PyErr_Occurred() || val < INT32_MIN || val > INT32_MAX) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for int32");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.int32_value = (int32_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for int32 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_UINT64:
             if (PyLong_Check(py_value)) {
                 unsigned long long val = PyLong_AsUnsignedLongLong(py_value);
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for uint64");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.uint64_value = (uint64_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for uint64 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_INT64:
             if (PyLong_Check(py_value)) {
                 long long val = PyLong_AsLongLong(py_value);
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Value out of range for int64");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.int64_value = (int64_t)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected integer for int64 type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_FLOAT:
             if (PyFloat_Check(py_value)) {
                 double val = PyFloat_AsDouble(py_value);
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error converting to float");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.float_value = (float)val;
             } else if (PyLong_Check(py_value)) {
                 // Allow integer to float conversion
                 long val = PyLong_AsLong(py_value);
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error converting integer to float");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.float_value = (float)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected float for float type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_DOUBLE:
             if (PyFloat_Check(py_value)) {
                 double val = PyFloat_AsDouble(py_value);
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error converting to double");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.double_value = val;
             } else if (PyLong_Check(py_value)) {
                 // Allow integer to double conversion
                 long val = PyLong_AsLong(py_value);
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error converting integer to double");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 ffi_value->value.double_value = (double)val;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected float for double type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_STRING:
             if (PyUnicode_Check(py_value)) {
                 const char* str = PyUnicode_AsUTF8(py_value);
                 if (!str) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error converting to string");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
                 
                 // Make a copy of the string
                 char* str_copy = polycall_core_malloc(ctx, strlen(str) + 1);
                 if (!str_copy) {
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Out of memory for string copy");
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 strcpy(str_copy, str);
                 ffi_value->value.string_value = str_copy;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected string for string type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_POINTER:
             if (PyCapsule_CheckExact(py_value)) {
                 ffi_value->value.pointer_value = PyCapsule_GetPointer(py_value, "LibPolyCall.Pointer");
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error extracting pointer from capsule");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected capsule for pointer type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
             if (PyCapsule_CheckExact(py_value)) {
                 ffi_value->value.struct_value = PyCapsule_GetPointer(py_value, "LibPolyCall.Struct");
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error extracting struct from capsule");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
             } else if (PyDict_Check(py_value)) {
                 // Convert dictionary to struct
                 // This would require more complex handling with struct field info
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Dict to struct conversion not fully implemented");
                 return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected capsule or dict for struct type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_ARRAY:
             if (PyList_Check(py_value) || PyTuple_Check(py_value)) {
                 // Convert list/tuple to array
                 // This would require more complex handling with array element info
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "List/tuple to array conversion not fully implemented");
                 return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             } else if (PyCapsule_CheckExact(py_value)) {
                 ffi_value->value.array_value = PyCapsule_GetPointer(py_value, "LibPolyCall.Array");
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error extracting array from capsule");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected list, tuple, or capsule for array type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_CALLBACK:
             if (PyCallable_Check(py_value)) {
                 // Convert callable to callback
                 // This would require complex callback wrapper creation
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Callable to callback conversion not fully implemented");
                 return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             } else if (PyCapsule_CheckExact(py_value)) {
                 ffi_value->value.callback_value = PyCapsule_GetPointer(py_value, "LibPolyCall.Callback");
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error extracting callback from capsule");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
             } else {
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Expected callable or capsule for callback type");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             break;
             
         case POLYCALL_FFI_TYPE_OBJECT:
             if (PyCapsule_CheckExact(py_value)) {
                 ffi_value->value.object_value = PyCapsule_GetPointer(py_value, "LibPolyCall.Object");
                 if (PyErr_Occurred()) {
                     PyErr_Clear();
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Error extracting object from capsule");
                     return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
                 }
             } else {
                 // For general objects, we could create a wrapper that holds a Python object
                 // with proper reference counting
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                   POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                   POLYCALL_ERROR_SEVERITY_ERROR,
                                   "Object conversion not fully implemented");
                 return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             }
             break;
             
         default:
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Unsupported FFI type: %d", expected_type);
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Helper to get element size for array element access
 static size_t get_element_size(polycall_ffi_type_t type) {
     switch (type) {
         case POLYCALL_FFI_TYPE_BOOL: return sizeof(bool);
         case POLYCALL_FFI_TYPE_CHAR: return sizeof(char);
         case POLYCALL_FFI_TYPE_UINT8: return sizeof(uint8_t);
         case POLYCALL_FFI_TYPE_INT8: return sizeof(int8_t);
         case POLYCALL_FFI_TYPE_UINT16: return sizeof(uint16_t);
         case POLYCALL_FFI_TYPE_INT16: return sizeof(int16_t);
         case POLYCALL_FFI_TYPE_UINT32: return sizeof(uint32_t);
         case POLYCALL_FFI_TYPE_INT32: return sizeof(int32_t);
         case POLYCALL_FFI_TYPE_UINT64: return sizeof(uint64_t);
         case POLYCALL_FFI_TYPE_INT64: return sizeof(int64_t);
         case POLYCALL_FFI_TYPE_FLOAT: return sizeof(float);
         case POLYCALL_FFI_TYPE_DOUBLE: return sizeof(double);
         case POLYCALL_FFI_TYPE_STRING: return sizeof(char*);
         case POLYCALL_FFI_TYPE_POINTER: return sizeof(void*);
         case POLYCALL_FFI_TYPE_STRUCT: return sizeof(void*); // Just the pointer size, not the struct size
         case POLYCALL_FFI_TYPE_ARRAY: return sizeof(void*);
         case POLYCALL_FFI_TYPE_CALLBACK: return sizeof(void*);
         case POLYCALL_FFI_TYPE_OBJECT: return sizeof(void*);
         default: return sizeof(void*); // Default to pointer size
     }
 }
 
 // Implementation of Python bridge API functions
 
 /**
  * @brief Initialize the Python language bridge
  */
 polycall_core_error_t polycall_python_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t** python_bridge,
     const polycall_python_bridge_config_t* config
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate bridge structure
     polycall_python_bridge_t* new_bridge = polycall_core_malloc(ctx, sizeof(polycall_python_bridge_t));
     if (!new_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to allocate Python bridge");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize basic state
     memset(new_bridge, 0, sizeof(polycall_python_bridge_t));
     new_bridge->core_ctx = ctx;
     new_bridge->ffi_ctx = ffi_ctx;
     new_bridge->user_data = config->user_data;
     new_bridge->gil_release_enabled = config->enable_gil_release;
     new_bridge->numpy_enabled = config->enable_numpy;
     new_bridge->pandas_enabled = config->enable_pandas;
     new_bridge->asyncio_enabled = config->enable_asyncio;
     
     // Initialize function registry
     polycall_core_error_t result = init_function_registry(
         ctx, &new_bridge->function_registry, 64);  // Default to 64 functions
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, new_bridge);
         return result;
     }
     
     // Initialize Python interpreter if needed
     if (config->initialize_python && !Py_IsInitialized()) {
         Py_Initialize();
         PyEval_InitThreads();  // Initialize threading
         new_bridge->owns_interpreter = true;
     }
     
     // Store main thread state for GIL handling
     new_bridge->main_thread_state = PyThreadState_Get();
     
     // If not initialized by us and python_handle is provided, use it
     if (config->python_handle) {
         // We could use the provided Python instance here
         // This is typically an advanced use case
     }
     
     // Set up module path if provided
     if (config->module_path) {
         PyObject* sys_path = PySys_GetObject("path");  // Borrowed reference
         if (sys_path && PyList_Check(sys_path)) {
             PyObject* path_str = PyUnicode_FromString(config->module_path);
             if (path_str) {
                 PyList_Append(sys_path, path_str);
                 Py_DECREF(path_str);
             } else {
                 PyErr_Clear();
             }
         }
     }
     
     // Import NumPy if enabled
     if (config->enable_numpy) {
         PyObject* numpy = PyImport_ImportModule("numpy");
         if (numpy) {
             new_bridge->numpy_module = numpy;  // Store reference
             new_bridge->numpy_enabled = true;
         } else {
             PyErr_Clear();
             new_bridge->numpy_enabled = false;
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                               POLYCALL_ERROR_SEVERITY_WARNING,
                               "NumPy requested but not available");
         }
     }
     
     // Import Pandas if enabled
     if (config->enable_pandas) {
         PyObject* pandas = PyImport_ImportModule("pandas");
         if (pandas) {
             new_bridge->pandas_module = pandas;  // Store reference
             new_bridge->pandas_enabled = true;
         } else {
             PyErr_Clear();
             new_bridge->pandas_enabled = false;
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                               POLYCALL_ERROR_SEVERITY_WARNING,
                               "Pandas requested but not available");
         }
     }
     
     // Set up bridge interface
     new_bridge->bridge_interface.language_name = "python";
     new_bridge->bridge_interface.version = "1.0.0";
     new_bridge->bridge_interface.convert_to_native = py_convert_to_native;
     new_bridge->bridge_interface.convert_from_native = py_convert_from_native;
     new_bridge->bridge_interface.register_function = py_register_function;
     new_bridge->bridge_interface.call_function = py_call_function;
     new_bridge->bridge_interface.acquire_memory = py_acquire_memory;
     new_bridge->bridge_interface.release_memory = py_release_memory;
     new_bridge->bridge_interface.handle_exception = py_handle_exception;
     new_bridge->bridge_interface.initialize = py_initialize;
     new_bridge->bridge_interface.cleanup = py_cleanup;
     new_bridge->bridge_interface.user_data = new_bridge;
     
     *python_bridge = new_bridge;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up Python language bridge
  */
 void polycall_python_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge
 ) {
     if (!ctx || !python_bridge) {
         return;
     }
     
     // Clean up function registry
     cleanup_function_registry(ctx, &python_bridge->function_registry);
     
     // Release NumPy and Pandas module references
     if (python_bridge->numpy_module) {
         Py_DECREF(python_bridge->numpy_module);
     }
     
     if (python_bridge->pandas_module) {
         Py_DECREF(python_bridge->pandas_module);
     }
     
     // If we own the Python interpreter, finalize it
     if (python_bridge->owns_interpreter) {
         Py_Finalize();
     }
     
     // Free the bridge structure
     polycall_core_free(ctx, python_bridge);
 }
 
 /**
  * @brief Register a Python function with the FFI system
  */
 polycall_core_error_t polycall_python_bridge_register_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* function_name,
     const char* module_name,
     const char* py_function_name,
     ffi_signature_t* signature,
     uint32_t flags
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !function_name || !module_name ||
         !py_function_name || !signature) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock function registry
     pthread_mutex_lock(&python_bridge->function_registry.mutex);
     
     // Check if function already exists
     if (find_function(&python_bridge->function_registry, function_name)) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING,
                           "Function %s already registered", function_name);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (python_bridge->function_registry.count >= python_bridge->function_registry.capacity) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Function registry full");
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Import module and get function
     PyObject* module = PyImport_ImportModule(module_name);
     if (!module) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         PyErr_Clear();
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to import module '%s'", module_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     PyObject* py_func = PyObject_GetAttrString(module, py_function_name);
     if (!py_func || !PyCallable_Check(py_func)) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         Py_DECREF(module);
         if (py_func) Py_DECREF(py_func);
         PyErr_Clear();
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Function '%s' not found in module '%s' or not callable",
                           py_function_name, module_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate and initialize function entry
     py_function_t* func = &python_bridge->function_registry.functions[python_bridge->function_registry.count];
     
     // Deep copy function name
     func->name = polycall_core_malloc(ctx, strlen(function_name) + 1);
     if (!func->name) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         Py_DECREF(module);
         Py_DECREF(py_func);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Out of memory for function name");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     strcpy(func->name, function_name);
     
     // Deep copy module name
     func->module_name = polycall_core_malloc(ctx, strlen(module_name) + 1);
     if (!func->module_name) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         polycall_core_free(ctx, func->name);
         Py_DECREF(module);
         Py_DECREF(py_func);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Out of memory for module name");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     strcpy(func->module_name, module_name);
     
     // Deep copy Python function name
     func->py_function_name = polycall_core_malloc(ctx, strlen(py_function_name) + 1);
     if (!func->py_function_name) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         polycall_core_free(ctx, func->module_name);
         polycall_core_free(ctx, func->name);
         Py_DECREF(module);
         Py_DECREF(py_func);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Out of memory for Python function name");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     strcpy(func->py_function_name, py_function_name);
     
     // Store module and function objects (new references)
     func->py_module = module;  // No need to increment ref, PyImport_ImportModule returns a new reference
     func->py_function = py_func;  // No need to increment ref, PyObject_GetAttrString returns a new reference
     
     // Initialize signature (simplified - would need a deeper copy in a real implementation)
     func->signature = *signature;
     func->flags = flags;
     
     // Increment function count
     python_bridge->function_registry.count++;
     
     pthread_mutex_unlock(&python_bridge->function_registry.mutex);
     
     // Register the function with the FFI system
     polycall_core_error_t result = polycall_ffi_expose_function(
         ctx, ffi_ctx, function_name, py_func, signature, "python", flags);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to expose function %s to FFI system", function_name);
         
         // Cleanup and rollback registration
         pthread_mutex_lock(&python_bridge->function_registry.mutex);
         python_bridge->function_registry.count--;
         polycall_core_free(ctx, func->py_function_name);
         polycall_core_free(ctx, func->module_name);
         polycall_core_free(ctx, func->name);
         Py_DECREF(module);
         Py_DECREF(py_func);
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Call a Python function through the FFI system
  */
 polycall_core_error_t polycall_python_bridge_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !function_name || 
         (!args && arg_count > 0) || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock function registry
     pthread_mutex_lock(&python_bridge->function_registry.mutex);
     
     // Find function
     py_function_t* func = find_function(&python_bridge->function_registry, function_name);
     if (!func) {
         pthread_mutex_unlock(&python_bridge->function_registry.mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Function %s not found", function_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get module and function (borrowed references from registry)
     PyObject* module = func->py_module;
     PyObject* py_func = func->py_function;
     
     // Unlock registry before making the call
     pthread_mutex_unlock(&python_bridge->function_registry.mutex);
     
     // Ensure we have the GIL
     PyGILState_STATE gil_state = PyGILState_Ensure();
     
     // Create Python arguments tuple
     PyObject* args_tuple = PyTuple_New(arg_count);
     if (!args_tuple) {
         PyGILState_Release(gil_state);
         PyErr_Clear();
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create Python arguments tuple");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Convert FFI values to Python values
     for (size_t i = 0; i < arg_count; i++) {
         PyObject* py_arg = ffi_to_python_value(ctx, python_bridge, &args[i]);
         if (!py_arg) {
             // Error during conversion
             Py_DECREF(args_tuple);
             PyGILState_Release(gil_state);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to convert argument %zu to Python value", i);
             return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
         }
         
         PyTuple_SET_ITEM(args_tuple, i, py_arg);  // Steals reference to py_arg
     }
     
     // Call Python function
     PyObject* py_result = PyObject_CallObject(py_func, args_tuple);
     Py_DECREF(args_tuple);
     
     // Check for Python exceptions
     if (!py_result) {
         // Get exception information
         PyObject* type, *value, *traceback;
         PyErr_Fetch(&type, &value, &traceback);
         
         const char* error_msg = "Unknown Python error";
         
         if (value) {
             PyObject* str_value = PyObject_Str(value);
             if (str_value) {
                 error_msg = PyUnicode_AsUTF8(str_value);
                 // We don't need to keep a reference to error_msg as it's owned by str_value
                 Py_DECREF(str_value);
             }
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_EXECUTION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Python exception: %s", error_msg);
         
         Py_XDECREF(type);
         Py_XDECREF(value);
         Py_XDECREF(traceback);
         
         PyGILState_Release(gil_state);
         return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
     }
     
     // Convert Python result to FFI value
     polycall_core_error_t conv_result = python_to_ffi_value(
         ctx, python_bridge, py_result, 
         func->signature.return_type, result);
     
     Py_DECREF(py_result);
     
     PyGILState_Release(gil_state);
     
     if (conv_result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           conv_result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to convert Python result to FFI value");
         return conv_result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert FFI value to Python value
  */
 polycall_core_error_t polycall_python_bridge_to_python_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const ffi_value_t* ffi_value,
     void** py_value
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !ffi_value || !py_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure we have the GIL
     PyGILState_STATE gil_state = PyGILState_Ensure();
     
     // Convert FFI value to Python value
     PyObject* result = ffi_to_python_value(ctx, python_bridge, ffi_value);
     
     PyGILState_Release(gil_state);
     
     if (!result) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to convert FFI value to Python value");
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     *py_value = result;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert Python value to FFI value
  */
 polycall_core_error_t polycall_python_bridge_from_python_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     void* py_value,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !py_value || !ffi_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure we have the GIL
     PyGILState_STATE gil_state = PyGILState_Ensure();
     
     // Convert Python value to FFI value
     polycall_core_error_t result = python_to_ffi_value(
         ctx, python_bridge, (PyObject*)py_value, expected_type, ffi_value);
     
     PyGILState_Release(gil_state);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to convert Python value to FFI value");
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Execute Python code string
  */
 polycall_core_error_t polycall_python_bridge_exec_code(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* code,
     const char* module_name,
     ffi_value_t* result
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !code) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure we have the GIL
     PyGILState_STATE gil_state = PyGILState_Ensure();
     
     // Create a module dictionary for execution context
     PyObject* globals = PyDict_New();
     if (!globals) {
         PyGILState_Release(gil_state);
         PyErr_Clear();
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create globals dictionary");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Add builtins to globals
     PyObject* builtins = PyEval_GetBuiltins();
     PyDict_SetItemString(globals, "__builtins__", builtins);
     
     // Set module name if provided
     if (module_name) {
         PyObject* py_name = PyUnicode_FromString(module_name);
         if (py_name) {
             PyDict_SetItemString(globals, "__name__", py_name);
             Py_DECREF(py_name);
         }
     }
     
     // Execute code
     PyObject* py_result = PyRun_String(code, Py_file_input, globals, globals);
     
     if (!py_result) {
         // Get exception information
         PyObject* type, *value, *traceback;
         PyErr_Fetch(&type, &value, &traceback);
         
         const char* error_msg = "Unknown Python error";
         
         if (value) {
             PyObject* str_value = PyObject_Str(value);
             if (str_value) {
                 error_msg = PyUnicode_AsUTF8(str_value);
                 Py_DECREF(str_value);
             }
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_EXECUTION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Python execution error: %s", error_msg);
         
         Py_XDECREF(type);
         Py_XDECREF(value);
         Py_XDECREF(traceback);
         Py_DECREF(globals);
         
         PyGILState_Release(gil_state);
         return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
     }
     
     // If a result pointer is provided, convert the result
     if (result) {
         // For file input mode, the result is always None
         // To get a meaningful result, you'd need to extract it from globals
         // or use a different execution mode
         result->type = POLYCALL_FFI_TYPE_VOID;
     }
     
     Py_DECREF(py_result);
     Py_DECREF(globals);
     
     PyGILState_Release(gil_state);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Import a Python module
  */
 polycall_core_error_t polycall_python_bridge_import_module(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* module_name
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !module_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure we have the GIL
     PyGILState_STATE gil_state = PyGILState_Ensure();
     
     // Import module
     PyObject* module = PyImport_ImportModule(module_name);
     if (!module) {
         // Get exception information
         PyObject* type, *value, *traceback;
         PyErr_Fetch(&type, &value, &traceback);
         
         const char* error_msg = "Unknown import error";
         
         if (value) {
             PyObject* str_value = PyObject_Str(value);
             if (str_value) {
                 error_msg = PyUnicode_AsUTF8(str_value);
                 Py_DECREF(str_value);
             }
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to import module '%s': %s", module_name, error_msg);
         
         Py_XDECREF(type);
         Py_XDECREF(value);
         Py_XDECREF(traceback);
         
         PyGILState_Release(gil_state);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // We don't need to keep the module reference
     Py_DECREF(module);
     
     PyGILState_Release(gil_state);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Handle Python exception
  */
 polycall_core_error_t polycall_python_bridge_handle_exception(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     char* error_message,
     size_t message_size
 ) {
     if (!ctx || !ffi_ctx || !python_bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure we have the GIL
     PyGILState_STATE gil_state = PyGILState_Ensure();
     
     // Check if there is an exception
     if (!PyErr_Occurred()) {
         if (error_message && message_size > 0) {
             strncpy(error_message, "No Python exception", message_size - 1);
             error_message[message_size - 1] = '\0';
         }
         PyGILState_Release(gil_state);
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Get exception information
     PyObject* type, *value, *traceback;
     PyErr_Fetch(&type, &value, &traceback);
     
     const char* error_msg = "Unknown Python error";
     
     if (value) {
         PyObject* str_value = PyObject_Str(value);
         if (str_value) {
             error_msg = PyUnicode_AsUTF8(str_value);
             
             if (error_message && message_size > 0) {
                 strncpy(error_message, error_msg, message_size - 1);
                 error_message[message_size - 1] = '\0';
             }
             
             Py_DECREF(str_value);
         }
     }
     
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                       POLYCALL_CORE_ERROR_EXECUTION_FAILED,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Python exception: %s", error_msg);
     
     Py_XDECREF(type);
     Py_XDECREF(value);
     Py_XDECREF(traceback);
     
     PyGILState_Release(gil_state);
     return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
 }
 
 /**
  * @brief Get Python version information
  */
 polycall_core_error_t polycall_python_bridge_get_version(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     polycall_python_version_t* version
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !version) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get Python version
     version->major = PY_MAJOR_VERSION;
     version->minor = PY_MINOR_VERSION;
     version->patch = PY_MICRO_VERSION;
     
     // Check if compatible with this bridge implementation
     // This bridge is implemented for Python 3.6+
     version->is_compatible = 
         (version->major > 3) || 
         (version->major == 3 && version->minor >= 6);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Acquire/release the Python GIL
  */
 polycall_core_error_t polycall_python_bridge_gil_control(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     bool acquire
 ) {
     if (!ctx || !ffi_ctx || !python_bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Only allow GIL release if enabled
     if (!python_bridge->gil_release_enabled && !acquire) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_WARNING,
                           "GIL release is not enabled");
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     if (acquire) {
         // Acquire GIL
         PyEval_RestoreThread(python_bridge->main_thread_state);
     } else {
         // Release GIL
         python_bridge->main_thread_state = PyEval_SaveThread();
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get language bridge interface for Python
  */
 polycall_core_error_t polycall_python_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     language_bridge_t* bridge
 ) {
     if (!ctx || !ffi_ctx || !python_bridge || !bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy bridge interface
     *bridge = python_bridge->bridge_interface;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a default Python bridge configuration
  */
 polycall_python_bridge_config_t polycall_python_bridge_create_default_config(void) {
     polycall_python_bridge_config_t config;
     
     config.python_handle = NULL;
     config.initialize_python = true;
     config.enable_numpy = false;
     config.enable_pandas = false;
     config.enable_asyncio = false;
     config.enable_gil_release = true;
     config.module_path = NULL;
     config.user_data = NULL;
     
     return config;
 }
 
 // Language bridge interface implementations
 
 static polycall_core_error_t py_convert_to_native(
     polycall_core_context_t* ctx,
     const ffi_value_t* src,
     void* dest,
     ffi_type_info_t* dest_type
 ) {
     if (!ctx || !src || !dest || !dest_type) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract Python bridge from context
     polycall_python_bridge_t* py_bridge = (polycall_python_bridge_t*)ctx->user_data;
     if (!py_bridge) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // This is a simplified implementation that would need to be expanded
     // for a full-featured Python bridge
     
     // Ensure types are compatible
     if (src->type != dest_type->type) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Type mismatch: source=%d, dest=%d",
                           src->type, dest_type->type);
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     // Copy value based on type
     switch (src->type) {
         case POLYCALL_FFI_TYPE_BOOL:
             *((bool*)dest) = src->value.bool_value;
             break;
         
         case POLYCALL_FFI_TYPE_CHAR:
             *((char*)dest) = src->value.char_value;
             break;
         
         case POLYCALL_FFI_TYPE_UINT8:
             *((uint8_t*)dest) = src->value.uint8_value;
             break;
         
         case POLYCALL_FFI_TYPE_INT8:
             *((int8_t*)dest) = src->value.int8_value;
             break;
         
         case POLYCALL_FFI_TYPE_UINT16:
             *((uint16_t*)dest) = src->value.uint16_value;
             break;
         
         case POLYCALL_FFI_TYPE_INT16:
             *((int16_t*)dest) = src->value.int16_value;
             break;
         
         case POLYCALL_FFI_TYPE_UINT32:
             *((uint32_t*)dest) = src->value.uint32_value;
             break;
         
         case POLYCALL_FFI_TYPE_INT32:
             *((int32_t*)dest) = src->value.int32_value;
             break;
         
         case POLYCALL_FFI_TYPE_UINT64:
             *((uint64_t*)dest) = src->value.uint64_value;
             break;
         
         case POLYCALL_FFI_TYPE_INT64:
             *((int64_t*)dest) = src->value.int64_value;
             break;
         
         case POLYCALL_FFI_TYPE_FLOAT:
             *((float*)dest) = src->value.float_value;
             break;
         
         case POLYCALL_FFI_TYPE_DOUBLE:
             *((double*)dest) = src->value.double_value;
             break;
         
         case POLYCALL_FFI_TYPE_STRING:
             *((const char**)dest) = src->value.string_value;
             break;
         
         case POLYCALL_FFI_TYPE_POINTER:
             *((void**)dest) = src->value.pointer_value;
             break;
         
         case POLYCALL_FFI_TYPE_STRUCT:
             // For structs, would need to copy field by field
             // For simplicity, just copy the pointer
             *((void**)dest) = src->value.struct_value;
             break;
         
         case POLYCALL_FFI_TYPE_ARRAY:
             // For arrays, would need to copy element by element
             // For simplicity, just copy the pointer
             *((void**)dest) = src->value.array_value;
             break;
         
         case POLYCALL_FFI_TYPE_CALLBACK:
             *((void**)dest) = src->value.callback_value;
             break;
         
         case POLYCALL_FFI_TYPE_OBJECT:
             *((void**)dest) = src->value.object_value;
             break;
         
         default:
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Unsupported type: %d", src->type);
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t py_convert_from_native(
     polycall_core_context_t* ctx,
     const void* src,
     ffi_type_info_t* src_type,
     ffi_value_t* dest
 ) {
     if (!ctx || !src || !src_type || !dest) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract Python bridge from context
     polycall_python_bridge_t* py_bridge = (polycall_python_bridge_t*)ctx->user_data;
     if (!py_bridge) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Initialize destination
     dest->type = src_type->type;
     dest->type_info = src_type;
     
     // Copy value based on type
     switch (src_type->type) {
         case POLYCALL_FFI_TYPE_BOOL:
             dest->value.bool_value = *((const bool*)src);
             break;
         
         case POLYCALL_FFI_TYPE_CHAR:
             dest->value.char_value = *((const char*)src);
             break;
         
         case POLYCALL_FFI_TYPE_UINT8:
             dest->value.uint8_value = *((const uint8_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_INT8:
             dest->value.int8_value = *((const int8_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_UINT16:
             dest->value.uint16_value = *((const uint16_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_INT16:
             dest->value.int16_value = *((const int16_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_UINT32:
             dest->value.uint32_value = *((const uint32_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_INT32:
             dest->value.int32_value = *((const int32_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_UINT64:
             dest->value.uint64_value = *((const uint64_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_INT64:
             dest->value.int64_value = *((const int64_t*)src);
             break;
         
         case POLYCALL_FFI_TYPE_FLOAT:
             dest->value.float_value = *((const float*)src);
             break;
         
         case POLYCALL_FFI_TYPE_DOUBLE:
             dest->value.double_value = *((const double*)src);
             break;
         
         case POLYCALL_FFI_TYPE_STRING:
             dest->value.string_value = *((const char* const*)src);
             break;
         
         case POLYCALL_FFI_TYPE_POINTER:
             dest->value.pointer_value = *((void* const*)src);
             break;
         
         case POLYCALL_FFI_TYPE_STRUCT:
             dest->value.struct_value = *((void* const*)src);
             break;
         
         case POLYCALL_FFI_TYPE_ARRAY:
             dest->value.array_value = *((void* const*)src);
             break;
         
         case POLYCALL_FFI_TYPE_CALLBACK:
             dest->value.callback_value = *((void* const*)src);
             break;
         
         case POLYCALL_FFI_TYPE_OBJECT:
             dest->value.object_value = *((void* const*)src);
             break;
         
         default:
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Unsupported type: %d", src_type->type);
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t py_register_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     uint32_t flags
 ) {
     // This function is called by the FFI core when a function is registered
     // We don't need to do anything here because the Python bridge performs 
     // its registration directly with the FFI core
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t py_call_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     if (!ctx || !function_name || (!args && arg_count > 0) || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract Python bridge from context
     polycall_python_bridge_t* py_bridge = (polycall_python_bridge_t*)ctx->user_data;
     if (!py_bridge) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Delegate to the bridge call function
     return polycall_python_bridge_call_function(
         ctx, py_bridge->ffi_ctx, py_bridge, 
         function_name, args, arg_count, result);
 }
 
 static polycall_core_error_t py_acquire_memory(
     polycall_core_context_t* ctx,
     void* ptr,
     size_t size
 ) {
     // This function is called when memory is shared from another language to Python
     // In a real implementation, we might want to register it with Python's memory system
     // or create a Python-specific wrapper
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t py_release_memory(
     polycall_core_context_t* ctx,
     void* ptr
 ) {
     // This function is called when memory shared from Python to another language is released
     // In a real implementation, we might want to unregister it with Python's memory system
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t py_handle_exception(
     polycall_core_context_t* ctx,
     void* exception,
     char* message,
     size_t message_size
 ) {
     if (!ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract Python bridge from context
     polycall_python_bridge_t* py_bridge = (polycall_python_bridge_t*)ctx->user_data;
     if (!py_bridge) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Delegate to the bridge exception handler
     return polycall_python_bridge_handle_exception(
         ctx, py_bridge->ffi_ctx, py_bridge, message, message_size);
 }
 
static polycall_core_error_t py_initialize(
    polycall_core_context_t* ctx
) {
    // This function is called when the Python bridge is initialized
    // We don't need to do anything here because initialization is handled
    // in polycall_python_bridge_init
    
    return POLYCALL_CORE_SUCCESS;
}