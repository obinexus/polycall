/**
 * @file polycall_micro_security.h
 * @brief Security policy enforcement for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the security policy enforcement interfaces
 * for the LibPolyCall micro command system, integrating with the
 * FFI security model.
 */

 #ifndef POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H
 #define POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/ffi/security.h"
 #include "polycall/core/polycall/polycall_micro_context.h"
 #include "polycall/core/polycall/polycall_micro_component.h"
 #include "polycall/core/micro/polycall_micro_context.h"
 #include "polycall/core/micro/polycall_micro_component.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <pthread.h>
 #include <time.h>
 #include <stdbool.h>
 #include <stdint.h>
    #include <stdio.h> 
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct security_policy security_policy_t;
 typedef struct security_context security_context_t;
 
// Maximum number of audit callbacks
#define POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H

// Maximum number of component permissions
#define POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H

// Maximum audit log size
#define POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H

// Maximum length of detail string in audit entries
#define POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H

// Permission map entry
typedef struct {
    const char* component_name;  // Component name
    polycall_permission_t permissions;  // Component permissions
} permission_map_entry_t;

// Audit callback entry
typedef struct {
    security_audit_callback_t callback;  // Callback function
    void* user_data;  // User data for callback
} audit_callback_entry_t;

// Audit log entry
typedef struct audit_log_entry {
    polycall_security_audit_entry_t entry;  // Audit entry data
    struct audit_log_entry* next;  // Next entry in log
} audit_log_entry_t;

// Security context structure
struct security_context {
    polycall_permission_t permissions;  // Current permissions
    const char* component_name;  // Associated component name
    bool is_trusted;  // Whether component is trusted
    bool can_escalate;  // Whether component can escalate privileges
};

