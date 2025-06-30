/**
 * @file ffi_config.c
 * @brief Configuration module implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the configuration system for LibPolyCall FFI, providing
 * a comprehensive, extensible configuration interface for all FFI components.
 */

 #include "polycall/core/ffi/ffi_config.h"
 #include "polycall/core/ffi/security.h"
 #include "polycall/core/ffi/type_system.h"
 #include "polycall/core/ffi/memory_bridge.h"
 #include "polycall/core/ffi/performance.h"
 #include "polycall/core/ffi/c_bridge.h"
 #include "polycall/core/ffi/js_bridge.h"
 #include "polycall/core/ffi/jvm_bridge.h"
 #include "polycall/core/ffi/python_bridge.h"
 #include "polycall/core/ffi/protocol_bridge.h"
 
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <pthread.h>
 
 
 /**
  * @brief Initialize configuration module
  */
 polycall_core_error_t polycall_ffi_config_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t** config_ctx,
     const polycall_ffi_config_options_t* options
 ) {
     if (!ctx || !ffi_ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default options if not provided
     polycall_ffi_config_options_t default_options;
     if (!options) {
         default_options = polycall_ffi_config_create_default_options();
         options = &default_options;
     }
     
     // Allocate configuration context
     polycall_ffi_config_context_t* new_ctx = (polycall_ffi_config_context_t*)
         polycall_core_malloc(ctx, sizeof(polycall_ffi_config_context_t));
     
     if (!new_ctx) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to allocate configuration context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_ffi_config_context_t));
     new_ctx->core_ctx = ctx;
     new_ctx->ffi_ctx = ffi_ctx;
     
     // Copy options
     if (options->config_file_path) {
         new_ctx->options.config_file_path = strdup(options->config_file_path);
     }
     if (options->provider_name) {
         new_ctx->options.provider_name = strdup(options->provider_name);
     }
     new_ctx->options.enable_persistence = options->enable_persistence;
     new_ctx->options.enable_change_notification = options->enable_change_notification;
     new_ctx->options.validate_configuration = options->validate_configuration;
     new_ctx->options.provider_data = options->provider_data;
     
     // Initialize mutex
     if (pthread_mutex_init(&new_ctx->mutex, NULL) != 0) {
         if (new_ctx->options.config_file_path) {
             free((void*)new_ctx->options.config_file_path);
         }
         if (new_ctx->options.provider_name) {
             free((void*)new_ctx->options.provider_name);
         }
         polycall_core_free(ctx, new_ctx);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to initialize configuration mutex");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Initialize sections
     for (int i = 0; i <= POLYCALL_CONFIG_SECTION_USER; i++) {
         new_ctx->sections[i].section_id = (polycall_config_section_t)i;
         new_ctx->sections[i].entries = NULL;
     }
     
     // Register default providers
     polycall_core_error_t result = register_default_providers(ctx, ffi_ctx, new_ctx);
     if (result != POLYCALL_CORE_SUCCESS) {
         pthread_mutex_destroy(&new_ctx->mutex);
         if (new_ctx->options.config_file_path) {
             free((void*)new_ctx->options.config_file_path);
         }
         if (new_ctx->options.provider_name) {
             free((void*)new_ctx->options.provider_name);
         }
         polycall_core_free(ctx, new_ctx);
         return result;
     }
     
     // Initialize default configuration
     initialize_default_configuration(ctx, ffi_ctx, new_ctx);
     
     // Load configuration from file if enabled
     if (new_ctx->options.enable_persistence && new_ctx->options.config_file_path) {
         result = polycall_ffi_config_load_file(ctx, ffi_ctx, new_ctx, new_ctx->options.config_file_path);
         // We continue even if loading fails, just use defaults
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               result,
                               POLYCALL_ERROR_SEVERITY_WARNING,
                               "Failed to load configuration from %s, using defaults",
                               new_ctx->options.config_file_path);
         }
     }
     
     *config_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up configuration module
  */
 void polycall_ffi_config_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     if (!ctx || !config_ctx) {
         return;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Save configuration if persistence is enabled
     if (config_ctx->options.enable_persistence && config_ctx->options.config_file_path) {
         polycall_ffi_config_save_file(ctx, ffi_ctx, config_ctx, config_ctx->options.config_file_path);
     }
     
     // Clean up sections
     for (int i = 0; i <= POLYCALL_CONFIG_SECTION_USER; i++) {
         config_entry_t* entry = config_ctx->sections[i].entries;
         
         while (entry) {
             config_entry_t* next = entry->next;
             free_config_value(ctx, &entry->value);
             polycall_core_free(ctx, entry);
             entry = next;
         }
     }
     
     // Clean up providers
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (config_ctx->providers[i].cleanup) {
             config_ctx->providers[i].cleanup(ctx, config_ctx->providers[i].user_data);
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     
     // Destroy mutex
     pthread_mutex_destroy(&config_ctx->mutex);
     
     // Free options strings
     if (config_ctx->options.config_file_path) {
         free((void*)config_ctx->options.config_file_path);
     }
     if (config_ctx->options.provider_name) {
         free((void*)config_ctx->options.provider_name);
     }
     
     // Free context
     polycall_core_free(ctx, config_ctx);
 }
 
 /**
  * @brief Register configuration provider
  */
 polycall_core_error_t polycall_ffi_config_register_provider(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     const polycall_config_provider_t* provider
 ) {
     if (!ctx || !config_ctx || !provider || !provider->provider_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Check if provider already exists
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (strcmp(config_ctx->providers[i].provider_name, provider->provider_name) == 0) {
             pthread_mutex_unlock(&config_ctx->mutex);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                               POLYCALL_ERROR_SEVERITY_WARNING,
                               "Configuration provider '%s' already registered",
                               provider->provider_name);
             return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
         }
     }
     
     // Check capacity
     if (config_ctx->provider_count >= MAX_CONFIG_PROVIDERS) {
         pthread_mutex_unlock(&config_ctx->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_RESOURCES,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Maximum number of configuration providers reached");
         return POLYCALL_CORE_ERROR_OUT_OF_RESOURCES;
     }
     
     // Register provider
     memcpy(&config_ctx->providers[config_ctx->provider_count], provider, sizeof(polycall_config_provider_t));
     config_ctx->provider_count++;
     
     // Initialize provider
     if (provider->initialize) {
         polycall_core_error_t result = provider->initialize(ctx, provider->user_data);
         if (result != POLYCALL_CORE_SUCCESS) {
             // Rollback registration
             config_ctx->provider_count--;
             pthread_mutex_unlock(&config_ctx->mutex);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to initialize configuration provider '%s'",
                               provider->provider_name);
             return result;
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get boolean configuration value
  */
 bool polycall_ffi_config_get_bool(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     bool default_value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return default_value;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Look for entry in memory
     config_entry_t* entry = find_config_entry(&config_ctx->sections[section_id], key);
     if (entry && entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_BOOL) {
         bool result = entry->value.value.bool_value;
         pthread_mutex_unlock(&config_ctx->mutex);
         return result;
     }
     
     // Try to load from providers
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (config_ctx->providers[i].load) {
             polycall_config_value_t value;
             memset(&value, 0, sizeof(value));
             
             if (config_ctx->providers[i].load(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &value) == POLYCALL_CORE_SUCCESS) {
                 if (value.type == POLYCALL_CONFIG_VALUE_TYPE_BOOL) {
                     bool result = value.value.bool_value;
                     
                     // Cache the result
                     polycall_config_value_t cached_value;
                     cached_value.type = POLYCALL_CONFIG_VALUE_TYPE_BOOL;
                     cached_value.value.bool_value = result;
                     set_config_value(ctx, config_ctx, section_id, key, &cached_value);
                     
                     pthread_mutex_unlock(&config_ctx->mutex);
                     return result;
                 }
                 free_config_value(ctx, &value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return default_value;
 }
 
 /**
  * @brief Get integer configuration value
  */
 int64_t polycall_ffi_config_get_int(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     int64_t default_value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return default_value;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Look for entry in memory
     config_entry_t* entry = find_config_entry(&config_ctx->sections[section_id], key);
     if (entry && entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_INT) {
         int64_t result = entry->value.value.int_value;
         pthread_mutex_unlock(&config_ctx->mutex);
         return result;
     }
     
     // Try to load from providers
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (config_ctx->providers[i].load) {
             polycall_config_value_t value;
             memset(&value, 0, sizeof(value));
             
             if (config_ctx->providers[i].load(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &value) == POLYCALL_CORE_SUCCESS) {
                 if (value.type == POLYCALL_CONFIG_VALUE_TYPE_INT) {
                     int64_t result = value.value.int_value;
                     
                     // Cache the result
                     polycall_config_value_t cached_value;
                     cached_value.type = POLYCALL_CONFIG_VALUE_TYPE_INT;
                     cached_value.value.int_value = result;
                     set_config_value(ctx, config_ctx, section_id, key, &cached_value);
                     
                     pthread_mutex_unlock(&config_ctx->mutex);
                     return result;
                 }
                 free_config_value(ctx, &value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return default_value;
 }
 
 /**
  * @brief Get floating-point configuration value
  */
 double polycall_ffi_config_get_float(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     double default_value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return default_value;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Look for entry in memory
     config_entry_t* entry = find_config_entry(&config_ctx->sections[section_id], key);
     if (entry && entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_FLOAT) {
         double result = entry->value.value.float_value;
         pthread_mutex_unlock(&config_ctx->mutex);
         return result;
     }
     
     // Try to load from providers
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (config_ctx->providers[i].load) {
             polycall_config_value_t value;
             memset(&value, 0, sizeof(value));
             
             if (config_ctx->providers[i].load(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &value) == POLYCALL_CORE_SUCCESS) {
                 if (value.type == POLYCALL_CONFIG_VALUE_TYPE_FLOAT) {
                     double result = value.value.float_value;
                     
                     // Cache the result
                     polycall_config_value_t cached_value;
                     cached_value.type = POLYCALL_CONFIG_VALUE_TYPE_FLOAT;
                     cached_value.value.float_value = result;
                     set_config_value(ctx, config_ctx, section_id, key, &cached_value);
                     
                     pthread_mutex_unlock(&config_ctx->mutex);
                     return result;
                 }
                 free_config_value(ctx, &value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return default_value;
 }
 
 /**
  * @brief Get string configuration value
  */
 char* polycall_ffi_config_get_string(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const char* default_value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return default_value ? strdup(default_value) : NULL;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Look for entry in memory
     config_entry_t* entry = find_config_entry(&config_ctx->sections[section_id], key);
     if (entry && entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_STRING && entry->value.value.string_value) {
         char* result = strdup(entry->value.value.string_value);
         pthread_mutex_unlock(&config_ctx->mutex);
         return result;
     }
     
     // Try to load from providers
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (config_ctx->providers[i].load) {
             polycall_config_value_t value;
             memset(&value, 0, sizeof(value));
             
             if (config_ctx->providers[i].load(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &value) == POLYCALL_CORE_SUCCESS) {
                 if (value.type == POLYCALL_CONFIG_VALUE_TYPE_STRING && value.value.string_value) {
                     char* result = strdup(value.value.string_value);
                     
                     // Cache the result
                     polycall_config_value_t cached_value;
                     cached_value.type = POLYCALL_CONFIG_VALUE_TYPE_STRING;
                     cached_value.value.string_value = strdup(value.value.string_value);
                     set_config_value(ctx, config_ctx, section_id, key, &cached_value);
                     
                     free_config_value(ctx, &value);
                     pthread_mutex_unlock(&config_ctx->mutex);
                     return result;
                 }
                 free_config_value(ctx, &value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return default_value ? strdup(default_value) : NULL;
 }
 
 /**
  * @brief Get object configuration value
  */
 void* polycall_ffi_config_get_object(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return NULL;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Look for entry in memory
     config_entry_t* entry = find_config_entry(&config_ctx->sections[section_id], key);
     if (entry && entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_OBJECT) {
         void* result = entry->value.value.object_value;
         pthread_mutex_unlock(&config_ctx->mutex);
         return result;
     }
     
     // Try to load from providers
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (config_ctx->providers[i].load) {
             polycall_config_value_t value;
             memset(&value, 0, sizeof(value));
             
             if (config_ctx->providers[i].load(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &value) == POLYCALL_CORE_SUCCESS) {
                 if (value.type == POLYCALL_CONFIG_VALUE_TYPE_OBJECT) {
                     void* result = value.value.object_value;
                     
                     // Cache the result (we don't free the object here)
                     value.object_free = NULL;
                     set_config_value(ctx, config_ctx, section_id, key, &value);
                     
                     pthread_mutex_unlock(&config_ctx->mutex);
                     return result;
                 }
                 free_config_value(ctx, &value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return NULL;
 }
 
 /**
  * @brief Set boolean configuration value
  */
 polycall_core_error_t polycall_ffi_config_set_bool(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     bool value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Prepare value
     polycall_config_value_t config_value;
     memset(&config_value, 0, sizeof(config_value));
     config_value.type = POLYCALL_CONFIG_VALUE_TYPE_BOOL;
     config_value.value.bool_value = value;
     
     // Set value
     polycall_core_error_t result = set_config_value(ctx, config_ctx, section_id, key, &config_value);
     
     // Save to all providers if successful
     if (result == POLYCALL_CORE_SUCCESS && config_ctx->options.enable_persistence) {
         for (size_t i = 0; i < config_ctx->provider_count; i++) {
             if (config_ctx->providers[i].save) {
                 config_ctx->providers[i].save(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &config_value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return result;
 }
 
 /**
  * @brief Set integer configuration value
  */
 polycall_core_error_t polycall_ffi_config_set_int(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     int64_t value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Prepare value
     polycall_config_value_t config_value;
     memset(&config_value, 0, sizeof(config_value));
     config_value.type = POLYCALL_CONFIG_VALUE_TYPE_INT;
     config_value.value.int_value = value;
     
     // Set value
     polycall_core_error_t result = set_config_value(ctx, config_ctx, section_id, key, &config_value);
     
     // Save to all providers if successful
     if (result == POLYCALL_CORE_SUCCESS && config_ctx->options.enable_persistence) {
         for (size_t i = 0; i < config_ctx->provider_count; i++) {
             if (config_ctx->providers[i].save) {
                 config_ctx->providers[i].save(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &config_value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return result;
 }
 
 /**
  * @brief Set floating-point configuration value
  */
 polycall_core_error_t polycall_ffi_config_set_float(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     double value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Prepare value
     polycall_config_value_t config_value;
     memset(&config_value, 0, sizeof(config_value));
     config_value.type = POLYCALL_CONFIG_VALUE_TYPE_FLOAT;
     config_value.value.float_value = value;
     
     // Set value
     polycall_core_error_t result = set_config_value(ctx, config_ctx, section_id, key, &config_value);
     
     // Save to all providers if successful
     if (result == POLYCALL_CORE_SUCCESS && config_ctx->options.enable_persistence) {
         for (size_t i = 0; i < config_ctx->provider_count; i++) {
             if (config_ctx->providers[i].save) {
                 config_ctx->providers[i].save(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &config_value);
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return result;
 }
 
 /**
  * @brief Set string configuration value
  */
 polycall_core_error_t polycall_ffi_config_set_string(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const char* value
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Prepare value
     polycall_config_value_t config_value;
     memset(&config_value, 0, sizeof(config_value));
     config_value.type = POLYCALL_CONFIG_VALUE_TYPE_STRING;
     config_value.value.string_value = value ? strdup(value) : NULL;
     
     // Set value
     polycall_core_error_t result = set_config_value(ctx, config_ctx, section_id, key, &config_value);
     
     // Save to all providers if successful
     if (result == POLYCALL_CORE_SUCCESS && config_ctx->options.enable_persistence) {
         for (size_t i = 0; i < config_ctx->provider_count; i++) {
             if (config_ctx->providers[i].save) {
                 config_ctx->providers[i].save(ctx, config_ctx->providers[i].user_data,
                                            section_id, key, &config_value);
             }
         }
     }
     
     // Free the temporary string value (it's been copied in set_config_value)
     if (config_value.value.string_value) {
         free(config_value.value.string_value);
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return result;
 }
 
 /**
  * @brief Set object configuration value
  */
 polycall_core_error_t polycall_ffi_config_set_object(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     void* value,
     void (*object_free)(void* object)
 ) {
     if (!ctx || !config_ctx || !key || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Prepare value
     polycall_config_value_t config_value;
     memset(&config_value, 0, sizeof(config_value));
     config_value.type = POLYCALL_CONFIG_VALUE_TYPE_OBJECT;
     config_value.value.object_value = value;
     config_value.object_free = object_free;
     
     // Set value
     polycall_core_error_t result = set_config_value(ctx, config_ctx, section_id, key, &config_value);
     
     // Object values are typically not saved to providers
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return result;
 }
 
 /**
  * @brief Register configuration change handler
  */
 polycall_core_error_t polycall_ffi_config_register_change_handler(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     polycall_config_change_handler_t handler,
     void* user_data
 ) {
     if (!ctx || !config_ctx || !handler || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!config_ctx->options.enable_change_notification) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_WARNING,
                           "Change notification is disabled");
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Check if handler already exists
     for (size_t i = 0; i < config_ctx->handler_count; i++) {
         if (config_ctx->handlers[i].active &&
             config_ctx->handlers[i].handler == handler &&
             config_ctx->handlers[i].user_data == user_data &&
             config_ctx->handlers[i].section_id == section_id &&
             ((key == NULL && config_ctx->handlers[i].key[0] == '\0') ||
              (key != NULL && strcmp(config_ctx->handlers[i].key, key) == 0))) {
             
             pthread_mutex_unlock(&config_ctx->mutex);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                               POLYCALL_ERROR_SEVERITY_WARNING,
                               "Change handler already registered");
             return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
         }
     }
     
     // Find unused slot or create new one
     size_t handler_index = config_ctx->handler_count;
     for (size_t i = 0; i < config_ctx->handler_count; i++) {
         if (!config_ctx->handlers[i].active) {
             handler_index = i;
             break;
         }
     }
     
     // Check capacity
     if (handler_index >= MAX_CHANGE_HANDLERS) {
         pthread_mutex_unlock(&config_ctx->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_OUT_OF_RESOURCES,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Maximum number of change handlers reached");
         return POLYCALL_CORE_ERROR_OUT_OF_RESOURCES;
     }
     
     // Initialize handler
     if (handler_index == config_ctx->handler_count) {
         config_ctx->handler_count++;
     }
     
     config_ctx->handlers[handler_index].active = true;
     config_ctx->handlers[handler_index].section_id = section_id;
     config_ctx->handlers[handler_index].handler = handler;
     config_ctx->handlers[handler_index].user_data = user_data;
     
     if (key) {
         strncpy(config_ctx->handlers[handler_index].key, key, MAX_CONFIG_KEY_LENGTH - 1);
         config_ctx->handlers[handler_index].key[MAX_CONFIG_KEY_LENGTH - 1] = '\0';
     } else {
         config_ctx->handlers[handler_index].key[0] = '\0';
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Unregister configuration change handler
  */
 polycall_core_error_t polycall_ffi_config_unregister_change_handler(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     polycall_config_change_handler_t handler,
     void* user_data
 ) {
     if (!ctx || !config_ctx || !handler || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Find and remove handler
     bool found = false;
     for (size_t i = 0; i < config_ctx->handler_count; i++) {
         if (config_ctx->handlers[i].active &&
             config_ctx->handlers[i].handler == handler &&
             config_ctx->handlers[i].user_data == user_data &&
             config_ctx->handlers[i].section_id == section_id &&
             ((key == NULL && config_ctx->handlers[i].key[0] == '\0') ||
              (key != NULL && strcmp(config_ctx->handlers[i].key, key) == 0))) {
             
             config_ctx->handlers[i].active = false;
             found = true;
             break;
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     
     if (!found) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_WARNING,
                           "Change handler not found");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Load configuration from file
  */
 polycall_core_error_t polycall_ffi_config_load_file(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!ctx || !config_ctx || !file_path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Find a provider that supports file loading
     bool found_provider = false;
     polycall_core_error_t load_result = POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     
     // Try the file provider first if available
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (strcmp(config_ctx->providers[i].provider_name, "file") == 0) {
             // Set provider-specific data to file path
             config_ctx->providers[i].user_data = (void*)file_path;
             
             // Initialize just in case it's not already initialized
             if (config_ctx->providers[i].initialize) {
                 config_ctx->providers[i].initialize(ctx, config_ctx->providers[i].user_data);
             }
             
             // Special handling for file provider - load configuration from all sections
             for (int section = 0; section <= POLYCALL_CONFIG_SECTION_USER; section++) {
                 // Get all keys from this section
                 if (config_ctx->providers[i].enumerate) {
                     struct {
                         polycall_core_context_t* ctx;
                         polycall_ffi_config_context_t* config_ctx;
                         polycall_config_provider_t* provider;
                         polycall_config_section_t section;
                     } enum_data = { ctx, config_ctx, &config_ctx->providers[i], section };
                     
                     config_ctx->providers[i].enumerate(
                         ctx, 
                         config_ctx->providers[i].user_data,
                         section,
                         // Callback that loads each key
                         [](const char* key, void* data) {
                             auto enum_ctx = (decltype(enum_data)*)data;
                             
                             // Load value for this key
                             polycall_config_value_t value;
                             memset(&value, 0, sizeof(value));
                             
                             if (enum_ctx->provider->load(
                                     enum_ctx->ctx, 
                                     enum_ctx->provider->user_data,
                                     enum_ctx->section, 
                                     key, 
                                     &value) == POLYCALL_CORE_SUCCESS) {
                                 // Set value in memory
                                 set_config_value(
                                     enum_ctx->ctx, 
                                     enum_ctx->config_ctx, 
                                     enum_ctx->section, 
                                     key, 
                                     &value);
                                 
                                 // Free value if needed
                                 free_config_value(enum_ctx->ctx, &value);
                             }
                         },
                         &enum_data
                     );
                 }
             }
             
             found_provider = true;
             load_result = POLYCALL_CORE_SUCCESS;
             break;
         }
     }
     
     // If no file provider found, try other providers
     if (!found_provider) {
         // Find another provider that can handle the file
         for (size_t i = 0; i < config_ctx->provider_count; i++) {
             if (config_ctx->providers[i].initialize && 
                 config_ctx->providers[i].load && 
                 config_ctx->providers[i].enumerate) {
                 
                 // Set provider-specific data to file path if possible
                 if (config_ctx->providers[i].user_data == NULL) {
                     config_ctx->providers[i].user_data = (void*)file_path;
                 }
                 
                 // Initialize just in case it's not already initialized
                 config_ctx->providers[i].initialize(ctx, config_ctx->providers[i].user_data);
                 
                 // Load configuration from all sections
                 for (int section = 0; section <= POLYCALL_CONFIG_SECTION_USER; section++) {
                     // Get all keys from this section
                     struct {
                         polycall_core_context_t* ctx;
                         polycall_ffi_config_context_t* config_ctx;
                         polycall_config_provider_t* provider;
                         polycall_config_section_t section;
                     } enum_data = { ctx, config_ctx, &config_ctx->providers[i], section };
                     
                     config_ctx->providers[i].enumerate(
                         ctx, 
                         config_ctx->providers[i].user_data,
                         section,
                         // Callback that loads each key
                         [](const char* key, void* data) {
                             auto enum_ctx = (decltype(enum_data)*)data;
                             
                             // Load value for this key
                             polycall_config_value_t value;
                             memset(&value, 0, sizeof(value));
                             
                             if (enum_ctx->provider->load(
                                     enum_ctx->ctx, 
                                     enum_ctx->provider->user_data,
                                     enum_ctx->section, 
                                     key, 
                                     &value) == POLYCALL_CORE_SUCCESS) {
                                 // Set value in memory
                                 set_config_value(
                                     enum_ctx->ctx, 
                                     enum_ctx->config_ctx, 
                                     enum_ctx->section, 
                                     key, 
                                     &value);
                                 
                                 // Free value if needed
                                 free_config_value(enum_ctx->ctx, &value);
                             }
                         },
                         &enum_data
                     );
                 }
                 
                 found_provider = true;
                 load_result = POLYCALL_CORE_SUCCESS;
                 break;
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     
     if (!found_provider) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "No suitable configuration provider found for file loading");
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     return load_result;
 }
 
 /**
  * @brief Save configuration to file
  */
 polycall_core_error_t polycall_ffi_config_save_file(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!ctx || !config_ctx || !file_path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Find a provider that supports file saving
     bool found_provider = false;
     polycall_core_error_t save_result = POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     
     // Try the file provider first if available
     for (size_t i = 0; i < config_ctx->provider_count; i++) {
         if (strcmp(config_ctx->providers[i].provider_name, "file") == 0) {
             // Set provider-specific data to file path
             config_ctx->providers[i].user_data = (void*)file_path;
             
             // Initialize just in case it's not already initialized
             if (config_ctx->providers[i].initialize) {
                 config_ctx->providers[i].initialize(ctx, config_ctx->providers[i].user_data);
             }
             
             // Special handling for file provider - save configuration from all sections
             for (int section = 0; section <= POLYCALL_CONFIG_SECTION_USER; section++) {
                 // Process all entries in this section
                 config_entry_t* entry = config_ctx->sections[section].entries;
                 while (entry) {
                     // Save each entry if it's not an object
                     if (entry->value.type != POLYCALL_CONFIG_VALUE_TYPE_OBJECT || 
                         (entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_OBJECT && entry->value.value.object_value != NULL)) {
                         
                         config_ctx->providers[i].save(
                             ctx,
                             config_ctx->providers[i].user_data,
                             section,
                             entry->key,
                             &entry->value
                         );
                     }
                     
                     entry = entry->next;
                 }
             }
             
             found_provider = true;
             save_result = POLYCALL_CORE_SUCCESS;
             break;
         }
     }
     
     // If no file provider found, try other providers
     if (!found_provider) {
         // Find another provider that can handle the file
         for (size_t i = 0; i < config_ctx->provider_count; i++) {
             if (config_ctx->providers[i].initialize && 
                 config_ctx->providers[i].save) {
                 
                 // Set provider-specific data to file path if possible
                 if (config_ctx->providers[i].user_data == NULL) {
                     config_ctx->providers[i].user_data = (void*)file_path;
                 }
                 
                 // Initialize just in case it's not already initialized
                 config_ctx->providers[i].initialize(ctx, config_ctx->providers[i].user_data);
                 
                 // Save configuration from all sections
                 for (int section = 0; section <= POLYCALL_CONFIG_SECTION_USER; section++) {
                     // Process all entries in this section
                     config_entry_t* entry = config_ctx->sections[section].entries;
                     while (entry) {
                         // Save each entry if it's not an object
                         if (entry->value.type != POLYCALL_CONFIG_VALUE_TYPE_OBJECT || 
                             (entry->value.type == POLYCALL_CONFIG_VALUE_TYPE_OBJECT && entry->value.value.object_value != NULL)) {
                             
                             config_ctx->providers[i].save(
                                 ctx,
                                 config_ctx->providers[i].user_data,
                                 section,
                                 entry->key,
                                 &entry->value
                             );
                         }
                         
                         entry = entry->next;
                     }
                 }
                 
                 found_provider = true;
                 save_result = POLYCALL_CORE_SUCCESS;
                 break;
             }
         }
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     
     if (!found_provider) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "No suitable configuration provider found for file saving");
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     return save_result;
 }
 
 /**
  * @brief Reset configuration to defaults
  */
 polycall_core_error_t polycall_ffi_config_reset_defaults(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id
 ) {
     if (!ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Reset specified section or all sections
     if (section_id <= POLYCALL_CONFIG_SECTION_USER) {
         // Reset specific section
         config_entry_t* entry = config_ctx->sections[section_id].entries;
         
         while (entry) {
             config_entry_t* next = entry->next;
             free_config_value(ctx, &entry->value);
             polycall_core_free(ctx, entry);
             entry = next;
         }
         
         config_ctx->sections[section_id].entries = NULL;
     } else {
         // Reset all sections
         for (int i = 0; i <= POLYCALL_CONFIG_SECTION_USER; i++) {
             config_entry_t* entry = config_ctx->sections[i].entries;
             
             while (entry) {
                 config_entry_t* next = entry->next;
                 free_config_value(ctx, &entry->value);
                 polycall_core_free(ctx, entry);
                 entry = next;
             }
             
             config_ctx->sections[i].entries = NULL;
         }
     }
     
     // Reinitialize default configuration
     initialize_default_configuration(ctx, ffi_ctx, config_ctx);
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Apply configuration to FFI system
  */
 polycall_core_error_t polycall_ffi_config_apply(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     if (!ctx || !ffi_ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&config_ctx->mutex);
     
     // Apply core configuration
     ffi_core_apply_config(ctx, ffi_ctx, config_ctx);
     
     // Apply security configuration if available
     if (ffi_ctx->security_ctx) {
         security_apply_config(ctx, ffi_ctx, ffi_ctx->security_ctx, config_ctx);
     }
     
     // Apply memory configuration if available
     if (ffi_ctx->memory_mgr) {
         memory_bridge_apply_config(ctx, ffi_ctx, ffi_ctx->memory_mgr, config_ctx);
     }
     
     // Apply type system configuration if available
     if (ffi_ctx->type_ctx) {
         type_system_apply_config(ctx, ffi_ctx, ffi_ctx->type_ctx, config_ctx);
     }
     
     // Apply performance configuration if available
     if (ffi_ctx->perf_mgr) {
         performance_apply_config(ctx, ffi_ctx, ffi_ctx->perf_mgr, config_ctx);
     }
     
     pthread_mutex_unlock(&config_ctx->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a default configuration options
  */
 polycall_ffi_config_options_t polycall_ffi_config_create_default_options(void) {
     polycall_ffi_config_options_t options;
     
     options.enable_persistence = true;
     options.enable_change_notification = true;
     options.validate_configuration = true;
     options.config_file_path = "polycall_ffi.conf";
     options.provider_name = "file";
     options.provider_data = NULL;
     
     return options;
 }
 
 /*------------------------------------------------------------------------*/
 /* Internal helper functions */
 /*------------------------------------------------------------------------*/
 
 /**
  * @brief Register default providers
  */
 static polycall_core_error_t register_default_providers(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Register file provider
     polycall_config_provider_t file_provider;
     memset(&file_provider, 0, sizeof(file_provider));
     file_provider.provider_name = "file";
     file_provider.user_data = NULL;
     
     // File provider functions (would be implemented elsewhere)
     file_provider.initialize = file_provider_initialize;
     file_provider.cleanup = file_provider_cleanup;
     file_provider.load = file_provider_load;
     file_provider.save = file_provider_save;
     file_provider.exists = file_provider_exists;
     file_provider.enumerate = file_provider_enumerate;
     
     polycall_core_error_t result = polycall_ffi_config_register_provider(
         ctx, ffi_ctx, config_ctx, &file_provider);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Register memory provider (always available)
     polycall_config_provider_t memory_provider;
     memset(&memory_provider, 0, sizeof(memory_provider));
     memory_provider.provider_name = "memory";
     memory_provider.user_data = NULL;
     
     // Memory provider doesn't need most functions, as values are stored in memory directly
     memory_provider.initialize = NULL;
     memory_provider.cleanup = NULL;
     memory_provider.load = NULL;
     memory_provider.save = NULL;
     memory_provider.exists = NULL;
     memory_provider.enumerate = NULL;
     
     result = polycall_ffi_config_register_provider(
         ctx, ffi_ctx, config_ctx, &memory_provider);
     
     return result;
 }
 
 /**
  * @brief Initialize default configuration
  */
 static void initialize_default_configuration(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Core FFI configuration
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_CORE, "secure_mode", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_CORE, "strict_types", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_CORE, "memory_isolation", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_CORE, "async_calls", false);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_CORE, "debug_mode", false);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_CORE, "trace_calls", false);
     
     // Security configuration
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_SECURITY, "security_level", POLYCALL_SECURITY_LEVEL_MEDIUM);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_SECURITY, "isolation_level", POLYCALL_ISOLATION_LEVEL_FUNCTION);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_SECURITY, "audit_level", POLYCALL_AUDIT_LEVEL_WARNING);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_SECURITY, "default_deny", true);
     
     // Memory management configuration
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_MEMORY, "shared_pool_size", 1024 * 1024); // 1MB
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_MEMORY, "use_cleanup_handlers", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_MEMORY, "track_allocations", true);
     
     // Type system configuration
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_TYPE, "type_capacity", 256);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_TYPE, "rule_capacity", 128);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_TYPE, "auto_register_primitives", true);
     
     // Performance configuration
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PERFORMANCE, "enable_call_caching", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PERFORMANCE, "enable_type_caching", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PERFORMANCE, "enable_call_batching", false);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PERFORMANCE, "optimization_level", POLYCALL_OPT_LEVEL_MODERATE);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PERFORMANCE, "cache_size", 1024);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PERFORMANCE, "cache_ttl_ms", 60000); // 1 minute
     
     // Protocol bridge configuration
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PROTOCOL, "enable_message_compression", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PROTOCOL, "enable_streaming", false);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PROTOCOL, "enable_fragmentation", true);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PROTOCOL, "max_message_size", 1024 * 1024); // 1MB
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PROTOCOL, "timeout_ms", 30000); // 30 seconds
     
     // C bridge configuration
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_C, "use_stdcall", false);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_C, "enable_var_args", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_C, "thread_safe", true);
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_C, "max_function_count", 1024);
     
     // JVM bridge configuration
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JVM, "create_vm_if_needed", true);
     polycall_ffi_config_set_string(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JVM, "classpath", ".");
     polycall_ffi_config_set_string(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JVM, "bridge_class", "com.polycall.JavaBridge");
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JVM, "enable_exception_handler", true);
     
     // JavaScript bridge configuration
     polycall_ffi_config_set_int(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JS, "runtime_type", POLYCALL_JS_RUNTIME_NODE);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JS, "enable_promise_integration", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JS, "enable_callback_conversion", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_JS, "enable_object_proxying", true);
     
     // Python bridge configuration
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PYTHON, "initialize_interpreter", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PYTHON, "enable_numpy_integration", true);
     polycall_ffi_config_set_bool(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PYTHON, "enable_error_translation", true);
     polycall_ffi_config_set_string(ctx, ffi_ctx, config_ctx, 
         POLYCALL_CONFIG_SECTION_PYTHON, "module_path", ".");
 }
 
 /**
  * @brief Notify change handlers
  */
 static void notify_change_handlers(
     polycall_core_context_t* ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const polycall_config_value_t* old_value,
     const polycall_config_value_t* new_value
 ) {
     if (!config_ctx->options.enable_change_notification) {
         return;
     }
     
     // Copy handlers to temporary array to avoid issues if handlers modify the list
     struct {
         polycall_config_change_handler_t handler;
         void* user_data;
     } handlers_to_notify[MAX_CHANGE_HANDLERS];
     
     size_t notify_count = 0;
     
     // Find handlers that match this section/key
     for (size_t i = 0; i < config_ctx->handler_count && notify_count < MAX_CHANGE_HANDLERS; i++) {
         if (config_ctx->handlers[i].active && 
             (config_ctx->handlers[i].section_id == section_id || config_ctx->handlers[i].section_id == (polycall_config_section_t)-1) &&
             (config_ctx->handlers[i].key[0] == '\0' || strcmp(config_ctx->handlers[i].key, key) == 0)) {
             
             handlers_to_notify[notify_count].handler = config_ctx->handlers[i].handler;
             handlers_to_notify[notify_count].user_data = config_ctx->handlers[i].user_data;
             notify_count++;
         }
     }
     
     // Call handlers outside of the lock
     for (size_t i = 0; i < notify_count; i++) {
         handlers_to_notify[i].handler(
             ctx, 
             section_id, 
             key, 
             old_value, 
             new_value, 
             handlers_to_notify[i].user_data
         );
     }
 }
 
 /**
  * @brief Free a configuration value
  */
 static void free_config_value(
     polycall_core_context_t* ctx,
     polycall_config_value_t* value
 ) {
     if (!value) {
         return;
     }
     
     switch (value->type) {
         case POLYCALL_CONFIG_VALUE_TYPE_STRING:
             if (value->value.string_value) {
                 free(value->value.string_value);
                 value->value.string_value = NULL;
             }
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_OBJECT:
             if (value->value.object_value && value->object_free) {
                 value->object_free(value->value.object_value);
                 value->value.object_value = NULL;
             }
             break;
             
         default:
             // Nothing to free for other types
             break;
     }
 }
 
 /**
  * @brief Find configuration entry
  */
 static config_entry_t* find_config_entry(
     config_section_t* section,
     const char* key
 ) {
     if (!section || !key) {
         return NULL;
     }
     
     config_entry_t* entry = section->entries;
     
     while (entry) {
         if (strcmp(entry->key, key) == 0) {
             return entry;
         }
         
         entry = entry->next;
     }
     
     return NULL;
 }
 
 /**
  * @brief Set configuration value
  */
 static polycall_core_error_t set_config_value(
     polycall_core_context_t* ctx,
     polycall_ffi_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const polycall_config_value_t* value
 ) {
     if (!ctx || !config_ctx || !key || !value || section_id > POLYCALL_CONFIG_SECTION_USER || 
         strlen(key) >= MAX_CONFIG_KEY_LENGTH) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find existing entry or create a new one
     config_entry_t* entry = find_config_entry(&config_ctx->sections[section_id], key);
     polycall_config_value_t old_value;
     bool is_new = false;
     
     if (entry) {
         // Save old value for notification
         memcpy(&old_value, &entry->value, sizeof(old_value));
         
         // Free old value
         free_config_value(ctx, &entry->value);
     } else {
         // Create new entry
         entry = polycall_core_malloc(ctx, sizeof(config_entry_t));
         if (!entry) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to allocate configuration entry");
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Initialize entry
         memset(entry, 0, sizeof(config_entry_t));
         strncpy(entry->key, key, MAX_CONFIG_KEY_LENGTH - 1);
         entry->key[MAX_CONFIG_KEY_LENGTH - 1] = '\0';
         
         // Add to section
         entry->next = config_ctx->sections[section_id].entries;
         config_ctx->sections[section_id].entries = entry;
         
         // Initialize old value for notification
         memset(&old_value, 0, sizeof(old_value));
         is_new = true;
     }
     
     // Set new value
     switch (value->type) {
         case POLYCALL_CONFIG_VALUE_TYPE_BOOL:
             entry->value.type = POLYCALL_CONFIG_VALUE_TYPE_BOOL;
             entry->value.value.bool_value = value->value.bool_value;
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_INT:
             entry->value.type = POLYCALL_CONFIG_VALUE_TYPE_INT;
             entry->value.value.int_value = value->value.int_value;
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_FLOAT:
             entry->value.type = POLYCALL_CONFIG_VALUE_TYPE_FLOAT;
             entry->value.value.float_value = value->value.float_value;
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_STRING:
             entry->value.type = POLYCALL_CONFIG_VALUE_TYPE_STRING;
             entry->value.value.string_value = value->value.string_value ? 
                 strdup(value->value.string_value) : NULL;
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_OBJECT:
             entry->value.type = POLYCALL_CONFIG_VALUE_TYPE_OBJECT;
             entry->value.value.object_value = value->value.object_value;
             entry->value.object_free = value->object_free;
             break;
             
         default:
             // Unknown type
             if (is_new) {
                 // Remove newly created entry
                 config_ctx->sections[section_id].entries = entry->next;
                 polycall_core_free(ctx, entry);
             } else {
                 // Restore old value
                 memcpy(&entry->value, &old_value, sizeof(entry->value));
             }
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Notify change handlers if value changed
     if (!is_new && config_ctx->options.enable_change_notification) {
         // Check if value actually changed
         bool changed = false;
         
         switch (value->type) {
             case POLYCALL_CONFIG_VALUE_TYPE_BOOL:
                 changed = (old_value.type != POLYCALL_CONFIG_VALUE_TYPE_BOOL || 
                           old_value.value.bool_value != value->value.bool_value);
                 break;
                 
             case POLYCALL_CONFIG_VALUE_TYPE_INT:
                 changed = (old_value.type != POLYCALL_CONFIG_VALUE_TYPE_INT || 
                           old_value.value.int_value != value->value.int_value);
                 break;
                 
             case POLYCALL_CONFIG_VALUE_TYPE_FLOAT:
                 changed = (old_value.type != POLYCALL_CONFIG_VALUE_TYPE_FLOAT || 
                           old_value.value.float_value != value->value.float_value);
                 break;
                 
             case POLYCALL_CONFIG_VALUE_TYPE_STRING:
                 changed = (old_value.type != POLYCALL_CONFIG_VALUE_TYPE_STRING || 
                           (old_value.value.string_value == NULL && value->value.string_value != NULL) ||
                           (old_value.value.string_value != NULL && value->value.string_value == NULL) ||
                           (old_value.value.string_value != NULL && value->value.string_value != NULL && 
                            strcmp(old_value.value.string_value, value->value.string_value) != 0));
                 break;
                 
             case POLYCALL_CONFIG_VALUE_TYPE_OBJECT:
                 changed = (old_value.type != POLYCALL_CONFIG_VALUE_TYPE_OBJECT || 
                           old_value.value.object_value != value->value.object_value);
                 break;
                 
             default:
                 // Unknown type
                 changed = true;
                 break;
         }
         
         if (changed) {
             notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, value);
         }
     }
     
     // Free old value
     if (!is_new) {
         free_config_value(ctx, &old_value);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /*------------------------------------------------------------------------*/
 /* File provider implementation */
 /*------------------------------------------------------------------------*/
 
 /**
  * @brief Initialize file provider
  */
 static polycall_core_error_t file_provider_initialize(
     polycall_core_context_t* ctx,
     void* user_data
 ) {
     // The file path is passed in user_data
     if (!user_data) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "No file path specified for file provider");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Nothing else to initialize
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up file provider
  */
 static void file_provider_cleanup(
     polycall_core_context_t* ctx,
     void* user_data
 ) {
     // Nothing to clean up
 }
 
 /**
  * @brief Load configuration from file
  */
 static polycall_core_error_t file_provider_load(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     const char* key,
     polycall_config_value_t* value
 ) {
     if (!ctx || !user_data || !key || !value || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* file_path = (const char*)user_data;
     
     // Open file
     FILE* file = fopen(file_path, "r");
     if (!file) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_IO_ERROR,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to open configuration file %s", file_path);
         return POLYCALL_CORE_ERROR_IO_ERROR;
     }
     
     // Find section and key
     char line[1024];
     char section_name[64];
     bool in_section = false;
     bool found = false;
     
     // Convert section ID to name
     const char* section_names[] = {
         "Core",
         "Security",
         "Memory",
         "Type",
         "Performance",
         "Protocol",
         "C",
         "JVM",
         "JavaScript",
         "Python"
     };
     
     if (section_id < sizeof(section_names) / sizeof(section_names[0])) {
         strncpy(section_name, section_names[section_id], sizeof(section_name) - 1);
         section_name[sizeof(section_name) - 1] = '\0';
     } else if (section_id >= POLYCALL_CONFIG_SECTION_USER) {
         snprintf(section_name, sizeof(section_name), "User%d", 
                  (int)(section_id - POLYCALL_CONFIG_SECTION_USER));
     } else {
         snprintf(section_name, sizeof(section_name), "Unknown%d", (int)section_id);
     }
     
     while (fgets(line, sizeof(line), file) && !found) {
         // Skip leading whitespace
         char* ptr = line;
         while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
             ptr++;
         }
         
         // Skip empty lines and comments
         if (*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#' || *ptr == ';') {
             continue;
         }
         
         // Check for section
         if (*ptr == '[') {
             ptr++;
             char* end = strchr(ptr, ']');
             if (end) {
                 *end = '\0';
                 in_section = (strcmp(ptr, section_name) == 0);
             }
             continue;
         }
         
         // Process key-value pair if in the correct section
         if (in_section) {
             char* equals = strchr(ptr, '=');
             if (equals) {
                 // Extract key
                 *equals = '\0';
                 char* end_key = equals - 1;
                 
                 // Trim trailing whitespace from key
                 while (end_key > ptr && (*end_key == ' ' || *end_key == '\t')) {
                     *end_key = '\0';
                     end_key--;
                 }
                 
                 // Check if this is the key we're looking for
                 if (strcmp(ptr, key) == 0) {
                     // Extract value
                     char* val_ptr = equals + 1;
                     
                     // Trim leading whitespace from value
                     while (*val_ptr && (*val_ptr == ' ' || *val_ptr == '\t')) {
                         val_ptr++;
                     }
                     
                     // Trim trailing whitespace and newline from value
                     char* end_val = val_ptr + strlen(val_ptr) - 1;
                     while (end_val >= val_ptr && 
                            (*end_val == ' ' || *end_val == '\t' || 
                             *end_val == '\n' || *end_val == '\r')) {
                         *end_val = '\0';
                         end_val--;
                     }
                     
                     // Determine value type and parse
                     if (strcmp(val_ptr, "true") == 0 || strcmp(val_ptr, "yes") == 0 || 
                         strcmp(val_ptr, "on") == 0 || strcmp(val_ptr, "1") == 0) {
                         // Boolean true
                         value->type = POLYCALL_CONFIG_VALUE_TYPE_BOOL;
                         value->value.bool_value = true;
                         found = true;
                     } else if (strcmp(val_ptr, "false") == 0 || strcmp(val_ptr, "no") == 0 || 
                                strcmp(val_ptr, "off") == 0 || strcmp(val_ptr, "0") == 0) {
                         // Boolean false
                         value->type = POLYCALL_CONFIG_VALUE_TYPE_BOOL;
                         value->value.bool_value = false;
                         found = true;
                     } else if (strchr(val_ptr, '.') || strchr(val_ptr, 'e') || strchr(val_ptr, 'E')) {
                         // Floating-point
                         char* end;
                         double dbl_val = strtod(val_ptr, &end);
                         
                         if (*end == '\0') {
                             value->type = POLYCALL_CONFIG_VALUE_TYPE_FLOAT;
                             value->value.float_value = dbl_val;
                             found = true;
                         } else {
                             // Not a valid number, treat as string
                             value->type = POLYCALL_CONFIG_VALUE_TYPE_STRING;
                             value->value.string_value = strdup(val_ptr);
                             found = true;
                         }
                     } else {
                         // Try integer
                         char* end;
                         int64_t int_val = strtoll(val_ptr, &end, 0);
                         
                         if (*end == '\0') {
                             value->type = POLYCALL_CONFIG_VALUE_TYPE_INT;
                             value->value.int_value = int_val;
                             found = true;
                         } else {
                             // Not a valid number, treat as string
                             value->type = POLYCALL_CONFIG_VALUE_TYPE_STRING;
                             value->value.string_value = strdup(val_ptr);
                             found = true;
                         }
                     }
                 }
             }
         }
     }
     
     fclose(file);
     
     if (!found) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_NOT_FOUND,
                           POLYCALL_ERROR_SEVERITY_INFO,
                           "Configuration key %s not found in section %s", 
                           key, section_name);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Save configuration to file
  */
 static polycall_core_error_t file_provider_save(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     const char* key,
     const polycall_config_value_t* value
 ) {
     if (!ctx || !user_data || !key || !value || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* file_path = (const char*)user_data;
     
     // Read existing file into memory
     FILE* file = fopen(file_path, "r");
     char** lines = NULL;
     size_t line_count = 0;
     size_t line_capacity = 0;
     
     if (file) {
         char line[1024];
         
         while (fgets(line, sizeof(line), file)) {
             if (line_count >= line_capacity) {
                 line_capacity = line_capacity ? line_capacity * 2 : 64;
                 char** new_lines = realloc(lines, line_capacity * sizeof(char*));
                 if (!new_lines) {
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                                       POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                       POLYCALL_ERROR_SEVERITY_ERROR,
                                       "Failed to allocate memory for file lines");
                     
                     for (size_t i = 0; i < line_count; i++) {
                         free(lines[i]);
                     }
                     free(lines);
                     fclose(file);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 lines = new_lines;
             }
             
             lines[line_count] = strdup(line);
             line_count++;
         }
         
         fclose(file);
     }
     
     // Convert section ID to name
     char section_name[64];
     const char* section_names[] = {
         "Core",
         "Security",
         "Memory",
         "Type",
         "Performance",
         "Protocol",
         "C",
         "JVM",
         "JavaScript",
         "Python"
     };
     
     if (section_id < sizeof(section_names) / sizeof(section_names[0])) {
         strncpy(section_name, section_names[section_id], sizeof(section_name) - 1);
         section_name[sizeof(section_name) - 1] = '\0';
     } else if (section_id >= POLYCALL_CONFIG_SECTION_USER) {
         snprintf(section_name, sizeof(section_name), "User%d", 
                  (int)(section_id - POLYCALL_CONFIG_SECTION_USER));
     } else {
         snprintf(section_name, sizeof(section_name), "Unknown%d", (int)section_id);
     }
     
     // Convert value to string
     char value_str[1024];
     
     switch (value->type) {
         case POLYCALL_CONFIG_VALUE_TYPE_BOOL:
             strncpy(value_str, value->value.bool_value ? "true" : "false", sizeof(value_str) - 1);
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_INT:
             snprintf(value_str, sizeof(value_str), "%lld", (long long)value->value.int_value);
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_FLOAT:
             snprintf(value_str, sizeof(value_str), "%g", value->value.float_value);
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_STRING:
             strncpy(value_str, value->value.string_value ? value->value.string_value : "", 
                    sizeof(value_str) - 1);
             break;
             
         case POLYCALL_CONFIG_VALUE_TYPE_OBJECT:
             // Objects are not saved to file
             if (lines) {
                 for (size_t i = 0; i < line_count; i++) {
                     free(lines[i]);
                 }
                 free(lines);
             }
             return POLYCALL_CORE_SUCCESS;
             
         default:
             // Unknown type
             if (lines) {
                 for (size_t i = 0; i < line_count; i++) {
                     free(lines[i]);
                 }
                 free(lines);
             }
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     value_str[sizeof(value_str) - 1] = '\0';
     
     // Process file
     file = fopen(file_path, "w");
     if (!file) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                           POLYCALL_CORE_ERROR_IO_ERROR,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to open configuration file %s for writing", file_path);
         
         if (lines) {
             for (size_t i = 0; i < line_count; i++) {
                 free(lines[i]);
             }
             free(lines);
         }
         
         return POLYCALL_CORE_ERROR_IO_ERROR;
     }
     
     if (lines) {
         // Update existing file
         bool in_section = false;
         bool found = false;
         
         for (size_t i = 0; i < line_count; i++) {
             char* line = lines[i];
             
             // Skip leading whitespace
             char* ptr = line;
             while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                 ptr++;
             }
             
             // Check for section
             if (*ptr == '[') {
                 char section_line[64];
                 strncpy(section_line, ptr, sizeof(section_line) - 1);
                 section_line[sizeof(section_line) - 1] = '\0';
                 
                 char* end = strchr(section_line, ']');
                 if (end) {
                     *end = '\0';
                     in_section = (strcmp(section_line + 1, section_name) == 0);
                 }
             }
             
             // Process key-value pair if in the correct section
             if (in_section && !found) {
                 char line_copy[1024];
                 strncpy(line_copy, line, sizeof(line_copy) - 1);
                 line_copy[sizeof(line_copy) - 1] = '\0';
                 
                 char* ptr = line_copy;
                 
                 // Skip leading whitespace
                 while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                     ptr++;
                 }
                 
                 // Skip empty lines and comments
                 if (*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#' || *ptr == ';') {
                     fputs(line, file);
                     continue;
                 }
                 
                 // Check if this is a key-value pair
                 char* equals = strchr(ptr, '=');
                 if (equals) {
                     // Extract key
                     *equals = '\0';
                     char* end_key = equals - 1;
                     
                     // Trim trailing whitespace from key
                     while (end_key > ptr && (*end_key == ' ' || *end_key == '\t')) {
                         *end_key = '\0';
                         end_key--;
                     }
                     
                     // Check if this is the key we're looking for
                     if (strcmp(ptr, key) == 0) {
                         // Update value
                         fprintf(file, "%s = %s\n", key, value_str);
                         found = true;
                         continue;
                     }
                 }
             }
             
             // Write line as-is
             fputs(line, file);
         }
         
         // If key not found, add it
         if (!found) {
             // Find or create section
             if (!in_section) {
                 // Look for next section
                 size_t insert_pos = line_count;
                 
                 for (size_t i = 0; i < line_count; i++) {
                     char* line = lines[i];
                     
                     // Skip leading whitespace
                     char* ptr = line;
                     while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                         ptr++;
                     }
                     
                     // Check for section
                     if (*ptr == '[') {
                         char section_line[64];
                         strncpy(section_line, ptr, sizeof(section_line) - 1);
                         section_line[sizeof(section_line) - 1] = '\0';
                         
                         char* end = strchr(section_line, ']');
                         if (end) {
                             *end = '\0';
                             if (strcmp(section_line + 1, section_name) == 0) {
                                 in_section = true;
                                 break;
                             }
                         }
                     }
                 }
                 
                 if (!in_section) {
                     // Add new section
                     fprintf(file, "\n[%s]\n", section_name);
                 }
             }
             
             // Add key-value pair
             fprintf(file, "%s = %s\n", key, value_str);
         }
         
         // Free lines
         for (size_t i = 0; i < line_count; i++) {
             free(lines[i]);
         }
         free(lines);
     } else {
         // Create new file
         fprintf(file, "[%s]\n", section_name);
         fprintf(file, "%s = %s\n", key, value_str);
     }
     
     fclose(file);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Check if configuration exists
  */
 static polycall_core_error_t file_provider_exists(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     const char* key,
     bool* exists
 ) {
     if (!ctx || !user_data || !key || !exists || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* file_path = (const char*)user_data;
     
     // Initialize result
     *exists = false;
     
     // Open file
     FILE* file = fopen(file_path, "r");
     if (!file) {
         return POLYCALL_CORE_SUCCESS; // Not an error, just doesn't exist
     }
     
     // Find section and key
     char line[1024];
     char section_name[64];
     bool in_section = false;
     
     // Convert section ID to name
     const char* section_names[] = {
         "Core",
         "Security",
         "Memory",
         "Type",
         "Performance",
         "Protocol",
         "C",
         "JVM",
         "JavaScript",
         "Python"
     };
     
     if (section_id < sizeof(section_names) / sizeof(section_names[0])) {
         strncpy(section_name, section_names[section_id], sizeof(section_name) - 1);
         section_name[sizeof(section_name) - 1] = '\0';
     } else if (section_id >= POLYCALL_CONFIG_SECTION_USER) {
         snprintf(section_name, sizeof(section_name), "User%d", 
                  (int)(section_id - POLYCALL_CONFIG_SECTION_USER));
     } else {
         snprintf(section_name, sizeof(section_name), "Unknown%d", (int)section_id);
     }
     
     while (fgets(line, sizeof(line), file)) {
         // Skip leading whitespace
         char* ptr = line;
         while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
             ptr++;
         }
         
         // Skip empty lines and comments
         if (*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#' || *ptr == ';') {
             continue;
         }
         
         // Check for section
         if (*ptr == '[') {
             ptr++;
             char* end = strchr(ptr, ']');
             if (end) {
                 *end = '\0';
                 in_section = (strcmp(ptr, section_name) == 0);
             }
             continue;
         }
         
         // Process key-value pair if in the correct section
         if (in_section) {
             char* equals = strchr(ptr, '=');
             if (equals) {
                 // Extract key
                 *equals = '\0';
                 char* end_key = equals - 1;
                 
                 // Trim trailing whitespace from key
                 while (end_key > ptr && (*end_key == ' ' || *end_key == '\t')) {
                     *end_key = '\0';
                     end_key--;
                 }
                 
                 // Check if this is the key we're looking for
                 if (strcmp(ptr, key) == 0) {
                     *exists = true;
                     break;
                 }
             }
         }
     }
     
     fclose(file);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Enumerate configuration keys
  */
 static polycall_core_error_t file_provider_enumerate(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     void (*callback)(const char* key, void* callback_data),
     void* callback_data
 ) {
     if (!ctx || !user_data || !callback || section_id > POLYCALL_CONFIG_SECTION_USER) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* file_path = (const char*)user_data;
     
     // Open file
     FILE* file = fopen(file_path, "r");
     if (!file) {
         return POLYCALL_CORE_SUCCESS; // Not an error, just doesn't exist
     }
     
     // Find section and enumerate keys
     char line[1024];
     char section_name[64];
     bool in_section = false;
     
     // Convert section ID to name
     const char* section_names[] = {
         "Core",
         "Security",
         "Memory",
         "Type",
         "Performance",
         "Protocol",
         "C",
         "JVM",
         "JavaScript",
         "Python"
     };
     
     if (section_id < sizeof(section_names) / sizeof(section_names[0])) {
         strncpy(section_name, section_names[section_id], sizeof(section_name) - 1);
         section_name[sizeof(section_name) - 1] = '\0';
     } else if (section_id >= POLYCALL_CONFIG_SECTION_USER) {
         snprintf(section_name, sizeof(section_name), "User%d", 
                  (int)(section_id - POLYCALL_CONFIG_SECTION_USER));
     } else {
         snprintf(section_name, sizeof(section_name), "Unknown%d", (int)section_id);
     }
     
     while (fgets(line, sizeof(line), file)) {
         // Skip leading whitespace
         char* ptr = line;
         while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
             ptr++;
         }
         
         // Skip empty lines and comments
         if (*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#' || *ptr == ';') {
             continue;
         }
         
         // Check for section
         if (*ptr == '[') {
             ptr++;
             char* end = strchr(ptr, ']');
             if (end) {
                 *end = '\0';
                 in_section = (strcmp(ptr, section_name) == 0);
             }
             continue;
         }
         
         // Process key-value pair if in the correct section
         if (in_section) {
             char* equals = strchr(ptr, '=');
             if (equals) {
                 // Extract key
                 *equals = '\0';
                 char* end_key = equals - 1;
                 
                 // Trim trailing whitespace from key
                 while (end_key > ptr && (*end_key == ' ' || *end_key == '\t')) {
                     *end_key = '\0';
                     end_key--;
                 }
                 
                 // Call callback with key
                 callback(ptr, callback_data);
             }
         }
     }
     
     fclose(file);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /*------------------------------------------------------------------------*/
 /* Apply configuration to FFI components */
 /*------------------------------------------------------------------------*/
 
 /**
  * @brief Apply configuration to FFI core
  */
 static void ffi_core_apply_config(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Update FFI flags based on configuration
     polycall_ffi_flags_t flags = 0;
     
     if (polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_CORE, "secure_mode", true)) {
         flags |= POLYCALL_FFI_FLAG_SECURE;
     }
     
     if (polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_CORE, "strict_types", true)) {
         flags |= POLYCALL_FFI_FLAG_STRICT_TYPES;
     }
     
     if (polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_CORE, "memory_isolation", true)) {
         flags |= POLYCALL_FFI_FLAG_MEMORY_ISOLATION;
     }
     
     if (polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_CORE, "async_calls", false)) {
         flags |= POLYCALL_FFI_FLAG_ASYNC;
     }
     
     if (polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_CORE, "debug_mode", false)) {
         flags |= POLYCALL_FFI_FLAG_DEBUG;
     }
     
     if (polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_CORE, "trace_calls", false)) {
         flags |= POLYCALL_FFI_FLAG_TRACE;
     }
     
     // Apply flags to FFI context
     ffi_ctx->flags = flags;
 }
 
 /**
  * @brief Apply configuration to security component
  */
 static void security_apply_config(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Create new security configuration
     security_config_t security_config;
     
     // Set security level
     security_config.security_level = (polycall_security_level_t)
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                    POLYCALL_CONFIG_SECTION_SECURITY, "security_level", 
                                    POLYCALL_SECURITY_LEVEL_MEDIUM);
     
     // Set isolation level
     security_config.isolation_level = (polycall_isolation_level_t)
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                    POLYCALL_CONFIG_SECTION_SECURITY, "isolation_level", 
                                    POLYCALL_ISOLATION_LEVEL_FUNCTION);
     
     // Set audit level
     security_config.audit_level = (polycall_audit_level_t)
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                    POLYCALL_CONFIG_SECTION_SECURITY, "audit_level", 
                                    POLYCALL_AUDIT_LEVEL_WARNING);
     
     // Set default deny policy
     security_config.default_deny = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_SECURITY, "default_deny", 
                                     true);
     
     // Apply configuration to security context
     polycall_security_configure(ctx, ffi_ctx, security_ctx, &security_config);
 }
 
 /**
  * @brief Apply configuration to memory bridge
  */
 static void memory_bridge_apply_config(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_bridge_t* memory_bridge,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Create new memory bridge configuration
     memory_bridge_config_t memory_config;
     
     // Set shared pool size
     memory_config.shared_pool_size = 
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                   POLYCALL_CONFIG_SECTION_MEMORY, "shared_pool_size", 
                                   1024 * 1024); // Default 1MB
     
     // Set cleanup handler flag
     memory_config.use_cleanup_handlers = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_MEMORY, "use_cleanup_handlers", 
                                     true);
     
     // Set allocation tracking flag
     memory_config.track_allocations = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_MEMORY, "track_allocations", 
                                     true);
     
     // Apply configuration to memory bridge
     polycall_memory_bridge_configure(ctx, ffi_ctx, memory_bridge, &memory_config);
 }
 
 /**
  * @brief Apply configuration to type system
  */
 static void type_system_apply_config(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     type_mapping_context_t* type_ctx,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Create new type system configuration
     type_system_config_t type_config;
     
     // Set type capacity
     type_config.type_capacity = 
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                   POLYCALL_CONFIG_SECTION_TYPE, "type_capacity", 
                                   256);
     
     // Set rule capacity
     type_config.rule_capacity = 
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                   POLYCALL_CONFIG_SECTION_TYPE, "rule_capacity", 
                                   128);
     
     // Set auto-register primitives flag
     type_config.auto_register_primitives = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_TYPE, "auto_register_primitives", 
                                     true);
     
     // Apply configuration to type system
     polycall_type_configure(ctx, ffi_ctx, type_ctx, &type_config);
 }
 
 /**
  * @brief Apply configuration to performance manager
  */
 static void performance_apply_config(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     polycall_ffi_config_context_t* config_ctx
 ) {
     // Create new performance configuration
     performance_config_t perf_config;
     
     // Set call caching flag
     perf_config.enable_call_caching = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_PERFORMANCE, "enable_call_caching", 
                                     true);
     
     // Set type caching flag
     perf_config.enable_type_caching = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_PERFORMANCE, "enable_type_caching", 
                                     true);
     
     // Set call batching flag
     perf_config.enable_call_batching = 
         polycall_ffi_config_get_bool(ctx, ffi_ctx, config_ctx, 
                                     POLYCALL_CONFIG_SECTION_PERFORMANCE, "enable_call_batching", 
                                     false);
     
     // Set optimization level
     perf_config.opt_level = (polycall_optimization_level_t)
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                   POLYCALL_CONFIG_SECTION_PERFORMANCE, "optimization_level", 
                                   POLYCALL_OPT_LEVEL_MODERATE);
     
     // Set cache size
     perf_config.cache_size = 
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                   POLYCALL_CONFIG_SECTION_PERFORMANCE, "cache_size", 
                                   1024);
     
     // Set cache TTL
     perf_config.cache_ttl_ms = 
         polycall_ffi_config_get_int(ctx, ffi_ctx, config_ctx, 
                                   POLYCALL_CONFIG_SECTION_PERFORMANCE, "cache_ttl_ms", 
                                   60000); // Default 1 minute
     
     // Apply configuration to performance manager
     polycall_performance_configure(ctx, ffi_ctx, perf_mgr, &perf_config);
 }
 
