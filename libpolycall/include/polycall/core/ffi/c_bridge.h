/**
 * @file c_bridge.h
 * @brief C language bridge for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the C language bridge for LibPolyCall FFI, providing
 * a native interface for C code to interact with other languages through
 * the FFI system.
 */

 #ifndef POLYCALL_FFI_C_BRIDGE_H_H
 #define POLYCALL_FFI_C_BRIDGE_H_H
 
 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/polycall/core/polycall.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief C bridge handle (opaque)
  */
 typedef struct polycall_c_bridge polycall_c_bridge_t;
 
 /**
  * @brief C bridge configuration
  */
 typedef struct {
     bool use_stdcall;           /**< Use stdcall calling convention */
     bool enable_var_args;       /**< Enable variadic function support */
     bool thread_safe;           /**< Enable thread safety */
     size_t max_function_count;  /**< Maximum number of registered functions */
     void* user_data;            /**< User data */
 } polycall_c_bridge_config_t;
 
 /**
  * @brief Initialize the C language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge Pointer to receive C bridge handle
  * @param config Bridge configuration
  * @return Error code
  */
 polycall_core_error_t polycall_c_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t** c_bridge,
     const polycall_c_bridge_config_t* config
 );
 
 /**
  * @brief Clean up C language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge C bridge handle to clean up
  */
 void polycall_c_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge
 );
 
 /**
  * @brief Register a C function with the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge C bridge handle
  * @param function_name Function name
  * @param function_ptr Function pointer
  * @param return_type Return type
  * @param param_types Parameter types
  * @param param_count Parameter count
  * @param flags Function flags
  * @return Error code
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
 );
 
 /**
  * @brief Call a C function through the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge C bridge handle
  * @param function_name Function name
  * @param args Function arguments
  * @param arg_count Argument count
  * @param result Pointer to receive function result
  * @return Error code
  */
 polycall_core_error_t polycall_c_bridge_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 );
 
 /**
  * @brief Register a struct type with the C bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge C bridge handle
  * @param struct_name Struct name
  * @param field_types Field types
  * @param field_names Field names
  * @param field_offsets Field offsets
  * @param field_count Field count
  * @param struct_size Total struct size
  * @param alignment Struct alignment
  * @return Error code
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
 );
 
 /**
  * @brief Set up callback handling for C functions
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge C bridge handle
  * @param callback_type Callback signature type
  * @param callback_fn Callback function pointer
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_c_bridge_setup_callback(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     ffi_type_info_t* callback_type,
     void* callback_fn,
     void* user_data
 );
 
 /**
  * @brief Get language bridge interface for C
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param c_bridge C bridge handle
  * @param bridge Pointer to receive language bridge interface
  * @return Error code
  */
 polycall_core_error_t polycall_c_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_c_bridge_t* c_bridge,
     language_bridge_t* bridge
 );
 
 /**
  * @brief Create a default C bridge configuration
  *
  * @return Default configuration
  */
 polycall_c_bridge_config_t polycall_c_bridge_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_C_BRIDGE_H_H */