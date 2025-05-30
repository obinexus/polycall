/**
 * @file python_bridge.h
 * @brief Python language bridge for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the Python language bridge for LibPolyCall FFI, providing
 * an interface for Python code to interact with other languages through the
 * FFI system.
 */

 #ifndef POLYCALL_FFI_PYTHON_BRIDGE_H_H
 #define POLYCALL_FFI_PYTHON_BRIDGE_H_H
 
 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Python bridge handle (opaque)
  */
 typedef struct polycall_python_bridge polycall_python_bridge_t;
 
 /**
  * @brief Python version information
  */
 typedef struct {
     int major;           /**< Major version */
     int minor;           /**< Minor version */
     int patch;           /**< Patch level */
     bool is_compatible;  /**< Whether the version is compatible */
 } polycall_python_version_t;
 
 /**
  * @brief Python bridge configuration
  */
 typedef struct {
     void* python_handle;        /**< Python interpreter state (PyInterpreterState*) */
     bool initialize_python;     /**< Initialize Python if not already initialized */
     bool enable_numpy;          /**< Enable NumPy integration */
     bool enable_pandas;         /**< Enable Pandas integration */
     bool enable_asyncio;        /**< Enable asyncio integration */
     bool enable_gil_release;    /**< Enable GIL release during long operations */
     const char* module_path;    /**< Additional module search path */
     void* user_data;            /**< User data */
 } polycall_python_bridge_config_t;
 
 /**
  * @brief Initialize the Python language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Pointer to receive Python bridge handle
  * @param config Bridge configuration
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t** python_bridge,
     const polycall_python_bridge_config_t* config
 );
 
 /**
  * @brief Clean up Python language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle to clean up
  */
 void polycall_python_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge
 );
 
 /**
  * @brief Register a Python function with the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param function_name Function name
  * @param module_name Python module name
  * @param py_function_name Python function name
  * @param signature Function signature
  * @param flags Function flags
  * @return Error code
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
 );
 
 /**
  * @brief Call a Python function through the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param function_name Function name
  * @param args Function arguments
  * @param arg_count Argument count
  * @param result Pointer to receive function result
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 );
 
 /**
  * @brief Convert FFI value to Python value
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param ffi_value FFI value to convert
  * @param py_value Pointer to receive Python value (PyObject**)
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_to_python_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const ffi_value_t* ffi_value,
     void** py_value
 );
 
 /**
  * @brief Convert Python value to FFI value
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param py_value Python value to convert (PyObject*)
  * @param expected_type Expected FFI type
  * @param ffi_value Pointer to receive FFI value
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_from_python_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     void* py_value,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 );
 
 /**
  * @brief Execute Python code string
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param code Python code string
  * @param module_name Module name for execution context
  * @param result Pointer to receive execution result
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_exec_code(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* code,
     const char* module_name,
     ffi_value_t* result
 );
 
 /**
  * @brief Import a Python module
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param module_name Module name to import
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_import_module(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     const char* module_name
 );
 
 /**
  * @brief Handle Python exception
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param error_message Buffer to receive error message
  * @param message_size Size of error message buffer
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_handle_exception(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     char* error_message,
     size_t message_size
 );
 
 /**
  * @brief Get Python version information
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param version Pointer to receive version information
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_get_version(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     polycall_python_version_t* version
 );
 
 /**
  * @brief Acquire/release the Python GIL
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param acquire Whether to acquire (true) or release (false) the GIL
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_gil_control(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     bool acquire
 );
 
 /**
  * @brief Get language bridge interface for Python
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param python_bridge Python bridge handle
  * @param bridge Pointer to receive language bridge interface
  * @return Error code
  */
 polycall_core_error_t polycall_python_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_python_bridge_t* python_bridge,
     language_bridge_t* bridge
 );
 
 /**
  * @brief Create a default Python bridge configuration
  *
  * @return Default configuration
  */
 polycall_python_bridge_config_t polycall_python_bridge_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_PYTHON_BRIDGE_H_H */