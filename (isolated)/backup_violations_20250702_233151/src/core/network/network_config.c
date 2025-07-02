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
#include "polycall/core/network/network_config.h"

 * @file network_config.c
 * @brief Network configuration implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the configuration management for LibPolyCall's network module,
 * providing consistent configuration handling across network components.
 */

 #include "polycall/core/network/network_config.h"

// Forward declarations of static functions
static polycall_core_error_t apply_defaults(polycall_core_context_t* ctx, polycall_network_config_t* config);
static polycall_core_error_t load_config_from_file(polycall_core_context_t* ctx, polycall_network_config_t* config);
static polycall_core_error_t save_config_to_file(polycall_core_context_t* ctx, polycall_network_config_t* config);
 /**
  * @brief Create a network configuration context
  */
 polycall_core_error_t polycall_network_config_create(
     polycall_core_context_t* ctx,
     polycall_network_config_t** config,
     const char* config_file
 ) {
     if (!ctx || !config) {
         return (polycall_core_error_t)POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate configuration context
     polycall_network_config_t* new_config = polycall_core_malloc(ctx, sizeof(polycall_network_config_t));
     if (!new_config) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize configuration context
     memset(new_config, 0, sizeof(polycall_network_config_t));
     new_config->core_ctx = ctx;
     
     // Set config file path if provided
     if (config_file) {
         strncpy(new_config->config_file, config_file, sizeof(new_config->config_file) - 1);
     }
     
     // Apply default configuration
     polycall_core_error_t result = apply_defaults(ctx, new_config);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, new_config);
         return result;
     }
     
     // Load configuration from file if provided
     if (config_file && config_file[0] != '\0') {
         load_config_from_file(ctx, new_config);
         // Note: We ignore file load errors and keep defaults
     }
     
     new_config->initialized = true;
     *config = new_config;
     
     return (polycall_core_error_t)POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Destroy a network configuration context
  */
 void polycall_network_config_destroy(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config
 ) {
     if (!ctx || !config) {
         return;
     }
     
     // Save configuration if modified
     if (config->modified && config->config_file[0] != '\0') {
         save_config_to_file(ctx, config);
     }
     
     // Free configuration entries
     free_config_entries(ctx, config->entries);
     
     // Free configuration context
     polycall_core_free(ctx, config);
 }
 
 /**
  * @brief Set configuration validation callback
  */
 polycall_core_error_t polycall_network_config_set_validator(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     polycall_network_config_validate_fn validator,
     void* user_data
 ) {
     if (!ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     config->validate_callback = validator;
     config->validate_user_data = user_data;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set integer configuration value
  */
 polycall_core_error_t polycall_network_config_set_int(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     int value
 ) {
     if (!ctx || !config || !section || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find existing entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (entry) {
         // Update existing entry
         if (entry->type != CONFIG_TYPE_INT) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         // Check if value changed
         if (entry->value.int_value != value) {
             entry->value.int_value = value;
             config->modified = true;
         }
     } else {
         // Add new entry
         polycall_core_error_t result = add_config_entry(ctx, config, section, key, CONFIG_TYPE_INT, &value, NULL);
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     }
     
     // Validate configuration if callback is registered
     if (config->validate_callback) {
         bool valid = config->validate_callback(ctx, config, config->validate_user_data);
         if (!valid) {
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
    /**
    * @brief Set string configuration value
    */
polycall_core_error_t polycall_network_config_set_string(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config,
    const char* section,
    const char* key,
    const char* value
) {
    if (!ctx || !config || !section || !key || !value) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find existing entry
    config_entry_t* entry = find_config_entry(config, section, key);
    
    if (entry) {
        // Update existing entry
        if (entry->type != CONFIG_TYPE_STRING) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Check if value changed
        if (strcmp(entry->value.string_value, value) != 0) {
            free(entry->value.string_value);
            entry->value.string_value = strdup(value);
            config->modified = true;
        }
    } else {
        // Add new entry
        polycall_core_error_t result = add_config_entry(ctx, config, section, key, CONFIG_TYPE_STRING, NULL, value);
        if (result != POLYCALL_CORE_SUCCESS) {
            return result;
        }
    }
    
    // Validate configuration if callback is registered
    if (config->validate_callback) {
        bool valid = config->validate_callback(ctx, config, config->validate_user_data);
        if (!valid) {
            return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}
 /**
  * @brief Get integer configuration value
  */
 polycall_core_error_t polycall_network_config_get_int(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     int* value
 ) {
     if (!ctx || !config || !section || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (!entry) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check type
     if (entry->type != CONFIG_TYPE_INT) {
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     // Return value
     *value = entry->value.int_value;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set unsigned integer configuration value
  */
 polycall_core_error_t polycall_network_config_set_uint(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     unsigned int value
 ) {
     if (!ctx || !config || !section || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find existing entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (entry) {
         // Update existing entry
         if (entry->type != CONFIG_TYPE_UINT) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         // Check if value changed
         if (entry->value.uint_value != value) {
             entry->value.uint_value = value;
             config->modified = true;
         }
     } else {
         // Add new entry
         polycall_core_error_t result = add_config_entry(ctx, config, section, key, CONFIG_TYPE_UINT, &value, NULL);
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     }
     
     // Validate configuration if callback is registered
     if (config->validate_callback) {
         bool valid = config->validate_callback(ctx, config, config->validate_user_data);
         if (!valid) {
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get unsigned integer configuration value
  */
 polycall_core_error_t polycall_network_config_get_uint(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     unsigned int* value
 ) {
     if (!ctx || !config || !section || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (!entry) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check type
     if (entry->type != CONFIG_TYPE_UINT) {
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     // Return value
     *value = entry->value.uint_value;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set boolean configuration value
  */
 polycall_core_error_t polycall_network_config_set_bool(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     bool value
 ) {
     if (!ctx || !config || !section || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find existing entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (entry) {
         // Update existing entry
         if (entry->type != CONFIG_TYPE_BOOL) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         // Check if value changed
         if (entry->value.bool_value != value) {
             entry->value.bool_value = value;
             config->modified = true;
         }
     } else {
         // Add new entry
         polycall_core_error_t result = add_config_entry(ctx, config, section, key, CONFIG_TYPE_BOOL, &value, NULL);
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     }
     
     // Validate configuration if callback is registered
     if (config->validate_callback) {
         bool valid = config->validate_callback(ctx, config, config->validate_user_data);
         if (!valid) {
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get boolean configuration value
  */
 polycall_core_error_t polycall_network_config_get_bool(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     bool* value
 ) {
     if (!ctx || !config || !section || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (!entry) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check type
     if (entry->type != CONFIG_TYPE_BOOL) {
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     // Return value
     *value = entry->value.bool_value;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set string configuration value
  */
 polycall_core_error_t polycall_network_config_set_string(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     const char* value
 ) {
     if (!ctx || !config || !section || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find existing entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (entry) {
         // Update existing entry
         if (entry->type != CONFIG_TYPE_STRING) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         // Check if value changed
         if (strcmp(entry->value.string_value, value) != 0) {
             // Free old string
             polycall_core_free(ctx, entry->value.string_value);
             
             // Allocate and copy new string
             entry->value.string_value = polycall_core_malloc(ctx, strlen(value) + 1);
             if (!entry->value.string_value) {
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             strcpy(entry->value.string_value, value);
             config->modified = true;
         }
     } else {
         // Add new entry
         polycall_core_error_t result = add_config_entry(ctx, config, section, key, CONFIG_TYPE_STRING, value, NULL);
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     }
     
     // Validate configuration if callback is registered
     if (config->validate_callback) {
         bool valid = config->validate_callback(ctx, config, config->validate_user_data);
         if (!valid) {
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get string configuration value
  */
 polycall_core_error_t polycall_network_config_get_string(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     char* value,
     size_t max_length
 ) {
     if (!ctx || !config || !section || !key || !value || max_length == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (!entry) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check type
     if (entry->type != CONFIG_TYPE_STRING) {
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     // Check buffer size
     if (strlen(entry->value.string_value) >= max_length) {
         return POLYCALL_CORE_ERROR_BUFFER_UNDERFLOW;
     }
     
     // Return value
     strcpy(value, entry->value.string_value);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set float configuration value
  */
 polycall_core_error_t polycall_network_config_set_float(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     float value
 ) {
     if (!ctx || !config || !section || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find existing entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (entry) {
         // Update existing entry
         if (entry->type != CONFIG_TYPE_FLOAT) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         // Check if value changed
         if (entry->value.float_value != value) {
             entry->value.float_value = value;
             config->modified = true;
         }
     } else {
         // Add new entry
         polycall_core_error_t result = add_config_entry(ctx, config, section, key, CONFIG_TYPE_FLOAT, &value, NULL);
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     }
     
     // Validate configuration if callback is registered
     if (config->validate_callback) {
         bool valid = config->validate_callback(ctx, config, config->validate_user_data);
         if (!valid) {
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get float configuration value
  */
 polycall_core_error_t polycall_network_config_get_float(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     float* value
 ) {
     if (!ctx || !config || !section || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find entry
     config_entry_t* entry = find_config_entry(config, section, key);
     
     if (!entry) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check type
     if (entry->type != CONFIG_TYPE_FLOAT) {
         return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
     }
     
     // Return value
     *value = entry->value.float_value;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Load configuration from file
  */
 polycall_core_error_t polycall_network_config_load(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* filename
 ) {
     if (!ctx || !config || !filename) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Update config file path
     strncpy(config->config_file, filename, sizeof(config->config_file) - 1);
     
     // Load configuration
     return load_config_from_file(ctx, config);
 }
 
 /**
  * @brief Save configuration to file
  */
 polycall_core_error_t polycall_network_config_save(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* filename
 ) {
     if (!ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Update config file path if provided
     if (filename) {
         strncpy(config->config_file, filename, sizeof(config->config_file) - 1);
     }
     
     // Check if config file path is set
     if (config->config_file[0] == '\0') {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Save configuration
     return save_config_to_file(ctx, config);
 }
 
 /**
  * @brief Reset configuration to defaults
  */
 polycall_core_error_t polycall_network_config_reset(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config
 ) {
     if (!ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Free existing entries
     free_config_entries(ctx, config->entries);
     config->entries = NULL;
     
     // Apply defaults
     polycall_core_error_t result = apply_defaults(ctx, config);
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     config->modified = true;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Enumerate configuration keys
  */
 polycall_core_error_t polycall_network_config_enumerate(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     bool (*callback)(const char* section, const char* key, void* user_data),
     void* user_data
 ) {
     if (!ctx || !config || !section || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Iterate through entries
     config_entry_t* entry = config->entries;
     while (entry) {
         // If section matches or empty section for all entries
         if (strcmp(section, "") == 0 || strcmp(entry->section, section) == 0) {
             // Call callback
             if (!callback(entry->section, entry->key, user_data)) {
                 // Callback requested termination
                 break;
             }
         }
         
         entry = entry->next;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Find configuration entry
  */
 static config_entry_t* find_config_entry(
     polycall_network_config_t* config,
     const char* section,
     const char* key
 ) {
     if (!config || !section || !key) {
         return NULL;
     }
     
     config_entry_t* entry = config->entries;
     while (entry) {
         if (strcmp(entry->section, section) == 0 && strcmp(entry->key, key) == 0) {
             return entry;
         }
         
         entry = entry->next;
     }
     
     return NULL;
 }
 
 /**
  * @brief Add configuration entry
  */
 static polycall_core_error_t add_config_entry(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     const char* section,
     const char* key,
     config_value_type_t type,
     const void* value,
     const char* description
 ) {
     if (!ctx || !config || !section || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create new entry
     config_entry_t* entry = polycall_core_malloc(ctx, sizeof(config_entry_t));
     if (!entry) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize entry
     memset(entry, 0, sizeof(config_entry_t));
     strncpy(entry->section, section, sizeof(entry->section) - 1);
     strncpy(entry->key, key, sizeof(entry->key) - 1);
     entry->type = type;
     
     // Set description if provided
     if (description) {
         strncpy(entry->description, description, sizeof(entry->description) - 1);
     }
     
     // Set value based on type
     switch (type) {
         case CONFIG_TYPE_INT:
             entry->value.int_value = *(const int*)value;
             break;
             
         case CONFIG_TYPE_UINT:
             entry->value.uint_value = *(const unsigned int*)value;
             break;
             
         case CONFIG_TYPE_BOOL:
             entry->value.bool_value = *(const bool*)value;
             break;
             
         case CONFIG_TYPE_STRING:
             entry->value.string_value = polycall_core_malloc(ctx, strlen((const char*)value) + 1);
             if (!entry->value.string_value) {
                 polycall_core_free(ctx, entry);
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             strcpy(entry->value.string_value, (const char*)value);
             break;
             
         case CONFIG_TYPE_FLOAT:
             entry->value.float_value = *(const float*)value;
             break;
             
         default:
             polycall_core_free(ctx, entry);
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Add to list
     entry->next = config->entries;
     config->entries = entry;
     
     config->modified = true;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Free configuration entries
  */
 static void free_config_entries(
     polycall_core_context_t* ctx,
     config_entry_t* entries
 ) {
     config_entry_t* entry = entries;
     while (entry) {
         config_entry_t* next = entry->next;
         
         // Free string value if applicable
         if (entry->type == CONFIG_TYPE_STRING && entry->value.string_value) {
             polycall_core_free(ctx, entry->value.string_value);
         }
         
         // Free entry
         polycall_core_free(ctx, entry);
         
         entry = next;
     }
 }
 
/**
 * @brief Load configuration from file
 */
static polycall_core_error_t load_config_from_file(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config
) __attribute__((unused));

static polycall_core_error_t load_config_from_file(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config
) {
     if (!ctx || !config || !config->config_file[0]) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Open file
     FILE* file = fopen(config->config_file, "r");
     if (!file) {
         return POLYCALL_CORE_ERROR_FILE_NOT_FOUND;
     }
     
     // Read file
     char line[512];
     char current_section[64] = "";
     int line_number = 0;
     
     while (fgets(line, sizeof(line), file)) {
         line_number++;
         
         // Remove trailing newline
         size_t len = strlen(line);
         if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
             line[len - 1] = '\0';
         }
         
         // Skip empty lines and comments
         if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
             continue;
         }
         
         // Check for section header
         if (line[0] == '[' && line[len - 1] == ']') {
             // Extract section name
             line[len - 1] = '\0';
             strncpy(current_section, line + 1, sizeof(current_section) - 1);
             continue;
         }
         
         // Parse key-value pair
         char* equals = strchr(line, '=');
         if (!equals) {
             // Invalid line, skip
             continue;
         }
         
         // Extract key and value
         *equals = '\0';
         char* key = line;
         char* value = equals + 1;
         
         // Trim whitespace
         while (*key && isspace(*key)) key++;
         char* key_end = key + strlen(key) - 1;
         while (key_end > key && isspace(*key_end)) *key_end-- = '\0';
         
         while (*value && isspace(*value)) value++;
         char* value_end = value + strlen(value) - 1;
         while (value_end > value && isspace(*value_end)) *value_end-- = '\0';
         
         // Skip if key is empty
         if (*key == '\0') {
             continue;
         }
         
         // Find existing entry
         config_entry_t* entry = find_config_entry(config, current_section, key);
         
         if (entry) {
             // Update existing entry
             switch (entry->type) {
                 case CONFIG_TYPE_INT:
                     entry->value.int_value = atoi(value);
                     break;
                     
                 case CONFIG_TYPE_UINT:
                     entry->value.uint_value = (unsigned int)strtoul(value, NULL, 10);
                     break;
                     
                 case CONFIG_TYPE_BOOL:
                     if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || 
                         strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                         entry->value.bool_value = true;
                     } else {
                         entry->value.bool_value = false;
                     }
                     break;
                     
                 case CONFIG_TYPE_STRING:
                     // Free old string
                     if (entry->value.string_value) {
                         polycall_core_free(ctx, entry->value.string_value);
                     }
                     
                     // Allocate and copy new string
                     entry->value.string_value = polycall_core_malloc(ctx, strlen(value) + 1);
                     if (!entry->value.string_value) {
                         fclose(file);
                         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                     }
                     strcpy(entry->value.string_value, value);
                     break;
                     
                 case CONFIG_TYPE_FLOAT:
                     entry->value.float_value = (float)atof(value);
                     break;
             }
         }
         // Note: We don't add new entries from the file that don't already exist in memory
         // This ensures we only load known configuration values
     }
     
     // Close file
     fclose(file);
     
     // Validate configuration if callback is registered
     if (config->validate_callback) {
         bool valid = config->validate_callback(ctx, config, config->validate_user_data);
         if (!valid) {
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     // Configuration was just loaded from file, so it's not considered modified
     config->modified = false;
     
     return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Save configuration to file
 */
/**
 * @brief Save configuration to file
 */
static polycall_core_error_t save_config_to_file(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config
) __attribute__((unused));

static polycall_core_error_t save_config_to_file(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config
) {
     if (!ctx || !config || !config->config_file[0]) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Open file
     FILE* file = fopen(config->config_file, "w");
     if (!file) {
         return POLYCALL_CORE_ERROR_IO_ERROR;
     }
     
     // Write header
     fprintf(file, "# LibPolyCall Network Configuration\n");
     fprintf(file, "# Generated on %s\n", __DATE__);
     fprintf(file, "# Version %d\n\n", NETWORK_CONFIG_VERSION);
     
     // Process entries
     // First, we need to build a list of all sections
     char sections[32][64];
     int section_count = 0;
     
     config_entry_t* entry = config->entries;
     while (entry) {
         // Check if section is already in the list
         bool found = false;
         for (int i = 0; i < section_count; i++) {
             if (strcmp(sections[i], entry->section) == 0) {
                 found = true;
                 break;
             }
         }
         
         // Add section if not found
         if (!found && section_count < 32) {
             strcpy(sections[section_count], entry->section);
             section_count++;
         }
         
         entry = entry->next;
     }
     
     // Now process each section
     for (int i = 0; i < section_count; i++) {
         // Write section header
         fprintf(file, "[%s]\n", sections[i]);
         
         // Write entries for this section
         entry = config->entries;
         while (entry) {
             if (strcmp(entry->section, sections[i]) == 0) {
                 // Write description if available
                 if (entry->description[0] != '\0') {
                     fprintf(file, "# %s\n", entry->description);
                 }
                 
                 // Write key-value pair
                 fprintf(file, "%s = ", entry->key);
                 
                 // Write value based on type
                 switch (entry->type) {
                     case CONFIG_TYPE_INT:
                         fprintf(file, "%d\n", entry->value.int_value);
                         break;
                         
                     case CONFIG_TYPE_UINT:
                         fprintf(file, "%u\n", entry->value.uint_value);
                         break;
                         
                     case CONFIG_TYPE_BOOL:
                         fprintf(file, "%s\n", entry->value.bool_value ? "true" : "false");
                         break;
                         
                     case CONFIG_TYPE_STRING:
                         fprintf(file, "%s\n", entry->value.string_value ? entry->value.string_value : "");
                         break;
                         
                     case CONFIG_TYPE_FLOAT:
                         fprintf(file, "%.6f\n", entry->value.float_value);
                         break;
                 }
             }
             
             entry = entry->next;
         }
         
         // Add empty line between sections
         fprintf(file, "\n");
     }
     
     // Close file
     fclose(file);
     
     // Configuration was just saved, so it's not modified
     config->modified = false;
     
     return POLYCALL_CORE_SUCCESS;
}
static polycall_core_error_t save_config_to_file(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config
) {
     if (!ctx || !config || !config->config_file[0]) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Open file
     FILE* file = fopen(config->config_file, "w");
     if (!file) {
         return POLYCALL_CORE_ERROR_IO_ERROR;
     }
     
     // Write header
     fprintf(file, "# LibPolyCall Network Configuration\n");
     fprintf(file, "# Generated on %s\n", __DATE__);
     fprintf(file, "# Version %d\n\n", NETWORK_CONFIG_VERSION);
     
     // Process entries
     // First, we need to build a list of all sections
     char sections[32][64];
     int section_count = 0;
     
     config_entry_t* entry = config->entries;
     while (entry) {
         // Check if section is already in the list
         bool found = false;
         for (int i = 0; i < section_count; i++) {
             if (strcmp(sections[i], entry->section) == 0) {
                 found = true;
                 break;
             }
         }
         
         // Add section if not found
         if (!found && section_count < 32) {
             strcpy(sections[section_count], entry->section);
             section_count++;
         }
         
         entry = entry->next;
     }
     
     // Now process each section
     for (int i = 0; i < section_count; i++) {
         // Write section header
         fprintf(file, "[%s]\n", sections[i]);
         
         // Write entries for this section
         entry = config->entries;
         while (entry) {
             if (strcmp(entry->section, sections[i]) == 0) {
                 // Write description if available
                 if (entry->description[0] != '\0') {
                     fprintf(file, "# %s\n", entry->description);
                 }
                 
                 // Write key-value pair
                 fprintf(file, "%s = ", entry->key);
                 
                 // Write value based on type
                 switch (entry->type) {
                     case CONFIG_TYPE_INT:
                         fprintf(file, "%d\n", entry->value.int_value);
                         break;
                         
                     case CONFIG_TYPE_UINT:
                         fprintf(file, "%u\n", entry->value.uint_value);
                         break;
                         
                     case CONFIG_TYPE_BOOL:
                         fprintf(file, "%s\n", entry->value.bool_value ? "true" : "false");
                         break;
                         
                     case CONFIG_TYPE_STRING:
                         fprintf(file, "%s\n", entry->value.string_value ? entry->value.string_value : "");
                         break;
                         
                     case CONFIG_TYPE_FLOAT:
                         fprintf(file, "%.6f\n", entry->value.float_value);
                         break;
                 }
             }
             
             entry = entry->next;
         }
         
         // Add empty line between sections
         fprintf(file, "\n");
     }
     
     // Close file
     fclose(file);
     
     // Configuration was just saved, so it's not modified
     config->modified = false;
     
     return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Apply default configuration
 */
static polycall_core_error_t apply_defaults(
    polycall_core_context_t* ctx,
    polycall_network_config_t* config
) {
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Apply default configuration
  */
 static polycall_core_error_t apply_defaults(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config
 ) {
     if (!ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // General settings
     int buffer_size = 8192;
     add_config_entry(ctx, config, SECTION_GENERAL, "buffer_size", CONFIG_TYPE_INT, &buffer_size, 
                     "Default I/O buffer size in bytes");
     
     unsigned int connection_timeout = 30000;
     add_config_entry(ctx, config, SECTION_GENERAL, "connection_timeout", CONFIG_TYPE_UINT, &connection_timeout, 
                     "Connection timeout in milliseconds");
     
     unsigned int operation_timeout = 30000;
     add_config_entry(ctx, config, SECTION_GENERAL, "operation_timeout", CONFIG_TYPE_UINT, &operation_timeout, 
                     "Operation timeout in milliseconds");
     
     unsigned int max_connections = 100;
     add_config_entry(ctx, config, SECTION_GENERAL, "max_connections", CONFIG_TYPE_UINT, &max_connections, 
                     "Maximum number of simultaneous connections");
     
     unsigned int max_message_size = 1048576;
     add_config_entry(ctx, config, SECTION_GENERAL, "max_message_size", CONFIG_TYPE_UINT, &max_message_size, 
                     "Maximum message size in bytes (1MB)");
     
     // Security settings
     bool enable_tls = false;
     add_config_entry(ctx, config, SECTION_SECURITY, "enable_tls", CONFIG_TYPE_BOOL, &enable_tls, 
                     "Enable TLS/SSL encryption");
     
     bool enable_encryption = false;
     add_config_entry(ctx, config, SECTION_SECURITY, "enable_encryption", CONFIG_TYPE_BOOL, &enable_encryption, 
                     "Enable message encryption");
     
     const char* tls_cert_file = "";
     add_config_entry(ctx, config, SECTION_SECURITY, "tls_cert_file", CONFIG_TYPE_STRING, tls_cert_file, 
                     "TLS certificate file path");
     
     const char* tls_key_file = "";
     add_config_entry(ctx, config, SECTION_SECURITY, "tls_key_file", CONFIG_TYPE_STRING, tls_key_file, 
                     "TLS private key file path");
     
     const char* tls_ca_file = "";
     add_config_entry(ctx, config, SECTION_SECURITY, "tls_ca_file", CONFIG_TYPE_STRING, tls_ca_file, 
                     "TLS CA certificate file path");
     
     // Performance settings
     bool enable_compression = true;
     add_config_entry(ctx, config, SECTION_PERFORMANCE, "enable_compression", CONFIG_TYPE_BOOL, &enable_compression, 
                     "Enable message compression");
     
     unsigned int thread_pool_size = 4;
     add_config_entry(ctx, config, SECTION_PERFORMANCE, "thread_pool_size", CONFIG_TYPE_UINT, &thread_pool_size, 
                     "Thread pool size for I/O operations");
     
     bool enable_call_batching = true;
     add_config_entry(ctx, config, SECTION_PERFORMANCE, "enable_call_batching", CONFIG_TYPE_BOOL, &enable_call_batching, 
                     "Enable batching of multiple calls");
     
     unsigned int batch_size = 32;
     add_config_entry(ctx, config, SECTION_PERFORMANCE, "batch_size", CONFIG_TYPE_UINT, &batch_size, 
                     "Maximum number of calls in a batch");
     
     // Advanced settings
     bool enable_auto_reconnect = true;
     add_config_entry(ctx, config, SECTION_ADVANCED, "enable_auto_reconnect", CONFIG_TYPE_BOOL, &enable_auto_reconnect, 
                     "Enable automatic reconnection on connection loss");
     
     unsigned int reconnect_delay = 5000;
     add_config_entry(ctx, config, SECTION_ADVANCED, "reconnect_delay", CONFIG_TYPE_UINT, &reconnect_delay, 
                     "Delay between reconnection attempts in milliseconds");
     
     unsigned int max_reconnect_attempts = 5;
     add_config_entry(ctx, config, SECTION_ADVANCED, "max_reconnect_attempts", CONFIG_TYPE_UINT, &max_reconnect_attempts, 
                     "Maximum number of reconnection attempts");
     
     unsigned int keep_alive_interval = 60000;
     add_config_entry(ctx, config, SECTION_ADVANCED, "keep_alive_interval", CONFIG_TYPE_UINT, &keep_alive_interval, 
                     "Keep-alive interval in milliseconds");
     
     bool enable_protocol_dispatch = true;
     add_config_entry(ctx, config, SECTION_ADVANCED, "enable_protocol_dispatch", CONFIG_TYPE_BOOL, &enable_protocol_dispatch, 
                     "Enable automatic protocol message dispatching");
     
     // Configuration was just initialized, so it's not considered modified
     config->modified = false;
     
     return POLYCALL_CORE_SUCCESS;
 }  
    /**
    * @brief Free configuration
    */
    polycall_core_error_t polycall_network_config_free(
        polycall_core_context_t* ctx,
        polycall_network_config_t* config
    ) {
        if (!ctx || !config) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Free entries
        free_config_entries(ctx, config->entries);
        
        // Free config structure
        polycall_core_free(ctx, config);
        
        return POLYCALL_CORE_SUCCESS;
    }

