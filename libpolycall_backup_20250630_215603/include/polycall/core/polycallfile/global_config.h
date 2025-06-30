/**
 * @file global_config.h
 * @brief Global Configuration System Interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the interface for the global configuration system,
 * which provides centralized configuration management for all LibPolyCall
 * components, following the Program-First design approach.
 */

 #ifndef POLYCALL_CONFIG_POLYCALLFILE_GLOBAL_CONFIG_H_H
 #define POLYCALL_CONFIG_POLYCALLFILE_GLOBAL_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Security configuration section
  */
 typedef struct {
     bool enable_security;              /**< Enable security features */
     int enforcement_level;             /**< Security enforcement level (0-3) */
     bool enable_encryption;            /**< Enable encryption */
     uint32_t minimum_key_size;         /**< Minimum key size for cryptography */
 } polycall_global_security_config_t;
 
 /**
  * @brief Networking configuration section
  */
 typedef struct {
     uint32_t default_timeout_ms;       /**< Default timeout in milliseconds */
     uint32_t max_connections;          /**< Maximum number of connections */
     bool enable_compression;           /**< Enable data compression */
 } polycall_global_networking_config_t;
 
 /**
  * @brief Telemetry configuration section
  */
 typedef struct {
     bool enable_telemetry;             /**< Enable telemetry collection */
     float sampling_rate;               /**< Telemetry sampling rate (0.0-1.0) */
     uint32_t buffer_size;              /**< Telemetry buffer size in bytes */
 } polycall_global_telemetry_config_t;
 
 /**
  * @brief Memory management configuration section
  */
 typedef struct {
     uint32_t pool_size;                /**< Memory pool size in bytes */
     bool use_static_allocation;        /**< Use static memory allocation */
 } polycall_global_memory_config_t;
 
 /**
  * @brief Global configuration structure
  */
 typedef struct {
     char library_version[16];               /**< Library version string */
     int log_level;                          /**< Logging level (0-5) */
     bool enable_tracing;                    /**< Enable function call tracing */
     uint32_t max_message_size;              /**< Maximum message size in bytes */
     polycall_global_security_config_t security;      /**< Security configuration */
     polycall_global_networking_config_t networking;  /**< Networking configuration */
     polycall_global_telemetry_config_t telemetry;    /**< Telemetry configuration */
     polycall_global_memory_config_t memory;          /**< Memory configuration */
 } polycall_global_config_t;
 
 /**
  * @brief Global configuration context (opaque)
  */
 typedef struct polycall_global_config_context polycall_global_config_context_t;
 
 /**
  * @brief Initialize global configuration context
  *
  * @param core_ctx Core context
  * @param config_ctx Pointer to receive the created configuration context
  * @param config Initial configuration, or NULL to use defaults
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_init(
     polycall_core_context_t* core_ctx,
     polycall_global_config_context_t** config_ctx,
     const polycall_global_config_t* config
 );
 
 /**
  * @brief Load global configuration from file
  *
  * @param config_ctx Configuration context
  * @param file_path Path to configuration file, or NULL for default path
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_load(
     polycall_global_config_context_t* config_ctx,
     const char* file_path
 );
 
 /**
  * @brief Save global configuration to file
  *
  * @param config_ctx Configuration context
  * @param file_path Path to save configuration, or NULL for default/previous path
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_save(
     polycall_global_config_context_t* config_ctx,
     const char* file_path
 );
 
 /**
  * @brief Get global configuration parameter by name
  *
  * @param config_ctx Configuration context
  * @param param_name Parameter name (can use dot notation, e.g. "security.enable_encryption")
  * @param param_value Pointer to receive parameter value
  * @param value_size Size of the value buffer
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_get_param(
     polycall_global_config_context_t* config_ctx,
     const char* param_name,
     void* param_value,
     size_t value_size
 );
 
 /**
  * @brief Set global configuration parameter by name
  *
  * @param config_ctx Configuration context
  * @param param_name Parameter name (can use dot notation, e.g. "security.enable_encryption")
  * @param param_value Pointer to parameter value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_set_param(
     polycall_global_config_context_t* config_ctx,
     const char* param_name,
     const void* param_value
 );
 
 /**
  * @brief Get the entire global configuration
  *
  * @param config_ctx Configuration context
  * @param config Pointer to receive the configuration
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_get(
     polycall_global_config_context_t* config_ctx,
     polycall_global_config_t* config
 );
 
 /**
  * @brief Set the entire global configuration
  *
  * @param config_ctx Configuration context
  * @param config Configuration to set
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_set(
     polycall_global_config_context_t* config_ctx,
     const polycall_global_config_t* config
 );
 
 /**
  * @brief Register configuration change callback
  *
  * @param config_ctx Configuration context
  * @param callback Callback function to invoke when configuration changes
  * @param user_data User data to pass to callback
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_global_config_register_callback(
     polycall_global_config_context_t* config_ctx,
     void (*callback)(void* user_data),
     void* user_data
 );
 
 /**
  * @brief Validate global configuration parameters
  *
  * @param config Configuration to validate
  * @param error_message Buffer to receive error message
  * @param error_message_size Size of error message buffer
  * @return bool True if valid, false otherwise
  */
 bool polycall_global_config_validate(
     const polycall_global_config_t* config,
     char* error_message,
     size_t error_message_size
 );
 
 /**
  * @brief Create default global configuration
  *
  * @return polycall_global_config_t Default configuration
  */
 polycall_global_config_t polycall_global_config_create_default(void);
 
 /**
  * @brief Apply global configuration to micro component
  *
  * @param config_ctx Configuration context
  * @param micro_config Micro component configuration to update
  */
 void polycall_global_config_apply_to_micro(
     polycall_global_config_context_t* config_ctx,
     void* micro_config
 );
 
 /**
  * @brief Apply global configuration to network component
  *
  * @param config_ctx Configuration context
  * @param network_config Network configuration to update
  */
 void polycall_global_config_apply_to_network(
     polycall_global_config_context_t* config_ctx,
     void* network_config
 );
 
 /**
  * @brief Apply global configuration to protocol component
  *
  * @param config_ctx Configuration context
  * @param protocol_config Protocol configuration to update
  */
 void polycall_global_config_apply_to_protocol(
     polycall_global_config_context_t* config_ctx,
     void* protocol_config
 );
 
 /**
  * @brief Cleanup global configuration context
  *
  * @param core_ctx Core context
  * @param config_ctx Configuration context to clean up
  */
 void polycall_global_config_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_global_config_context_t* config_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_POLYCALLFILE_GLOBAL_CONFIG_H_H */