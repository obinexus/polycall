/**
 * @file polycall_micro_context.h
 * @brief Micro command context and main API for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the main interface for LibPolyCall's micro command system,
 * which provides lightweight command execution with component isolation.
 */

 #ifndef POLYCALL_MICRO_POLYCALL_MICRO_CONTEXT_H_H
 #define POLYCALL_MICRO_POLYCALL_MICRO_CONTEXT_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/ffi/ffi_core.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct polycall_micro_context polycall_micro_context_t;
 typedef struct polycall_micro_component polycall_micro_component_t;
 typedef struct polycall_micro_command polycall_micro_command_t;
 typedef struct polycall_micro_config polycall_micro_config_t;
 
 /**
  * @brief Isolation level for micro components
  */
 typedef enum {
     POLYCALL_ISOLATION_NONE = 0,      // No isolation (shared memory)
     POLYCALL_ISOLATION_MEMORY = 1,     // Memory isolation only
     POLYCALL_ISOLATION_RESOURCES = 2,  // Memory and resource isolation
     POLYCALL_ISOLATION_SECURITY = 3,   // Memory, resource, and security isolation
     POLYCALL_ISOLATION_STRICT = 4      // Complete isolation
 } polycall_isolation_level_t;
 
 /**
  * @brief Component state enumeration
  */
 typedef enum {
     POLYCALL_COMPONENT_STATE_UNINITIALIZED = 0,
     POLYCALL_COMPONENT_STATE_STARTING = 1,
     POLYCALL_COMPONENT_STATE_RUNNING = 2,
     POLYCALL_COMPONENT_STATE_PAUSED = 3,
     POLYCALL_COMPONENT_STATE_STOPPING = 4,
     POLYCALL_COMPONENT_STATE_STOPPED = 5,
     POLYCALL_COMPONENT_STATE_ERROR = 6
 } polycall_component_state_t;
 
 /**
  * @brief Micro command callback function type
  *
  * Handles execution of micro commands
  */
 typedef polycall_core_error_t (*polycall_command_handler_t)(
     polycall_core_context_t* ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     void* params,
     void* result,
     void* user_data
 );
 
 /**
  * @brief Command flags for micro commands
  */
 typedef enum {
     POLYCALL_COMMAND_FLAG_NONE = 0,
     POLYCALL_COMMAND_FLAG_ASYNC = (1 << 0),         // Execute asynchronously
     POLYCALL_COMMAND_FLAG_SECURE = (1 << 1),        // Requires security verification
     POLYCALL_COMMAND_FLAG_PRIVILEGED = (1 << 2),    // Requires elevated privileges
     POLYCALL_COMMAND_FLAG_READONLY = (1 << 3),      // Command does not modify component state
     POLYCALL_COMMAND_FLAG_CRITICAL = (1 << 4),      // Critical system command
     POLYCALL_COMMAND_FLAG_RESTRICTED = (1 << 5),    // Command with restricted access
     POLYCALL_COMMAND_FLAG_EXTERNAL = (1 << 6),      // Command accessible from external sources
     POLYCALL_COMMAND_FLAG_INTERNAL = (1 << 7)       // Command for internal use only
 } polycall_command_flags_t;
 
 /**
  * @brief Micro context configuration
  */
 struct polycall_micro_config {
     size_t max_components;                    // Maximum number of components
     size_t max_commands;                      // Maximum number of commands
     size_t default_memory_quota;              // Default memory quota per component (bytes)
     uint32_t default_cpu_quota;               // Default CPU quota (milliseconds)
     uint32_t default_io_quota;                // Default I/O quota (operations)
     polycall_isolation_level_t default_isolation; // Default isolation level
     bool enable_security;                     // Enable security policy enforcement
     bool enable_resource_limits;              // Enable resource limitation
     void* user_data;                          // User data
     void (*error_callback)(                   // Error callback
         polycall_core_context_t* ctx,
         polycall_micro_context_t* micro_ctx,
         polycall_core_error_t error,
         const char* message,
         void* user_data
     );
 };
 
 /**
  * @brief Initialize micro command subsystem
  * 
  * @param core_ctx Core context
  * @param micro_ctx Pointer to receive micro context
  * @param config Configuration
  * @return Error code
  */
 polycall_core_error_t polycall_micro_init(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t** micro_ctx,
     const polycall_micro_config_t* config
 );
 
 /**
  * @brief Cleanup micro command subsystem
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  */
 void polycall_micro_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx
 );
 
 /**
  * @brief Create a component
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Pointer to receive component
  * @param name Component name
  * @param isolation_level Isolation level
  * @return Error code
  */
 polycall_core_error_t polycall_micro_create_component(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t** component,
     const char* name,
     polycall_isolation_level_t isolation_level
 );
 
 /**
  * @brief Destroy a component
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to destroy
  * @return Error code
  */
 polycall_core_error_t polycall_micro_destroy_component(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component
 );
 
 /**
  * @brief Find a component by name
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param name Component name
  * @param component Pointer to receive component
  * @return Error code
  */
 polycall_core_error_t polycall_micro_find_component(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     const char* name,
     polycall_micro_component_t** component
 );
 
 /**
  * @brief Register a command with a component
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to register command with
  * @param command Pointer to receive command
  * @param name Command name
  * @param handler Command handler
  * @param flags Command flags
  * @param user_data User data for handler
  * @return Error code
  */
 polycall_core_error_t polycall_micro_register_command(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     polycall_micro_command_t** command,
     const char* name,
     polycall_command_handler_t handler,
     polycall_command_flags_t flags,
     void* user_data
 );
 
 /**
  * @brief Execute a command on a component
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to execute command on
  * @param command_name Command name
  * @param params Command parameters
  * @param result Pointer to receive command result
  * @return Error code
  */
 polycall_core_error_t polycall_micro_execute_command(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     const char* command_name,
     void* params,
     void* result
 );
 
 /**
  * @brief Execute a command asynchronously
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to execute command on
  * @param command_name Command name
  * @param params Command parameters
  * @param callback Completion callback
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_micro_execute_command_async(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     const char* command_name,
     void* params,
     void (*callback)(
         polycall_core_context_t* ctx,
         polycall_micro_context_t* micro_ctx,
         polycall_micro_component_t* component,
         const char* command_name,
         void* result,
         polycall_core_error_t error,
         void* user_data
     ),
     void* user_data
 );
 
 /**
  * @brief Set component resource limits
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to set limits for
  * @param memory_quota Memory quota in bytes
  * @param cpu_quota CPU quota in milliseconds
  * @param io_quota I/O quota in operations
  * @return Error code
  */
 polycall_core_error_t polycall_micro_set_resource_limits(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     size_t memory_quota,
     uint32_t cpu_quota,
     uint32_t io_quota
 );
 
 /**
  * @brief Start a component
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to start
  * @return Error code
  */
 polycall_core_error_t polycall_micro_start_component(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component
 );
 
 /**
  * @brief Stop a component
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to stop
  * @return Error code
  */
 polycall_core_error_t polycall_micro_stop_component(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component
 );
 
 /**
  * @brief Get component state
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to get state for
  * @param state Pointer to receive state
  * @return Error code
  */
 polycall_core_error_t polycall_micro_get_component_state(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     polycall_component_state_t* state
 );
 
 /**
  * @brief Set component user data
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to set data for
  * @param user_data User data
  * @return Error code
  */
 polycall_core_error_t polycall_micro_set_component_data(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     void* user_data
 );
 
 /**
  * @brief Get component user data
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param component Component to get data for
  * @param user_data Pointer to receive user data
  * @return Error code
  */
 polycall_core_error_t polycall_micro_get_component_data(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     void** user_data
 );
 
 /**
  * @brief Create default micro configuration
  * 
  * @return Default configuration
  */
 polycall_micro_config_t polycall_micro_create_default_config(void);
 
 /**
  * @brief Integrate with FFI subsystem
  * 
  * @param core_ctx Core context
  * @param micro_ctx Micro context
  * @param ffi_ctx FFI context
  * @return Error code 
  */
 polycall_core_error_t polycall_micro_integrate_ffi(
     polycall_core_context_t* core_ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_ffi_context_t* ffi_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_MICRO_POLYCALL_MICRO_CONTEXT_H_H */