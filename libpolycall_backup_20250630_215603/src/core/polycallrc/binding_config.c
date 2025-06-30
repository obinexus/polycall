/**
 * @file binding_config.c
 * @brief Binding configuration implementation for LibPolyCall
 * @author Based on Nnamdi Okpala's design
 *
 * This file implements binding-specific configuration loading and management,
 * with support for ignore patterns to exclude specific files and directories.
 */

 #include "polycall/core/config/polycallrc/binding_config.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/config/config_parser.h"
 #include "polycall/core/config/path_utils.h"
 #include "polycall/core/config/ignore/polycall_ignore.h"
 
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 #define POLYCALL_BINDING_CONFIG_MAGIC 0xB1ND1N6C
 #define MAX_CONFIG_SECTIONS 32
 #define MAX_KEY_LENGTH 128
 #define MAX_VALUE_LENGTH 1024
 #define MAX_PATH_LENGTH 512
 
 #define DEFAULT_RC_FILENAME ".polycallrc"
 #define DEFAULT_IGNORE_FILENAME ".polycallrc.ignore"
 
 /**
  * @brief Config value structure
  */
 typedef struct {
     char* key;
     char* value;
 } binding_config_value_t;
 
 /**
  * @brief Config section structure
  */
 typedef struct {
     char* name;
     binding_config_value_t* values;
     size_t value_count;
     size_t value_capacity;
 } binding_config_section_t;
 
 /**
  * @brief Binding configuration context structure
  */
 struct polycall_binding_config_context {
     uint32_t magic;                             // Magic number for validation
     polycall_core_context_t* core_ctx;          // Core context
     
     binding_config_section_t* sections;         // Array of config sections
     size_t section_count;                       // Number of sections
     size_t section_capacity;                    // Capacity of sections array
     
     char config_file_path[MAX_PATH_LENGTH];     // Path to configuration file
     bool has_config_file;                       // Whether a config file is loaded
     
     polycall_ignore_context_t* ignore_ctx;      // Ignore context
     bool use_ignore_patterns;                   // Whether to use ignore patterns
 };
 
 /**
  * @brief Validate binding configuration context
  */
 static bool validate_binding_config_context(
     polycall_binding_config_context_t* ctx
 ) {
     return ctx && ctx->magic == POLYCALL_BINDING_CONFIG_MAGIC;
 }
 
 /**
  * @brief Ensure a section has enough capacity for a new value
  * 
  * @param ctx Core context
  * @param section Section to check
  * @return bool True if successful, false on allocation failure
  */
 static bool ensure_value_capacity(
     polycall_core_context_t* ctx,
     binding_config_section_t* section
 ) {
     if (section->value_count == section->value_capacity) {
         // Expand capacity
         size_t new_capacity = section->value_capacity == 0 ? 4 : section->value_capacity * 2;
         binding_config_value_t* new_values = polycall_core_realloc(
             ctx,
             section->values,
             new_capacity * sizeof(binding_config_value_t)
         );
         
         if (!new_values) {
             return false;
         }
         
         section->values = new_values;
         section->value_capacity = new_capacity;
     }
     
     return true;
 }
 
 /**
  * @brief Ensure the context has enough capacity for a new section
  * 
  * @param ctx Binding config context
  * @return bool True if successful, false on allocation failure
  */
 static bool ensure_section_capacity(
     polycall_binding_config_context_t* ctx
 ) {
     if (ctx->section_count == ctx->section_capacity) {
         // Expand capacity
         size_t new_capacity = ctx->section_capacity == 0 ? 4 : ctx->section_capacity * 2;
         binding_config_section_t* new_sections = polycall_core_realloc(
             ctx->core_ctx,
             ctx->sections,
             new_capacity * sizeof(binding_config_section_t)
         );
         
         if (!new_sections) {
             return false;
         }
         
         ctx->sections = new_sections;
         ctx->section_capacity = new_capacity;
     }
     
     return true;
 }
 
 /**
  * @brief Find a section by name
  * 
  * @param ctx Binding config context
  * @param section_name Section name
  * @return binding_config_section_t* Found section, or NULL if not found
  */
 static binding_config_section_t* find_section(
     polycall_binding_config_context_t* ctx,
     const char* section_name
 ) {
     for (size_t i = 0; i < ctx->section_count; i++) {
         if (strcmp(ctx->sections[i].name, section_name) == 0) {
             return &ctx->sections[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Find a value in a section
  * 
  * @param section Section to search
  * @param key Key to find
  * @return char* Value string, or NULL if not found
  */
 static char* find_value(
     binding_config_section_t* section,
     const char* key
 ) {
     for (size_t i = 0; i < section->value_count; i++) {
         if (strcmp(section->values[i].key, key) == 0) {
             return section->values[i].value;
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Create a new section
  * 
  * @param ctx Binding config context
  * @param section_name Section name
  * @return binding_config_section_t* Created section, or NULL on failure
  */
 static binding_config_section_t* create_section(
     polycall_binding_config_context_t* ctx,
     const char* section_name
 ) {
     // Ensure capacity
     if (!ensure_section_capacity(ctx)) {
         return NULL;
     }
     
     // Create new section
     binding_config_section_t* section = &ctx->sections[ctx->section_count++];
     memset(section, 0, sizeof(binding_config_section_t));
     
     // Copy section name
     section->name = polycall_core_strdup(ctx->core_ctx, section_name);
     if (!section->name) {
         ctx->section_count--;
         return NULL;
     }
     
     return section;
 }
 
 /**
  * @brief Add or update a value in a section
  * 
  * @param ctx Core context
  * @param section Section to modify
  * @param key Key to set
  * @param value Value to set
  * @return bool True if successful, false on failure
  */
 static bool set_section_value(
     polycall_core_context_t* ctx,
     binding_config_section_t* section,
     const char* key,
     const char* value
 ) {
     // Check for existing key
     for (size_t i = 0; i < section->value_count; i++) {
         if (strcmp(section->values[i].key, key) == 0) {
             // Update existing value
             char* new_value = polycall_core_strdup(ctx, value);
             if (!new_value) {
                 return false;
             }
             
             polycall_core_free(ctx, section->values[i].value);
             section->values[i].value = new_value;
             return true;
         }
     }
     
     // Ensure capacity
     if (!ensure_value_capacity(ctx, section)) {
         return false;
     }
     
     // Add new key-value pair
     binding_config_value_t* kv = &section->values[section->value_count++];
     
     kv->key = polycall_core_strdup(ctx, key);
     if (!kv->key) {
         section->value_count--;
         return false;
     }
     
     kv->value = polycall_core_strdup(ctx, value);
     if (!kv->value) {
         polycall_core_free(ctx, kv->key);
         section->value_count--;
         return false;
     }
     
     return true;
 }
 
 /**
  * @brief Initialize a binding configuration context
  */
 polycall_core_error_t polycall_binding_config_init(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t** config_ctx,
     const polycall_binding_config_options_t* options
 ) {
     if (!core_ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_binding_config_context_t* ctx = polycall_core_malloc(
         core_ctx,
         sizeof(polycall_binding_config_context_t)
     );
     
     if (!ctx) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate binding config context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(ctx, 0, sizeof(polycall_binding_config_context_t));
     ctx->magic = POLYCALL_BINDING_CONFIG_MAGIC;
     ctx->core_ctx = core_ctx;
     
     // Set default options
     ctx->use_ignore_patterns = options ? options->use_ignore_patterns : true;
     
     // Initialize ignore context if needed
     if (ctx->use_ignore_patterns) {
         polycall_core_error_t result = polycall_ignore_context_init(
             core_ctx,
             &ctx->ignore_ctx,
             false  // Case-insensitive by default
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_core_free(core_ctx, ctx);
             return result;
         }
     }
     
     // Allocate initial sections array
     ctx->sections = polycall_core_malloc(
         core_ctx,
         MAX_CONFIG_SECTIONS * sizeof(binding_config_section_t)
     );
     
     if (!ctx->sections) {
         if (ctx->ignore_ctx) {
             polycall_ignore_context_cleanup(core_ctx, ctx->ignore_ctx);
         }
         polycall_core_free(core_ctx, ctx);
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate binding config sections");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     ctx->section_count = 0;
     ctx->section_capacity = MAX_CONFIG_SECTIONS;
     
     *config_ctx = ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up a binding configuration context
  */
 void polycall_binding_config_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx)) {
         return;
     }
     
     // Free sections and values
     if (config_ctx->sections) {
         for (size_t i = 0; i < config_ctx->section_count; i++) {
             binding_config_section_t* section = &config_ctx->sections[i];
             
             if (section->name) {
                 polycall_core_free(core_ctx, section->name);
             }
             
             if (section->values) {
                 for (size_t j = 0; j < section->value_count; j++) {
                     if (section->values[j].key) {
                         polycall_core_free(core_ctx, section->values[j].key);
                     }
                     if (section->values[j].value) {
                         polycall_core_free(core_ctx, section->values[j].value);
                     }
                 }
                 
                 polycall_core_free(core_ctx, section->values);
             }
         }
         
         polycall_core_free(core_ctx, config_ctx->sections);
     }
     
     // Cleanup ignore context
     if (config_ctx->ignore_ctx) {
         polycall_ignore_context_cleanup(core_ctx, config_ctx->ignore_ctx);
     }
     
     // Invalidate and free context
     config_ctx->magic = 0;
     polycall_core_free(core_ctx, config_ctx);
 }
 
 /**
  * @brief Parse a configuration line into section, key, and value
  * 
  * @param line Line to parse
  * @param section Output section name (can be NULL)
  * @param key Output key (can be NULL)
  * @param value Output value (can be NULL)
  * @param section_size Size of section buffer
  * @param key_size Size of key buffer
  * @param value_size Size of value buffer
  * @return bool True if successful, false if the line is invalid
  */
 static bool parse_config_line(
     const char* line,
     char* section,
     char* key,
     char* value,
     size_t section_size,
     size_t key_size,
     size_t value_size
 ) {
     // Skip leading whitespace
     while (*line && (*line == ' ' || *line == '\t')) {
         line++;
     }
     
     // Skip empty lines and comments
     if (*line == '\0' || *line == '#') {
         return false;
     }
     
     // Check for section header [section]
     if (*line == '[') {
         const char* end = strchr(line + 1, ']');
         if (!end) {
             return false;  // Malformed section header
         }
         
         if (section && section_size > 0) {
             size_t len = end - (line + 1);
             if (len >= section_size) {
                 len = section_size - 1;
             }
             
             strncpy(section, line + 1, len);
             section[len] = '\0';
         }
         
         if (key) key[0] = '\0';
         if (value) value[0] = '\0';
         return true;
     }
     
     // Parse key=value
     const char* equals = strchr(line, '=');
     if (!equals) {
         return false;  // No equals sign
     }
     
     if (key && key_size > 0) {
         size_t key_len = equals - line;
         
         // Trim trailing whitespace from key
         while (key_len > 0 && (line[key_len - 1] == ' ' || line[key_len - 1] == '\t')) {
             key_len--;
         }
         
         if (key_len >= key_size) {
             key_len = key_size - 1;
         }
         
         strncpy(key, line, key_len);
         key[key_len] = '\0';
     }
     
     if (value && value_size > 0) {
         const char* val_start = equals + 1;
         
         // Skip leading whitespace in value
         while (*val_start && (*val_start == ' ' || *val_start == '\t')) {
             val_start++;
         }
         
         // Check for quoted value
         if (*val_start == '"') {
             val_start++;  // Skip opening quote
             
             const char* quote_end = strchr(val_start, '"');
             if (!quote_end) {
                 // Unterminated quote, use the rest of the line
                 strncpy(value, val_start, value_size - 1);
                 value[value_size - 1] = '\0';
             } else {
                 size_t val_len = quote_end - val_start;
                 if (val_len >= value_size) {
                     val_len = value_size - 1;
                 }
                 
                 strncpy(value, val_start, val_len);
                 value[val_len] = '\0';
             }
         } else {
             // Unquoted value
             size_t val_len = strlen(val_start);
             
             // Trim trailing whitespace
             while (val_len > 0 && (val_start[val_len - 1] == ' ' || val_start[val_len - 1] == '\t' ||
                                  val_start[val_len - 1] == '\r' || val_start[val_len - 1] == '\n')) {
                 val_len--;
             }
             
             if (val_len >= value_size) {
                 val_len = value_size - 1;
             }
             
             strncpy(value, val_start, val_len);
             value[val_len] = '\0';
         }
     }
     
     if (section) section[0] = '\0';
     return true;
 }
 
 /**
  * @brief Load binding configuration from a file
  */
 polycall_core_error_t polycall_binding_config_load(
     polycall_binding_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_binding_config_context(config_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* path = file_path;
     char resolved_path[MAX_PATH_LENGTH];
     
     // If no path provided, use default
     if (!path) {
         // Try to find .polycallrc in current directory
         if (polycall_path_resolve(
                 config_ctx->core_ctx,
                 DEFAULT_RC_FILENAME,
                 resolved_path,
                 sizeof(resolved_path)) == POLYCALL_CORE_SUCCESS) {
             path = resolved_path;
         } else {
             // Try to find .polycallrc in user's home directory
             char home_path[MAX_PATH_LENGTH];
             if (polycall_path_get_home_directory(
                     home_path,
                     sizeof(home_path)) == POLYCALL_CORE_SUCCESS) {
                 
                 snprintf(resolved_path, sizeof(resolved_path),
                         "%s/%s", home_path, DEFAULT_RC_FILENAME);
                 
                 if (polycall_path_file_exists(resolved_path)) {
                     path = resolved_path;
                 } else {
                     // No default configuration found
                     return POLYCALL_CORE_SUCCESS;
                 }
             } else {
                 // No home directory found
                 return POLYCALL_CORE_SUCCESS;
             }
         }
     }
     
     // Store file path
     strncpy(config_ctx->config_file_path, path, sizeof(config_ctx->config_file_path) - 1);
     config_ctx->config_file_path[sizeof(config_ctx->config_file_path) - 1] = '\0';
     config_ctx->has_config_file = true;
     
     // Load ignore patterns if configured
     if (config_ctx->use_ignore_patterns && config_ctx->ignore_ctx) {
         char ignore_path[MAX_PATH_LENGTH];
         
         // Try to load .polycallrc.ignore from the same directory
         char* last_slash = strrchr(path, '/');
         if (last_slash) {
             size_t dir_len = last_slash - path + 1;
             strncpy(ignore_path, path, dir_len);
             ignore_path[dir_len] = '\0';
             strncat(ignore_path, DEFAULT_IGNORE_FILENAME,
                    sizeof(ignore_path) - dir_len - 1);
             
             polycall_ignore_load_file(config_ctx->ignore_ctx, ignore_path);
         }
     }
     
     // Open configuration file
     FILE* file = fopen(path, "r");
     if (!file) {
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Parse configuration
     char line[MAX_PATH_LENGTH];
     char current_section[MAX_KEY_LENGTH] = "";
     char key[MAX_KEY_LENGTH];
     char value[MAX_VALUE_LENGTH];
     binding_config_section_t* section = NULL;
     
     while (fgets(line, sizeof(line), file)) {
         // Parse the line
         if (parse_config_line(
                 line,
                 current_section,
                 key,
                 value,
                 sizeof(current_section),
                 sizeof(key),
                 sizeof(value))) {
             
             if (current_section[0] != '\0') {
                 // Create or find section
                 section = find_section(config_ctx, current_section);
                 if (!section) {
                     section = create_section(config_ctx, current_section);
                     if (!section) {
                         fclose(file);
                         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                     }
                 }
             } else if (key[0] != '\0' && section) {
                 // Add value to current section
                 if (!set_section_value(config_ctx->core_ctx, section, key, value)) {
                     fclose(file);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
             }
         }
     }
     
     fclose(file);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Save binding configuration to a file
  */
 polycall_core_error_t polycall_binding_config_save(
     polycall_binding_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_binding_config_context(config_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     const char* path = file_path;
     
     // If no path provided, use stored path
     if (!path) {
         if (!config_ctx->has_config_file) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         path = config_ctx->config_file_path;
     }
     
     // Check if path should be ignored
     if (config_ctx->use_ignore_patterns && config_ctx->ignore_ctx &&
         polycall_ignore_should_ignore(config_ctx->ignore_ctx, path)) {
         POLYCALL_ERROR_SET(config_ctx->core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_VALIDATION_FAILED,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Config path is in the ignore list");
         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
     }
     
     // Open file for writing
     FILE* file = fopen(path, "w");
     if (!file) {
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Write header
     fprintf(file, "# LibPolyCall Binding Configuration\n");
     fprintf(file, "# Generated by polycall_binding_config\n\n");
     
     // Write sections and values
     for (size_t i = 0; i < config_ctx->section_count; i++) {
         binding_config_section_t* section = &config_ctx->sections[i];
         
         fprintf(file, "[%s]\n", section->name);
         
         for (size_t j = 0; j < section->value_count; j++) {
             binding_config_value_t* kv = &section->values[j];
             
             // Check if value needs quotes
             bool needs_quotes = false;
             for (const char* c = kv->value; *c; c++) {
                 if (*c == ' ' || *c == '\t' || *c == '=' || *c == '#') {
                     needs_quotes = true;
                     break;
                 }
             }
             
             if (needs_quotes) {
                 fprintf(file, "%s = \"%s\"\n", kv->key, kv->value);
             } else {
                 fprintf(file, "%s = %s\n", kv->key, kv->value);
             }
         }
         
         fprintf(file, "\n");
     }
     
     fclose(file);
     
     // If this was a new file, store its path
     if (!config_ctx->has_config_file) {
         strncpy(config_ctx->config_file_path, path, sizeof(config_ctx->config_file_path) - 1);
         config_ctx->config_file_path[sizeof(config_ctx->config_file_path) - 1] = '\0';
         config_ctx->has_config_file = true;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get a string value from the configuration
  */
 polycall_core_error_t polycall_binding_config_get_string(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx,
     const char* section_name,
     const char* key,
     char* value,
     size_t value_size
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx) ||
         !section_name || !key || !value || value_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find section
     binding_config_section_t* section = find_section(config_ctx, section_name);
     if (!section) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find value
     char* found_value = find_value(section, key);
     if (!found_value) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Copy value
     strncpy(value, found_value, value_size - 1);
     value[value_size - 1] = '\0';
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get an integer value from the configuration
  */
 polycall_core_error_t polycall_binding_config_get_int(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx,
     const char* section_name,
     const char* key,
     int64_t* value
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx) ||
         !section_name || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find section
     binding_config_section_t* section = find_section(config_ctx, section_name);
     if (!section) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find value
     char* found_value = find_value(section, key);
     if (!found_value) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Parse value
     char* end;
     *value = strtoll(found_value, &end, 0);
     
     if (end == found_value || *end != '\0') {
         return POLYCALL_CORE_ERROR_INVALID_FORMAT;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get a boolean value from the configuration
  */
 polycall_core_error_t polycall_binding_config_get_bool(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx,
     const char* section_name,
     const char* key,
     bool* value
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx) ||
         !section_name || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find section
     binding_config_section_t* section = find_section(config_ctx, section_name);
     if (!section) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find value
     char* found_value = find_value(section, key);
     if (!found_value) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Parse boolean value
     if (strcmp(found_value, "true") == 0 || 
         strcmp(found_value, "yes") == 0 || 
         strcmp(found_value, "1") == 0) {
         *value = true;
     } else if (strcmp(found_value, "false") == 0 || 
                strcmp(found_value, "no") == 0 || 
                strcmp(found_value, "0") == 0) {
         *value = false;
     } else {
         return POLYCALL_CORE_ERROR_INVALID_FORMAT;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set a string value in the configuration
  */
 polycall_core_error_t polycall_binding_config_set_string(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx,
     const char* section_name,
     const char* key,
     const char* value
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx) ||
         !section_name || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find or create section
     binding_config_section_t* section = find_section(config_ctx, section_name);
     if (!section) {
         section = create_section(config_ctx, section_name);
         if (!section) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     // Set value
     if (!set_section_value(core_ctx, section, key, value)) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set an integer value in the configuration
  */
 polycall_core_error_t polycall_binding_config_set_int(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx,
     const char* section_name,
     const char* key,
     int64_t value
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx) ||
         !section_name || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Convert to string
     char str_value[32];
     snprintf(str_value, sizeof(str_value), "%lld", (long long)value);
     
     // Set string value
     return polycall_binding_config_set_string(
         core_ctx,
         config_ctx,
         section_name,
         key,
         str_value
     );
 }
 
 /**
  * @brief Set a boolean value in the configuration
  */
 polycall_core_error_t polycall_binding_config_set_bool(
     polycall_core_context_t* core_ctx,
     polycall_binding_config_context_t* config_ctx,
     const char* section_name,
     const char* key,
     bool value
 ) {
     if (!core_ctx || !validate_binding_config_context(config_ctx) ||
         !section_name || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Convert to string
     const char* str_value = value ? "true" : "false";
     
     // Set string value
     return polycall_binding_config_set_string(
         core_ctx,
         config_ctx,
         section_name,
         key,
         str_value
     );
 }
 
 /**
  * @brief Add a path to the ignore list
  */
 polycall_core_error_t polycall_binding_config_add_ignore_pattern(
     polycall_binding_config_context_t* config_ctx,
     const char* pattern
 ) {
     if (!validate_binding_config_context(config_ctx) || !pattern) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!config_ctx->use_ignore_patterns || !config_ctx->ignore_ctx) {
         return POLYCALL_CORE_ERROR_NOT_SUPPORTED;
     }
     
     return polycall_ignore_add_pattern(config_ctx->ignore_ctx, pattern);
 }
 
 /**
  * @brief Check if a path should be ignored
  */
 bool polycall_binding_config_should_ignore(
     polycall_binding_config_context_t* config_ctx,
     const char* path
 ) {
     if (!validate_binding_config_context(config_ctx) || !path) {
         return false;
     }
     
     if (!config_ctx->use_ignore_patterns || !config_ctx->ignore_ctx) {
         return false;
     }
     
     return polycall_ignore_should_ignore(config_ctx->ignore_ctx, path);
 }
 
 /**
  * @brief Load ignore patterns from an ignore file
  */
 polycall_core_error_t polycall_binding_config_load_ignore_file(
     polycall_binding_config_context_t* config_ctx,
     const char* file_path
 ) {
     if (!validate_binding_config_context(config_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!config_ctx->use_ignore_patterns || !config_ctx->ignore_ctx) {
         return POLYCALL_CORE_ERROR_NOT_SUPPORTED;
     }
     
     const char* path = file_path;
     char resolved_path[MAX_PATH_LENGTH];
     
     // If no path provided, use default
     if (!path) {
         // Try to find .polycallrc.ignore in current directory
         if (polycall_path_resolve(
                 config_ctx->core_ctx,
                 DEFAULT_IGNORE_FILENAME,
                 resolved_path,
                 sizeof(resolved_path)) == POLYCALL_CORE_SUCCESS) {
             path = resolved_path;
         } else {
             // Try to find .polycallrc.ignore in user's home directory
             char home_path[MAX_PATH_LENGTH];
             if (polycall_path_get_home_directory(
                     home_path,
                     sizeof(home_path)) == POLYCALL_CORE_SUCCESS) {
                 
                 snprintf(resolved_path, sizeof(resolved_path),
                         "%s/%s", home_path, DEFAULT_IGNORE_FILENAME);
                 
                 if (polycall_path_file_exists(resolved_path)) {
                     path = resolved_path;
                 } else {
                     // No default ignore file found
                     return POLYCALL_CORE_SUCCESS;
                 }
             } else {
                 // No home directory found
                 return POLYCALL_CORE_SUCCESS;
             }
         }
     }
     
     return polycall_ignore_load_file(config_ctx->ignore_ctx, path);
 }