/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/config/global_config.h"

 * @file global_config.c
 * @brief Global Configuration System Implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements the global configuration system that provides centralized
 * configuration management for all LibPolyCall components, following the
 * Program-First design principles.
 */

 #include "polycall/core/config/polycallfile/global_config.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/config/config_parser.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <pthread.h>
 
 #define POLYCALL_GLOBAL_CONFIG_MAGIC 0xC0FFEE01
 #define MAX_CONFIG_CALLBACKS 16
 #define MAX_PATH_LENGTH 512
 #define DEFAULT_CONFIG_PATH "/etc/polycall/global.conf"
 
 /**
  * @brief Configuration change callback entry
  */
 typedef struct {
     void (*callback)(void* user_data);
     void* user_data;
 } config_callback_entry_t;
 
 /**
  * @brief Internal global configuration context structure
  */
 struct polycall_global_config_context {
     uint32_t magic;                            // Magic number for validation
     polycall_core_context_t* core_ctx;         // Core context
     polycall_global_config_t config;           // Current configuration
     
     // Path to configuration file
     char config_file_path[MAX_PATH_LENGTH];
     bool has_config_file;
     
     // Callback management
     config_callback_entry_t callbacks[MAX_CONFIG_CALLBACKS];
     size_t callback_count;
     
     // Thread synchronization
     pthread_mutex_t mutex;
 };
 
 /**
  * @brief Validate global configuration context
  */
 static bool validate_global_config_context(
     polycall_global_config_context_t* ctx
 ) {
     return ctx && ctx->magic == POLYCALL_GLOBAL_CONFIG_MAGIC;
 }
 
 /**
  * @brief Notify all registered callbacks about configuration change
  */
 static void notify_config_callbacks(
     polycall_global_config_context_t* config_ctx
 ) {
     if (!validate_global_config_context(config_ctx)) {
         return;
     }
     
     for (size_t i = 0; i < config_ctx->callback_count; i++) {
         if (config_ctx->callbacks[i].callback) {
             config_ctx->callbacks[i].callback(config_ctx->callbacks[i].user_data);
         }
     }
 }
 
 /**
  * @brief Initialize global configuration
  */
 polycall_core_error_t polycall_global_config_init(
     polycall_core_context_t* core_ctx,
     polycall_global_config_context_t** config_ctx,
     const polycall_global_config_t* config
 ) {
     if (!core_ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate configuration context
     polycall_global_config_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_global_config_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_global_config_context_t));
     
     // Set magic number for validation
     new_ctx->magic = POLYCALL_GLOBAL_CONFIG_MAGIC;
     new_ctx->core_ctx = core_ctx;
     
     // Initialize mutex
     if (pthread_mutex_init(&new_ctx->mutex, NULL) != 0) {
         polycall_core_free(core_ctx, new_ctx);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Copy initial configuration if provided, otherwise use defaults
     if (config) {
         memcpy(&new_ctx->config, config, sizeof(polycall_global_config_t));
     } else {
         new_ctx->config = polycall_global_config_create_default();
     }
     
     *config_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Load global configuration from file
  */
 polycall_core_error_t polycall_global_config_load(
     polycall_global_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_global_config_context(config_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* path = file_path ? file_path : DEFAULT_CONFIG_PATH;
     
     // Parse the configuration file
     polycall_config_t* parsed_config = polycall_parse_config_file(path);
     if (!parsed_config) {
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Store file path for future saves
     strncpy(config_ctx->config_file_path, path, sizeof(config_ctx->config_file_path) - 1);
     config_ctx->config_file_path[sizeof(config_ctx->config_file_path) - 1] = '\0';
     config_ctx->has_config_file = true;
     
     // Apply the parsed configuration to our global config
     // This is a simplified version, a real implementation would traverse
     // the parsed_config structure and apply values to config_ctx->config
     
     // Example of applying configuration values (pseudocode):
     // config_entry_t* entry = parsed_config->entries;
     // while (entry) {
     //     if (strcmp(entry->key, "log_level") == 0) {
     //         config_ctx->config.log_level = entry->value.int_value;
     //     } else if (strcmp(entry->key, "enable_security") == 0) {
     //         config_ctx->config.security.enable_security = entry->value.bool_value;
     //     }
     //     // ... handle other keys
     //     entry = entry->next;
     // }
     
     // Clean up parsed configuration
     polycall_config_destroy(parsed_config);
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     // Notify registered callbacks
     notify_config_callbacks(config_ctx);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Save global configuration to file
  */
 polycall_core_error_t polycall_global_config_save(
     polycall_global_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_global_config_context(config_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use provided path or stored path
     const char* path = file_path ? file_path : (
         config_ctx->has_config_file ? config_ctx->config_file_path : DEFAULT_CONFIG_PATH
     );
     
     FILE* file = fopen(path, "w");
     if (!file) {
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Write configuration to file
     fprintf(file, "# LibPolyCall Global Configuration\n");
     fprintf(file, "# Generated by polycall_global_config\n\n");
     
     fprintf(file, "[general]\n");
     fprintf(file, "library_version = \"%s\"\n", config_ctx->config.library_version);
     fprintf(file, "log_level = %d\n", config_ctx->config.log_level);
     fprintf(file, "enable_tracing = %s\n", config_ctx->config.enable_tracing ? "true" : "false");
     fprintf(file, "max_message_size = %u\n", config_ctx->config.max_message_size);
     
     fprintf(file, "\n[security]\n");
     fprintf(file, "enable_security = %s\n", config_ctx->config.security.enable_security ? "true" : "false");
     fprintf(file, "enforcement_level = %d\n", config_ctx->config.security.enforcement_level);
     fprintf(file, "enable_encryption = %s\n", config_ctx->config.security.enable_encryption ? "true" : "false");
     fprintf(file, "minimum_key_size = %u\n", config_ctx->config.security.minimum_key_size);
     
     fprintf(file, "\n[networking]\n");
     fprintf(file, "default_timeout_ms = %u\n", config_ctx->config.networking.default_timeout_ms);
     fprintf(file, "max_connections = %u\n", config_ctx->config.networking.max_connections);
     fprintf(file, "enable_compression = %s\n", config_ctx->config.networking.enable_compression ? "true" : "false");
     
     fprintf(file, "\n[telemetry]\n");
     fprintf(file, "enable_telemetry = %s\n", config_ctx->config.telemetry.enable_telemetry ? "true" : "false");
     fprintf(file, "sampling_rate = %f\n", config_ctx->config.telemetry.sampling_rate);
     fprintf(file, "buffer_size = %u\n", config_ctx->config.telemetry.buffer_size);
     
     fprintf(file, "\n[memory]\n");
     fprintf(file, "pool_size = %u\n", config_ctx->config.memory.pool_size);
     fprintf(file, "use_static_allocation = %s\n", config_ctx->config.memory.use_static_allocation ? "true" : "false");
     
     // Close file
     fclose(file);
     
     // Store file path if not already stored
     if (!config_ctx->has_config_file && file_path) {
         strncpy(config_ctx->config_file_path, file_path, sizeof(config_ctx->config_file_path) - 1);
         config_ctx->config_file_path[sizeof(config_ctx->config_file_path) - 1] = '\0';
         config_ctx->has_config_file = true;
     }
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get global configuration parameter by name
  */
 polycall_core_error_t polycall_global_config_get_param(
     polycall_global_config_context_t* config_ctx,
     const char* param_name,
     void* param_value,
     size_t value_size
 ) {
     if (!validate_global_config_context(config_ctx) || !param_name || !param_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     polycall_core_error_t result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     
     // String comparison to determine which parameter to retrieve
     // A more efficient approach could use a hash table or parameter IDs
     
     // General settings
     if (strcmp(param_name, "log_level") == 0 && value_size >= sizeof(int)) {
         *((int*)param_value) = config_ctx->config.log_level;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "enable_tracing") == 0 && value_size >= sizeof(bool)) {
         *((bool*)param_value) = config_ctx->config.enable_tracing;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "max_message_size") == 0 && value_size >= sizeof(uint32_t)) {
         *((uint32_t*)param_value) = config_ctx->config.max_message_size;
         result = POLYCALL_CORE_SUCCESS;
     }
     // Security settings
     else if (strcmp(param_name, "security.enable_security") == 0 && value_size >= sizeof(bool)) {
         *((bool*)param_value) = config_ctx->config.security.enable_security;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "security.enforcement_level") == 0 && value_size >= sizeof(int)) {
         *((int*)param_value) = config_ctx->config.security.enforcement_level;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "security.enable_encryption") == 0 && value_size >= sizeof(bool)) {
         *((bool*)param_value) = config_ctx->config.security.enable_encryption;
         result = POLYCALL_CORE_SUCCESS;
     }
     // Networking settings
     else if (strcmp(param_name, "networking.default_timeout_ms") == 0 && value_size >= sizeof(uint32_t)) {
         *((uint32_t*)param_value) = config_ctx->config.networking.default_timeout_ms;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "networking.max_connections") == 0 && value_size >= sizeof(uint32_t)) {
         *((uint32_t*)param_value) = config_ctx->config.networking.max_connections;
         result = POLYCALL_CORE_SUCCESS;
     }
     // Handle other parameters as needed
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     return result;
 }
 
 /**
  * @brief Set global configuration parameter by name
  */
 polycall_core_error_t polycall_global_config_set_param(
     polycall_global_config_context_t* config_ctx,
     const char* param_name,
     const void* param_value
 ) {
     if (!validate_global_config_context(config_ctx) || !param_name || !param_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     polycall_core_error_t result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     bool param_changed = false;
     
     // General settings
     if (strcmp(param_name, "log_level") == 0) {
         config_ctx->config.log_level = *((const int*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "enable_tracing") == 0) {
         config_ctx->config.enable_tracing = *((const bool*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "max_message_size") == 0) {
         config_ctx->config.max_message_size = *((const uint32_t*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     // Security settings
     else if (strcmp(param_name, "security.enable_security") == 0) {
         config_ctx->config.security.enable_security = *((const bool*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "security.enforcement_level") == 0) {
         config_ctx->config.security.enforcement_level = *((const int*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "security.enable_encryption") == 0) {
         config_ctx->config.security.enable_encryption = *((const bool*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     // Networking settings
     else if (strcmp(param_name, "networking.default_timeout_ms") == 0) {
         config_ctx->config.networking.default_timeout_ms = *((const uint32_t*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     else if (strcmp(param_name, "networking.max_connections") == 0) {
         config_ctx->config.networking.max_connections = *((const uint32_t*)param_value);
         param_changed = true;
         result = POLYCALL_CORE_SUCCESS;
     }
     // Handle other parameters as needed
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     // Notify callbacks if parameter was changed
     if (param_changed) {
         notify_config_callbacks(config_ctx);
     }
     
     return result;
 }
 
 /**
  * @brief Get the entire global configuration
  */
 polycall_core_error_t polycall_global_config_get(
     polycall_global_config_context_t* config_ctx,
     polycall_global_config_t* config
 ) {
     if (!validate_global_config_context(config_ctx) || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Copy configuration
     memcpy(config, &config_ctx->config, sizeof(polycall_global_config_t));
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set the entire global configuration
  */
 polycall_core_error_t polycall_global_config_set(
     polycall_global_config_context_t* config_ctx,
     const polycall_global_config_t* config
 ) {
     if (!validate_global_config_context(config_ctx) || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Copy configuration
     memcpy(&config_ctx->config, config, sizeof(polycall_global_config_t));
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     // Notify callbacks
     notify_config_callbacks(config_ctx);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register configuration change callback
  */
 polycall_core_error_t polycall_global_config_register_callback(
     polycall_global_config_context_t* config_ctx,
     void (*callback)(void* user_data),
     void* user_data
 ) {
     if (!validate_global_config_context(config_ctx) || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Check if we have space for another callback
     if (config_ctx->callback_count >= MAX_CONFIG_CALLBACKS) {
         pthread_mutex_unlock(&config_ctx->mutex);
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Register callback
     config_ctx->callbacks[config_ctx->callback_count].callback = callback;
     config_ctx->callbacks[config_ctx->callback_count].user_data = user_data;
     config_ctx->callback_count++;
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate global configuration parameters
  */
 bool polycall_global_config_validate(
     const polycall_global_config_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!config) {
         if (error_message && error_message_size > 0) {
             strncpy(error_message, "NULL configuration pointer", error_message_size - 1);
             error_message[error_message_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate log_level
     if (config->log_level < 0 || config->log_level > 5) {
         if (error_message && error_message_size > 0) {
             strncpy(error_message, "log_level must be between 0 and 5", error_message_size - 1);
             error_message[error_message_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate max_message_size
     if (config->max_message_size == 0 || config->max_message_size > 100 * 1024 * 1024) {
         if (error_message && error_message_size > 0) {
             strncpy(error_message, "max_message_size must be between 1 and 100MB", error_message_size - 1);
             error_message[error_message_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate security enforcement_level
     if (config->security.enforcement_level < 0 || config->security.enforcement_level > 3) {
         if (error_message && error_message_size > 0) {
             strncpy(error_message, "security.enforcement_level must be between 0 and 3", error_message_size - 1);
             error_message[error_message_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate networking default_timeout_ms
     if (config->networking.default_timeout_ms < 100 || config->networking.default_timeout_ms > 300000) {
         if (error_message && error_message_size > 0) {
             strncpy(error_message, "networking.default_timeout_ms must be between 100 and 300000", error_message_size - 1);
             error_message[error_message_size - 1] = '\0';
         }
         return false;
     }
     
     // Additional validations as needed
     
     return true;
 }
 
 /**
  * @brief Create default global configuration
  */
 polycall_global_config_t polycall_global_config_create_default(void) {
     polycall_global_config_t default_config;
     memset(&default_config, 0, sizeof(polycall_global_config_t));
     
     // Set library version
     strncpy(default_config.library_version, "1.0.0", sizeof(default_config.library_version) - 1);
     
     // General settings
     default_config.log_level = 2; // INFO level
     default_config.enable_tracing = false;
     default_config.max_message_size = 1024 * 1024; // 1MB
     
     // Security settings
     default_config.security.enable_security = true;
     default_config.security.enforcement_level = 1; // Medium
     default_config.security.enable_encryption = true;
     default_config.security.minimum_key_size = 2048;
     
     // Networking settings
     default_config.networking.default_timeout_ms = 5000; // 5 seconds
     default_config.networking.max_connections = 100;
     default_config.networking.enable_compression = true;
     
     // Telemetry settings
     default_config.telemetry.enable_telemetry = true;
     default_config.telemetry.sampling_rate = 0.1f; // 10% sampling
     default_config.telemetry.buffer_size = 64 * 1024; // 64KB
     
     // Memory settings
     default_config.memory.pool_size = 10 * 1024 * 1024; // 10MB
     default_config.memory.use_static_allocation = false;
     
     return default_config;
 }
 
 /**
  * @brief Apply global configuration to micro component
  */
 void polycall_global_config_apply_to_micro(
     polycall_global_config_context_t* config_ctx,
     void* micro_config
 ) {
     if (!validate_global_config_context(config_ctx) || !micro_config) {
         return;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Cast to micro_component_config_t* would be here in real implementation
     // and then apply appropriate settings from global config
     
     // This is a placeholder - in a real implementation, we would apply
     // relevant settings from config_ctx->config to the micro_config
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
 }
 
 /**
  * @brief Apply global configuration to network component
  */
 void polycall_global_config_apply_to_network(
     polycall_global_config_context_t* config_ctx,
     void* network_config
 ) {
     if (!validate_global_config_context(config_ctx) || !network_config) {
         return;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Cast to polycall_network_config_t* would be here in real implementation
     // and then apply appropriate settings from global config
     
     // This is a placeholder - in a real implementation, we would apply
     // relevant settings from config_ctx->config to the network_config
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
 }
 
 /**
  * @brief Apply global configuration to protocol component
  */
 void polycall_global_config_apply_to_protocol(
     polycall_global_config_context_t* config_ctx,
     void* protocol_config
 ) {
     if (!validate_global_config_context(config_ctx) || !protocol_config) {
         return;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Cast to protocol_config_t* would be here in real implementation
     // and then apply appropriate settings from global config
     
     // This is a placeholder - in a real implementation, we would apply
     // relevant settings from config_ctx->config to the protocol_config
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
 }
 
 /**
  * @brief Cleanup global configuration
  */
 void polycall_global_config_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_global_config_context_t* config_ctx
 ) {
     if (!core_ctx || !validate_global_config_context(config_ctx)) {
         return;
     }
     
     // Destroy mutex
     pthread_mutex_destroy(&config_ctx->mutex);
     
     // Clear sensitive data
     memset(config_ctx->callbacks, 0, sizeof(config_ctx->callbacks));
     
     // Invalidate magic number
     config_ctx->magic = 0;
     
     // Free context
     polycall_core_free(core_ctx, config_ctx);
 }
