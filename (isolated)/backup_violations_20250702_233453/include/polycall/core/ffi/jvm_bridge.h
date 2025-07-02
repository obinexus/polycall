/**
 * @file jvm_bridge.h
 * @brief JVM language bridge for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the JVM language bridge for LibPolyCall FFI, providing
 * an interface for Java and other JVM-based languages to interact with other
 * languages through the FFI system.
 */

 #ifndef POLYCALL_FFI_JVM_BRIDGE_H_H
 #define POLYCALL_FFI_JVM_BRIDGE_H_H
 #include <jni.h> 

 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/ffi/type_system.h"
 #include "polycall/core/ffi/memory_bridge.h"
 #include "polycall/core/ffi/security.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief JVM bridge handle (opaque)
  */
 typedef struct polycall_jvm_bridge polycall_jvm_bridge_t;
 
 /**
  * @brief JVM configuration
  */
 typedef struct {
     void* jvm_instance;                 /**< JVM instance (JavaVM*) */
     bool create_vm_if_needed;           /**< Create a JVM if one isn't provided */
     const char* classpath;              /**< Classpath for JVM initialization */
     const char* jvm_options;            /**< JVM options for initialization */
     const char* bridge_class;           /**< Java bridge class name */
     bool enable_exception_handler;      /**< Enable JVM exception handling */
     bool gc_notification;               /**< Enable GC notifications */
     bool direct_buffer_access;          /**< Use direct buffers for memory access */
     void* user_data;                    /**< User data */
 } polycall_jvm_bridge_config_t;
 
 /**
  * @brief Java method signature
  */
 typedef struct {
     const char* name;                   /**< Method name */
     const char* signature;              /**< Method signature in JNI format */
     bool is_static;                     /**< Whether the method is static */
     const char* class_name;             /**< Class name */
 } java_method_signature_t;
 
 /**
  * @brief Initialize the JVM language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge Pointer to receive JVM bridge handle
  * @param config Bridge configuration
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t** jvm_bridge,
     const polycall_jvm_bridge_config_t* config
 );
 
 /**
  * @brief Clean up JVM language bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle to clean up
  */
 void polycall_jvm_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge
 );
 
 /**
  * @brief Register a Java method with the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param function_name Function name
  * @param java_method Java method signature
  * @param flags Function flags
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_register_method(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const char* function_name,
     const java_method_signature_t* java_method,
     uint32_t flags
 );
 
 /**
  * @brief Call a Java method through the FFI system
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param function_name Function name
  * @param args Function arguments
  * @param arg_count Argument count
  * @param result Pointer to receive function result
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_call_method(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 );
 
 /**
  * @brief Convert FFI value to JVM value
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param ffi_value FFI value to convert
  * @param jni_env JNI environment pointer
  * @param java_value Pointer to receive Java value (jobject*)
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_to_java_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const ffi_value_t* ffi_value,
     void* jni_env,
     void** java_value
 );
 
 /**
  * @brief Convert JVM value to FFI value
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param java_value Java value to convert (jobject)
  * @param jni_env JNI environment pointer
  * @param expected_type Expected FFI type
  * @param ffi_value Pointer to receive FFI value
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_from_java_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     void* java_value,
     void* jni_env,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 );
 
 /**
  * @brief Register a Java callback function
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param callback_class Java callback class
  * @param callback_method Java callback method
  * @param signature Function signature
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_register_callback(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const char* callback_class,
     const char* callback_method,
     ffi_signature_t* signature
 );
 
 /**
  * @brief Handle Java exception
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param jni_env JNI environment pointer
  * @param error_message Buffer to receive error message
  * @param message_size Size of error message buffer
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_handle_exception(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     void* jni_env,
     char* error_message,
     size_t message_size
 );
 
 /**
  * @brief Get JNI environment for current thread
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param jni_env Pointer to receive JNI environment pointer
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_get_env(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     void** jni_env
 );
 
 /**
  * @brief Get language bridge interface for JVM
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param jvm_bridge JVM bridge handle
  * @param bridge Pointer to receive language bridge interface
  * @return Error code
  */
 polycall_core_error_t polycall_jvm_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     language_bridge_t* bridge
 );
 
 /**
  * @brief Java method entry structure
  */
 typedef struct {
    char* function_name;                 // Function name for FFI
    java_method_signature_t method_info; // Java method information
    jmethodID method_id;                 // JNI method ID
    jclass class_ref;                    // Global reference to class
    ffi_signature_t* signature;          // FFI function signature
    uint32_t flags;                      // Function flags
} java_method_entry_t;

/**
 * @brief Java callback structure
 */
typedef struct {
    char* callback_class;                // Class name
    char* callback_method;               // Method name
    jclass class_ref;                    // Global reference to class
    jmethodID method_id;                 // JNI method ID
    ffi_signature_t* signature;          // FFI function signature
    jobject instance;                    // Object instance (if non-static)
} java_callback_t;

/**
 * @brief JVM bridge structure
 */
struct polycall_jvm_bridge {
    polycall_core_context_t* core_ctx;   // Core context
    polycall_ffi_context_t* ffi_ctx;     // FFI context
    JavaVM* jvm;                         // JVM instance
    jclass bridge_class;                 // JVM bridge class
    bool owns_jvm;                       // Whether we created the JVM
    
    // Method registry
    java_method_entry_t* methods;        // Array of method entries
    size_t method_count;                 // Number of registered methods
    size_t method_capacity;              // Maximum number of methods
    
    // Callback registry
    java_callback_t* callbacks;          // Array of callbacks
    size_t callback_count;               // Number of registered callbacks
    size_t callback_capacity;            // Maximum number of callbacks
    
    // Configuration
    polycall_jvm_bridge_config_t config; // Configuration
    
    // Language bridge interface for FFI
    language_bridge_t bridge_interface;  // Bridge interface
};

// Forward declarations for language bridge functions
static polycall_core_error_t jvm_convert_to_native(
    polycall_core_context_t* ctx,
    const ffi_value_t* src,
    void* dest,
    ffi_type_info_t* dest_type
);

static polycall_core_error_t jvm_convert_from_native(
    polycall_core_context_t* ctx,
    const void* src,
    ffi_type_info_t* src_type,
    ffi_value_t* dest
);

static polycall_core_error_t jvm_register_function(
    polycall_core_context_t* ctx,
    const char* function_name,
    void* function_ptr,
    ffi_signature_t* signature,
    uint32_t flags
);

static polycall_core_error_t jvm_call_function(
    polycall_core_context_t* ctx,
    const char* function_name,
    ffi_value_t* args,
    size_t arg_count,
    ffi_value_t* result
);

static polycall_core_error_t jvm_acquire_memory(
    polycall_core_context_t* ctx,
    void* ptr,
    size_t size
);

static polycall_core_error_t jvm_release_memory(
    polycall_core_context_t* ctx,
    void* ptr
);

static polycall_core_error_t jvm_handle_exception(
    polycall_core_context_t* ctx,
    void* exception,
    char* message,
    size_t message_size
);

static polycall_core_error_t jvm_initialize(
    polycall_core_context_t* ctx
);

static void jvm_cleanup(
    polycall_core_context_t* ctx
);

 /**
  * @brief Create a default JVM bridge configuration
  *
  * @return Default configuration
  */
 polycall_jvm_bridge_config_t polycall_jvm_bridge_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_JVM_BRIDGE_H_H */