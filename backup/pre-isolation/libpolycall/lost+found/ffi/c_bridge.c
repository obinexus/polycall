/**
 * @file c_bridge.c
 * @brief C language bridge implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the C language bridge for LibPolyCall FFI, providing
 * a native interface for C code to interact with other languages through
 * the FFI system.
 */

 #include "polycall/core/ffi/c_bridge.h"
 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/ffi/type_system.h"
 #include "polycall/core/ffi/memory_bridge.h"
 #include "polycall/core/ffi/security.h"
 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>
 #include <pthread.h>
 
 // Define error sources if not already defined
 #ifndef POLYCALL_ERROR_SOURCE_FFI
 #define POLYCALL_ERROR_SOURCE_FFI 2
 #endif
 
 // Type definitions for function registry
 typedef struct {
     char* name;               // Function name
     void* function_ptr;       // Function pointer
     ffi_signature_t signature; // Function signature
     uint32_t flags;           // Function flags
 } c_function_t;
 
 typedef struct {
     c_function_t* functions;  // Array of functions
     size_t count;             // Current number of functions
     size_t capacity;          // Maximum number of functions
     pthread_mutex_t mutex;    // Thread safety mutex
 } c_function_registry_t;
 
 // Type definitions for struct registry
 typedef struct {
     char* name;               // Struct name
     ffi_type_info_t type_info; // Type information
 } c_struct_t;
 
 typedef struct {
     c_struct_t* structs;      // Array of structs
     size_t count;             // Current number of structs
     size_t capacity;          // Maximum number of structs
     pthread_mutex_t mutex;    // Thread safety mutex
 } c_struct_registry_t;
 
 // Callback information
 typedef struct {
     ffi_type_info_t type_info; // Callback type info
     void* callback_fn;        // Callback function pointer
     void* user_data;          // User data for callback
 } c_callback_t;
 
 typedef struct {
     c_callback_t* callbacks;   // Array of callbacks
     size_t count;              // Current number of callbacks
     size_t capacity;           // Maximum number of callbacks
     pthread_mutex_t mutex;     // Thread safety mutex
 } c_callback_registry_t;
 
 /**
  * @brief Complete C bridge structure
  */
 struct polycall_c_bridge {
     polycall_core_context_t* core_ctx;
     polycall_ffi_context_t* ffi_ctx;
     bool use_stdcall;
     bool enable_var_args;
     bool thread_safe;
     size_t max_function_count;
     void* user_data;
     
     // Function registry
     c_function_registry_t function_registry;
     
     // Struct registry
     c_struct_registry_t struct_registry;
     
     // Callback registry
     c_callback_registry_t callback_registry;
     
     // Language bridge interface
     language_bridge_t bridge_interface;
 };
 
 // Forward declarations for language bridge functions
 static polycall_core_error_t c_convert_to_native(
     polycall_core_context_t* ctx,
     const ffi_value_t* src,
     void* dest,
     ffi_type_info_t* dest_type
 );
 
 static polycall_core_error_t c_convert_from_native(
     polycall_core_context_t* ctx,
     const void* src,
     ffi_type_info_t* src_type,
     ffi_value_t* dest
 );
 
 static polycall_core_error_t c_register_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     uint32_t flags
 );
 
 static polycall_core_error_t c_call_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 );
 
 static polycall_core_error_t c_acquire_memory(
     polycall_core_context_t* ctx,
     void* ptr,
     size_t size
 );
 
 static polycall_core_error_t c_release_memory(
     polycall_core_context_t* ctx,
     void* ptr
 );
 
 static polycall_core_error_t c_handle_exception(
     polycall_core_context_t* ctx,
     void* exception,
     char* message,
     size_t message_size
 );
 
 static polycall_core_error_t c_initialize(
     polycall_core_context_t* ctx
 );
 
 static void c_cleanup(
     polycall_core_context_t* ctx
 );
 
 // Helper functions
 static c_function_t* find_function(c_function_registry_t* registry, const char* name) {
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->functions[i].name, name) == 0) {
             return &registry->functions[i];
         }
     }
     return NULL;
 }
 
 static c_struct_t* find_struct(c_struct_registry_t* registry, const char* name) {
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->structs[i].name, name) == 0) {
             return &registry->structs[i];
         }
     }
     return NULL;
 }
 
 // Initialize function registry
 static polycall_core_error_t init_function_registry(
     polycall_core_context_t* ctx,
     c_function_registry_t* registry,
     size_t capacity
 ) {
     registry->functions = polycall_core_malloc(ctx, capacity * sizeof(c_function_t));
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
     c_function_registry_t* registry
 ) {
     if (!registry->functions) {
         return;
     }
     
     for (size_t i = 0; i < registry->count; i++) {
         polycall_core_free(ctx, registry->functions[i].name);
         // Clean up signature
         if (registry->functions[i].signature.param_types) {
             polycall_core_free(ctx, registry->functions[i].signature.param_types);
         }
         if (registry->functions[i].signature.param_type_infos) {
             polycall_core_free(ctx, registry->functions[i].signature.param_type_infos);
         }
         if (registry->functions[i].signature.param_names) {
             for (size_t j = 0; j < registry->functions[i].signature.param_count; j++) {
                 if (registry->functions[i].signature.param_names[j]) {
                     polycall_core_free(ctx, (void*)registry->functions[i].signature.param_names[j]);
                 }
             }
             polycall_core_free(ctx, registry->functions[i].signature.param_names);
         }
         if (registry->functions[i].signature.param_optional) {
             polycall_core_free(ctx, registry->functions[i].signature.param_optional);
         }
     }
     
     polycall_core_free(ctx, registry->functions);
     registry->functions = NULL;
     registry->count = 0;
     registry->capacity = 0;
     
     pthread_mutex_destroy(&registry->mutex);
 }
 
 // Initialize struct registry
 static polycall_core_error_t init_struct_registry(
     polycall_core_context_t* ctx,
     c_struct_registry_t* registry,
     size_t capacity
 ) {
     registry->structs = polycall_core_malloc(ctx, capacity * sizeof(c_struct_t));
     if (!registry->structs) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     registry->count = 0;
     registry->capacity = capacity;
     
     if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
         polycall_core_free(ctx, registry->structs);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Cleanup struct registry
 static void cleanup_struct_registry(
     polycall_core_context_t* ctx,
     c_struct_registry_t* registry
 ) {
     if (!registry->structs) {
         return;
     }
     
     for (size_t i = 0; i < registry->count; i++) {
         polycall_core_free(ctx, registry->structs[i].name);
         // Clean up type_info if needed
         if (registry->structs[i].type_info.type == POLYCALL_FFI_TYPE_STRUCT && 
             registry->structs[i].type_info.details.struct_info.type_info) {
             polycall_core_free(ctx, registry->structs[i].type_info.details.struct_info.type_info);
         }
     }
     
     polycall_core_free(ctx, registry->structs);
     registry->structs = NULL;
     registry->count = 0;
     registry->capacity = 0;
     
     pthread_mutex_destroy(&registry->mutex);
 }
 
 // Initialize callback registry
 static polycall_core_error_t init_callback_registry(
     polycall_core_context_t* ctx,
     c_callback_registry_t* registry,
     size_t capacity
 ) {
     registry->callbacks = polycall_core_malloc(ctx, capacity * sizeof(c_callback_t));
     if (!registry->callbacks) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     registry->count = 0;
     registry->capacity = capacity;
     
     if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
         polycall_core_free(ctx, registry->callbacks);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Cleanup callback registry
 static void cleanup_callback_registry(
     polycall_core_context_t* ctx,
     c_callback_registry_t* registry
 ) {
     if (!registry->callbacks) {
         return;
     }
     
     for (size_t i = 0; i < registry->count; i++) {
         // Clean up type_info if needed
         if (registry->callbacks[i].type_info.type == POLYCALL_FFI_TYPE_CALLBACK && 
             registry->callbacks[i].type_info.details.callback_info.param_types) {
             polycall_core_free(ctx, registry->callbacks[i].type_info.details.callback_info.param_types);
         }
     }
     
     polycall_core_free(ctx, registry->callbacks);
     registry->callbacks = NULL;
     registry->count = 0;
     registry->capacity = 0;
     
     pthread_mutex_destroy(&registry->mutex);
 }
 
 // Register primitive C types with FFI system
 static polycall_core_error_t register_primitive_types(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     type_mapping_context_t* type_ctx
 ) {
     const char* language = "c";
     
     // Define primitive type mappings
     struct {
         const char* c_type;
         polycall_ffi_type_t ffi_type;
     } primitive_types[] = {
         {"void", POLYCALL_FFI_TYPE_VOID},
         {"bool", POLYCALL_FFI_TYPE_BOOL},
         {"char", POLYCALL_FFI_TYPE_CHAR},
         {"unsigned char", POLYCALL_FFI_TYPE_UINT8},
         {"uint8_t", POLYCALL_FFI_TYPE_UINT8},
         {"int8_t", POLYCALL_FFI_TYPE_INT8},
         {"signed char", POLYCALL_FFI_TYPE_INT8},
         {"unsigned short", POLYCALL_FFI_TYPE_UINT16},
         {"uint16_t", POLYCALL_FFI_TYPE_UINT16},
         {"short", POLYCALL_FFI_TYPE_INT16},
         {"int16_t", POLYCALL_FFI_TYPE_INT16},
         {"unsigned int", POLYCALL_FFI_TYPE_UINT32},
         {"uint32_t", POLYCALL_FFI_TYPE_UINT32},
         {"int", POLYCALL_FFI_TYPE_INT32},
         {"int32_t", POLYCALL_FFI_TYPE_INT32},
         {"unsigned long long", POLYCALL_FFI_TYPE_UINT64},
         {"uint64_t", POLYCALL_FFI_TYPE_UINT64},
         {"long long", POLYCALL_FFI_TYPE_INT64},
         {"int64_t", POLYCALL_FFI_TYPE_INT64},
         {"float", POLYCALL_FFI_TYPE_FLOAT},
         {"double", POLYCALL_FFI_TYPE_DOUBLE},
         {"char*", POLYCALL_FFI_TYPE_STRING},
         {"const char*", POLYCALL_FFI_TYPE_STRING},
         {"void*", POLYCALL_FFI_TYPE_POINTER}
     };
     
     // Register each primitive type
     for (size_t i = 0; i < sizeof(primitive_types) / sizeof(primitive_types[0]); i++) {
         ffi_type_info_t type_info;
         memset(&type_info, 0, sizeof(type_info));
         type_info.type = primitive_types[i].ffi_type;
         
         polycall_core_error_t result = polycall_type_register(
             ctx, ffi_ctx, type_ctx, &type_info, language);
             
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to register primitive type %s", primitive_types[i].c_type);
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Initialize the C language bridge
  */
 polycall_core_error_t polycall_c_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t** c_bridge,
     const polycall_c_bridge_config_t* config
 ) {
     if (!ctx || !ffi_ctx || !c_bridge || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     // Allocate memory for the C bridge
     *c_bridge = polycall_core_malloc(ctx, sizeof(polycall_c_bridge_t));
     if (!*c_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate C bridge");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     // Initialize bridge with configuration
     (*c_bridge)->core_ctx = ctx;
     (*c_bridge)->ffi_ctx = ffi_ctx;
     (*c_bridge)->use_stdcall = config->use_stdcall;
     (*c_bridge)->enable_var_args = config->enable_var_args;
     (*c_bridge)->thread_safe = config->thread_safe;
     (*c_bridge)->max_function_count = config->max_function_count > 0 ? 
                                      config->max_function_count : 1024;
     (*c_bridge)->user_data = config->user_data;
     
     // Initialize function registry
     polycall_core_error_t result = init_function_registry(
         ctx, &(*c_bridge)->function_registry, (*c_bridge)->max_function_count);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, *c_bridge);
         *c_bridge = NULL;
         return result;
     }
     
     // Initialize struct registry
     result = init_struct_registry(ctx, &(*c_bridge)->struct_registry, 256);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_function_registry(ctx, &(*c_bridge)->function_registry);
         polycall_core_free(ctx, *c_bridge);
         *c_bridge = NULL;
         return result;
     }
     
     // Initialize callback registry
     result = init_callback_registry(ctx, &(*c_bridge)->callback_registry, 64);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_struct_registry(ctx, &(*c_bridge)->struct_registry);
         cleanup_function_registry(ctx, &(*c_bridge)->function_registry);
         polycall_core_free(ctx, *c_bridge);
         *c_bridge = NULL;
         return result;
     }
 
     // Set up language bridge interface
     (*c_bridge)->bridge_interface.language_name = "c";
     (*c_bridge)->bridge_interface.version = "1.0.0";
     (*c_bridge)->bridge_interface.convert_to_native = c_convert_to_native;
     (*c_bridge)->bridge_interface.convert_from_native = c_convert_from_native;
     (*c_bridge)->bridge_interface.register_function = c_register_function;
     (*c_bridge)->bridge_interface.call_function = c_call_function;
     (*c_bridge)->bridge_interface.acquire_memory = c_acquire_memory;
     (*c_bridge)->bridge_interface.release_memory = c_release_memory;
     (*c_bridge)->bridge_interface.handle_exception = c_handle_exception;
     (*c_bridge)->bridge_interface.initialize = c_initialize;
     (*c_bridge)->bridge_interface.cleanup = c_cleanup;
     (*c_bridge)->bridge_interface.user_data = *c_bridge;
     
     // Get the type system from FFI context
     type_mapping_context_t* type_ctx = NULL;
     result = polycall_ffi_get_type_context(ctx, ffi_ctx, &type_ctx);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_callback_registry(ctx, &(*c_bridge)->callback_registry);
         cleanup_struct_registry(ctx, &(*c_bridge)->struct_registry);
         cleanup_function_registry(ctx, &(*c_bridge)->function_registry);
         polycall_core_free(ctx, *c_bridge);
         *c_bridge = NULL;
         return result;
     }
 
     // Register C primitive types with the type system
     result = register_primitive_types(ctx, ffi_ctx, type_ctx);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_callback_registry(ctx, &(*c_bridge)->callback_registry);
         cleanup_struct_registry(ctx, &(*c_bridge)->struct_registry);
         cleanup_function_registry(ctx, &(*c_bridge)->function_registry);
         polycall_core_free(ctx, *c_bridge);
         *c_bridge = NULL;
         return result;
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up C language bridge
  */
 void polycall_c_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge
 ) {
     if (!ctx || !ffi_ctx || !c_bridge) {
         return;
     }
 
     // Clean up callback registry
     cleanup_callback_registry(ctx, &c_bridge->callback_registry);
     
     // Clean up struct registry
     cleanup_struct_registry(ctx, &c_bridge->struct_registry);
     
     // Clean up function registry
     cleanup_function_registry(ctx, &c_bridge->function_registry);
 
     // Free the bridge structure
     polycall_core_free(ctx, c_bridge);
 }
 
 /**
  * @brief Register a C function with the FFI system
  */
 polycall_core_error_t polycall_c_bridge_register_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     const char* function_name,
     void* function_ptr,
     polycall_ffi_type_t return_type,
     polycall_ffi_type_t* param_types,
     size_t param_count,
     uint32_t flags
 ) {
     if (!ctx || !ffi_ctx || !c_bridge || !function_name || !function_ptr) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create function signature
     ffi_signature_t signature;
     memset(&signature, 0, sizeof(signature));
     signature.return_type = return_type;
     signature.param_count = param_count;
     
     if (param_count > 0 && param_types) {
         // Allocate and copy parameter types
         signature.param_types = polycall_core_malloc(ctx, param_count * sizeof(polycall_ffi_type_t));
         if (!signature.param_types) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(signature.param_types, param_types, param_count * sizeof(polycall_ffi_type_t));
         
         // Allocate parameter type infos (but don't initialize them)
         signature.param_type_infos = polycall_core_malloc(ctx, param_count * sizeof(ffi_type_info_t*));
         if (!signature.param_type_infos) {
             polycall_core_free(ctx, signature.param_types);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memset(signature.param_type_infos, 0, param_count * sizeof(ffi_type_info_t*));
         
         // Allocate parameter names (but don't initialize them)
         signature.param_names = polycall_core_malloc(ctx, param_count * sizeof(char*));
         if (!signature.param_names) {
             polycall_core_free(ctx, signature.param_type_infos);
             polycall_core_free(ctx, signature.param_types);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memset(signature.param_names, 0, param_count * sizeof(char*));
         
         // Allocate parameter optional flags
         signature.param_optional = polycall_core_malloc(ctx, param_count * sizeof(bool));
         if (!signature.param_optional) {
             polycall_core_free(ctx, signature.param_names);
             polycall_core_free(ctx, signature.param_type_infos);
             polycall_core_free(ctx, signature.param_types);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memset(signature.param_optional, 0, param_count * sizeof(bool));
     }
     
     signature.variadic = c_bridge->enable_var_args;
     
     // Lock function registry
     if (c_bridge->thread_safe) {
         pthread_mutex_lock(&c_bridge->function_registry.mutex);
     }
     
     // Check if function already exists
     if (find_function(&c_bridge->function_registry, function_name)) {
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->function_registry.mutex);
         }
         
         if (signature.param_optional) polycall_core_free(ctx, signature.param_optional);
         if (signature.param_names) polycall_core_free(ctx, signature.param_names);
         if (signature.param_type_infos) polycall_core_free(ctx, signature.param_type_infos);
         if (signature.param_types) polycall_core_free(ctx, signature.param_types);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Function %s already registered", function_name);
                           
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (c_bridge->function_registry.count >= c_bridge->function_registry.capacity) {
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->function_registry.mutex);
         }
         
         if (signature.param_optional) polycall_core_free(ctx, signature.param_optional);
         if (signature.param_names) polycall_core_free(ctx, signature.param_names);
         if (signature.param_type_infos) polycall_core_free(ctx, signature.param_type_infos);
         if (signature.param_types) polycall_core_free(ctx, signature.param_types);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Function registry full");
                           
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Add function to registry
     c_function_t* func = &c_bridge->function_registry.functions[c_bridge->function_registry.count++];
     
     // Duplicate function name
     func->name = polycall_core_malloc(ctx, strlen(function_name) + 1);
     if (!func->name) {
         c_bridge->function_registry.count--; // Revert count increase
         
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->function_registry.mutex);
         }
         
         if (signature.param_optional) polycall_core_free(ctx, signature.param_optional);
         if (signature.param_names) polycall_core_free(ctx, signature.param_names);
         if (signature.param_type_infos) polycall_core_free(ctx, signature.param_type_infos);
         if (signature.param_types) polycall_core_free(ctx, signature.param_types);
         
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strcpy(func->name, function_name);
     func->function_ptr = function_ptr;
     func->signature = signature;
     func->flags = flags;
     
     if (c_bridge->thread_safe) {
         pthread_mutex_unlock(&c_bridge->function_registry.mutex);
     }
     
     // Register function with FFI system
     polycall_core_error_t result = polycall_ffi_expose_function(
         ctx, ffi_ctx, function_name, function_ptr, &signature, "c", flags);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to expose function %s to FFI system", function_name);
     }
     
     return result;
 }
 
 /**
  * @brief Call a C function through the FFI system
  */
 polycall_core_error_t polycall_c_bridge_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     if (!ctx || !ffi_ctx || !c_bridge || !function_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock function registry
     if (c_bridge->thread_safe) {
         pthread_mutex_lock(&c_bridge->function_registry.mutex);
     }
     
     // Find function
     c_function_t* func = find_function(&c_bridge->function_registry, function_name);
     
     if (!func) {
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->function_registry.mutex);
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Argument count mismatch: expected %zu, got %zu",
                           func->signature.param_count, arg_count);
                           
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Unlock function registry before making the call
     if (c_bridge->thread_safe) {
         pthread_mutex_unlock(&c_bridge->function_registry.mutex);
     }
     
     // Prepare arguments for C function call
     // This is a simplified approach that works for basic types
     // For complex functions, we would need to use a more sophisticated approach like libffi
     
     // Allocate memory for arguments
     void** arg_ptrs = NULL;
     if (arg_count > 0) {
         arg_ptrs = polycall_core_malloc(ctx, arg_count * sizeof(void*));
         if (!arg_ptrs) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate memory for argument pointers");
                               
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Convert FFI values to native C values
         for (size_t i = 0; i < arg_count; i++) {
             // Determine size based on type
             size_t arg_size = 0;
             switch (args[i].type) {
                 case POLYCALL_FFI_TYPE_BOOL: arg_size = sizeof(bool); break;
                 case POLYCALL_FFI_TYPE_CHAR: arg_size = sizeof(char); break;
                 case POLYCALL_FFI_TYPE_UINT8: arg_size = sizeof(uint8_t); break;
                 case POLYCALL_FFI_TYPE_INT8: arg_size = sizeof(int8_t); break;
                 case POLYCALL_FFI_TYPE_UINT16: arg_size = sizeof(uint16_t); break;
                 case POLYCALL_FFI_TYPE_INT16: arg_size = sizeof(int16_t); break;
                 case POLYCALL_FFI_TYPE_UINT32: arg_size = sizeof(uint32_t); break;
                 case POLYCALL_FFI_TYPE_INT32: arg_size = sizeof(int32_t); break;
                 case POLYCALL_FFI_TYPE_UINT64: arg_size = sizeof(uint64_t); break;
                 case POLYCALL_FFI_TYPE_INT64: arg_size = sizeof(int64_t); break;
                 case POLYCALL_FFI_TYPE_FLOAT: arg_size = sizeof(float); break;
                 case POLYCALL_FFI_TYPE_DOUBLE: arg_size = sizeof(double); break;
                 case POLYCALL_FFI_TYPE_STRING: arg_size = sizeof(char*); break;
                 case POLYCALL_FFI_TYPE_POINTER: arg_size = sizeof(void*); break;
                 case POLYCALL_FFI_TYPE_STRUCT:
                     if (args[i].type_info && args[i].type_info->details.struct_info.size > 0) {
                         arg_size = args[i].type_info->details.struct_info.size;
                     } else {
                         arg_size = sizeof(void*); // Use pointer if size unknown
                     }
                     break;
                 case POLYCALL_FFI_TYPE_ARRAY: arg_size = sizeof(void*); break;
                 case POLYCALL_FFI_TYPE_CALLBACK: arg_size = sizeof(void*); break;
                 case POLYCALL_FFI_TYPE_OBJECT: arg_size = sizeof(void*); break;
                 default: arg_size = sizeof(void*); break; // Default to pointer size
             }
             
             // Allocate memory for the argument
             arg_ptrs[i] = polycall_core_malloc(ctx, arg_size);
             if (!arg_ptrs[i]) {
                 // Clean up previously allocated memory
                 for (size_t j = 0; j < i; j++) {
                     polycall_core_free(ctx, arg_ptrs[j]);
                 }
                 polycall_core_free(ctx, arg_ptrs);
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to allocate memory for argument %zu", i);
                                   
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             // Convert FFI value to native C value
             ffi_type_info_t type_info;
             memset(&type_info, 0, sizeof(type_info));
             type_info.type = args[i].type;
             
             polycall_core_error_t conv_result = c_convert_to_native(
                 ctx, &args[i], arg_ptrs[i], &type_info);
                 
             if (conv_result != POLYCALL_CORE_SUCCESS) {
                 // Clean up allocated memory
                 for (size_t j = 0; j <= i; j++) {
                     polycall_core_free(ctx, arg_ptrs[j]);
                 }
                 polycall_core_free(ctx, arg_ptrs);
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   conv_result,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to convert argument %zu to native C value", i);
                                   
                 return conv_result;
             }
         }
     }
     
     // Prepare result buffer if needed
     void* result_buffer = NULL;
     if (func->signature.return_type != POLYCALL_FFI_TYPE_VOID && result) {
         // Determine size based on return type
         size_t result_size = 0;
         switch (func->signature.return_type) {
             case POLYCALL_FFI_TYPE_BOOL: result_size = sizeof(bool); break;
             case POLYCALL_FFI_TYPE_CHAR: result_size = sizeof(char); break;
             case POLYCALL_FFI_TYPE_UINT8: result_size = sizeof(uint8_t); break;
             case POLYCALL_FFI_TYPE_INT8: result_size = sizeof(int8_t); break;
             case POLYCALL_FFI_TYPE_UINT16: result_size = sizeof(uint16_t); break;
             case POLYCALL_FFI_TYPE_INT16: result_size = sizeof(int16_t); break;
             case POLYCALL_FFI_TYPE_UINT32: result_size = sizeof(uint32_t); break;
             case POLYCALL_FFI_TYPE_INT32: result_size = sizeof(int32_t); break;
             case POLYCALL_FFI_TYPE_UINT64: result_size = sizeof(uint64_t); break;
             case POLYCALL_FFI_TYPE_INT64: result_size = sizeof(int64_t); break;
             case POLYCALL_FFI_TYPE_FLOAT: result_size = sizeof(float); break;
             case POLYCALL_FFI_TYPE_DOUBLE: result_size = sizeof(double); break;
             case POLYCALL_FFI_TYPE_STRING: result_size = sizeof(char*); break;
             case POLYCALL_FFI_TYPE_POINTER: result_size = sizeof(void*); break;
             default: result_size = sizeof(void*); break; // Default to pointer size
         }
         
         result_buffer = polycall_core_malloc(ctx, result_size);
         if (!result_buffer) {
             // Clean up argument memory
             if (arg_ptrs) {
                 for (size_t i = 0; i < arg_count; i++) {
                     polycall_core_free(ctx, arg_ptrs[i]);
                 }
                 polycall_core_free(ctx, arg_ptrs);
             }
             
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate memory for function result");
                               
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     // Call the function
     // Note: This is a simplified approach and doesn't handle all possible function signatures
     // In a real implementation, we would use a more sophisticated approach like libffi
     
     // This is a basic implementation that handles up to 4 arguments
     // For a complete implementation, we would need to use a more flexible approach
     
     typedef void (*func0)(void);
     typedef void (*func1)(void*);
     typedef void (*func2)(void*, void*);
     typedef void (*func3)(void*, void*, void*);
     typedef void (*func4)(void*, void*, void*, void*);
     
     if (func->signature.return_type == POLYCALL_FFI_TYPE_VOID) {
         // Void return type
         switch (arg_count) {
             case 0:
                 ((func0)func->function_ptr)();
                 break;
             case 1:
                 ((func1)func->function_ptr)(arg_ptrs[0]);
                 break;
             case 2:
                 ((func2)func->function_ptr)(arg_ptrs[0], arg_ptrs[1]);
                 break;
             case 3:
                 ((func3)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2]);
                 break;
             case 4:
                 ((func4)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2], arg_ptrs[3]);
                 break;
             default:
                 // Clean up allocated memory
                 if (result_buffer) {
                     polycall_core_free(ctx, result_buffer);
                 }
                 
                 if (arg_ptrs) {
                     for (size_t i = 0; i < arg_count; i++) {
                         polycall_core_free(ctx, arg_ptrs[i]);
                     }
                     polycall_core_free(ctx, arg_ptrs);
                 }
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Too many arguments: %zu", arg_count);
                                   
                 return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
         }
     } else {
         // Non-void return type
         typedef bool (*func_bool0)(void);
         typedef bool (*func_bool1)(void*);
         typedef bool (*func_bool2)(void*, void*);
         typedef bool (*func_bool3)(void*, void*, void*);
         typedef bool (*func_bool4)(void*, void*, void*, void*);
         
         typedef int (*func_int0)(void);
         typedef int (*func_int1)(void*);
         typedef int (*func_int2)(void*, void*);
         typedef int (*func_int3)(void*, void*, void*);
         typedef int (*func_int4)(void*, void*, void*, void*);
         
         typedef double (*func_double0)(void);
         typedef double (*func_double1)(void*);
         typedef double (*func_double2)(void*, void*);
         typedef double (*func_double3)(void*, void*, void*);
         typedef double (*func_double4)(void*, void*, void*, void*);
         
         typedef void* (*func_ptr0)(void);
         typedef void* (*func_ptr1)(void*);
         typedef void* (*func_ptr2)(void*, void*);
         typedef void* (*func_ptr3)(void*, void*, void*);
         typedef void* (*func_ptr4)(void*, void*, void*, void*);
         
         // Call based on return type and argument count
         // This is a simplified implementation that handles only a few common return types
         
         if (func->signature.return_type == POLYCALL_FFI_TYPE_BOOL) {
             bool result_value = false;
             
             switch (arg_count) {
                 case 0:
                     result_value = ((func_bool0)func->function_ptr)();
                     break;
                 case 1:
                     result_value = ((func_bool1)func->function_ptr)(arg_ptrs[0]);
                     break;
                 case 2:
                     result_value = ((func_bool2)func->function_ptr)(arg_ptrs[0], arg_ptrs[1]);
                     break;
                 case 3:
                     result_value = ((func_bool3)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2]);
                     break;
                 case 4:
                     result_value = ((func_bool4)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2], arg_ptrs[3]);
                     break;
                 default:
                     // Clean up allocated memory
                     if (result_buffer) {
                         polycall_core_free(ctx, result_buffer);
                     }
                     
                     if (arg_ptrs) {
                         for (size_t i = 0; i < arg_count; i++) {
                             polycall_core_free(ctx, arg_ptrs[i]);
                         }
                         polycall_core_free(ctx, arg_ptrs);
                     }
                     
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                       POLYCALL_ERROR_SEVERITY_ERROR, 
                                       "Too many arguments: %zu", arg_count);
                                       
                     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             }
             
             // Store result
             if (result_buffer) {
                 *((bool*)result_buffer) = result_value;
             }
         } else if (func->signature.return_type == POLYCALL_FFI_TYPE_INT32) {
             int result_value = 0;
             
             switch (arg_count) {
                 case 0:
                     result_value = ((func_int0)func->function_ptr)();
                     break;
                 case 1:
                     result_value = ((func_int1)func->function_ptr)(arg_ptrs[0]);
                     break;
                 case 2:
                     result_value = ((func_int2)func->function_ptr)(arg_ptrs[0], arg_ptrs[1]);
                     break;
                 case 3:
                     result_value = ((func_int3)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2]);
                     break;
                 case 4:
                     result_value = ((func_int4)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2], arg_ptrs[3]);
                     break;
                 default:
                     // Clean up allocated memory
                     if (result_buffer) {
                         polycall_core_free(ctx, result_buffer);
                     }
                     
                     if (arg_ptrs) {
                         for (size_t i = 0; i < arg_count; i++) {
                             polycall_core_free(ctx, arg_ptrs[i]);
                         }
                         polycall_core_free(ctx, arg_ptrs);
                     }
                     
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                       POLYCALL_ERROR_SEVERITY_ERROR, 
                                       "Too many arguments: %zu", arg_count);
                                       
                     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             }
             
             // Store result
             if (result_buffer) {
                 *((int*)result_buffer) = result_value;
             }
         } else if (func->signature.return_type == POLYCALL_FFI_TYPE_DOUBLE) {
             double result_value = 0.0;
             
             switch (arg_count) {
                 case 0:
                     result_value = ((func_double0)func->function_ptr)();
                     break;
                 case 1:
                     result_value = ((func_double1)func->function_ptr)(arg_ptrs[0]);
                     break;
                 case 2:
                     result_value = ((func_double2)func->function_ptr)(arg_ptrs[0], arg_ptrs[1]);
                     break;
                 case 3:
                     result_value = ((func_double3)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2]);
                     break;
                 case 4:
                     result_value = ((func_double4)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2], arg_ptrs[3]);
                     break;
                 default:
                     // Clean up allocated memory
                     if (result_buffer) {
                         polycall_core_free(ctx, result_buffer);
                     }
                     
                     if (arg_ptrs) {
                         for (size_t i = 0; i < arg_count; i++) {
                             polycall_core_free(ctx, arg_ptrs[i]);
                         }
                         polycall_core_free(ctx, arg_ptrs);
                     }
                     
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                       POLYCALL_ERROR_SEVERITY_ERROR, 
                                       "Too many arguments: %zu", arg_count);
                                       
                     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             }
             
             // Store result
             if (result_buffer) {
                 *((double*)result_buffer) = result_value;
             }
         } else if (func->signature.return_type == POLYCALL_FFI_TYPE_POINTER || 
                  func->signature.return_type == POLYCALL_FFI_TYPE_STRING ||
                  func->signature.return_type == POLYCALL_FFI_TYPE_OBJECT) {
             // Handle pointer-like return types
             void* result_value = NULL;
             
             switch (arg_count) {
                 case 0:
                     result_value = ((func_ptr0)func->function_ptr)();
                     break;
                 case 1:
                     result_value = ((func_ptr1)func->function_ptr)(arg_ptrs[0]);
                     break;
                 case 2:
                     result_value = ((func_ptr2)func->function_ptr)(arg_ptrs[0], arg_ptrs[1]);
                     break;
                 case 3:
                     result_value = ((func_ptr3)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2]);
                     break;
                 case 4:
                     result_value = ((func_ptr4)func->function_ptr)(arg_ptrs[0], arg_ptrs[1], arg_ptrs[2], arg_ptrs[3]);
                     break;
                 default:
                     // Clean up allocated memory
                     if (result_buffer) {
                         polycall_core_free(ctx, result_buffer);
                     }
                     
                     if (arg_ptrs) {
                         for (size_t i = 0; i < arg_count; i++) {
                             polycall_core_free(ctx, arg_ptrs[i]);
                         }
                         polycall_core_free(ctx, arg_ptrs);
                     }
                     
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                       POLYCALL_ERROR_SEVERITY_ERROR, 
                                       "Too many arguments: %zu", arg_count);
                                       
                     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             }
             
             // Store result
             if (result_buffer) {
                 *((void**)result_buffer) = result_value;
             }
         } else {
             // Unsupported return type
             // Clean up allocated memory
             if (result_buffer) {
                 polycall_core_free(ctx, result_buffer);
             }
             
             if (arg_ptrs) {
                 for (size_t i = 0; i < arg_count; i++) {
                     polycall_core_free(ctx, arg_ptrs[i]);
                 }
                 polycall_core_free(ctx, arg_ptrs);
             }
             
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Unsupported return type: %d", func->signature.return_type);
                               
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
         }
     }
     
     // Convert result to FFI value if needed
     if (result_buffer && result) {
         ffi_type_info_t type_info;
         memset(&type_info, 0, sizeof(type_info));
         type_info.type = func->signature.return_type;
         
         polycall_core_error_t conv_result = c_convert_from_native(
             ctx, result_buffer, &type_info, result);
             
         if (conv_result != POLYCALL_CORE_SUCCESS) {
             // Clean up allocated memory
             polycall_core_free(ctx, result_buffer);
             
             if (arg_ptrs) {
                 for (size_t i = 0; i < arg_count; i++) {
                     polycall_core_free(ctx, arg_ptrs[i]);
                 }
                 polycall_core_free(ctx, arg_ptrs);
             }
             
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               conv_result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to convert function result to FFI value");
                               
             return conv_result;
         }
     }
     
     // Clean up allocated memory
     if (result_buffer) {
         polycall_core_free(ctx, result_buffer);
     }
     
     if (arg_ptrs) {
         for (size_t i = 0; i < arg_count; i++) {
             polycall_core_free(ctx, arg_ptrs[i]);
         }
         polycall_core_free(ctx, arg_ptrs);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t c_acquire_memory(
     polycall_core_context_t* ctx,
     void* ptr,
     size_t size
 ) {
     // This function is called when memory is shared from another language to C
     // We don't need to do anything special here because C has direct access to memory
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t c_release_memory(
     polycall_core_context_t* ctx,
     void* ptr
 ) {
     // This function is called when memory shared from C to another language is released
     // We don't need to do anything special here because memory management is handled elsewhere
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t c_handle_exception(
     polycall_core_context_t* ctx,
     void* exception,
     char* message,
     size_t message_size
 ) {
     // C doesn't have exceptions in the traditional sense
     // This function would handle C signal-based errors if needed
     
     if (message && message_size > 0) {
         strncpy(message, "C error occurred", message_size - 1);
         message[message_size - 1] = '\0';
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t c_initialize(
     polycall_core_context_t* ctx
 ) {
     // This function is called when the C bridge is initialized
     // We don't need to do anything special here
     
     return POLYCALL_CORE_SUCCESS;
 }
 
static void c_cleanup(
    polycall_core_context_t* ctx
) {
    // This function is called when the C bridge is cleaned up
    // We don't need to do anything special here
}
 
 /**
  * @brief Register a struct type with the C bridge
  */
 polycall_core_error_t polycall_c_bridge_register_struct(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     const char* struct_name,
     polycall_ffi_type_t* field_types,
     const char** field_names,
     size_t* field_offsets,
     size_t field_count,
     size_t struct_size,
     size_t alignment
 ) {
     if (!ctx || !ffi_ctx || !c_bridge || !struct_name || 
         (!field_types && field_count > 0) || 
         (!field_names && field_count > 0) || 
         (!field_offsets && field_count > 0) ||
         struct_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get the type system context
     type_mapping_context_t* type_ctx = NULL;
     polycall_core_error_t result = polycall_ffi_get_type_context(ctx, ffi_ctx, &type_ctx);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get type context");
         return result;
     }
     
     // Lock struct registry
     if (c_bridge->thread_safe) {
         pthread_mutex_lock(&c_bridge->struct_registry.mutex);
     }
     
     // Check if struct already exists
     if (find_struct(&c_bridge->struct_registry, struct_name)) {
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Struct %s already registered", struct_name);
                           
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (c_bridge->struct_registry.count >= c_bridge->struct_registry.capacity) {
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Struct registry full");
                           
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
    // Add struct to registry
    c_struct_t* new_struct = &c_bridge->struct_registry.structs[c_bridge->struct_registry.count];
    
    // Declare these variables early so they can be referenced in cleanup code
    polycall_ffi_type_t* field_types_copy = NULL;
    const char** field_names_copy = NULL;
    size_t* field_offsets_copy = NULL;
    
    // 1. Create and initialize struct type information
    ffi_type_info_t type_info;
    memset(&type_info, 0, sizeof(type_info));
    type_info.type = POLYCALL_FFI_TYPE_STRUCT;
    type_info.details.struct_info.size = struct_size;
    type_info.details.struct_info.alignment = alignment;
    type_info.details.struct_info.field_count = field_count;
    
    // 2. Allocate memory for struct fields
    if (field_count > 0) {
        // Allocate field types
        polycall_ffi_type_t* field_types_copy = polycall_core_malloc(ctx, field_count * sizeof(polycall_ffi_type_t));
        if (!field_types_copy) {
            if (c_bridge->thread_safe) {
                pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
            }
            
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for field types");
                              
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Allocate field names
        const char** field_names_copy = polycall_core_malloc(ctx, field_count * sizeof(char*));
        if (!field_names_copy) {
            polycall_core_free(ctx, field_types_copy);
            
            if (c_bridge->thread_safe) {
                pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
            }
            
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for field names");
                              
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Allocate field offsets
        size_t* field_offsets_copy = polycall_core_malloc(ctx, field_count * sizeof(size_t));
        if (!field_offsets_copy) {
            polycall_core_free(ctx, field_names_copy);
            polycall_core_free(ctx, field_types_copy);
            
            if (c_bridge->thread_safe) {
                pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
            }
            
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for field offsets");
                              
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // 3. Copy field information
        memcpy(field_types_copy, field_types, field_count * sizeof(polycall_ffi_type_t));
        
        // Copy field names (deep copy)
        for (size_t i = 0; i < field_count; i++) {
            if (field_names[i]) {
                size_t name_len = strlen(field_names[i]) + 1;
                char* name_copy = polycall_core_malloc(ctx, name_len);
                if (!name_copy) {
                    // Clean up previously allocated names
                    for (size_t j = 0; j < i; j++) {
                        polycall_core_free(ctx, (void*)field_names_copy[j]);
                    }
                    polycall_core_free(ctx, field_offsets_copy);
                    polycall_core_free(ctx, field_names_copy);
                    polycall_core_free(ctx, field_types_copy);
                    
                    if (c_bridge->thread_safe) {
                        pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
                    }
                    
                    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                      POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                      POLYCALL_ERROR_SEVERITY_ERROR, 
                                      "Failed to allocate memory for field name");
                                      
                    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                }
                strcpy(name_copy, field_names[i]);
                field_names_copy[i] = name_copy;
            } else {
                field_names_copy[i] = NULL;
            }
        }
        
        memcpy(field_offsets_copy, field_offsets, field_count * sizeof(size_t));
        
        // Set struct info fields - using correct struct field names
        type_info.details.struct_info.types = field_types_copy;
        type_info.details.struct_info.names = field_names_copy;
        type_info.details.struct_info.offsets = field_offsets_copy;
    }
    
    // 4. Add the struct to the registry
    // Duplicate struct name
    new_struct->name = polycall_core_malloc(ctx, strlen(struct_name) + 1);
    if (!new_struct->name) {
        // Clean up allocated memory if needed
        if (field_count > 0) {
            for (size_t i = 0; i < field_count; i++) {
                if (field_names_copy[i]) {
                    polycall_core_free(ctx, (void*)field_names_copy[i]);
                }
            }
            polycall_core_free(ctx, field_offsets_copy);
            polycall_core_free(ctx, field_names_copy);
            polycall_core_free(ctx, field_types_copy);
        }
        
        if (c_bridge->thread_safe) {
            pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
        }
        
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate memory for struct name");
                          
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    strcpy(new_struct->name, struct_name);
    new_struct->type_info = type_info;
    
    // Increment struct count
    c_bridge->struct_registry.count++;
    
    // Unlock registry before registering with FFI system
    if (c_bridge->thread_safe) {
        pthread_mutex_unlock(&c_bridge->struct_registry.mutex);
    }
    
    // 5. Register the struct with the FFI type system
    polycall_core_error_t result = polycall_type_register(
        ctx, ffi_ctx, type_ctx, &type_info, "c");
    
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to register struct %s with FFI system", struct_name);
                          
        // Clean up struct from registry (would need to implement this)
        // For now, we just leave it in the registry as cleanup will handle it later
        // This can lead to inconsistency between registry and FFI system, though
    }
    
    return result;

    }
    
/**
  * @brief Set up callback handling for C functions
  */
 polycall_core_error_t polycall_c_bridge_setup_callback(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     ffi_type_info_t* callback_type,
     void* callback_fn,
     void* user_data
 ) {
     if (!ctx || !ffi_ctx || !c_bridge || !callback_type || !callback_fn || 
         callback_type->type != POLYCALL_FFI_TYPE_CALLBACK) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock callback registry
     if (c_bridge->thread_safe) {
         pthread_mutex_lock(&c_bridge->callback_registry.mutex);
     }
     
     // Check if registry is full
     if (c_bridge->callback_registry.count >= c_bridge->callback_registry.capacity) {
         if (c_bridge->thread_safe) {
             pthread_mutex_unlock(&c_bridge->callback_registry.mutex);
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Callback registry full");
                           
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Add callback to registry
     c_callback_t* cb = &c_bridge->callback_registry.callbacks[c_bridge->callback_registry.count++];
     
     // Copy callback information
     cb->type_info = *callback_type;
     cb->callback_fn = callback_fn;
     cb->user_data = user_data;
     
     // Make a deep copy of the parameter types if needed
     if (callback_type->details.callback_info.param_count > 0 && 
         callback_type->details.callback_info.param_types) {
         
         size_t param_count = callback_type->details.callback_info.param_count;
         polycall_ffi_type_t* param_types = polycall_core_malloc(ctx, 
             param_count * sizeof(polycall_ffi_type_t));
             
         if (!param_types) {
             c_bridge->callback_registry.count--; // Revert count increase
             
             if (c_bridge->thread_safe) {
                 pthread_mutex_unlock(&c_bridge->callback_registry.mutex);
             }
             
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate callback parameter types");
                               
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(param_types, callback_type->details.callback_info.param_types, 
               param_count * sizeof(polycall_ffi_type_t));
               
         cb->type_info.details.callback_info.param_types = param_types;
     }
     
     if (c_bridge->thread_safe) {
         pthread_mutex_unlock(&c_bridge->callback_registry.mutex);
     }
     
     // For now, we don't need to register the callback with the FFI system
     // because the callback is called directly by the C code
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get language bridge interface for C
  */
 polycall_core_error_t polycall_c_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     language_bridge_t* bridge
 ) {
     if (!ctx || !ffi_ctx || !c_bridge || !bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy the bridge interface
     memcpy(bridge, &c_bridge->bridge_interface, sizeof(language_bridge_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a default C bridge configuration
  */
 polycall_c_bridge_config_t polycall_c_bridge_create_default_config(void) {
     polycall_c_bridge_config_t config;
     
     config.use_stdcall = false;
     config.enable_var_args = true;
     config.thread_safe = true;
     config.max_function_count = 1024;
     config.user_data = NULL;
     
     return config;
 }
 
 // Implementation of language bridge functions
 
 static polycall_core_error_t c_convert_to_native(
     polycall_core_context_t* ctx,
     const ffi_value_t* src,
     void* dest,
     ffi_type_info_t* dest_type
 ) {
     if (!ctx || !src || !dest || !dest_type) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure types are compatible
     if (src->type != dest_type->type) {
         // TODO: Handle type conversion if needed
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Type mismatch: source=%d, dest=%d", 
                           src->type, dest_type->type);
                           
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Convert based on type
     switch (src->type) {
         case POLYCALL_FFI_TYPE_VOID:
             // Nothing to do
             break;
             
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
             // For strings, we just copy the pointer
             *((const char**)dest) = src->value.string_value;
             break;
             
         case POLYCALL_FFI_TYPE_POINTER:
             // For pointers, we just copy the pointer
             *((void**)dest) = src->value.pointer_value;
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
             // For structs, we need to copy the memory
             if (src->value.struct_value && dest_type->details.struct_info.size > 0) {
                 memcpy(dest, src->value.struct_value, dest_type->details.struct_info.size);
             }
             break;
             
         case POLYCALL_FFI_TYPE_ARRAY:
             // For arrays, we need to copy the memory
             if (src->value.array_value && 
                 dest_type->details.array_info.element_count > 0 &&
                 dest_type->details.array_info.element_type != POLYCALL_FFI_TYPE_VOID) {
                 
                 // Calculate size based on element type and count
                 size_t element_size = 0;
                 switch (dest_type->details.array_info.element_type) {
                     case POLYCALL_FFI_TYPE_BOOL: element_size = sizeof(bool); break;
                     case POLYCALL_FFI_TYPE_CHAR: element_size = sizeof(char); break;
                     case POLYCALL_FFI_TYPE_UINT8: element_size = sizeof(uint8_t); break;
                     case POLYCALL_FFI_TYPE_INT8: element_size = sizeof(int8_t); break;
                     case POLYCALL_FFI_TYPE_UINT16: element_size = sizeof(uint16_t); break;
                     case POLYCALL_FFI_TYPE_INT16: element_size = sizeof(int16_t); break;
                     case POLYCALL_FFI_TYPE_UINT32: element_size = sizeof(uint32_t); break;
                     case POLYCALL_FFI_TYPE_INT32: element_size = sizeof(int32_t); break;
                     case POLYCALL_FFI_TYPE_UINT64: element_size = sizeof(uint64_t); break;
                     case POLYCALL_FFI_TYPE_INT64: element_size = sizeof(int64_t); break;
                     case POLYCALL_FFI_TYPE_FLOAT: element_size = sizeof(float); break;
                     case POLYCALL_FFI_TYPE_DOUBLE: element_size = sizeof(double); break;
                     case POLYCALL_FFI_TYPE_STRING: element_size = sizeof(char*); break;
                     case POLYCALL_FFI_TYPE_POINTER: element_size = sizeof(void*); break;
                     default: element_size = 0; break;
                 }
                 
                 if (element_size > 0) {
                     size_t array_size = element_size * dest_type->details.array_info.element_count;
                     memcpy(dest, src->value.array_value, array_size);
                 }
             }
             break;
             
         case POLYCALL_FFI_TYPE_CALLBACK:
             // For callbacks, we just copy the pointer
             *((void**)dest) = src->value.callback_value;
             break;
             
         case POLYCALL_FFI_TYPE_OBJECT:
             // For objects, we just copy the pointer
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
 
 static polycall_core_error_t c_convert_from_native(
     polycall_core_context_t* ctx,
     const void* src,
     ffi_type_info_t* src_type,
     ffi_value_t* dest
 ) {
     if (!ctx || !src || !src_type || !dest) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize destination
     dest->type = src_type->type;
     dest->type_info = src_type;
     
     // Convert based on type
     switch (src_type->type) {
         case POLYCALL_FFI_TYPE_VOID:
             // Nothing to do
             break;
             
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
             // For strings, we just copy the pointer
             dest->value.string_value = *((const char* const*)src);
             break;
             
         case POLYCALL_FFI_TYPE_POINTER:
             // For pointers, we just copy the pointer
             dest->value.pointer_value = *((void* const*)src);
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
             // For structs, we need to allocate memory and copy
             if (src_type->details.struct_info.size > 0) {
                 void* struct_value = polycall_core_malloc(ctx, src_type->details.struct_info.size);
                 if (!struct_value) {
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                       POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                       POLYCALL_ERROR_SEVERITY_ERROR, 
                                       "Failed to allocate memory for struct");
                                       
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(struct_value, src, src_type->details.struct_info.size);
                 dest->value.struct_value = struct_value;
             }
             break;
             
         case POLYCALL_FFI_TYPE_ARRAY:
             // For arrays, we need to allocate memory and copy
             if (src_type->details.array_info.element_count > 0 &&
                 src_type->details.array_info.element_type != POLYCALL_FFI_TYPE_VOID) {
                 
                 // Calculate size based on element type and count
                 size_t element_size = 0;
                 switch (src_type->details.array_info.element_type) {
                     case POLYCALL_FFI_TYPE_BOOL: element_size = sizeof(bool); break;
                     case POLYCALL_FFI_TYPE_CHAR: element_size = sizeof(char); break;
                     case POLYCALL_FFI_TYPE_UINT8: element_size = sizeof(uint8_t); break;
                     case POLYCALL_FFI_TYPE_INT8: element_size = sizeof(int8_t); break;
                     case POLYCALL_FFI_TYPE_UINT16: element_size = sizeof(uint16_t); break;
                     case POLYCALL_FFI_TYPE_INT16: element_size = sizeof(int16_t); break;
                     case POLYCALL_FFI_TYPE_UINT32: element_size = sizeof(uint32_t); break;
                     case POLYCALL_FFI_TYPE_INT32: element_size = sizeof(int32_t); break;
                     case POLYCALL_FFI_TYPE_UINT64: element_size = sizeof(uint64_t); break;
                     case POLYCALL_FFI_TYPE_INT64: element_size = sizeof(int64_t); break;
                     case POLYCALL_FFI_TYPE_FLOAT: element_size = sizeof(float); break;
                     case POLYCALL_FFI_TYPE_DOUBLE: element_size = sizeof(double); break;
                     case POLYCALL_FFI_TYPE_STRING: element_size = sizeof(char*); break;
                     case POLYCALL_FFI_TYPE_POINTER: element_size = sizeof(void*); break;
                     default: element_size = 0; break;
                 }
                 
                 if (element_size > 0) {
                     size_t array_size = element_size * src_type->details.array_info.element_count;
                     void* array_value = polycall_core_malloc(ctx, array_size);
                     if (!array_value) {
                         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                           POLYCALL_ERROR_SEVERITY_ERROR, 
                                           "Failed to allocate memory for array");
                                           
                         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                     }
                     
                     memcpy(array_value, src, array_size);
                     dest->value.array_value = array_value;
                 }
             }
             break;
             
         case POLYCALL_FFI_TYPE_CALLBACK:
             // For callbacks, we just copy the pointer
             dest->value.callback_value = *((void* const*)src);
             break;
             
         case POLYCALL_FFI_TYPE_OBJECT:
             // For objects, we just copy the pointer
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
 
 static polycall_core_error_t c_register_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     uint32_t flags
 ) {
     // This function is called by the FFI core when a function is registered
     // We don't need to do anything here because the C bridge is the "native" bridge
     // and all functions are already registered with the C bridge directly
     
     return POLYCALL_CORE_SUCCESS;
 }
 
