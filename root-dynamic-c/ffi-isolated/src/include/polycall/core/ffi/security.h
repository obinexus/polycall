/**
 * @file security.h
 * @brief Security layer for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the security layer for LibPolyCall FFI, implementing
 * a zero-trust security model with access controls, auditing, and isolation
 * for cross-language function calls.
 */

 #ifndef POLYCALL_FFI_SECURITY_H_H
 #define POLYCALL_FFI_SECURITY_H_H
 
 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/polycall/polycall_core.h"
    #include "polycall/core/polycall/polycall_context.h"
    #include "polycall/core/polycall/polycall_error.h"
    #include "polycall/core/polycall/polycall_types.h"
    #include "polycall/core/polycall/polycall_error.h"
    #include "polycall/core/polycall/polycall_context.h"
    #include "polycall/core/polycall/polycall_config.h"
    
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <pthread.h>
    #include <stdio.h>
    #include <string.h>
    
 #ifdef __cplusplus
 extern "C" {
 #endif
 #define POLYCALL_FFI_SECURITY_H_H
 #define POLYCALL_FFI_SECURITY_H_H
 #define POLYCALL_FFI_SECURITY_H_H
 #define POLYCALL_FFI_SECURITY_H_H
 
     // Define FFI error source if not defined
     #ifndef POLYCALL_FFI_SECURITY_H_H
     #define POLYCALL_FFI_SECURITY_H_H
     #endif

     /* Forward declaration for security context type */
typedef struct polycall_ffi_security_context polycall_ffi_security_context_t;

 /**
  * @brief Access control list entry structure
  */
 typedef struct {
     char function_id[128];           // Function name or pattern
     char caller_language[64];        // Caller language or pattern
     char caller_context[64];         // Caller context or pattern
     permission_set_t required_permissions; // Required permissions
     polycall_isolation_level_t isolation_level; // Required isolation level
     bool enabled;                   // Whether entry is enabled
 } acl_entry_t;
 
 /**
  * @brief Access control list structure
  */
 struct access_control_list {
     acl_entry_t entries[MAX_ACL_ENTRIES]; // ACL entries
     size_t count;                  // Number of entries
     bool default_deny;             // Default deny policy
     pthread_mutex_t mutex;         // Thread safety mutex
 };
 
 /**
  * @brief Permission registry structure
  */
 struct permission_registry {
     struct {
         char name[64];             // Permission name
         permission_set_t value;    // Permission bit value
         char description[256];     // Permission description
     } permissions[64];             // Array of permissions
     size_t count;                  // Number of permissions
     pthread_mutex_t mutex;         // Thread safety mutex
 };
 
 /**
  * @brief Audit policy structure
  */
 typedef struct {
     polycall_audit_level_t level;  // Audit level
     bool log_to_file;              // Log to file
     bool log_to_console;           // Log to console
     char log_file[256];            // Log file path
     uint32_t max_entries;          // Maximum entries in memory
 } audit_policy_t;
 
 /**
  * @brief Audit log structure
  */
 struct audit_log {
     audit_event_t events[MAX_AUDIT_ENTRIES]; // Audit events
     size_t count;                  // Number of events
     size_t index;                  // Current index (circular buffer)
     audit_policy_t policy;         // Audit policy
     audit_callback_t callback;     // Audit callback function
     void* user_data;               // User data for callback
     pthread_mutex_t mutex;         // Thread safety mutex
     FILE* log_file;                // Log file handle
 };
 
 /**
  * @brief Security policy structure
  */
 struct security_policy {
     polycall_security_level_t security_level; // Security level
     polycall_isolation_level_t isolation_level; // Isolation level
     bool enforce_call_validation;  // Enforce call validation
     bool enforce_type_safety;      // Enforce type safety
     bool enforce_memory_isolation; // Enforce memory isolation
     bool allow_dynamic_registration; // Allow dynamic function registration
 };
 
 /**
  * @brief Memory region isolation entry
  */
 typedef struct {
     void* region;                  // Memory region
     size_t size;                   // Region size
     char owner_language[64];       // Owner language
     polycall_isolation_level_t isolation_level; // Isolation level
 } isolation_region_t;
 
 /**
  * @brief Isolation manager structure
  */
 struct isolation_manager {
     isolation_region_t regions[1024]; // Isolated memory regions
     size_t count;                  // Number of regions
     pthread_mutex_t mutex;         // Thread safety mutex
 };
 
 /**
  * @brief Security context structure
  */
 struct security_context {
     uint32_t magic;                 // Magic number for validation
     polycall_core_context_t* core_ctx; // Core context
     access_control_list_t* acl;     // Access control list
     permission_registry_t* permissions; // Permission registry
     audit_log_t* audit_log;         // Audit log
     security_policy_t* policy;      // Security policy
     isolation_manager_t* isolation; // Isolation manager
 };
 
 /**
  * @brief Security context (opaque)
  */
 typedef struct security_context security_context_t;
 
 /**
  * @brief Access control list (opaque)
  */
 typedef struct access_control_list access_control_list_t;
 
 /**
  * @brief Permission registry (opaque)
  */
 typedef struct permission_registry permission_registry_t;
 
 /**
  * @brief Audit log (opaque)
  */
 typedef struct audit_log audit_log_t;
 
 /**
  * @brief Security policy (opaque)
  */
 typedef struct security_policy security_policy_t;
 
 /**
  * @brief Isolation manager (opaque)
  */
 typedef struct isolation_manager isolation_manager_t;
 
 /**
  * @brief Permission set type
  */
 typedef uint64_t permission_set_t;
 
 /**
  * @brief Standard permission bits
  */
 typedef enum {
     POLYCALL_PERM_NONE = 0,
     POLYCALL_PERM_EXECUTE = (1ULL << 0),          /**< Execute functions */
     POLYCALL_PERM_READ_MEMORY = (1ULL << 1),      /**< Read memory */
     POLYCALL_PERM_WRITE_MEMORY = (1ULL << 2),     /**< Write memory */
     POLYCALL_PERM_ALLOCATE_MEMORY = (1ULL << 3),  /**< Allocate memory */
     POLYCALL_PERM_SHARE_MEMORY = (1ULL << 4),     /**< Share memory */
     POLYCALL_PERM_NETWORK = (1ULL << 5),          /**< Network access */
     POLYCALL_PERM_FILE_READ = (1ULL << 6),        /**< File read access */
     POLYCALL_PERM_FILE_WRITE = (1ULL << 7),       /**< File write access */
     POLYCALL_PERM_SYSTEM = (1ULL << 8),           /**< System calls */
     POLYCALL_PERM_DANGEROUS = (1ULL << 9),        /**< Dangerous operations */
     POLYCALL_PERM_ADMIN = (1ULL << 10),           /**< Administrative operations */
     POLYCALL_PERM_USER = (1ULL << 32)             /**< Start of user-defined permissions */
 } polycall_permission_bits_t;
 
 /**
  * @brief Security level
  */
 typedef enum {
     POLYCALL_SECURITY_LEVEL_NONE = 0,      /**< No security (dangerous) */
     POLYCALL_SECURITY_LEVEL_LOW,           /**< Low security */
     POLYCALL_SECURITY_LEVEL_MEDIUM,        /**< Medium security */
     POLYCALL_SECURITY_LEVEL_HIGH,          /**< High security */
     POLYCALL_SECURITY_LEVEL_MAXIMUM        /**< Maximum security */
 } polycall_security_level_t;
 
 /**
  * @brief Isolation level
  */
 typedef enum {
     POLYCALL_ISOLATION_LEVEL_NONE = 0,     /**< No isolation (dangerous) */
     POLYCALL_ISOLATION_LEVEL_SHARED,       /**< Shared memory space */
     POLYCALL_ISOLATION_LEVEL_FUNCTION,     /**< Function-level isolation */
     POLYCALL_ISOLATION_LEVEL_MODULE,       /**< Module-level isolation */
     POLYCALL_ISOLATION_LEVEL_PROCESS,      /**< Process-level isolation */
     POLYCALL_ISOLATION_LEVEL_CONTAINER     /**< Container-level isolation */
 } polycall_isolation_level_t;
 
 /**
  * @brief Audit level
  */
 typedef enum {
     POLYCALL_AUDIT_LEVEL_NONE = 0,         /**< No auditing */
     POLYCALL_AUDIT_LEVEL_ERROR,            /**< Audit errors only */
     POLYCALL_AUDIT_LEVEL_WARNING,          /**< Audit warnings and errors */
     POLYCALL_AUDIT_LEVEL_INFO,             /**< Audit informational events */
     POLYCALL_AUDIT_LEVEL_DEBUG,            /**< Audit debug events */
     POLYCALL_AUDIT_LEVEL_TRACE             /**< Audit all events (verbose) */
 } polycall_audit_level_t;
 
 /**
  * @brief Security validation result
  */
 typedef struct {
     bool allowed;                           /**< Whether access is allowed */
     polycall_permission_set_t missing_permissions; /**< Missing permissions */
     char error_message[256];                /**< Error message if not allowed */
 } security_result_t;
 
 /**
  * @brief Security policy entry
  */
 typedef struct {
     const char* function_id;                /**< Function ID or pattern */
     const char* caller_language;            /**< Caller language or pattern */
     const char* caller_context;             /**< Caller context or pattern */
     polycall_permission_set_t required_permissions; /**< Required permissions */
     polycall_isolation_level_t isolation_level; /**< Required isolation level */
     bool enabled;                           /**< Whether entry is enabled */
 } security_policy_entry_t;
 
 /**
  * @brief Audit event
  */
 typedef struct {
     uint64_t timestamp;                     /**< Event timestamp */
     const char* source_language;            /**< Source language */
     const char* target_language;            /**< Target language */
     const char* function_name;              /**< Function name */
     const char* action;                     /**< Action performed */
     security_result_t result;               /**< Security result */
     const char* details;                    /**< Additional details */
 } audit_event_t;
 
 /**
  * @brief Security configuration
  */
 typedef struct {
     polycall_security_level_t security_level; /**< Security level */
     polycall_isolation_level_t isolation_level; /**< Isolation level */
     polycall_audit_level_t audit_level;      /**< Audit level */
     const char* policy_file;                /**< Policy file path (optional) */
     size_t policy_entry_capacity;           /**< Maximum policy entries */
     size_t audit_capacity;                  /**< Audit log capacity */
     bool default_deny;                      /**< Default deny policy */
     void* user_data;                        /**< User data */
 } security_config_t;
 
 /**
  * @brief Audit callback function type
  */
 typedef void (*audit_callback_t)(
     polycall_core_context_t* ctx,
     const audit_event_t* event,
     void* user_data
 );
 
 /**
  * @brief Initialize security context
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Pointer to receive security context
  * @param config Security configuration
  * @return Error code
  */
 polycall_core_error_t polycall_security_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t** security_ctx,
     const security_config_t* config
 );
 
 /**
  * @brief Clean up security context
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context to clean up
  */
 void polycall_security_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx
 );
 
 /**
  * @brief Verify function access
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param function_name Function name
  * @param source_language Source language
  * @param source_context Source context (optional)
  * @param result Pointer to receive security result
  * @return Error code
  */
 polycall_core_error_t polycall_security_verify_access(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* function_name,
     const char* source_language,
     const char* source_context,
     security_result_t* result
 );
 
 /**
  * @brief Register a function with security attributes
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param function_name Function name
  * @param source_language Source language
  * @param required_permissions Required permissions
  * @param isolation_level Required isolation level
  * @return Error code
  */
 polycall_core_error_t polycall_security_register_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* function_name,
     const char* source_language,
     polycall_permission_set_t required_permissions,
     polycall_isolation_level_t isolation_level
 );
 
 /**
  * @brief Audit a security-relevant event
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param source_language Source language
  * @param target_language Target language
  * @param function_name Function name
  * @param action Action performed
  * @param result Security result
  * @param details Additional details
  * @return Error code
  */
 polycall_core_error_t polycall_security_audit_event(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* source_language,
     const char* target_language,
     const char* function_name,
     const char* action,
     security_result_t* result,
     const char* details
 );
 
 /**
  * @brief Register audit callback
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param callback Audit callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_security_register_audit_callback(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     audit_callback_t callback,
     void* user_data
 );
 
 /**
  * @brief Enforce isolation between components
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param source_language Source language
  * @param target_language Target language
  * @param isolation_level Isolation level
  * @return Error code
  */
 polycall_core_error_t polycall_security_enforce_isolation(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* source_language,
     const char* target_language,
     polycall_isolation_level_t isolation_level
 );
 
 /**
  * @brief Validate function parameters
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param function_name Function name
  * @param params Parameters to validate
  * @param param_count Parameter count
  * @param result Pointer to receive security result
  * @return Error code
  */
 polycall_core_error_t polycall_security_validate_parameters(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* function_name,
     const ffi_value_t* params,
     size_t param_count,
     security_result_t* result
 );
 
 /**
  * @brief Load security policy from file
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param policy_file Policy file path
  * @return Error code
  */
 polycall_core_error_t polycall_security_load_policy(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* policy_file
 );
 
 /**
  * @brief Add policy entry
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param entry Policy entry
  * @return Error code
  */
 polycall_core_error_t polycall_security_add_policy_entry(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const security_policy_entry_t* entry
 );
 
 /**
  * @brief Remove policy entry
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @param function_id Function ID
  * @param caller_language Caller language
  * @return Error code
  */
 polycall_core_error_t polycall_security_remove_policy_entry(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* function_id,
     const char* caller_language
 );
 
 /**
  * @brief Get security level
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param security_ctx Security context
  * @return Security level
  */
 polycall_security_level_t polycall_security_get_level(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx
 );
 
 /**
  * @brief Create a default security configuration
  *
  * @return Default configuration
  */
 security_config_t polycall_security_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_SECURITY_H_H */