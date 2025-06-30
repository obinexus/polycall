/**
 * @file ffi_core.h
 * @brief Core Foreign Function Interface module for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the core FFI functionality for LibPolyCall, enabling 
 * cross-language interoperability with the Program-First design philosophy.
 * It provides the foundation for language bridges, type conversion, and 
 * function dispatch across language boundaries.
 */

 #ifndef POLYCALL_FFI_FFI_CORE_H_H
 #define POLYCALL_FFI_FFI_CORE_H_H

 
 #include <stdlib.h>
 #include <string.h>
    #include <assert.h>
    #include <pthread.h>
  #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/ffi/performance.h"

 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif

 /**
  * @brief FFI module version information
  */
 #define POLYCALL_FFI_FFI_CORE_H_H
 #define POLYCALL_FFI_FFI_CORE_H_H
 #define POLYCALL_FFI_FFI_CORE_H_H
 
 /**
  * @brief FFI context type identifier
  */
 #define POLYCALL_FFI_FFI_CORE_H_H




    // Forward declarations for internal functions
    static uint64_t calculate_hash(const void *data, size_t size);
    static uint64_t hash_function_call(const char *function_name, ffi_value_t *args, size_t arg_count);
    static uint64_t hash_function_result(ffi_value_t *result);
    static polycall_core_error_t init_call_cache(polycall_core_context_t *ctx, call_cache_t **cache, size_t capacity, uint32_t ttl_ms);
    static void cleanup_call_cache(polycall_core_context_t *ctx, call_cache_t *cache);
    static polycall_core_error_t init_type_cache(polycall_core_context_t *ctx, type_cache_t **cache, size_t capacity);
    static void cleanup_type_cache(polycall_core_context_t *ctx, type_cache_t *cache);
    static ffi_value_t *clone_ffi_value(polycall_core_context_t *ctx, const ffi_value_t *src);
    static void free_ffi_value(polycall_core_context_t *ctx, ffi_value_t *value);
    static uint64_t get_current_time_ms(void);
    static int compare_trace_entries(const void *a, const void *b);
    static void process_cache_expiry(polycall_core_context_t *ctx, call_cache_t *cache);
 // FFI context structure
 struct polycall_ffi_context {
     polycall_context_ref_t context_ref;   // Reference for context system
     polycall_core_context_t* core_ctx;    // Core context reference
     ffi_registry_t* registry;             // Function registry
     type_mapping_context_t* type_ctx;     // Type mapping context
     memory_manager_t* memory_mgr;         // Memory manager
     security_context_t* security_ctx;     // Security context
     performance_manager_t* perf_mgr;      // Performance manager (optional)
     polycall_ffi_flags_t flags;           // FFI flags
     void* user_data;                      // User data pointer
 };
 
// Function entry structure
typedef struct {
    char* name;                       // Function name
    void* function_ptr;               // Function pointer
    ffi_signature_t* signature;       // Function signature
    char* language;                   // Source language
    uint32_t flags;                   // Function flags
} function_entry_t;

// Language entry structure
typedef struct {
    char* language;                   // Language name
    language_bridge_t bridge;         // Language bridge interface
} language_entry_t;

// Function registry structure
struct ffi_registry {
    function_entry_t* functions;          // Function entries
    size_t capacity;                      // Maximum capacity
    size_t count;                         // Current count
    
