/**
 * @file polycall_config.h
 * @brief Core configuration module for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the core configuration system for LibPolyCall, providing
 * a comprehensive configuration interface for all LibPolyCall components.
 */

 #ifndef POLYCALL_POLYCALL_POLYCALL_CONFIG_H_H
 #define POLYCALL_POLYCALL_POLYCALL_CONFIG_H_H
 
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdarg.h>
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_public.h"
 #include "polycall/core/polycall/polycall_event.h"
 #include "polycall/core/polycall/polycall_logger.h"
    
 #ifdef __cplusplus
 extern "C" {
 #endif

/**
 * @brief Configuration context opaque structure
 */
typedef struct polycall_config_context polycall_config_context_t;
/**
 * @brief Core configuration structure
 */
typedef struct polycall_core_config {
    uint32_t flags;              /**< Configuration flags */
    size_t memory_pool_size;     /**< Size of memory pool */
    void* user_data;            /**< User data for callbacks */
    void (*error_callback)(void* user_data, const char* message); /**< Error callback function */
} polycall_core_config_t;
/**
 * @brief Configuration value types
 */
typedef enum {
    POLYCALL_CONFIG_VALUE_BOOLEAN = 0, /**< Boolean value */
    POLYCALL_CONFIG_VALUE_INTEGER,     /**< Integer value */
    POLYCALL_CONFIG_VALUE_FLOAT,       /**< Floating-point value */
    POLYCALL_CONFIG_VALUE_STRING,      /**< String value */
    POLYCALL_CONFIG_VALUE_OBJECT       /**< Complex object value */
} polycall_config_value_type_t;

/**
 * @brief Configuration value
 */
typedef struct polycall_config_value {
    polycall_config_value_type_t type;
    union {
        bool bool_value;
        int64_t int_value;
        double float_value;
        char* string_value;
        void* object_value;
    } value;
    void (*object_free)(void* object); /**< Function to free object value */
} polycall_config_value_t;

/**
 * @brief Configuration section types
 */
typedef enum {
    POLYCALL_CONFIG_SECTION_CORE = 0,    /**< Core configuration */
    POLYCALL_CONFIG_SECTION_SECURITY,    /**< Security configuration */
    POLYCALL_CONFIG_SECTION_MEMORY,      /**< Memory management configuration */
    POLYCALL_CONFIG_SECTION_JS,          /**< JavaScript bridge configuration */
    POLYCALL_CONFIG_SECTION_PYTHON,      /**< Python bridge configuration */
    POLYCALL_CONFIG_SECTION_USER = 0x1000 /**< Start of user-defined sections */
} polycall_config_section_t;

/* Configuration value structure already defined above */

 /**
  * @brief Configuration options
  */
 typedef struct {
     bool enable_persistence;         /**< Enable configuration persistence */
     bool enable_change_notification; /**< Enable change notification callbacks */
     bool auto_load;                  /**< Auto-load configuration on init */
     bool auto_save;                  /**< Auto-save configuration on change */
     bool validate_on_load;           /**< Validate configuration on load */
     bool validate_on_save;           /**< Validate configuration on save */
     const char* config_path;         /**< Path to configuration file */
 } polycall_config_options_t;
 
 /**
  * @brief Configuration provider interface
  */
 typedef struct {
     /**
      * @brief Initialize the configuration provider
      * @param ctx Core context
      * @param user_data User data for the provider
      * @return Error code
      */
     polycall_core_error_t (*initialize)(
         polycall_core_context_t* ctx,
         void* user_data
     );
     
     /**
      * @brief Clean up the configuration provider
      * @param ctx Core context
      * @param user_data User data for the provider
      */
     void (*cleanup)(
         polycall_core_context_t* ctx,
         void* user_data
     );
     
     /**
      * @brief Load configuration
      * @param ctx Core context
      * @param user_data User data for the provider
      * @param section_id Section identifier
      * @param key Configuration key
      * @param value Pointer to receive configuration value
      * @return Error code
      */
     polycall_core_error_t (*load)(
         polycall_core_context_t* ctx,
         void* user_data,
         polycall_config_section_t section_id,
         const char* key,
         polycall_config_value_t* value
     );
     
     /**
      * @brief Save configuration
      * @param ctx Core context
      * @param user_data User data for the provider
      * @param section_id Section identifier
      * @param key Configuration key
      * @param value Configuration value to save
      * @return Error code
      */
     polycall_core_error_t (*save)(
         polycall_core_context_t* ctx,
         void* user_data,
         polycall_config_section_t section_id,
         const char* key,
         const polycall_config_value_t* value
     );
     
     /**
      * @brief Check if configuration exists
      * @param ctx Core context
      * @param user_data User data for the provider
      * @param section_id Section identifier
      * @param key Configuration key
      * @param exists Pointer to receive existence flag
      * @return Error code
      */
     polycall_core_error_t (*exists)(
         polycall_core_context_t* ctx,
         void* user_data,
         polycall_config_section_t section_id,
         const char* key,
         bool* exists
     );
     
     /**
      * @brief Enumerate configuration keys
      * @param ctx Core context
      * @param user_data User data for the provider
      * @param section_id Section identifier
      * @param callback Callback to invoke for each key
      * @param callback_data User data for callback
      * @return Error code
      */
     polycall_core_error_t (*enumerate)(
         polycall_core_context_t* ctx,
         void* user_data,
         polycall_config_section_t section_id,
         void (*callback)(const char* key, void* callback_data),
         void* callback_data
     );
     
     const char* provider_name; /**< Provider name */
     void* user_data;          /**< User data for the provider */
 } polycall_config_provider_t;
 
 /**
  * @brief Initialize configuration system
  * 
  * @param ctx Core context
  * @param config_ctx Pointer to receive the created configuration context
  * @param options Configuration options, or NULL for defaults
  * @return Error code
  */
 polycall_core_error_t polycall_config_init(
     polycall_core_context_t* ctx,
     polycall_config_context_t** config_ctx,
     const polycall_config_options_t* options
 );
 
 /**
  * @brief Create default configuration options
  * 
  * @return Default options
  */
 polycall_config_options_t polycall_config_default_options(void);
 
 /**
  * @brief Clean up configuration system
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  */
 void polycall_config_cleanup(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx
 );
 
 /**
  * @brief Load configuration from file
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param file_path Path to configuration file, or NULL to use path from options
  * @return Error code
  */
 polycall_core_error_t polycall_config_load(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     const char* file_path
 );
 
 /**
  * @brief Save configuration to file
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param file_path Path to configuration file, or NULL to use path from options
  * @return Error code
  */
 polycall_core_error_t polycall_config_save(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     const char* file_path
 );
 
 /**
  * @brief Register configuration provider
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param provider Provider interface
  * @return Error code
  */
 polycall_core_error_t polycall_config_register_provider(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     const polycall_config_provider_t* provider
 );
 
 /**
  * @brief Register configuration change handler
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier, or -1 for all sections
  * @param key Key to monitor, or NULL for all keys
  * @param handler Change handler function
  * @param user_data User data for handler
  * @param handler_id Pointer to receive handler ID
  * @return Error code
  */
 /**
  * @brief Configuration change handler function type
  */
 typedef void (*polycall_config_change_handler_t)(
    polycall_core_context_t* ctx,
    polycall_config_section_t section_id,
    const char* key,
    const polycall_config_value_t* old_value,
    const polycall_config_value_t* new_value,
    void* user_data
 );

 /**
  * @brief Register configuration change handler
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier, or -1 for all sections
  * @param key Key to monitor, or NULL for all keys
  * @param handler Change handler function
  * @param user_data User data for handler
  * @param handler_id Pointer to receive handler ID
  * @return Error code
  */
 polycall_core_error_t polycall_config_register_change_handler(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     polycall_config_change_handler_t handler,
     void* user_data,
     uint32_t* handler_id
 );

 
 /**
  * @brief Get boolean configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param default_value Default value if key not found
  * @return Boolean value
  */
 bool polycall_config_get_bool(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     bool default_value
 );
 
 /**
  * @brief Set boolean configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param value Boolean value
  * @return Error code
  */
 polycall_core_error_t polycall_config_set_bool(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     bool value
 );
 
 /**
  * @brief Get integer configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param default_value Default value if key not found
  * @return Integer value
  */
 int64_t polycall_config_get_int(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     int64_t default_value
 );

/* Configuration change handler function type already defined above */

/**
 * @brief Load configuration from file
 *
 * @param ctx Context
 * @param config Configuration to update
 * @param filename File to load
 * @return Error code
 */
polycall_error_t polycall_load_config_file(
    polycall_core_context_t* ctx,
    polycall_config_context_t* config,
    const char* filename
);
 /**
  * @brief Set integer configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param value Integer value
  * @return Error code
  */
 polycall_core_error_t polycall_config_set_int(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     int64_t value
 );
 
 /**
  * @brief Get float configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param default_value Default value if key not found
  * @return Float value
  */
 double polycall_config_get_float(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     double default_value
 );
 
 /**
  * @brief Set float configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param value Float value
  * @return Error code
  */
 polycall_core_error_t polycall_config_set_float(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     double value
 );
 
 /**
  * @brief Get string configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param buffer Buffer to receive string value
  * @param buffer_size Size of buffer
  * @param default_value Default value if key not found
  * @return Error code
  */
 polycall_core_error_t polycall_config_get_string(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     char* buffer,
     size_t buffer_size,
     const char* default_value
 );
 
 /**
  * @brief Set string configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param value String value
  * @return Error code
  */
 polycall_core_error_t polycall_config_set_string(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const char* value
 );
 
 /**
  * @brief Get object configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param value Pointer to receive object value
  * @return Error code
  */
 polycall_core_error_t polycall_config_get_object(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     void** value
 );
 
 /**
  * @brief Set object configuration value
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @param value Object value
  * @param object_free Function to free object value
  * @return Error code
  */
 polycall_core_error_t polycall_config_set_object(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     void* value,
     void (*object_free)(void* object)
 );
 
 /**
  * @brief Check if configuration key exists
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @return true if key exists, false otherwise
  */
 bool polycall_config_exists(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key
 );
 
 /**
  * @brief Remove configuration key
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param key Configuration key
  * @return Error code
  */
 polycall_core_error_t polycall_config_remove(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key
 );
 
 /**
  * @brief Enumerate configuration keys
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param section_id Section identifier
  * @param callback Callback to invoke for each key
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_config_enumerate(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     void (*callback)(const char* key, void* user_data),
     void* user_data
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_POLYCALL_POLYCALL_CONFIG_H_H */