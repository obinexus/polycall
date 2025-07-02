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
#include "polycall/telemetry/telemetry_config.h"

 * @file polycall_telemetry_config.c
 * @brief Telemetry Configuration System Implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements the configuration system for telemetry components, supporting
 * centralized management of telemetry settings across all components.
 */

 #include "polycall/telemetry/telemetry_config.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <pthread.h>
 
 #define POLYCALL_TELEMETRY_CONFIG_MAGIC 0xD4C5B6A7
 #define MAX_CONFIG_CALLBACKS 16
 
 /**
  * @brief Configuration callback entry
  */
 typedef struct {
     void (*callback)(const polycall_telemetry_config_t* new_config, void* user_data);
     void* user_data;
 } config_callback_entry_t;
 
 /**
  * @brief Internal telemetry configuration context structure
  */
 struct polycall_telemetry_config_context {
     uint32_t magic;                            // Magic number for validation
     polycall_core_context_t* core_ctx;         // Core context
     polycall_telemetry_config_t config;        // Current configuration
     
     // Callback management
     config_callback_entry_t callbacks[MAX_CONFIG_CALLBACKS];
     size_t callback_count;
     
     // Thread synchronization
     pthread_mutex_t mutex;
     
     // File path for configuration
     char config_file_path[512];
     bool has_config_file;
 };
 
 /**
  * @brief Validate telemetry configuration context
  */
 static bool validate_config_context(
     polycall_telemetry_config_context_t* ctx
 ) {
     return ctx && ctx->magic == POLYCALL_TELEMETRY_CONFIG_MAGIC;
 }
 
 /**
  * @brief Notify all registered callbacks about configuration change
  */
 static void notify_config_callbacks(
     polycall_telemetry_config_context_t* config_ctx
 ) {
     if (!validate_config_context(config_ctx)) {
         return;
     }
     
     for (size_t i = 0; i < config_ctx->callback_count; i++) {
         if (config_ctx->callbacks[i].callback) {
             config_ctx->callbacks[i].callback(
                 &config_ctx->config,
                 config_ctx->callbacks[i].user_data
             );
         }
     }
 }
 
 /**
  * @brief Initialize telemetry configuration
  */
 polycall_core_error_t polycall_telemetry_config_init(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_config_context_t** config_ctx,
     const polycall_telemetry_config_t* config
 ) {
     if (!core_ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate configuration context
     polycall_telemetry_config_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_telemetry_config_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_telemetry_config_context_t));
     
     // Set magic number for validation
     new_ctx->magic = POLYCALL_TELEMETRY_CONFIG_MAGIC;
     new_ctx->core_ctx = core_ctx;
     
     // Initialize mutex
     if (pthread_mutex_init(&new_ctx->mutex, NULL) != 0) {
         polycall_core_free(core_ctx, new_ctx);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Copy initial configuration if provided, otherwise use defaults
     if (config) {
         memcpy(&new_ctx->config, config, sizeof(polycall_telemetry_config_t));
     } else {
         new_ctx->config = polycall_telemetry_config_create_default();
     }
     
     *config_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Load telemetry configuration from JSON file
  */
 polycall_core_error_t polycall_telemetry_config_load(
     polycall_telemetry_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_config_context(config_ctx) || !file_path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     FILE* file = fopen(file_path, "r");
     if (!file) {
         return POLYCALL_CORE_ERROR_FILE_NOT_FOUND;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Store file path for future saves
     strncpy(config_ctx->config_file_path, file_path, sizeof(config_ctx->config_file_path) - 1);
     config_ctx->config_file_path[sizeof(config_ctx->config_file_path) - 1] = '\0';
     config_ctx->has_config_file = true;
     
     // TODO: Implement proper JSON parsing
     // For now, this is a placeholder implementation
     
     // Read file content (placeholder)
     char buffer[1024];
     size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
     buffer[bytes_read] = '\0';
     
     // Close file
     fclose(file);
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     // Notify registered callbacks
     notify_config_callbacks(config_ctx);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Save telemetry configuration to JSON file
  */
 polycall_core_error_t polycall_telemetry_config_save(
     polycall_telemetry_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_config_context(config_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use provided path or stored path
     const char* path = file_path ? file_path : (
         config_ctx->has_config_file ? config_ctx->config_file_path : NULL
     );
     
     if (!path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     FILE* file = fopen(path, "w");
     if (!file) {
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // TODO: Implement proper JSON serialization
     // For now, write a placeholder JSON structure
     
     fprintf(file, "{\n");
     fprintf(file, "  \"enable_telemetry\": %s,\n", 
             config_ctx->config.enable_telemetry ? "true" : "false");
     fprintf(file, "  \"min_severity\": %d,\n", 
             config_ctx->config.min_severity);
     fprintf(file, "  \"max_event_queue_size\": %u,\n", 
             config_ctx->config.max_event_queue_size);
     fprintf(file, "  \"format\": %d,\n", 
             config_ctx->config.format);
     fprintf(file, "  \"enable_security_tracking\": %s\n", 
             config_ctx->config.enable_security_tracking ? "true" : "false");
     fprintf(file, "}\n");
     
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
  * @brief Apply configuration to telemetry system
  */
 polycall_core_error_t polycall_telemetry_config_apply(
     polycall_telemetry_config_context_t* config_ctx,
     polycall_telemetry_context_t* telemetry_ctx
 ) {
     if (!validate_config_context(config_ctx) || !telemetry_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Create telemetry configuration from our comprehensive config
     polycall_telemetry_config_t telemetry_config = {
         .enable_telemetry = config_ctx->config.enable_telemetry,
         .min_severity = config_ctx->config.min_severity,
         .max_event_queue_size = config_ctx->config.max_event_queue_size,
         .enable_encryption = config_ctx->config.enable_encryption,
         .enable_compression = config_ctx->config.enable_compression,
         .log_file_path = "",  // Copy path from our config
         .log_rotation_size_mb = config_ctx->config.max_log_size_mb
     };
     
     // Copy output path
     strncpy(telemetry_config.log_file_path, config_ctx->config.output_path, 
             sizeof(telemetry_config.log_file_path) - 1);
     telemetry_config.log_file_path[sizeof(telemetry_config.log_file_path) - 1] = '\0';
     
     // TODO: Apply configuration to telemetry context
     // This is a placeholder - in a real implementation, we would call internal
     // functions to update the telemetry system configuration
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Update specific configuration parameter
  */
 polycall_core_error_t polycall_telemetry_config_update_param(
     polycall_telemetry_config_context_t* config_ctx,
     const char* param_name,
     const void* param_value
 ) {
     if (!validate_config_context(config_ctx) || !param_name || !param_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     bool param_updated = false;
     
     // Update parameter based on name
     // This approach uses string comparison, which could be improved
     // with a more efficient parameter ID or enum-based approach
     if (strcmp(param_name, "enable_telemetry") == 0) {
         config_ctx->config.enable_telemetry = *((bool*)param_value);
         param_updated = true;
     }
     else if (strcmp(param_name, "min_severity") == 0) {
         config_ctx->config.min_severity = *((polycall_telemetry_severity_t*)param_value);
         param_updated = true;
     }
     else if (strcmp(param_name, "max_event_queue_size") == 0) {
         config_ctx->config.max_event_queue_size = *((uint32_t*)param_value);
         param_updated = true;
     }
     else if (strcmp(param_name, "format") == 0) {
         config_ctx->config.format = *((polycall_telemetry_format_t*)param_value);
         param_updated = true;
     }
     else if (strcmp(param_name, "destination") == 0) {
         config_ctx->config.destination = *((polycall_telemetry_destination_t*)param_value);
         param_updated = true;
     }
     else if (strcmp(param_name, "output_path") == 0) {
         strncpy(config_ctx->config.output_path, (const char*)param_value, 
                 sizeof(config_ctx->config.output_path) - 1);
         config_ctx->config.output_path[sizeof(config_ctx->config.output_path) - 1] = '\0';
         param_updated = true;
     }
     // Add more parameters as needed
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     // Notify callbacks if parameter was updated
     if (param_updated) {
         notify_config_callbacks(config_ctx);
         return POLYCALL_CORE_SUCCESS;
     }
     
     return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
 }
 
 /**
  * @brief Get current telemetry configuration
  */
 polycall_core_error_t polycall_telemetry_config_get(
     polycall_telemetry_config_context_t* config_ctx,
     polycall_telemetry_config_t* config
 ) {
     if (!validate_config_context(config_ctx) || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock configuration
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Copy configuration
     memcpy(config, &config_ctx->config, sizeof(polycall_telemetry_config_t));
     
     // Unlock configuration
     pthread_mutex_unlock(&config_ctx->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register configuration change callback
  */
 polycall_core_error_t polycall_telemetry_config_register_callback(
     polycall_telemetry_config_context_t* config_ctx,
     void (*callback)(const polycall_telemetry_config_t* new_config, void* user_data),
     void* user_data
 ) {
     if (!validate_config_context(config_ctx) || !callback) {
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
  * @brief Create default telemetry configuration
  */
 polycall_telemetry_config_t polycall_telemetry_config_create_default(void) {
     polycall_telemetry_config_t default_config;
     memset(&default_config, 0, sizeof(polycall_telemetry_config_t));
     
     // Set default values
     default_config.enable_telemetry = true;
     default_config.min_severity = POLYCALL_TELEMETRY_INFO;
     default_config.max_event_queue_size = 1024;
     
     default_config.format = TELEMETRY_FORMAT_JSON;
     default_config.destination = TELEMETRY_DEST_FILE;
     strncpy(default_config.output_path, "/var/log/polycall_telemetry.log", 
             sizeof(default_config.output_path) - 1);
     default_config.enable_compression = false;
     default_config.enable_encryption = false;
     
     default_config.sampling_mode = TELEMETRY_SAMPLING_NONE;
     default_config.sampling_interval = 1000;
     default_config.sampling_rate = 1.0f;
     
     default_config.use_buffering = true;
     default_config.buffer_flush_interval_ms = 5000;
     default_config.buffer_size = 64 * 1024;
     
     default_config.rotation_policy = TELEMETRY_ROTATION_SIZE;
     default_config.max_log_size_mb = 10;
     default_config.max_log_age_hours = 24;
     default_config.max_log_files = 5;
     
     default_config.enable_security_tracking = true;
     default_config.security_event_retention_days = 90;
     default_config.enable_integrity_verification = true;
     
     default_config.enable_advanced_analytics = true;
     default_config.enable_pattern_matching = true;
     default_config.analytics_window_ms = 3600000;
     
     default_config.forward_to_core_logging = true;
     default_config.integrate_with_edge = false;
     default_config.forward_to_external_systems = false;
     
     return default_config;
 }
 
 /**
  * @brief Validate telemetry configuration
  */
 bool polycall_telemetry_config_validate(
     const polycall_telemetry_config_t* config,
     char* error_message,
     size_t buffer_size
 ) {
     if (!config) {
         if (error_message && buffer_size > 0) {
             strncpy(error_message, "NULL configuration pointer", buffer_size - 1);
             error_message[buffer_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate max_event_queue_size
     if (config->max_event_queue_size == 0) {
         if (error_message && buffer_size > 0) {
             strncpy(error_message, "max_event_queue_size must be greater than 0", buffer_size - 1);
             error_message[buffer_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate sampling_rate
     if (config->sampling_mode != TELEMETRY_SAMPLING_NONE && 
         (config->sampling_rate <= 0.0f || config->sampling_rate > 1.0f)) {
         if (error_message && buffer_size > 0) {
             strncpy(error_message, "sampling_rate must be between 0.0 and 1.0", buffer_size - 1);
             error_message[buffer_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate format
     if (config->format > TELEMETRY_FORMAT_CUSTOM) {
         if (error_message && buffer_size > 0) {
             strncpy(error_message, "Invalid format value", buffer_size - 1);
             error_message[buffer_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate destination
     if (config->destination > TELEMETRY_DEST_CALLBACK) {
         if (error_message && buffer_size > 0) {
             strncpy(error_message, "Invalid destination value", buffer_size - 1);
             error_message[buffer_size - 1] = '\0';
         }
         return false;
     }
     
     // Validate output_path for file destination
     if (config->destination == TELEMETRY_DEST_FILE && 
         config->output_path[0] == '\0') {
         if (error_message && buffer_size > 0) {
             strncpy(error_message, "output_path required for file destination", buffer_size - 1);
             error_message[buffer_size - 1] = '\0';
         }
         return false;
     }
     
     // All validations passed
     return true;
 }
 
 /**
  * @brief Cleanup telemetry configuration
  */
 void polycall_telemetry_config_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_config_context_t* config_ctx
 ) {
     if (!core_ctx || !validate_config_context(config_ctx)) {
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
