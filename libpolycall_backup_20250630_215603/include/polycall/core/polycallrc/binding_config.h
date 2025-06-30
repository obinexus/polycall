/**
 * @file binding_config.h
 * @brief Binding-specific configuration interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the binding-specific configuration management system,
 * providing APIs for loading, parsing, and accessing configurations from .polycallrc
 * files with enhanced read-only mode support.
 */

 #ifndef POLYCALL_CONFIG_POLYCALLRC_BINDING_CONFIG_H_H
 #define POLYCALL_CONFIG_POLYCALLRC_BINDING_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <stdio.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Binding configuration options
  */
 typedef struct {
     bool read_only;              // Whether to open configurations in read-only mode
     bool enable_validation;      // Whether to validate configurations
     bool auto_load;              // Whether to automatically load configurations
     bool auto_save;              // Whether to automatically save configurations on changes
     const char* config_path;     // Configuration file path or NULL for default
 } polycall_binding_config_options_t;
 
 /**
  * @brief Binding configuration context (opaque)
  */
 typedef struct polycall_binding_config_context polycall_binding_config_context_t;
 
 /**
  * @brief Initialize binding configuration context with default options
  * 
  * @param ctx Core context
  * @param binding_ctx Pointer to receive the created binding configuration context
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_init(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t** binding_ctx
 );
 
 /**
  * @brief Initialize binding configuration context with specified options
  * 
  * @param ctx Core context
  * @param binding_ctx Pointer to receive the created binding configuration context
  * @param options Binding configuration options
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_init_with_options(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t** binding_ctx,
     const polycall_binding_config_options_t* options
 );
 
 /**
  * @brief Create default binding configuration options
  * 
  * @return polycall_binding_config_options_t Default options
  */
 polycall_binding_config_options_t polycall_binding_config_default_options(void);
 
 /**
  * @brief Load binding configuration from file
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param file_path Path to configuration file, or NULL to search in standard locations
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_load(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* file_path
 );
 
 /**
  * @brief Save binding configuration to file
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param file_path Path to save to, or NULL to use the current path
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_save(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* file_path
 );
 
 /**
  * @brief Reset binding configuration to defaults
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_reset(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx
 );
 
 /**
  * @brief Get configuration value as string
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param section Section name
  * @param key Key name
  * @param buffer Buffer to receive string value
  * @param buffer_size Size of buffer
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_get_string(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* section,
     const char* key,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Get configuration value as integer
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param section Section name
  * @param key Key name
  * @param value Pointer to receive integer value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_get_int(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* section,
     const char* key,
     int64_t* value
 );
 
 /**
  * @brief Get configuration value as boolean
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param section Section name
  * @param key Key name
  * @param value Pointer to receive boolean value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_get_bool(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* section,
     const char* key,
     bool* value
 );
 
 /**
  * @brief Set configuration value (string)
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param section Section name
  * @param key Key name
  * @param value String value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_set_string(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* section,
     const char* key,
     const char* value
 );
 
 /**
  * @brief Set configuration value (integer)
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param section Section name
  * @param key Key name
  * @param value Integer value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_set_int(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* section,
     const char* key,
     int64_t value
 );
 
 /**
  * @brief Set configuration value (boolean)
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param section Section name
  * @param key Key name
  * @param value Boolean value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_set_bool(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* section,
     const char* key,
     bool value
 );
 
 /**
  * @brief Set generic configuration value by path
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param path Full path to the configuration value (e.g., "micro.component_name.key")
  * @param value String representation of value
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_set_value(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     const char* path,
     const char* value
 );
 
 /**
  * @brief Check if configuration has been modified
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @return bool True if modified, false otherwise
  */
 bool polycall_binding_config_is_modified(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx
 );
 
 /**
  * @brief Serialize binding configuration to file
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  * @param file File pointer to write to
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_binding_config_serialize(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx,
     FILE* file
 );
 
 /**
  * @brief Cleanup binding configuration context
  * 
  * @param ctx Core context
  * @param binding_ctx Binding configuration context
  */
 void polycall_binding_config_cleanup(
     polycall_core_context_t* ctx,
     polycall_binding_config_context_t* binding_ctx
 );
 
 /**
 * @brief Save binding configuration to a file
 * 
 * @param config_ctx Binding config context
 * @param file_path Path to save configuration (NULL for default)
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_save(
    polycall_binding_config_context_t* config_ctx,
    const char* file_path
);

/**
 * @brief Get a string value from the configuration
 * 
 * @param core_ctx Core context
 * @param config_ctx Binding config context
 * @param section_name Section name
 * @param key Key to get
 * @param value Buffer to receive value
 * @param value_size Size of value buffer
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_get_string(
    polycall_core_context_t* core_ctx,
    polycall_binding_config_context_t* config_ctx,
    const char* section_name,
    const char* key,
    char* value,
    size_t value_size
);

/**
 * @brief Get an integer value from the configuration
 * 
 * @param core_ctx Core context
 * @param config_ctx Binding config context
 * @param section_name Section name
 * @param key Key to get
 * @param value Pointer to receive value
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_get_int(
    polycall_core_context_t* core_ctx,
    polycall_binding_config_context_t* config_ctx,
    const char* section_name,
    const char* key,
    int64_t* value
);

/**
 * @brief Get a boolean value from the configuration
 * 
 * @param core_ctx Core context
 * @param config_ctx Binding config context
 * @param section_name Section name
 * @param key Key to get
 * @param value Pointer to receive value
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_get_bool(
    polycall_core_context_t* core_ctx,
    polycall_binding_config_context_t* config_ctx,
    const char* section_name,
    const char* key,
    bool* value
);

/**
 * @brief Set a string value in the configuration
 * 
 * @param core_ctx Core context
 * @param config_ctx Binding config context
 * @param section_name Section name
 * @param key Key to set
 * @param value Value to set
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_set_string(
    polycall_core_context_t* core_ctx,
    polycall_binding_config_context_t* config_ctx,
    const char* section_name,
    const char* key,
    const char* value
);

/**
 * @brief Set an integer value in the configuration
 * 
 * @param core_ctx Core context
 * @param config_ctx Binding config context
 * @param section_name Section name
 * @param key Key to set
 * @param value Value to set
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_set_int(
    polycall_core_context_t* core_ctx,
    polycall_binding_config_context_t* config_ctx,
    const char* section_name,
    const char* key,
    int64_t value
);

/**
 * @brief Set a boolean value in the configuration
 * 
 * @param core_ctx Core context
 * @param config_ctx Binding config context
 * @param section_name Section name
 * @param key Key to set
 * @param value Value to set
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_set_bool(
    polycall_core_context_t* core_ctx,
    polycall_binding_config_context_t* config_ctx,
    const char* section_name,
    const char* key,
    bool value
);

/**
 * @brief Add a pattern to the ignore list
 * 
 * @param config_ctx Binding config context
 * @param pattern Pattern to add
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_add_ignore_pattern(
    polycall_binding_config_context_t* config_ctx,
    const char* pattern
);

/**
 * @brief Check if a path should be ignored
 * 
 * @param config_ctx Binding config context
 * @param path Path to check
 * @return bool True if the path should be ignored, false otherwise
 */
bool polycall_binding_config_should_ignore(
    polycall_binding_config_context_t* config_ctx,
    const char* path
);

/**
 * @brief Load ignore patterns from a file
 * 
 * @param config_ctx Binding config context
 * @param file_path Path to ignore file (NULL for default)
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_binding_config_load_ignore_file(
    polycall_binding_config_context_t* config_ctx,
    const char* file_path
);

 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_POLYCALLRC_BINDING_CONFIG_H_H */