// Security policy structure
struct security_policy {
    bool enforce_policy;  // Whether to enforce policy
    polycall_permission_t default_permissions;  // Default permissions
    const char* policy_file;  // Policy file path
    bool allow_privilege_escalation;  // Allow privilege escalation
    bool verify_commands;  // Verify commands before execution
    bool audit_events;  // Audit security events
    const char* audit_log_file;  // Audit log file path
    permission_map_entry_t* permission_map;  // Component permission map
    size_t permission_map_size;  // Permission map size
    size_t permission_map_capacity;  // Permission map capacity
    audit_callback_entry_t audit_callbacks[MAX_AUDIT_CALLBACKS];  // Audit callbacks
    size_t audit_callback_count;  // Number of audit callbacks
    audit_log_entry_t* audit_log_head;  // Head of audit log
    audit_log_entry_t* audit_log_tail;  // Tail of audit log
    size_t audit_log_size;  // Size of audit log
    pthread_mutex_t mutex;  // Mutex for thread safety
};
 /**
  * @brief Security permission types
  */
 typedef enum {
     POLYCALL_PERMISSION_NONE = 0,
     POLYCALL_PERMISSION_EXECUTE = (1 << 0),       // Permission to execute commands
     POLYCALL_PERMISSION_READ = (1 << 1),          // Permission to read component data
     POLYCALL_PERMISSION_WRITE = (1 << 2),         // Permission to write component data
     POLYCALL_PERMISSION_MEMORY = (1 << 3),        // Permission to allocate memory
     POLYCALL_PERMISSION_IO = (1 << 4),            // Permission to perform I/O operations
     POLYCALL_PERMISSION_NETWORK = (1 << 5),       // Permission to access network
     POLYCALL_PERMISSION_FILESYSTEM = (1 << 6),    // Permission to access filesystem
     POLYCALL_PERMISSION_ADMIN = (1 << 7)          // Administrative permissions
 } polycall_permission_t;
 
 /**
  * @brief Security policy configuration
  */
 typedef struct {
     bool enforce_policy;                    // Whether to enforce policy
     polycall_permission_t default_permissions; // Default permissions
     const char* policy_file;                // Policy file path (optional)
     bool allow_privilege_escalation;        // Allow privilege escalation
     bool verify_commands;                   // Verify commands before execution
     bool audit_events;                      // Audit security events
     const char* audit_log_file;             // Audit log file path (optional)
 } security_policy_config_t;
 
 /**
  * @brief Security attributes for commands
  */
 struct command_security_attributes {
     polycall_permission_t required_permissions; // Required permissions
     bool allow_untrusted;                    // Allow execution by untrusted components
     bool require_verification;               // Require verification before execution
     const char* restricted_to_component;     // Restrict to specific component (or NULL)
     bool audit_execution;                    // Audit command execution
 };
 
 /**
  * @brief Security event types
  */
 typedef enum {
     POLYCALL_SECURITY_EVENT_PERMISSION_DENIED = 0,
     POLYCALL_SECURITY_EVENT_COMMAND_EXECUTED = 1,
     POLYCALL_SECURITY_EVENT_POLICY_VIOLATION = 2,
     POLYCALL_SECURITY_EVENT_PRIVILEGE_ESCALATION = 3,
     POLYCALL_SECURITY_EVENT_COMPONENT_CREATED = 4,
     POLYCALL_SECURITY_EVENT_COMPONENT_DESTROYED = 5,
     POLYCALL_SECURITY_EVENT_POLICY_LOADED = 6,
     POLYCALL_SECURITY_EVENT_POLICY_UPDATED = 7
 } polycall_security_event_t;
 
 /**
  * @brief Security audit entry
  */
 typedef struct {
     uint64_t timestamp;                     // Event timestamp
     polycall_security_event_t event_type;    // Event type
     const char* component_name;              // Component name
     const char* command_name;                // Command name (if applicable)
     polycall_permission_t permissions;       // Permissions (if applicable)
     const char* details;                     // Additional details
 } polycall_security_audit_entry_t;
 
 /**
  * @brief Security audit callback function type
  */
 typedef void (*security_audit_callback_t)(
     polycall_core_context_t* ctx,
     polycall_micro_context_t* micro_ctx,
     const polycall_security_audit_entry_t* entry,
     void* user_data
 );
 
 /**
  * @brief Initialize security policy
  * 
  * @param ctx Core context
  * @param policy Pointer to receive policy
  * @param config Policy configuration
  * @return Error code
  */
 polycall_core_error_t security_policy_init(
     polycall_core_context_t* ctx,
     security_policy_t** policy,
     const security_policy_config_t* config
 );
 
 /**
  * @brief Clean up security policy
  * 
  * @param ctx Core context
  * @param policy Policy to clean up
  */
 void security_policy_cleanup(
     polycall_core_context_t* ctx,
     security_policy_t* policy
 );
 
 /**
  * @brief Load security policy from file
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param file_path Policy file path
  * @return Error code
  */
 polycall_core_error_t security_policy_load(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     const char* file_path
 );
 
 /**
  * @brief Save security policy to file
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param file_path Policy file path
  * @return Error code
  */
 polycall_core_error_t security_policy_save(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     const char* file_path
 );
 
 /**
  * @brief Create security context for a component
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param component Component to create context for
  * @param security_ctx Pointer to receive security context
  * @return Error code
  */
 polycall_core_error_t security_policy_create_context(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_micro_component_t* component,
     security_context_t** security_ctx
 );
 
 /**
  * @brief Check if a component has a permission
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param component Component to check
  * @param permission Permission to check
  * @param has_permission Pointer to receive result
  * @return Error code
  */
 polycall_core_error_t security_policy_check_permission(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_micro_component_t* component,
     polycall_permission_t permission,
     bool* has_permission
 );
 
 /**
  * @brief Grant permission to a component
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param component Component to grant permission to
  * @param permission Permission to grant
  * @return Error code
  */
 polycall_core_error_t security_policy_grant_permission(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_micro_component_t* component,
     polycall_permission_t permission
 );
 
 /**
  * @brief Revoke permission from a component
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param component Component to revoke permission from
  * @param permission Permission to revoke
  * @return Error code
  */
 polycall_core_error_t security_policy_revoke_permission(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_micro_component_t* component,
     polycall_permission_t permission
 );
 
 /**
  * @brief Verify command execution
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param component Component executing command
  * @param command Command to execute
  * @param is_allowed Pointer to receive result
  * @return Error code
  */
 polycall_core_error_t security_policy_verify_command(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_micro_component_t* component,
     polycall_micro_command_t* command,
     bool* is_allowed
 );
 
 /**
  * @brief Audit security event
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param event_type Event type
  * @param component Component involved
  * @param command Command involved (or NULL)
  * @param details Additional details
  * @return Error code
  */
 polycall_core_error_t security_policy_audit_event(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_security_event_t event_type,
     polycall_micro_component_t* component,
     polycall_micro_command_t* command,
     const char* details
 );
 
 /**
  * @brief Register security audit callback
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param callback Callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t security_policy_register_audit_callback(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     security_audit_callback_t callback,
     void* user_data
 );
 
 /**
  * @brief Get security attributes for a command
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param command Command to get attributes for
  * @param attributes Pointer to receive attributes
  * @return Error code
  */
 polycall_core_error_t security_policy_get_command_attributes(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     polycall_micro_command_t* command,
     command_security_attributes_t** attributes
 );
 
 /**
  * @brief Create default security policy configuration
  * 
  * @return Default configuration
  */
 security_policy_config_t security_policy_create_default_config(void);
 
 /**
  * @brief Create command security attributes
  * 
  * @param ctx Core context
  * @param attributes Pointer to receive attributes
  * @param required_permissions Required permissions
  * @return Error code
  */
 polycall_core_error_t security_create_command_attributes(
     polycall_core_context_t* ctx,
     command_security_attributes_t** attributes,
     polycall_permission_t required_permissions
 );
 
 /**
  * @brief Integrate with FFI security subsystem
  * 
  * @param ctx Core context
  * @param policy Security policy
  * @param ffi_security FFI security context
  * @return Error code
  */
 polycall_core_error_t security_policy_integrate_ffi(
     polycall_core_context_t* ctx,
     security_policy_t* policy,
     ffi_security_context_t* ffi_security
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_MICRO_POLYCALL_MICRO_SECURITY_H_H */