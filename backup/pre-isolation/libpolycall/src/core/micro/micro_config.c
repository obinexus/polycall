/**
#include "polycall/core/micro/micro_config.h"

 * @file micro_config.c
 * @brief Configuration system implementation for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the configuration interface for the LibPolyCall micro command
 * system, handling parsing, validation, and application of configurations for
 * micro components and commands.
 */

 #include "polycall/core/micro/polycall_micro_config.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <string.h>
 #include <ctype.h>
 #include <stdbool.h>
 
 // Maximum number of components in configuration
 #define MAX_COMPONENTS 64
 
 // Maximum line length for config files
 #define MAX_LINE_LENGTH 1024
 
 // Maximum section nesting depth
 #define MAX_SECTION_DEPTH 8
 
 // Maximum number of includes
 #define MAX_INCLUDES 16
 
 // Tokenizer states
 typedef enum {
     TOKEN_STATE_INITIAL,
     TOKEN_STATE_IDENTIFIER,
     TOKEN_STATE_NUMBER,
     TOKEN_STATE_STRING,
     TOKEN_STATE_COMMENT,
     TOKEN_STATE_SYMBOL
 } tokenizer_state_t;
 
 // Token types
 typedef enum {
     TOKEN_IDENTIFIER,
     TOKEN_NUMBER,
     TOKEN_STRING,
     TOKEN_SYMBOL,
     TOKEN_EOF
 } token_type_t;
 
 // Token structure
 typedef struct {
     token_type_t type;
     char value[MAX_LINE_LENGTH];
     int line;
     int column;
 } token_t;
 
 // Parser states
 typedef enum {
     PARSER_STATE_INITIAL,
     PARSER_STATE_SECTION,
     PARSER_STATE_PROPERTY,
     PARSER_STATE_VALUE,
     PARSER_STATE_ARRAY,
     PARSER_STATE_MACRO
 } parser_state_t;
 
 // Config value types
 typedef enum {
     CONFIG_VALUE_STRING,
     CONFIG_VALUE_NUMBER,
     CONFIG_VALUE_BOOLEAN,
     CONFIG_VALUE_ARRAY,
     CONFIG_VALUE_NULL
 } config_value_type_t;
 
 // Config value structure
 typedef struct config_value {
     config_value_type_t type;
     union {
         char* string_value;
         double number_value;
         bool boolean_value;
         struct {
             struct config_value** items;
             size_t count;
             size_t capacity;
         } array;
     } data;
 } config_value_t;
 
 // Config property structure
 typedef struct config_property {
     char name[64];
     config_value_t value;
     struct config_property* next;
 } config_property_t;
 
 // Config section structure
 typedef struct config_section {
     char name[64];
     config_property_t* properties;
     struct config_section* sections;
     size_t section_count;
     size_t section_capacity;
     struct config_section* parent;
 } config_section_t;
 
 // Tokenizer structure
 typedef struct {
     const char* input;
     size_t input_len;
     size_t pos;
     int line;
     int column;
     token_t current_token;
 } tokenizer_t;
 
 // Parse context structure
 typedef struct {
     tokenizer_t tokenizer;
     config_section_t* root;
     char error_message[MAX_LINE_LENGTH];
     int error_line;
     bool has_error;
     const char* file_path;
 } parse_context_t;
 
 // Micro component configuration with internal fields
 typedef struct {
     micro_component_config_t config;
     bool is_modified;
 } component_config_internal_t;
 
 // Micro configuration manager structure
 struct micro_config_manager {
     component_config_internal_t* components;
     size_t component_count;
     size_t component_capacity;
     config_section_t* global_config;
     config_section_t* binding_config;
     micro_config_manager_options_t options;
     char error_message[MAX_LINE_LENGTH];
     int error_line;
     bool has_error;
     bool is_loaded;
 };
 
 // Forward declarations of internal functions
 static polycall_core_error_t parse_config_file(
     polycall_core_context_t* ctx,
     const char* file_path,
     config_section_t** config,
     char* error_message,
     size_t error_message_size,
     int* error_line
 );
 
 static polycall_core_error_t process_token(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx
 );
 
 static token_t get_next_token(tokenizer_t* tokenizer);
 
 static polycall_core_error_t parse_section(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_section_t* parent,
     config_section_t** section
 );
 
 static polycall_core_error_t parse_property(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_section_t* section
 );
 
 static polycall_core_error_t parse_value(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_value_t* value
 );
 
 static polycall_core_error_t parse_array(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_value_t* value
 );
 
 static void free_config_section(
     polycall_core_context_t* ctx,
     config_section_t* section
 );
 
 static void free_config_property(
     polycall_core_context_t* ctx,
     config_property_t* property
 );
 
 static void free_config_value(
     polycall_core_context_t* ctx,
     config_value_t* value
 );
 
 static polycall_core_error_t extract_component_configs(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     config_section_t* micro_section
 );
 
 static polycall_core_error_t apply_component_config(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     polycall_micro_context_t* micro_ctx,
     const micro_component_config_t* component_config
 );
 
 static config_property_t* find_property(
     config_section_t* section,
     const char* name
 );
 
 static config_section_t* find_section(
     config_section_t* parent,
     const char* name
 );
 
 static bool get_string_value(
     config_section_t* section,
     const char* property_name,
     char* buffer,
     size_t buffer_size
 );
 
 static bool get_number_value(
     config_section_t* section,
     const char* property_name,
     double* value
 );
 
 static bool get_integer_value(
     config_section_t* section,
     const char* property_name,
     int64_t* value
 );
 
 static bool get_boolean_value(
     config_section_t* section,
     const char* property_name,
     bool* value
 );
 
 static bool get_string_array(
     config_section_t* section,
     const char* property_name,
     char** array,
     size_t array_size,
     size_t* count
 );
 
 /**
  * @brief Initialize micro configuration manager
  */
 polycall_core_error_t micro_config_manager_init(
     polycall_core_context_t* ctx,
     micro_config_manager_t** manager,
     const micro_config_manager_options_t* options
 ) {
     if (!ctx || !manager || !options) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate manager
     micro_config_manager_t* new_manager = polycall_core_malloc(ctx, sizeof(micro_config_manager_t));
     if (!new_manager) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate configuration manager");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize manager
     memset(new_manager, 0, sizeof(micro_config_manager_t));
     
     // Copy options
     memcpy(&new_manager->options, options, sizeof(micro_config_manager_options_t));
     
     // Allocate components array
     new_manager->component_capacity = MAX_COMPONENTS;
     new_manager->components = polycall_core_malloc(ctx, 
                                                   new_manager->component_capacity * 
                                                   sizeof(component_config_internal_t));
     if (!new_manager->components) {
         polycall_core_free(ctx, new_manager);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate components array");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize components array
     memset(new_manager->components, 0, 
            new_manager->component_capacity * sizeof(component_config_internal_t));
     
     *manager = new_manager;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup micro configuration manager
  */
 void micro_config_manager_cleanup(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager
 ) {
     if (!ctx || !manager) {
         return;
     }
     
     // Free components
     if (manager->components) {
         // Free component configurations (strings, etc.)
         for (size_t i = 0; i < manager->component_count; i++) {
             micro_component_config_t* config = &manager->components[i].config;
             
             // Free allowed connections
             for (size_t j = 0; j < config->allowed_connections_count; j++) {
                 if (config->allowed_connections[j]) {
                     polycall_core_free(ctx, config->allowed_connections[j]);
                 }
             }
         }
         
         polycall_core_free(ctx, manager->components);
     }
     
     // Free configuration sections
     if (manager->global_config) {
         free_config_section(ctx, manager->global_config);
     }
     
     if (manager->binding_config) {
         free_config_section(ctx, manager->binding_config);
     }
     
     // Free manager
     polycall_core_free(ctx, manager);
 }
 
 /**
  * @brief Load micro configuration
  */
 polycall_core_error_t micro_config_manager_load(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     micro_config_load_status_t* status,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !manager || !status) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Set initial status
     *status = MICRO_CONFIG_LOAD_SUCCESS;
     
     // Reset error state
     manager->has_error = false;
     manager->error_message[0] = '\0';
     manager->error_line = 0;
     
     // Load global configuration if path is provided
     if (manager->options.global_config_path) {
         polycall_core_error_t result = parse_config_file(
             ctx,
             manager->options.global_config_path,
             &manager->global_config,
             manager->error_message,
             sizeof(manager->error_message),
             &manager->error_line
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             // Set error status based on result
             if (result == POLYCALL_CORE_ERROR_FILE_NOT_FOUND) {
                 *status = MICRO_CONFIG_LOAD_FILE_NOT_FOUND;
             } else if (result == POLYCALL_CORE_ERROR_PARSING_FAILED) {
                 *status = MICRO_CONFIG_LOAD_PARSE_ERROR;
             } else if (result == POLYCALL_CORE_ERROR_OUT_OF_MEMORY) {
                 *status = MICRO_CONFIG_LOAD_MEMORY_ERROR;
             }
             
             manager->has_error = true;
             
             // Copy error message if requested
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Error parsing global config file: %s (line %d)",
                          manager->error_message, manager->error_line);
             }
             
             // If fallback to defaults is disabled, return error
             if (!manager->options.fallback_to_defaults) {
                 return result;
             }
         }
     }
     
     // Load binding configuration if path is provided
     if (manager->options.binding_config_path) {
         polycall_core_error_t result = parse_config_file(
             ctx,
             manager->options.binding_config_path,
             &manager->binding_config,
             manager->error_message,
             sizeof(manager->error_message),
             &manager->error_line
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             // Set error status based on result
             if (result == POLYCALL_CORE_ERROR_FILE_NOT_FOUND) {
                 *status = MICRO_CONFIG_LOAD_FILE_NOT_FOUND;
             } else if (result == POLYCALL_CORE_ERROR_PARSING_FAILED) {
                 *status = MICRO_CONFIG_LOAD_PARSE_ERROR;
             } else if (result == POLYCALL_CORE_ERROR_OUT_OF_MEMORY) {
                 *status = MICRO_CONFIG_LOAD_MEMORY_ERROR;
             }
             
             manager->has_error = true;
             
             // Copy error message if requested
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Error parsing binding config file: %s (line %d)",
                          manager->error_message, manager->error_line);
             }
             
             // If fallback to defaults is disabled, return error
             if (!manager->options.fallback_to_defaults) {
                 return result;
             }
         }
     }
     
     // Extract component configurations
     if (manager->global_config) {
         config_section_t* micro_section = find_section(manager->global_config, "micro");
         if (micro_section) {
             polycall_core_error_t result = extract_component_configs(ctx, manager, micro_section);
             if (result != POLYCALL_CORE_SUCCESS) {
                 *status = MICRO_CONFIG_LOAD_PARSING_ERROR;
                 
                 if (error_message && error_message_size > 0) {
                     snprintf(error_message, error_message_size, 
                              "Error extracting component configurations: %s",
                              manager->error_message);
                 }
                 
                 return result;
             }
         }
     }
     
     // Validate configurations if requested
     if (manager->options.validate_on_load) {
         for (size_t i = 0; i < manager->component_count; i++) {
             micro_config_validation_status_t validation_status;
             
             polycall_core_error_t result = micro_config_validate_component(
                 ctx,
                 &manager->components[i].config,
                 &validation_status,
                 manager->error_message,
                 sizeof(manager->error_message)
             );
             
             if (result != POLYCALL_CORE_SUCCESS || validation_status != MICRO_CONFIG_VALIDATION_SUCCESS) {
                 *status = MICRO_CONFIG_LOAD_VALIDATION_ERROR;
                 
                 if (error_message && error_message_size > 0) {
                     snprintf(error_message, error_message_size, 
                              "Validation error for component '%s': %s",
                              manager->components[i].config.name,
                              manager->error_message);
                 }
                 
                 return result;
             }
         }
     }
     
     manager->is_loaded = true;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Apply configuration to micro context
  */
 polycall_core_error_t micro_config_manager_apply(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     polycall_micro_context_t* micro_ctx
 ) {
     if (!ctx || !manager || !micro_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if configuration is loaded
     if (!manager->is_loaded) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_NOT_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Configuration not loaded");
         return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
     }
     
     // Apply configuration to each component
     for (size_t i = 0; i < manager->component_count; i++) {
         polycall_core_error_t result = apply_component_config(
             ctx,
             manager,
             micro_ctx,
             &manager->components[i].config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to apply configuration for component '%s'",
                               manager->components[i].config.name);
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get component configuration by name
  */
 polycall_core_error_t micro_config_manager_get_component_config(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const char* component_name,
     micro_component_config_t** config
 ) {
     if (!ctx || !manager || !component_name || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find component by name
     for (size_t i = 0; i < manager->component_count; i++) {
         if (strcmp(manager->components[i].config.name, component_name) == 0) {
             *config = &manager->components[i].config;
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Component not found
     *config = NULL;
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                       POLYCALL_CORE_ERROR_NOT_FOUND,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "Component configuration '%s' not found", component_name);
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 /**
  * @brief Get all component configurations
  */
 polycall_core_error_t micro_config_manager_get_all_components(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     micro_component_config_t** configs,
     size_t* count
 ) {
     if (!ctx || !manager || !count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if output buffer is large enough
     if (configs && *count < manager->component_count) {
         *count = manager->component_count;
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Buffer too small for component configurations");
         return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
     }
     
     // Copy configurations to output buffer if provided
     if (configs) {
         for (size_t i = 0; i < manager->component_count; i++) {
             configs[i] = &manager->components[i].config;
         }
     }
     
     // Set actual count
     *count = manager->component_count;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Add component configuration
  */
 polycall_core_error_t micro_config_manager_add_component(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const micro_component_config_t* config
 ) {
     if (!ctx || !manager || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if component with same name already exists
     for (size_t i = 0; i < manager->component_count; i++) {
         if (strcmp(manager->components[i].config.name, config->name) == 0) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                               POLYCALL_CORE_ERROR_ALREADY_EXISTS,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Component configuration '%s' already exists", config->name);
             return POLYCALL_CORE_ERROR_ALREADY_EXISTS;
         }
     }
     
     // Check if we've reached the maximum number of components
     if (manager->component_count >= manager->component_capacity) {
         // Expand components array
         size_t new_capacity = manager->component_capacity * 2;
         component_config_internal_t* new_components = polycall_core_realloc(
             ctx,
             manager->components,
             new_capacity * sizeof(component_config_internal_t)
         );
         
         if (!new_components) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to expand components array");
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         manager->components = new_components;
         manager->component_capacity = new_capacity;
     }
     
     // Add component configuration
     component_config_internal_t* internal_config = &manager->components[manager->component_count];
     memset(internal_config, 0, sizeof(component_config_internal_t));
     memcpy(&internal_config->config, config, sizeof(micro_component_config_t));
     internal_config->is_modified = true;
     
     // Deep copy allowed connections
     for (size_t i = 0; i < config->allowed_connections_count; i++) {
         if (config->allowed_connections[i]) {
             size_t len = strlen(config->allowed_connections[i]) + 1;
             internal_config->config.allowed_connections[i] = polycall_core_malloc(ctx, len);
             
             if (!internal_config->config.allowed_connections[i]) {
                 // Cleanup previously allocated connections
                 for (size_t j = 0; j < i; j++) {
                     polycall_core_free(ctx, internal_config->config.allowed_connections[j]);
                 }
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                                   POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to allocate memory for allowed connection");
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             memcpy(internal_config->config.allowed_connections[i], config->allowed_connections[i], len);
         }
     }
     
     manager->component_count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Remove component configuration
  */
 polycall_core_error_t micro_config_manager_remove_component(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const char* component_name
 ) {
     if (!ctx || !manager || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find component by name
     size_t index = SIZE_MAX;
     for (size_t i = 0; i < manager->component_count; i++) {
         if (strcmp(manager->components[i].config.name, component_name) == 0) {
             index = i;
             break;
         }
     }
     
     if (index == SIZE_MAX) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_NOT_FOUND,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Component configuration '%s' not found", component_name);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Free allowed connections
     micro_component_config_t* config = &manager->components[index].config;
     for (size_t i = 0; i < config->allowed_connections_count; i++) {
         if (config->allowed_connections[i]) {
             polycall_core_free(ctx, config->allowed_connections[i]);
         }
     }
     
     // Remove component by shifting remaining items
     for (size_t i = index; i < manager->component_count - 1; i++) {
         memcpy(&manager->components[i], &manager->components[i + 1], sizeof(component_config_internal_t));
     }
     
     manager->component_count--;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate component configuration
  */
 polycall_core_error_t micro_config_validate_component(
     polycall_core_context_t* ctx,
     const micro_component_config_t* config,
     micro_config_validation_status_t* status,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !config || !status) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize status
     *status = MICRO_CONFIG_VALIDATION_SUCCESS;
     
     // Validate component name
     if (strlen(config->name) == 0) {
         *status = MICRO_CONFIG_VALIDATION_NAME_CONFLICT;
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Component name cannot be empty");
         }
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Validate isolation level
     if (config->isolation_level < POLYCALL_ISOLATION_NONE || 
         config->isolation_level > POLYCALL_ISOLATION_STRICT) {
         *status = MICRO_CONFIG_VALIDATION_INVALID_ISOLATION;
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, 
                      "Invalid isolation level: %d", config->isolation_level);
         }
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Validate resource quotas
     if (config->enforce_quotas) {
         // Memory quota should be reasonable (not too small or too large)
         if (config->memory_quota < 1024 || config->memory_quota > 1024 * 1024 * 1024) {
             *status = MICRO_CONFIG_VALIDATION_INVALID_QUOTA;
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Invalid memory quota: %zu bytes", config->memory_quota);
             }
             return POLYCALL_CORE_SUCCESS;
         }
         
         // CPU quota should be reasonable
         if (config->cpu_quota < 100 || config->cpu_quota > 60000) {
             *status = MICRO_CONFIG_VALIDATION_INVALID_QUOTA;
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Invalid CPU quota: %u ms", config->cpu_quota);
             }
             return POLYCALL_CORE_SUCCESS;
         }
         
         // I/O quota should be reasonable
         if (config->io_quota < 10 || config->io_quota > 10000) {
             *status = MICRO_CONFIG_VALIDATION_INVALID_QUOTA;
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Invalid I/O quota: %u operations", config->io_quota);
             }
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Validate allowed connections
     if (config->allowed_connections_count > 16) {
         *status = MICRO_CONFIG_VALIDATION_INVALID_SECURITY;
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, 
                      "Too many allowed connections: %zu", config->allowed_connections_count);
         }
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Validate commands
     for (size_t i = 0; i < config->command_count; i++) {
         // Command name should not be empty
         if (strlen(config->commands[i].name) == 0) {
             *status = MICRO_CONFIG_VALIDATION_INVALID_COMMAND;
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Command name at index %zu cannot be empty", i);
             }
             return POLYCALL_CORE_SUCCESS;
         }
         
         // Check for duplicate command names
         for (size_t j = 0; j < i; j++) {
             if (strcmp(config->commands[i].name, config->commands[j].name) == 0) {
                 *status = MICRO_CONFIG_VALIDATION_INVALID_COMMAND;
                 if (error_message && error_message_size > 0) {
                     snprintf(error_message, error_message_size, 
                              "Duplicate command name: %s", config->commands[i].name);
                 }
                 return POLYCALL_CORE_SUCCESS;
             }
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default configuration for a component
  */
 polycall_core_error_t micro_config_create_default_component(
     polycall_core_context_t* ctx,
     const char* component_name,
     micro_component_config_t** config
 ) {
     if (!ctx || !component_name || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate configuration
     micro_component_config_t* new_config = polycall_core_malloc(ctx, sizeof(micro_component_config_t));
     if (!new_config) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate component configuration");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize configuration with default values
     memset(new_config, 0, sizeof(micro_component_config_t));
     strncpy(new_config->name, component_name, sizeof(new_config->name) - 1);
     new_config->isolation_level = POLYCALL_ISOLATION_MEMORY;
     new_config->memory_quota = 10 * 1024 * 1024;  // 10MB
     new_config->cpu_quota = 1000;                // 1000ms
     new_config->io_quota = 1000;                 // 1000 operations
     new_config->enforce_quotas = true;
     
     // Default security settings
     new_config->default_permissions = POLYCALL_PERMISSION_EXECUTE | POLYCALL_PERMISSION_READ;
     new_config->require_authentication = true;
     new_config->audit_access = true;
     
     *config = new_config;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Save configuration to file
  */
 polycall_core_error_t micro_config_manager_save(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     const char* file_path
 ) {
     if (!ctx || !manager || !file_path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Open file for writing
     FILE* file = fopen(file_path, "w");
     if (!file) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to open file for writing: %s", file_path);
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Write header
     fprintf(file, "# LibPolyCall Micro Component Configuration\n");
     fprintf(file, "# Generated configuration file\n\n");
     
     // Write micro section
     fprintf(file, "micro {\n");
     
     // Write component configurations
     for (size_t i = 0; i < manager->component_count; i++) {
         micro_component_config_t* config = &manager->components[i].config;
         
         fprintf(file, "    component %s {\n", config->name);
         
         // Write isolation level
         const char* isolation_str = "memory";
         switch (config->isolation_level) {
             case POLYCALL_ISOLATION_NONE:
                 isolation_str = "none";
                 break;
             case POLYCALL_ISOLATION_MEMORY:
                 isolation_str = "memory";
                 break;
             case POLYCALL_ISOLATION_RESOURCES:
                 isolation_str = "resources";
                 break;
             case POLYCALL_ISOLATION_SECURITY:
                 isolation_str = "security";
                 break;
             case POLYCALL_ISOLATION_STRICT:
                 isolation_str = "strict";
                 break;
         }
         fprintf(file, "        isolation_level = \"%s\"\n", isolation_str);
         
         // Write resource quotas
         fprintf(file, "        memory_quota = %zuB\n", config->memory_quota);
         fprintf(file, "        cpu_quota = %ums\n", config->cpu_quota);
         fprintf(file, "        io_quota = %u\n", config->io_quota);
         fprintf(file, "        enforce_quotas = %s\n", config->enforce_quotas ? "true" : "false");
         
         // Write security settings
         fprintf(file, "        require_authentication = %s\n", 
                 config->require_authentication ? "true" : "false");
         fprintf(file, "        audit_access = %s\n", 
                 config->audit_access ? "true" : "false");
         
         // Write allowed connections
         if (config->allowed_connections_count > 0) {
             fprintf(file, "        allowed_connections = [");
             for (size_t j = 0; j < config->allowed_connections_count; j++) {
                 if (j > 0) {
                     fprintf(file, ", ");
                 }
                 fprintf(file, "\"%s\"", config->allowed_connections[j]);
             }
             fprintf(file, "]\n");
         }
         
         // Write commands
         if (config->command_count > 0) {
             fprintf(file, "        commands {\n");
             for (size_t j = 0; j < config->command_count; j++) {
                 fprintf(file, "            %s {\n", config->commands[j].name);
                 
                 // Write command flags
                 fprintf(file, "                flags = ");
                 
                 bool first_flag = true;
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_ASYNC) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "async");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_SECURE) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "secure");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_PRIVILEGED) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "privileged");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_READONLY) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "readonly");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_CRITICAL) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "critical");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_RESTRICTED) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "restricted");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_EXTERNAL) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "external");
                     first_flag = false;
                 }
                 if (config->commands[j].flags & POLYCALL_COMMAND_FLAG_INTERNAL) {
                     fprintf(file, "%s\"%s\"", first_flag ? "" : " | ", "internal");
                     first_flag = false;
                 }
                 
                 if (first_flag) {
                     fprintf(file, "\"none\"");
                 }
                 
                 fprintf(file, "\n");
                 
                 // Write required permissions
                 fprintf(file, "                required_permissions = ");
                 
                 bool first_perm = true;
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_EXECUTE) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "execute");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_READ) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "read");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_WRITE) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "write");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_MEMORY) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "memory");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_IO) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "io");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_NETWORK) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "network");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_FILESYSTEM) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "filesystem");
                     first_perm = false;
                 }
                 if (config->commands[j].required_permissions & POLYCALL_PERMISSION_ADMIN) {
                     fprintf(file, "%s\"%s\"", first_perm ? "" : " | ", "admin");
                     first_perm = false;
                 }
                 
                 if (first_perm) {
                     fprintf(file, "\"none\"");
                 }
                 
                 fprintf(file, "\n");
                 
                 fprintf(file, "            }\n");
             }
             fprintf(file, "        }\n");
         }
         
         fprintf(file, "    }\n\n");
     }
     
     fprintf(file, "}\n");
     
     // Close file
     fclose(file);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default configuration manager options
  */
 micro_config_manager_options_t micro_config_create_default_options(void) {
     micro_config_manager_options_t options;
     
     options.global_config_path = "config.Polycallfile";
     options.binding_config_path = ".polycallrc";
     options.fallback_to_defaults = true;
     options.validate_on_load = true;
     options.error_callback = NULL;
     options.user_data = NULL;
     
     return options;
 }
 
 /**
  * @brief Parse configuration file
  */
 static polycall_core_error_t parse_config_file(
     polycall_core_context_t* ctx,
     const char* file_path,
     config_section_t** config,
     char* error_message,
     size_t error_message_size,
     int* error_line
 ) {
     if (!ctx || !file_path || !config || !error_message || !error_line) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Open file
     FILE* file = fopen(file_path, "r");
     if (!file) {
         snprintf(error_message, error_message_size, "Failed to open file: %s", file_path);
         *error_line = 0;
         return POLYCALL_CORE_ERROR_FILE_NOT_FOUND;
     }
     
     // Read file contents
     fseek(file, 0, SEEK_END);
     size_t file_size = ftell(file);
     fseek(file, 0, SEEK_SET);
     
     char* buffer = polycall_core_malloc(ctx, file_size + 1);
     if (!buffer) {
         fclose(file);
         snprintf(error_message, error_message_size, "Failed to allocate memory for file contents");
         *error_line = 0;
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     size_t bytes_read = fread(buffer, 1, file_size, file);
     fclose(file);
     
     if (bytes_read != file_size) {
         polycall_core_free(ctx, buffer);
         snprintf(error_message, error_message_size, "Failed to read file contents");
         *error_line = 0;
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     buffer[file_size] = '\0';
     
     // Create parse context
     parse_context_t parse_ctx;
     memset(&parse_ctx, 0, sizeof(parse_context_t));
     
     // Initialize tokenizer
     parse_ctx.tokenizer.input = buffer;
     parse_ctx.tokenizer.input_len = file_size;
     parse_ctx.tokenizer.pos = 0;
     parse_ctx.tokenizer.line = 1;
     parse_ctx.tokenizer.column = 1;
     parse_ctx.file_path = file_path;
     
     // Allocate root section
     config_section_t* root_section = polycall_core_malloc(ctx, sizeof(config_section_t));
     if (!root_section) {
         polycall_core_free(ctx, buffer);
         snprintf(error_message, error_message_size, "Failed to allocate root section");
         *error_line = 0;
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize root section
     memset(root_section, 0, sizeof(config_section_t));
     strcpy(root_section->name, "root");
     
     // Set root section in parse context
     parse_ctx.root = root_section;
     
     // Get first token
     parse_ctx.tokenizer.current_token = get_next_token(&parse_ctx.tokenizer);
     
     // Parse tokens
     polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
     while (parse_ctx.tokenizer.current_token.type != TOKEN_EOF) {
         result = process_token(ctx, &parse_ctx);
         if (result != POLYCALL_CORE_SUCCESS) {
             break;
         }
     }
     
     // Free buffer
     polycall_core_free(ctx, buffer);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         // Free root section on error
         free_config_section(ctx, root_section);
         
         // Copy error message and line number
         *error_line = parse_ctx.error_line;
         snprintf(error_message, error_message_size, "%s", parse_ctx.error_message);
         
         return result;
     }
     
     // Return root section
     *config = root_section;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Process token
  */
 static polycall_core_error_t process_token(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx
 ) {
     if (!ctx || !parse_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     token_t* token = &parse_ctx->tokenizer.current_token;
     
     // Handle different token types
     switch (token->type) {
         case TOKEN_IDENTIFIER: {
             // Check if next token is opening brace (section)
             tokenizer_t tokenizer_backup = parse_ctx->tokenizer;
             token_t next_token = get_next_token(&parse_ctx->tokenizer);
             
             if (next_token.type == TOKEN_SYMBOL && strcmp(next_token.value, "{") == 0) {
                 // Parse section
                 config_section_t* section = NULL;
                 polycall_core_error_t result = parse_section(ctx, parse_ctx, parse_ctx->root, &section);
                 if (result != POLYCALL_CORE_SUCCESS) {
                     return result;
                 }
             } else {
                 // Not a section, restore tokenizer and parse property
                 parse_ctx->tokenizer = tokenizer_backup;
                 polycall_core_error_t result = parse_property(ctx, parse_ctx, parse_ctx->root);
                 if (result != POLYCALL_CORE_SUCCESS) {
                     return result;
                 }
             }
             break;
         }
         
         case TOKEN_SYMBOL: {
             // Skip semicolons
             if (strcmp(token->value, ";") == 0) {
                 parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
                 break;
             }
             
             // Skip commas
             if (strcmp(token->value, ",") == 0) {
                 parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
                 break;
             }
             
             // Error on unexpected symbols
             snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                      "Unexpected symbol: %s", token->value);
             parse_ctx->error_line = token->line;
             parse_ctx->has_error = true;
             return POLYCALL_CORE_ERROR_PARSING_FAILED;
         }
         
         case TOKEN_EOF:
             // End of file
             break;
             
         default:
             // Unexpected token
             snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                      "Unexpected token type: %d", token->type);
             parse_ctx->error_line = token->line;
             parse_ctx->has_error = true;
             return POLYCALL_CORE_ERROR_PARSING_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get next token
  */
 static token_t get_next_token(tokenizer_t* tokenizer) {
     if (!tokenizer) {
         token_t eof_token;
         memset(&eof_token, 0, sizeof(token_t));
         eof_token.type = TOKEN_EOF;
         return eof_token;
     }
     
     // Skip whitespace
     while (tokenizer->pos < tokenizer->input_len && 
            isspace((unsigned char)tokenizer->input[tokenizer->pos])) {
         if (tokenizer->input[tokenizer->pos] == '\n') {
             tokenizer->line++;
             tokenizer->column = 1;
         } else {
             tokenizer->column++;
         }
         tokenizer->pos++;
     }
     
     // Check for EOF
     if (tokenizer->pos >= tokenizer->input_len) {
         token_t eof_token;
         memset(&eof_token, 0, sizeof(token_t));
         eof_token.type = TOKEN_EOF;
         return eof_token;
     }
     
     // Initialize token
     token_t token;
     memset(&token, 0, sizeof(token_t));
     token.line = tokenizer->line;
     token.column = tokenizer->column;
     
     // Check for comment
     if (tokenizer->input[tokenizer->pos] == '#') {
         // Skip comment until end of line
         while (tokenizer->pos < tokenizer->input_len && 
                tokenizer->input[tokenizer->pos] != '\n') {
             tokenizer->pos++;
             tokenizer->column++;
         }
         
         // Recursive call to get next token
         return get_next_token(tokenizer);
     }
     
     // Check for string
     if (tokenizer->input[tokenizer->pos] == '"') {
         token.type = TOKEN_STRING;
         tokenizer->pos++;
         tokenizer->column++;
         
         size_t value_len = 0;
         while (tokenizer->pos < tokenizer->input_len && 
                tokenizer->input[tokenizer->pos] != '"') {
             if (tokenizer->input[tokenizer->pos] == '\\' && 
                 tokenizer->pos + 1 < tokenizer->input_len) {
                 // Handle escape sequences
                 tokenizer->pos++;
                 tokenizer->column++;
                 
                 if (tokenizer->input[tokenizer->pos] == 'n') {
                     token.value[value_len++] = '\n';
                 } else if (tokenizer->input[tokenizer->pos] == 't') {
                     token.value[value_len++] = '\t';
                 } else if (tokenizer->input[tokenizer->pos] == 'r') {
                     token.value[value_len++] = '\r';
                 } else if (tokenizer->input[tokenizer->pos] == '"') {
                     token.value[value_len++] = '"';
                 } else if (tokenizer->input[tokenizer->pos] == '\\') {
                     token.value[value_len++] = '\\';
                 } else {
                     // Unknown escape sequence, just use the character
                     token.value[value_len++] = tokenizer->input[tokenizer->pos];
                 }
             } else {
                 token.value[value_len++] = tokenizer->input[tokenizer->pos];
             }
             
             tokenizer->pos++;
             tokenizer->column++;
             
             if (value_len >= MAX_LINE_LENGTH - 1) {
                 break;  // Avoid buffer overflow
             }
         }
         
         token.value[value_len] = '\0';
         
         // Skip closing quote
         if (tokenizer->pos < tokenizer->input_len && tokenizer->input[tokenizer->pos] == '"') {
             tokenizer->pos++;
             tokenizer->column++;
         }
         
         return token;
     }
     
     // Check for identifier
     if (isalpha((unsigned char)tokenizer->input[tokenizer->pos]) || tokenizer->input[tokenizer->pos] == '_') {
         token.type = TOKEN_IDENTIFIER;
         
         size_t value_len = 0;
         while (tokenizer->pos < tokenizer->input_len && 
                (isalnum((unsigned char)tokenizer->input[tokenizer->pos]) || 
                 tokenizer->input[tokenizer->pos] == '_')) {
             token.value[value_len++] = tokenizer->input[tokenizer->pos];
             tokenizer->pos++;
             tokenizer->column++;
             
             if (value_len >= MAX_LINE_LENGTH - 1) {
                 break;  // Avoid buffer overflow
             }
         }
         
         token.value[value_len] = '\0';
         
         return token;
     }
     
     // Check for number
     if (isdigit((unsigned char)tokenizer->input[tokenizer->pos]) || 
         (tokenizer->input[tokenizer->pos] == '.' && 
          tokenizer->pos + 1 < tokenizer->input_len && 
          isdigit((unsigned char)tokenizer->input[tokenizer->pos + 1]))) {
         token.type = TOKEN_NUMBER;
         
         size_t value_len = 0;
         while (tokenizer->pos < tokenizer->input_len && 
                (isdigit((unsigned char)tokenizer->input[tokenizer->pos]) || 
                 tokenizer->input[tokenizer->pos] == '.' ||
                 tokenizer->input[tokenizer->pos] == 'e' ||
                 tokenizer->input[tokenizer->pos] == 'E' ||
                 tokenizer->input[tokenizer->pos] == '+' ||
                 tokenizer->input[tokenizer->pos] == '-')) {
             token.value[value_len++] = tokenizer->input[tokenizer->pos];
             tokenizer->pos++;
             tokenizer->column++;
             
             if (value_len >= MAX_LINE_LENGTH - 1) {
                 break;  // Avoid buffer overflow
             }
         }
         
         // Check for unit suffixes (KB, MB, GB, ms, s, etc.)
         if (tokenizer->pos < tokenizer->input_len && 
             (isalpha((unsigned char)tokenizer->input[tokenizer->pos]) ||
              tokenizer->input[tokenizer->pos] == 'B')) {
             
             size_t unit_start = tokenizer->pos;
             while (tokenizer->pos < tokenizer->input_len && 
                    isalpha((unsigned char)tokenizer->input[tokenizer->pos])) {
                 token.value[value_len++] = tokenizer->input[tokenizer->pos];
                 tokenizer->pos++;
                 tokenizer->column++;
                 
                 if (value_len >= MAX_LINE_LENGTH - 1) {
                     break;  // Avoid buffer overflow
                 }
             }
         }
         
         token.value[value_len] = '\0';
         
         return token;
     }
     
     // Check for symbol
     token.type = TOKEN_SYMBOL;
     token.value[0] = tokenizer->input[tokenizer->pos];
     token.value[1] = '\0';
     tokenizer->pos++;
     tokenizer->column++;
     
     return token;
 }
 
 /**
  * @brief Parse section
  */
 static polycall_core_error_t parse_section(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_section_t* parent,
     config_section_t** section
 ) {
     if (!ctx || !parse_ctx || !parent || !section) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get section name (should be an identifier)
     token_t* token = &parse_ctx->tokenizer.current_token;
     if (token->type != TOKEN_IDENTIFIER) {
         snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                  "Expected section name, got token type %d", token->type);
         parse_ctx->error_line = token->line;
         parse_ctx->has_error = true;
         return POLYCALL_CORE_ERROR_PARSING_FAILED;
     }
     
     // Create section
     config_section_t* new_section = NULL;
     
     // Check if section already exists
     for (size_t i = 0; i < parent->section_count; i++) {
         if (strcmp(parent->sections[i].name, token->value) == 0) {
             new_section = &parent->sections[i];
             break;
         }
     }
     
     if (!new_section) {
         // Allocate new section if not found
         if (parent->section_count >= parent->section_capacity) {
             size_t new_capacity = parent->section_capacity == 0 ? 8 : parent->section_capacity * 2;
             config_section_t* new_sections = polycall_core_realloc(
                 ctx,
                 parent->sections,
                 new_capacity * sizeof(config_section_t)
             );
             
             if (!new_sections) {
                 snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                          "Failed to allocate memory for section");
                 parse_ctx->error_line = token->line;
                 parse_ctx->has_error = true;
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             parent->sections = new_sections;
             parent->section_capacity = new_capacity;
         }
         
         new_section = &parent->sections[parent->section_count++];
         memset(new_section, 0, sizeof(config_section_t));
         strncpy(new_section->name, token->value, sizeof(new_section->name) - 1);
         new_section->parent = parent;
     }
     
     // Move to next token (should be opening brace)
     parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
     token = &parse_ctx->tokenizer.current_token;
     
     if (token->type != TOKEN_SYMBOL || strcmp(token->value, "{") != 0) {
         snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                  "Expected opening brace, got %s", token->value);
         parse_ctx->error_line = token->line;
         parse_ctx->has_error = true;
         return POLYCALL_CORE_ERROR_PARSING_FAILED;
     }
     
     // Move to first token inside section
     parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
     
     // Parse section contents until closing brace
     while (parse_ctx->tokenizer.current_token.type != TOKEN_EOF) {
         token = &parse_ctx->tokenizer.current_token;
         
         // Check for closing brace
         if (token->type == TOKEN_SYMBOL && strcmp(token->value, "}") == 0) {
             // Move to next token after closing brace
             parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
             break;
         }
         
         if (token->type == TOKEN_IDENTIFIER) {
             // Check if next token is opening brace (section)
             tokenizer_t tokenizer_backup = parse_ctx->tokenizer;
             token_t next_token = get_next_token(&parse_ctx->tokenizer);
             
             if (next_token.type == TOKEN_SYMBOL && strcmp(next_token.value, "{") == 0) {
                 // Parse nested section
                 config_section_t* nested_section = NULL;
                 polycall_core_error_t result = parse_section(ctx, parse_ctx, new_section, &nested_section);
                 if (result != POLYCALL_CORE_SUCCESS) {
                     return result;
                 }
             } else {
                 // Not a section, restore tokenizer and parse property
                 parse_ctx->tokenizer = tokenizer_backup;
                 polycall_core_error_t result = parse_property(ctx, parse_ctx, new_section);
                 if (result != POLYCALL_CORE_SUCCESS) {
                     return result;
                 }
             }
         } else {
             // Unexpected token
             snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                      "Unexpected token type %d in section contents", token->type);
             parse_ctx->error_line = token->line;
             parse_ctx->has_error = true;
             return POLYCALL_CORE_ERROR_PARSING_FAILED;
         }
     }
     
     // Return section
     *section = new_section;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Parse property
  */
 static polycall_core_error_t parse_property(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_section_t* section
 ) {
     if (!ctx || !parse_ctx || !section) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get property name (should be an identifier)
     token_t* token = &parse_ctx->tokenizer.current_token;
     if (token->type != TOKEN_IDENTIFIER) {
         snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                  "Expected property name, got token type %d", token->type);
         parse_ctx->error_line = token->line;
         parse_ctx->has_error = true;
         return POLYCALL_CORE_ERROR_PARSING_FAILED;
     }
     
     // Save property name
     char property_name[64];
     strncpy(property_name, token->value, sizeof(property_name) - 1);
     property_name[sizeof(property_name) - 1] = '\0';
     
     // Move to next token (should be equals sign)
     parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
     token = &parse_ctx->tokenizer.current_token;
     
     if (token->type != TOKEN_SYMBOL || strcmp(token->value, "=") != 0) {
         snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                  "Expected equals sign, got %s", token->value);
         parse_ctx->error_line = token->line;
         parse_ctx->has_error = true;
         return POLYCALL_CORE_ERROR_PARSING_FAILED;
     }
     
     // Move to property value
     parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
     
     // Allocate property
     config_property_t* property = polycall_core_malloc(ctx, sizeof(config_property_t));
     if (!property) {
         snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                  "Failed to allocate memory for property");
         parse_ctx->error_line = token->line;
         parse_ctx->has_error = true;
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize property
     memset(property, 0, sizeof(config_property_t));
     strncpy(property->name, property_name, sizeof(property->name) - 1);
     
     // Parse value
     polycall_core_error_t result = parse_value(ctx, parse_ctx, &property->value);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, property);
         return result;
     }
     
     // Check for semicolon
     token = &parse_ctx->tokenizer.current_token;
     if (token->type == TOKEN_SYMBOL && strcmp(token->value, ";") == 0) {
         // Skip semicolon
         parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
     }
     
     // Add property to section
     property->next = section->properties;
     section->properties = property;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Parse value
  */
 static polycall_core_error_t parse_value(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_value_t* value
 ) {
     if (!ctx || !parse_ctx || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     token_t* token = &parse_ctx->tokenizer.current_token;
     
     // Initialize value
     memset(value, 0, sizeof(config_value_t));
     
     switch (token->type) {
         case TOKEN_STRING: {
             // String value
             value->type = CONFIG_VALUE_STRING;
             value->data.string_value = polycall_core_malloc(ctx, strlen(token->value) + 1);
             if (!value->data.string_value) {
                 snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                          "Failed to allocate memory for string value");
                 parse_ctx->error_line = token->line;
                 parse_ctx->has_error = true;
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             strcpy(value->data.string_value, token->value);
             
             // Move to next token
             parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
             break;
         }
         
         case TOKEN_NUMBER: {
             // Number value
             value->type = CONFIG_VALUE_NUMBER;
             value->data.number_value = atof(token->value);
             
             // Move to next token
             parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
             break;
         }
         
         case TOKEN_IDENTIFIER: {
             // Boolean or null value
             if (strcmp(token->value, "true") == 0) {
                 value->type = CONFIG_VALUE_BOOLEAN;
                 value->data.boolean_value = true;
             } else if (strcmp(token->value, "false") == 0) {
                 value->type = CONFIG_VALUE_BOOLEAN;
                 value->data.boolean_value = false;
             } else if (strcmp(token->value, "null") == 0) {
                 value->type = CONFIG_VALUE_NULL;
             } else {
                 // Identifier is treated as a string
                 value->type = CONFIG_VALUE_STRING;
                 value->data.string_value = polycall_core_malloc(ctx, strlen(token->value) + 1);
                 if (!value->data.string_value) {
                     snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                              "Failed to allocate memory for identifier value");
                     parse_ctx->error_line = token->line;
                     parse_ctx->has_error = true;
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 strcpy(value->data.string_value, token->value);
             }
             
             // Move to next token
             parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
             break;
         }
         
         case TOKEN_SYMBOL: {
             // Check for array
             if (strcmp(token->value, "[") == 0) {
                 return parse_array(ctx, parse_ctx, value);
             } else {
                 snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                          "Unexpected symbol in value: %s", token->value);
                 parse_ctx->error_line = token->line;
                 parse_ctx->has_error = true;
                 return POLYCALL_CORE_ERROR_PARSING_FAILED;
             }
             break;
         }
         
         default:
             snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                      "Unexpected token type in value: %d", token->type);
             parse_ctx->error_line = token->line;
             parse_ctx->has_error = true;
             return POLYCALL_CORE_ERROR_PARSING_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Parse array
  */
 static polycall_core_error_t parse_array(
     polycall_core_context_t* ctx,
     parse_context_t* parse_ctx,
     config_value_t* value
 ) {
     if (!ctx || !parse_ctx || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize array value
     value->type = CONFIG_VALUE_ARRAY;
     value->data.array.items = NULL;
     value->data.array.count = 0;
     value->data.array.capacity = 0;
     
     // Skip opening bracket
     parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
     
     // Parse array elements
     while (parse_ctx->tokenizer.current_token.type != TOKEN_EOF) {
         token_t* token = &parse_ctx->tokenizer.current_token;
         
         // Check for closing bracket
         if (token->type == TOKEN_SYMBOL && strcmp(token->value, "]") == 0) {
             // Skip closing bracket
             parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
             break;
         }
         
         // Check for comma separator
         if (value->data.array.count > 0) {
             if (token->type != TOKEN_SYMBOL || strcmp(token->value, ",") != 0) {
                 snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                          "Expected comma separator in array, got %s", token->value);
                 parse_ctx->error_line = token->line;
                 parse_ctx->has_error = true;
                 return POLYCALL_CORE_ERROR_PARSING_FAILED;
             }
             
             // Skip comma
             parse_ctx->tokenizer.current_token = get_next_token(&parse_ctx->tokenizer);
         }
         
         // Ensure array capacity
         if (value->data.array.count >= value->data.array.capacity) {
             size_t new_capacity = value->data.array.capacity == 0 ? 8 : value->data.array.capacity * 2;
             config_value_t** new_items = polycall_core_realloc(
                 ctx,
                 value->data.array.items,
                 new_capacity * sizeof(config_value_t*)
             );
             
             if (!new_items) {
                 snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                          "Failed to allocate memory for array items");
                 parse_ctx->error_line = token->line;
                 parse_ctx->has_error = true;
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             value->data.array.items = new_items;
             value->data.array.capacity = new_capacity;
         }
         
         // Allocate new item
         config_value_t* item = polycall_core_malloc(ctx, sizeof(config_value_t));
         if (!item) {
             snprintf(parse_ctx->error_message, sizeof(parse_ctx->error_message),
                      "Failed to allocate memory for array item");
             parse_ctx->error_line = token->line;
             parse_ctx->has_error = true;
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Parse item value
         polycall_core_error_t result = parse_value(ctx, parse_ctx, item);
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_core_free(ctx, item);
             return result;
         }
         
         // Add item to array
         value->data.array.items[value->data.array.count++] = item;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Free config section
  */
 static void free_config_section(
     polycall_core_context_t* ctx,
     config_section_t* section
 ) {
     if (!ctx || !section) {
         return;
     }
     
     // Free properties
     config_property_t* property = section->properties;
     while (property) {
         config_property_t* next = property->next;
         free_config_property(ctx, property);
         property = next;
     }
     
     // Free nested sections
     for (size_t i = 0; i < section->section_count; i++) {
         free_config_section(ctx, &section->sections[i]);
     }
     
     // Free sections array
     if (section->sections) {
         polycall_core_free(ctx, section->sections);
     }
 }
 
 /**
  * @brief Free config property
  */
 static void free_config_property(
     polycall_core_context_t* ctx,
     config_property_t* property
 ) {
     if (!ctx || !property) {
         return;
     }
     
     // Free value
     free_config_value(ctx, &property->value);
     
     // Free property
     polycall_core_free(ctx, property);
 }
 
 /**
  * @brief Free config value
  */
 static void free_config_value(
     polycall_core_context_t* ctx,
     config_value_t* value
 ) {
     if (!ctx || !value) {
         return;
     }
     
     switch (value->type) {
         case CONFIG_VALUE_STRING:
             if (value->data.string_value) {
                 polycall_core_free(ctx, value->data.string_value);
             }
             break;
             
         case CONFIG_VALUE_ARRAY:
             for (size_t i = 0; i < value->data.array.count; i++) {
                 if (value->data.array.items[i]) {
                     free_config_value(ctx, value->data.array.items[i]);
                     polycall_core_free(ctx, value->data.array.items[i]);
                 }
             }
             
             if (value->data.array.items) {
                 polycall_core_free(ctx, value->data.array.items);
             }
             break;
             
         default:
             // Nothing to free for other value types
             break;
     }
 }
 
 /**
  * @brief Extract component configurations
  */
 static polycall_core_error_t extract_component_configs(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     config_section_t* micro_section
 ) {
     if (!ctx || !manager || !micro_section) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find component sections
     for (size_t i = 0; i < micro_section->section_count; i++) {
         config_section_t* section = &micro_section->sections[i];
         
         // Check if this is a component section
         if (strcmp(section->name, "component") == 0) {
             // Component section must have a name property
             config_property_t* name_prop = find_property(section, "name");
             if (!name_prop || name_prop->value.type != CONFIG_VALUE_STRING) {
                 continue;  // Skip component without name
             }
             
             // Create component configuration
             micro_component_config_t component_config;
             memset(&component_config, 0, sizeof(micro_component_config_t));
             
             // Set component name
             strncpy(component_config.name, name_prop->value.data.string_value, 
                    sizeof(component_config.name) - 1);
             
             // Set isolation level
             config_property_t* isolation_prop = find_property(section, "isolation_level");
             if (isolation_prop && isolation_prop->value.type == CONFIG_VALUE_STRING) {
                 const char* isolation_str = isolation_prop->value.data.string_value;
                 
                 if (strcmp(isolation_str, "none") == 0) {
                     component_config.isolation_level = POLYCALL_ISOLATION_NONE;
                 } else if (strcmp(isolation_str, "memory") == 0) {
                     component_config.isolation_level = POLYCALL_ISOLATION_MEMORY;
                 } else if (strcmp(isolation_str, "resources") == 0) {
                     component_config.isolation_level = POLYCALL_ISOLATION_RESOURCES;
                 } else if (strcmp(isolation_str, "security") == 0) {
                     component_config.isolation_level = POLYCALL_ISOLATION_SECURITY;
                 } else if (strcmp(isolation_str, "strict") == 0) {
                     component_config.isolation_level = POLYCALL_ISOLATION_STRICT;
                 } else {
                     // Default to memory isolation
                     component_config.isolation_level = POLYCALL_ISOLATION_MEMORY;
                 }
             } else {
                 // Default to memory isolation
                 component_config.isolation_level = POLYCALL_ISOLATION_MEMORY;
             }
             
             // Set resource quotas
             config_property_t* memory_quota_prop = find_property(section, "memory_quota");
             if (memory_quota_prop && memory_quota_prop->value.type == CONFIG_VALUE_NUMBER) {
                 component_config.memory_quota = (size_t)memory_quota_prop->value.data.number_value;
             } else {
                 // Default memory quota
                 component_config.memory_quota = 10 * 1024 * 1024;  // 10MB
             }
             
             config_property_t* cpu_quota_prop = find_property(section, "cpu_quota");
             if (cpu_quota_prop && cpu_quota_prop->value.type == CONFIG_VALUE_NUMBER) {
                 component_config.cpu_quota = (uint32_t)cpu_quota_prop->value.data.number_value;
             } else {
                 // Default CPU quota
                 component_config.cpu_quota = 1000;  // 1000ms
             }
             
             config_property_t* io_quota_prop = find_property(section, "io_quota");
             if (io_quota_prop && io_quota_prop->value.type == CONFIG_VALUE_NUMBER) {
                 component_config.io_quota = (uint32_t)io_quota_prop->value.data.number_value;
             } else {
                 // Default I/O quota
                 component_config.io_quota = 1000;  // 1000 operations
             }
             
             config_property_t* enforce_quotas_prop = find_property(section, "enforce_quotas");
             if (enforce_quotas_prop && enforce_quotas_prop->value.type == CONFIG_VALUE_BOOLEAN) {
                 component_config.enforce_quotas = enforce_quotas_prop->value.data.boolean_value;
             } else {
                 // Default to enforcing quotas
                 component_config.enforce_quotas = true;
             }
             
             // Set security settings
             config_property_t* require_auth_prop = find_property(section, "require_authentication");
             if (require_auth_prop && require_auth_prop->value.type == CONFIG_VALUE_BOOLEAN) {
                 component_config.require_authentication = require_auth_prop->value.data.boolean_value;
             } else {
                 // Default to requiring authentication
                 component_config.require_authentication = true;
             }
             
             config_property_t* audit_access_prop = find_property(section, "audit_access");
             if (audit_access_prop && audit_access_prop->value.type == CONFIG_VALUE_BOOLEAN) {
                 component_config.audit_access = audit_access_prop->value.data.boolean_value;
             } else {
                 // Default to auditing access
                 component_config.audit_access = true;
             }
             
             // Set allowed connections
             config_property_t* allowed_connections_prop = find_property(section, "allowed_connections");
             if (allowed_connections_prop && allowed_connections_prop->value.type == CONFIG_VALUE_ARRAY) {
                 // Extract allowed connections from array
                 for (size_t j = 0; j < allowed_connections_prop->value.data.array.count && 
                                    j < sizeof(component_config.allowed_connections) / sizeof(char*); j++) {
                     
                     config_value_t* conn_value = allowed_connections_prop->value.data.array.items[j];
                     if (conn_value && conn_value->type == CONFIG_VALUE_STRING) {
                         // Allocate memory for connection name
                         component_config.allowed_connections[j] = polycall_core_malloc(
                             ctx, strlen(conn_value->data.string_value) + 1);
                         
                         if (!component_config.allowed_connections[j]) {
                             // Clean up previously allocated connections
                             for (size_t k = 0; k < j; k++) {
                                 polycall_core_free(ctx, component_config.allowed_connections[k]);
                             }
                             
                             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                               POLYCALL_ERROR_SEVERITY_ERROR, 
                                               "Failed to allocate memory for allowed connection");
                             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                         }
                         
                         strcpy(component_config.allowed_connections[j], conn_value->data.string_value);
                         component_config.allowed_connections_count++;
                     }
                 }
             }
             
             // Extract command configurations
             config_section_t* commands_section = find_section(section, "commands");
             if (commands_section) {
                 for (size_t j = 0; j < commands_section->section_count && 
                                   j < sizeof(component_config.commands) / sizeof(component_config.commands[0]); j++) {
                     
                     config_section_t* cmd_section = &commands_section->sections[j];
                     
                     // Set command name
                     strncpy(component_config.commands[j].name, cmd_section->name,
                            sizeof(component_config.commands[j].name) - 1);
                     
                     // Set command flags
                     config_property_t* flags_prop = find_property(cmd_section, "flags");
                     if (flags_prop && flags_prop->value.type == CONFIG_VALUE_STRING) {
                         const char* flags_str = flags_prop->value.data.string_value;
                         
                         // Parse flags string
                         if (strstr(flags_str, "async")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_ASYNC;
                         }
                         if (strstr(flags_str, "secure")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_SECURE;
                         }
                         if (strstr(flags_str, "privileged")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_PRIVILEGED;
                         }
                         if (strstr(flags_str, "readonly")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_READONLY;
                         }
                         if (strstr(flags_str, "critical")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_CRITICAL;
                         }
                         if (strstr(flags_str, "restricted")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_RESTRICTED;
                         }
                         if (strstr(flags_str, "external")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_EXTERNAL;
                         }
                         if (strstr(flags_str, "internal")) {
                             component_config.commands[j].flags |= POLYCALL_COMMAND_FLAG_INTERNAL;
                         }
                     }
                     
                     // Set required permissions
                     config_property_t* perms_prop = find_property(cmd_section, "required_permissions");
                     if (perms_prop && perms_prop->value.type == CONFIG_VALUE_STRING) {
                         const char* perms_str = perms_prop->value.data.string_value;
                         
                         // Parse permissions string
                         if (strstr(perms_str, "execute")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_EXECUTE;
                         }
                         if (strstr(perms_str, "read")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_READ;
                         }
                         if (strstr(perms_str, "write")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_WRITE;
                         }
                         if (strstr(perms_str, "memory")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_MEMORY;
                         }
                         if (strstr(perms_str, "io")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_IO;
                         }
                         if (strstr(perms_str, "network")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_NETWORK;
                         }
                         if (strstr(perms_str, "filesystem")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_FILESYSTEM;
                         }
                         if (strstr(perms_str, "admin")) {
                             component_config.commands[j].required_permissions |= POLYCALL_PERMISSION_ADMIN;
                         }
                     } else {
                         // Default to execute permission
                         component_config.commands[j].required_permissions = POLYCALL_PERMISSION_EXECUTE;
                     }
                     
                     component_config.command_count++;
                 }
             }
             
             // Add component configuration to manager
             polycall_core_error_t result = micro_config_manager_add_component(
                 ctx, manager, &component_config);
                 
             if (result != POLYCALL_CORE_SUCCESS) {
                 // Clean up allowed connections
                 for (size_t j = 0; j < component_config.allowed_connections_count; j++) {
                     polycall_core_free(ctx, component_config.allowed_connections[j]);
                 }
                 
                 return result;
             }
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Apply component configuration
  */
 static polycall_core_error_t apply_component_config(
     polycall_core_context_t* ctx,
     micro_config_manager_t* manager,
     polycall_micro_context_t* micro_ctx,
     const micro_component_config_t* component_config
 ) {
     if (!ctx || !manager || !micro_ctx || !component_config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Try to find existing component
     polycall_micro_component_t* component = NULL;
     polycall_core_error_t result = polycall_micro_find_component(
         ctx, micro_ctx, component_config->name, &component);
         
     if (result == POLYCALL_CORE_SUCCESS) {
         // Component exists, update its configuration
         
         // Update resource limits
         if (component_config->enforce_quotas) {
             result = polycall_micro_set_resource_limits(
                 ctx,
                 micro_ctx,
                 component,
                 component_config->memory_quota,
                 component_config->cpu_quota,
                 component_config->io_quota
             );
             
             if (result != POLYCALL_CORE_SUCCESS) {
                 return result;
             }
         }
         
         // Update security context (if necessary)
         // In a full implementation, we would update the security context here
         
     } else {
         // Component doesn't exist, create it
         result = polycall_micro_create_component(
             ctx,
             micro_ctx,
             &component,
             component_config->name,
             component_config->isolation_level
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
         
         // Set resource limits
         if (component_config->enforce_quotas) {
             result = polycall_micro_set_resource_limits(
                 ctx,
                 micro_ctx,
                 component,
                 component_config->memory_quota,
                 component_config->cpu_quota,
                 component_config->io_quota
             );
             
             if (result != POLYCALL_CORE_SUCCESS) {
                 polycall_micro_destroy_component(ctx, micro_ctx, component);
                 return result;
             }
         }
         
         // Register commands
         for (size_t i = 0; i < component_config->command_count; i++) {
             // In a full implementation, we would register each command
             // For this implementation, we'll just log that we would register the command
             POLYCALL_LOG(ctx, POLYCALL_LOG_INFO, 
                         "Would register command '%s' for component '%s'",
                         component_config->commands[i].name,
                         component_config->name);
         }
         
         // Start component
         result = polycall_micro_start_component(ctx, micro_ctx, component);
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_micro_destroy_component(ctx, micro_ctx, component);
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Find property in section
  */
 static config_property_t* find_property(
     config_section_t* section,
     const char* name
 ) {
     if (!section || !name) {
         return NULL;
     }
     
     config_property_t* property = section->properties;
     while (property) {
         if (strcmp(property->name, name) == 0) {
             return property;
         }
         property = property->next;
     }
     
     return NULL;
 }
 
 /**
  * @brief Find section in parent
  */
 static config_section_t* find_section(
     config_section_t* parent,
     const char* name
 ) {
     if (!parent || !name) {
         return NULL;
     }
     
     for (size_t i = 0; i < parent->section_count; i++) {
         if (strcmp(parent->sections[i].name, name) == 0) {
             return &parent->sections[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Get string value from section
  */
 static bool get_string_value(
     config_section_t* section,
     const char* property_name,
     char* buffer,
     size_t buffer_size
 ) {
     if (!section || !property_name || !buffer || buffer_size == 0) {
         return false;
     }
     
     config_property_t* property = find_property(section, property_name);
     if (!property || property->value.type != CONFIG_VALUE_STRING) {
         return false;
     }
     
     strncpy(buffer, property->value.data.string_value, buffer_size - 1);
     buffer[buffer_size - 1] = '\0';
     
     return true;
 }
 
 /**
  * @brief Get number value from section
  */
 static bool get_number_value(
     config_section_t* section,
     const char* property_name,
     double* value
 ) {
     if (!section || !property_name || !value) {
         return false;
     }
     
     config_property_t* property = find_property(section, property_name);
     if (!property || property->value.type != CONFIG_VALUE_NUMBER) {
         return false;
     }
     
     *value = property->value.data.number_value;
     
     return true;
 }
 
 /**
  * @brief Get integer value from section
  */
 static bool get_integer_value(
     config_section_t* section,
     const char* property_name,
     int64_t* value
 ) {
     double number_value;
     if (!get_number_value(section, property_name, &number_value)) {
         return false;
     }
     
     *value = (int64_t)number_value;
     
     return true;
 }
 
 /**
  * @brief Get boolean value from section
  */
 static bool get_boolean_value(
     config_section_t* section,
     const char* property_name,
     bool* value
 ) {
     if (!section || !property_name || !value) {
         return false;
     }
     
     config_property_t* property = find_property(section, property_name);
     if (!property || property->value.type != CONFIG_VALUE_BOOLEAN) {
         return false;
     }
     
     *value = property->value.data.boolean_value;
     
     return true;
 }
 
 /**
  * @brief Get string array from section
  */
 static bool get_string_array(
     config_section_t* section,
     const char* property_name,
     char** array,
     size_t array_size,
     size_t* count
 ) {
     if (!section || !property_name || !array || !count) {
         return false;
     }
     
     config_property_t* property = find_property(section, property_name);
     if (!property || property->value.type != CONFIG_VALUE_ARRAY) {
         return false;
     }
     
     *count = 0;
     for (size_t i = 0; i < property->value.data.array.count && i < array_size; i++) {
         config_value_t* item = property->value.data.array.items[i];
         if (item && item->type == CONFIG_VALUE_STRING) {
             array[i] = item->data.string_value;
             (*count)++;
         }
     }
     
     return *count > 0;
 }