    language_entry_t* languages;          // Language entries
    size_t language_capacity;             // Maximum language capacity
    size_t language_count;                // Current language count
};
 


 /**
  * @brief FFI flags
  */
 typedef enum {
     POLYCALL_FFI_FLAG_NONE = 0,
     POLYCALL_FFI_FLAG_SECURE = (1 << 0),             /**< Secure mode */
     POLYCALL_FFI_FLAG_STRICT_TYPES = (1 << 1),       /**< Strict type checking */
     POLYCALL_FFI_FLAG_MEMORY_ISOLATION = (1 << 2),   /**< Memory isolation */
     POLYCALL_FFI_FLAG_ASYNC = (1 << 3),              /**< Asynchronous calls */
     POLYCALL_FFI_FLAG_DEBUG = (1 << 4),              /**< Debug mode */
     POLYCALL_FFI_FLAG_TRACE = (1 << 5),              /**< Call tracing */
     POLYCALL_FFI_FLAG_USER = (1 << 16)               /**< Start of user-defined flags */
 } polycall_ffi_flags_t;
 
 /**
  * @brief FFI context (opaque)
  */
 typedef struct polycall_ffi_context polycall_ffi_context_t;
 
 /**
  * @brief Function registry (opaque)
  */
 typedef struct ffi_registry ffi_registry_t;
 
 /**
  * @brief FFI value types
  */
 typedef enum {
     POLYCALL_FFI_TYPE_VOID = 0,
     POLYCALL_FFI_TYPE_BOOL,
     POLYCALL_FFI_TYPE_CHAR,
     POLYCALL_FFI_TYPE_UINT8,
     POLYCALL_FFI_TYPE_INT8,
     POLYCALL_FFI_TYPE_UINT16,
     POLYCALL_FFI_TYPE_INT16,
     POLYCALL_FFI_TYPE_UINT32,
     POLYCALL_FFI_TYPE_INT32,
     POLYCALL_FFI_TYPE_UINT64,
     POLYCALL_FFI_TYPE_INT64,
     POLYCALL_FFI_TYPE_FLOAT,
     POLYCALL_FFI_TYPE_DOUBLE,
     POLYCALL_FFI_TYPE_STRING,
     POLYCALL_FFI_TYPE_POINTER,
     POLYCALL_FFI_TYPE_STRUCT,
     POLYCALL_FFI_TYPE_ARRAY,
     POLYCALL_FFI_TYPE_CALLBACK,
     POLYCALL_FFI_TYPE_OBJECT,
     POLYCALL_FFI_TYPE_USER = 0x1000   /**< Start of user-defined types */
 } polycall_ffi_type_t;
 
 /**
  * @brief FFI type information
  */
 typedef struct {
     polycall_ffi_type_t type;
     union {
         struct {
             const char* name;
             size_t size;
             size_t alignment;
             void* type_info;
             size_t field_count;           /* Number of fields */
             polycall_ffi_type_t* types;   /* Field types array */
             const char** names;           /* Field names array */
             size_t* offsets;              /* Field memory offsets array */
             
         } struct_info;
         
         struct {
             polycall_ffi_type_t element_type;
             size_t element_count;
             void* type_info;
         } array_info;
         
         struct {
             polycall_ffi_type_t return_type;
             size_t param_count;
             polycall_ffi_type_t* param_types;
         } callback_info;
         
         struct {
             const char* type_name;
             void* type_info;
         } object_info;
         
         struct {
             uint32_t type_id;
             void* type_info;
         } user_info;
     } details;
 } ffi_type_info_t;
 
 /**
  * @brief FFI value container
  */
 typedef struct {
     polycall_ffi_type_t type;
     union {
         bool bool_value;
         char char_value;
         uint8_t uint8_value;
         int8_t int8_value;
         uint16_t uint16_value;
         int16_t int16_value;
         uint32_t uint32_value;
         int32_t int32_value;
         uint64_t uint64_value;
         int64_t int64_value;
         float float_value;
         double double_value;
         const char* string_value;
         void* pointer_value;
         void* struct_value;
         void* array_value;
         void* callback_value;
         void* object_value;
         void* user_value;
     } value;
     ffi_type_info_t* type_info;
 } ffi_value_t;
 
 /**
  * @brief Function signature
  */
 typedef struct {
     polycall_ffi_type_t return_type;
     ffi_type_info_t* return_type_info;
     size_t param_count;
     polycall_ffi_type_t* param_types;
     ffi_type_info_t** param_type_infos;
     const char** param_names;
     bool* param_optional;
     bool variadic;
 } ffi_signature_t;
 
 /**
  * @brief Language bridge interface
  */
 typedef struct {
     const char* language_name;
     const char* version;
     
     // Type conversion functions
     polycall_core_error_t (*convert_to_native)(
         polycall_core_context_t* ctx,
         const ffi_value_t* src,
         void* dest,
         ffi_type_info_t* dest_type
     );
     
     polycall_core_error_t (*convert_from_native)(
         polycall_core_context_t* ctx,
         const void* src,
         ffi_type_info_t* src_type,
         ffi_value_t* dest
     );
     
     // Function handling
     polycall_core_error_t (*register_function)(
         polycall_core_context_t* ctx,
         const char* function_name,
         void* function_ptr,
         ffi_signature_t* signature,
         uint32_t flags
     );
     
     polycall_core_error_t (*call_function)(
         polycall_core_context_t* ctx,
         const char* function_name,
         ffi_value_t* args,
         size_t arg_count,
         ffi_value_t* result
     );
     
     // Memory management
     polycall_core_error_t (*acquire_memory)(
         polycall_core_context_t* ctx,
         void* ptr,
         size_t size
     );
     
     polycall_core_error_t (*release_memory)(
         polycall_core_context_t* ctx,
         void* ptr
     );
     
     // Exception handling
     polycall_core_error_t (*handle_exception)(
         polycall_core_context_t* ctx,
         void* exception,
         char* message,
         size_t message_size
     );
     
     // Lifecycle
     polycall_core_error_t (*initialize)(
         polycall_core_context_t* ctx
     );
     
     void (*cleanup)(
         polycall_core_context_t* ctx
     );
     
     void* user_data;
 } language_bridge_t;
 
 /**
  * @brief FFI configuration
  */
 typedef struct {
     polycall_ffi_flags_t flags;
     size_t memory_pool_size;
     size_t function_capacity;
     size_t type_capacity;
     void* user_data;
 } polycall_ffi_config_t;
 
 /**
  * @brief Initialize FFI module
  *
  * @param ctx Core context
  * @param ffi_ctx Pointer to receive FFI context
  * @param config FFI configuration
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t** ffi_ctx,
     const polycall_ffi_config_t* config
 );
 
 /**
  * @brief Clean up FFI module
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context to clean up
  */
 void polycall_ffi_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx
 );
 
 /**
  * @brief Register a language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param language_name Language name
  * @param bridge Language bridge interface
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_register_language(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const char* language_name,
     language_bridge_t* bridge
 );
 
 /**
  * @brief Expose a function to other languages
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param function_name Function name
  * @param function_ptr Function pointer
  * @param signature Function signature
  * @param source_language Source language
  * @param flags Function flags
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_expose_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     const char* source_language,
     uint32_t flags
 );
 
 /**
  * @brief Call a function in another language
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param function_name Function name
  * @param args Function arguments
  * @param arg_count Argument count
  * @param result Pointer to receive function result
  * @param target_language Target language
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result,
     const char* target_language
 );
 
 /**
  * @brief Register a type with the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param type_info Type information
  * @param language Language for the type
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_register_type(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_type_info_t* type_info,
     const char* language
 );
 
 /**
  * @brief Create a function signature
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param return_type Return type
  * @param param_types Parameter types
  * @param param_count Parameter count
  * @param signature Pointer to receive created signature
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_create_signature(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_type_t return_type,
     polycall_ffi_type_t* param_types,
     size_t param_count,
     ffi_signature_t** signature
 );
 
 /**
  * @brief Destroy a function signature
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param signature Signature to destroy
  */
 void polycall_ffi_destroy_signature(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_signature_t* signature
 );
 
 /**
  * @brief Create a FFI value
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param type Value type
  * @param value Pointer to receive created value
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_create_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_type_t type,
     ffi_value_t** value
 );
 
 /**
  * @brief Destroy a FFI value
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param value Value to destroy
  */
 void polycall_ffi_destroy_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_value_t* value
 );
 
 /**
  * @brief Set FFI value data
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param value FFI value to update
  * @param data Data pointer
  * @param size Data size
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_set_value_data(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_value_t* value,
     const void* data,
     size_t size
 );
 
 /**
  * @brief Get FFI value data
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param value FFI value to query
  * @param data Pointer to receive data pointer
  * @param size Pointer to receive data size
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_get_value_data(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const ffi_value_t* value,
     void** data,
     size_t* size
 );
 
 /**
  * @brief Get FFI context information
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param language_count Pointer to receive language count
  * @param function_count Pointer to receive function count
  * @param type_count Pointer to receive type count
  * @return Error code
  */
 polycall_core_error_t polycall_ffi_get_info(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     size_t* language_count,
     size_t* function_count,
     size_t* type_count
 );
 
 /**
  * @brief Get FFI version string
  *
  * @return Version string
  */
 const char* polycall_ffi_get_version(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_FFI_CORE_H_H */