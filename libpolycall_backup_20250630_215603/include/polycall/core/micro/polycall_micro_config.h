/**
 * @file polycall_micro_config.h
 * @brief Configuration system for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the configuration interface for the LibPolyCall micro command
 * system, providing structured configuration loading, validation, and application
 * for micro components and commands.
 */

 #ifndef POLYCALL_MICRO_POLYCALL_MICRO_CONFIG_H_H
 #define POLYCALL_MICRO_POLYCALL_MICRO_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_micro_context.h"
 #include "polycall/core/polycall/polycall_micro_component.h"
 #include "polycall/core/polycall/polycall_micro_resource.h"
 #include "polycall/core/polycall/polycall_micro_security.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct micro_config_manager micro_config_manager_t;
 typedef struct micro_component_config micro_component_config_t;
 
 /**
  * @brief Load status enumeration
  */
 typedef enum {
     MICRO_CONFIG_LOAD_SUCCESS = 0,           // Configuration loaded successfully
     MICRO_CONFIG_LOAD_FILE_NOT_FOUND = 1,    // Configuration file not found
     MICRO_CONFIG_LOAD_PARSE_ERROR = 2,       // Error parsing configuration
     MICRO_CONFIG_LOAD_VALIDATION_ERROR = 3,  // Configuration validation error
     MICRO_CONFIG_LOAD_MEMORY_ERROR = 4       // Memory allocation error
 } micro_config_load_status_t;
 
 /**
  * @brief Validation status for component configuration
  */
 typedef enum {
     MICRO_CONFIG_VALIDATION_SUCCESS = 0,         // Validation successful
     MICRO_CONFIG_VALIDATION_INVALID_ISOLATION = 1, // Invalid isolation level
     MICRO_CONFIG_VALIDATION_INVALID_QUOTA = 2,   // Invalid resource quota
     MICRO_CONFIG_VALIDATION_INVALID_SECURITY = 3, // Invalid security settings
     MICRO_CONFIG_VALIDATION_INVALID_COMMAND = 4, // Invalid command configuration
     MICRO_CONFIG_VALIDATION_NAME_CONFLICT = 5,   // Component name conflict
     MICRO_CONFIG_VALIDATION_REFERENCE_ERROR = 6  // Invalid reference to another component
 } micro_config_validation_status_t;
 
 /**
  * @brief Configuration manager options
  */
 typedef struct {
     const char* global_config_path;             // Path to global config.Polycallfile
     const char* binding_config_path;            // Path to binding-specific .polycallrc
     bool fallback_to_defaults;                  // Whether to fallback to defaults if config not found
     bool validate_on_load;                      // Whether to validate configuration on load
     void (*error_callback)(                     // Error callback for configuration issues
         polycall_core_context_t* ctx,
         const char* message,
         const char* file_path,
         int line_number,
         void* user_data
     );
     void* user_data;                           // User data for callback
 } micro_config_manager_options_t;
 
 /**
  * @brief Component configuration structure
  */
 struct micro_component_config {
     char name[64];                              // Component name
     polycall_isolation_level_t isolation_level; // Isolation level
     size_t memory_quota;                        // Memory quota in bytes
     uint32_t cpu_quota;                         // CPU quota in milliseconds
     uint32_t io_quota;                          // I/O quota in operations
     bool enforce_quotas;                        // Whether to enforce resource quotas
     
     // Security settings
     polycall_permission_t default_permissions;  // Default permissions
     bool require_authentication;                // Whether authentication is required
     bool audit_access;                          // Whether to audit access
     char* allowed_connections[16];              // List of allowed connections
     size_t allowed_connections_count;           // Number of allowed connections
     
     // Command settings
     struct {
         char name[64];                          // Command name
         polycall_command_flags_t flags;         // Command flags
         polycall_permission_t required_permissions; // Required permissions
     } commands[32];
     size_t command_count;                       // Number of commands
 };
 
 /**
  * @brief Initialize micro configuration manager
  * 
  * @param ctx Core context
  * @param manager Pointer to receive config manager
  * @param options Configuration options
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_init(
     polycall_core_context_t* ctx,
     micro_config_manager_t** manager,
     const micro_config_manager_options_t* options
 );
 
 /**
  * @brief Cleanup micro configuration manager
  * 
  * @param ctx Core context
  * @param manager Configuration manager to cleanup
  */
 void micro_config_manager_cleanup(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager
 );
 
 /**
  * @brief Load micro configuration
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param status Pointer to receive load status
  * @param error_message Buffer to receive error message (optional)
  * @param error_message_size Size of error message buffer
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_load(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     micro_config_load_status_t* status,
     char* error_message,
     size_t error_message_size
 );
 
 /**
  * @brief Apply configuration to micro context
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param micro_ctx Micro context to configure
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_apply(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     polycall_micro_context_t* micro_ctx
 );
 
 /**
  * @brief Get component configuration by name
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param component_name Component name
  * @param config Pointer to receive component configuration
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_get_component_config(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const char* component_name,
     micro_component_config_t** config
 );
 
 /**
  * @brief Get all component configurations
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param configs Array to receive configurations
  * @param count Pointer to size of array (in/out)
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_get_all_components(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     micro_component_config_t** configs,
     size_t* count
 );
 
 /**
  * @brief Add component configuration
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param config Component configuration to add
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_add_component(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const micro_component_config_t* config
 );
 
 /**
  * @brief Remove component configuration
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param component_name Component name to remove
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_remove_component(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const char* component_name
 );
 
 /**
  * @brief Validate component configuration
  * 
  * @param ctx Core context
  * @param config Component configuration to validate
  * @param status Pointer to receive validation status
  * @param error_message Buffer to receive error message (optional)
  * @param error_message_size Size of error message buffer
  * @return Error code
  */
 polycall_core_error_t micro_config_validate_component(
     polycall_core_context_t* ctx,
     const micro_component_config_t* config,
     micro_config_validation_status_t* status,
     char* error_message,
     size_t error_message_size
 );
 
 /**
  * @brief Create default configuration for a component
  * 
  * @param ctx Core context
  * @param component_name Component name
  * @param config Pointer to receive default configuration
  * @return Error code
  */
 polycall_core_error_t micro_config_create_default_component(
     polycall_core_context_t* ctx,
     const char* component_name,
     micro_component_config_t** config
 );
 
 /**
  * @brief Save configuration to file
  * 
  * @param ctx Core context
  * @param manager Configuration manager
  * @param file_path File path to save to
  * @return Error code
  */
 polycall_core_error_t micro_config_manager_save(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const char* file_path
 );
 
 /**
  * @brief Create default configuration manager options
  * 
  * @return Default options
  */
 micro_config_manager_options_t micro_config_create_default_options(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_MICRO_POLYCALL_MICRO_CONFIG_H_H */