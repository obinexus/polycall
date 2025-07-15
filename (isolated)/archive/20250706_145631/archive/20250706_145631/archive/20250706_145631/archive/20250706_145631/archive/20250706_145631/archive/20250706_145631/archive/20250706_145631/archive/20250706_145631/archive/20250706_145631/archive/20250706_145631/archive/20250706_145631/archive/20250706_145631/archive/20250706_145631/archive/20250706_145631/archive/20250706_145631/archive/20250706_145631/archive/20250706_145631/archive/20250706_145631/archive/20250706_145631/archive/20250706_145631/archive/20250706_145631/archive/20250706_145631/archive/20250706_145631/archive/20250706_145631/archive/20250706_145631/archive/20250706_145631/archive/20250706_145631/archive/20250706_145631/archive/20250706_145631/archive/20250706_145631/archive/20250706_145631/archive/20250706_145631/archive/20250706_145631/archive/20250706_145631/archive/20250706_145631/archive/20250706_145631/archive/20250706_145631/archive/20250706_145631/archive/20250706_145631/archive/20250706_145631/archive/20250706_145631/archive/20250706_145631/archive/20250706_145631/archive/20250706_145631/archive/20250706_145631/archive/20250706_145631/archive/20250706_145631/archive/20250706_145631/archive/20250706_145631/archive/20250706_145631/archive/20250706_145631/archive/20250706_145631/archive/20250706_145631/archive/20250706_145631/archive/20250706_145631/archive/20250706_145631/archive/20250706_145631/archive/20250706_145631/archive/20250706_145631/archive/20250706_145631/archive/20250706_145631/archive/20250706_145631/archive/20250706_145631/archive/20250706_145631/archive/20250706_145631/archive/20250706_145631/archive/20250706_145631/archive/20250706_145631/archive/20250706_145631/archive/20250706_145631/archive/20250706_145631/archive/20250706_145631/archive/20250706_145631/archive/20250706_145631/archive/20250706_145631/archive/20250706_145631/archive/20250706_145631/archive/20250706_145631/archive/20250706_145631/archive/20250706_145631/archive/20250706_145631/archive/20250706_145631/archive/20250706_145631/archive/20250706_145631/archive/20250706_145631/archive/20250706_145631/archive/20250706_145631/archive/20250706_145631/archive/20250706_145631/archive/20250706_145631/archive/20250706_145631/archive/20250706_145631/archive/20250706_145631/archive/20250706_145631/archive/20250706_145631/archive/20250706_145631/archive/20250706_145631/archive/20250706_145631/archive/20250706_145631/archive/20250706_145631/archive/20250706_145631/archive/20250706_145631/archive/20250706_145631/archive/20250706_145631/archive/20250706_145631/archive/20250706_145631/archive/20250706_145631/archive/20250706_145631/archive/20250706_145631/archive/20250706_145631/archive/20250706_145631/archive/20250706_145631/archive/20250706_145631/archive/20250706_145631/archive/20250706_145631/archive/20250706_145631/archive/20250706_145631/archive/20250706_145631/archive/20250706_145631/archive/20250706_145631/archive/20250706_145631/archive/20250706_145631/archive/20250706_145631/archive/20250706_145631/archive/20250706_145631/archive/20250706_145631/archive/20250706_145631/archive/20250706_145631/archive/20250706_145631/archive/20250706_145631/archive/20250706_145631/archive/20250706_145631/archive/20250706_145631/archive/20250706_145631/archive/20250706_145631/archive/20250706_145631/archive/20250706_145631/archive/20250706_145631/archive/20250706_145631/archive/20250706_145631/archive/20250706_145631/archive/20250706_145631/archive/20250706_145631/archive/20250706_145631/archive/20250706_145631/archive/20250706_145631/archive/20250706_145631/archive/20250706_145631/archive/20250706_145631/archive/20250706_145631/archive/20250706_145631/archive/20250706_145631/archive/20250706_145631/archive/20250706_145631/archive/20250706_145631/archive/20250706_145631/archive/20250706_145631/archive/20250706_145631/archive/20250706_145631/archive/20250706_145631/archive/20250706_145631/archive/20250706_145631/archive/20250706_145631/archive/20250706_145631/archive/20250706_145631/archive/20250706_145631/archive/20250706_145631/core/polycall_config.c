/**
#include "polycall/core/polycall/polycall_config.h"

 * @file polycall_config.c
 * @brief Core configuration implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the core configuration system defined in
 * polycall_config.h, providing a comprehensive configuration interface
 * for all LibPolyCall components.
 */

 #include "polycall/core/polycall/polycall_config.h"

 /* Maximum length for section/key strings */
 #define CONFIG_MAX_PATH_LENGTH 128
 
 /* Config node structure */
 typedef struct config_node {
     polycall_config_section_t section_id;
     char* key;
     polycall_config_value_t value;
     struct config_node* next;
 } config_node_t;
 
 /* Config provider node structure */
 typedef struct config_provider_node {
     polycall_config_provider_t provider;
     struct config_provider_node* next;
 } config_provider_node_t;
 
 /* Change handler structure */
 typedef struct change_handler_node {
     uint32_t id;
     polycall_config_section_t section_id;
     char* key;
     polycall_config_change_handler_t handler;
     void* user_data;
     struct change_handler_node* next;
 } change_handler_node_t;
 
 /* Configuration context implementation */
 struct polycall_config_context {
     polycall_config_options_t options;
     config_node_t* nodes;
     config_provider_node_t* providers;
     change_handler_node_t* handlers;
     uint32_t next_handler_id;
 };
 
 /* Helper function to make a copy of a value */
 static polycall_core_error_t copy_value(
     polycall_core_context_t* ctx,
     polycall_config_value_t* dest,
     const polycall_config_value_t* src
 ) {
     if (!ctx || !dest || !src) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     dest->type = src->type;
 
     switch (src->type) {
         case POLYCALL_CONFIG_VALUE_BOOLEAN:
             dest->value.bool_value = src->value.bool_value;
             break;
         case POLYCALL_CONFIG_VALUE_INTEGER:
             dest->value.int_value = src->value.int_value;
             break;
         case POLYCALL_CONFIG_VALUE_FLOAT:
             dest->value.float_value = src->value.float_value;
             break;
         case POLYCALL_CONFIG_VALUE_STRING:
             if (src->value.string_value) {
                 dest->value.string_value = polycall_core_malloc(ctx, strlen(src->value.string_value) + 1);
                 if (!dest->value.string_value) {
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 strcpy(dest->value.string_value, src->value.string_value);
             } else {
                 dest->value.string_value = NULL;
             }
             break;
         case POLYCALL_CONFIG_VALUE_OBJECT:
             dest->value.object_value = src->value.object_value;
             dest->object_free = src->object_free;
             break;
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Helper function to free a value */
 static void free_value(
     polycall_core_context_t* ctx,
     polycall_config_value_t* value
 ) {
     if (!ctx || !value) {
         return;
     }
 
     switch (value->type) {
         case POLYCALL_CONFIG_VALUE_STRING:
             if (value->value.string_value) {
                 polycall_core_free(ctx, value->value.string_value);
                 value->value.string_value = NULL;
             }
             break;
         case POLYCALL_CONFIG_VALUE_OBJECT:
             if (value->value.object_value && value->object_free) {
                 value->object_free(value->value.object_value);
                 value->value.object_value = NULL;
             }
             break;
         default:
             /* Nothing to free for other types */
             break;
     }
 }
 
 /* Helper function to find a node by section and key */
 static config_node_t* find_node(
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key
 ) {
     config_node_t* current = config_ctx->nodes;
 
     while (current) {
         if (current->section_id == section_id && 
             strcmp(current->key, key) == 0) {
             return current;
         }
         current = current->next;
     }
 
     return NULL;
 }
 
 /* Helper function to notify change handlers */
 static void notify_change_handlers(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const polycall_config_value_t* old_value,
     const polycall_config_value_t* new_value
 ) {
     change_handler_node_t* current = config_ctx->handlers;
 
     while (current) {
         if ((current->section_id == section_id || current->section_id == (polycall_config_section_t)-1) &&
             (!current->key || strcmp(current->key, key) == 0)) {
             current->handler(ctx, section_id, key, old_value, new_value, current->user_data);
         }
         current = current->next;
     }
 }
 
 /* Initialize configuration system */
 polycall_core_error_t polycall_config_init(
     polycall_core_context_t* ctx,
     polycall_config_context_t** config_ctx,
     const polycall_config_options_t* options
 ) {
     polycall_config_context_t* new_ctx;
     polycall_config_options_t default_options;
 
     if (!ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Allocate context */
     new_ctx = polycall_core_malloc(ctx, sizeof(polycall_config_context_t));
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     /* Initialize context */
     memset(new_ctx, 0, sizeof(polycall_config_context_t));
 
     /* Use provided options or defaults */
     if (options) {
         memcpy(&new_ctx->options, options, sizeof(polycall_config_options_t));
     } else {
         default_options = polycall_config_default_options();
         memcpy(&new_ctx->options, &default_options, sizeof(polycall_config_options_t));
     }
 
     /* Auto-load configuration if enabled */
     if (new_ctx->options.auto_load && new_ctx->options.config_path) {
         /* We'll implement this later when providers are registered */
     }
 
     *config_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Create default configuration options */
 polycall_config_options_t polycall_config_default_options(void) {
     polycall_config_options_t options;
 
     /* Initialize with defaults */
     memset(&options, 0, sizeof(polycall_config_options_t));
     options.enable_persistence = true;
     options.enable_change_notification = true;
     options.auto_load = false;
     options.auto_save = false;
     options.validate_on_load = true;
     options.validate_on_save = true;
     options.config_path = NULL;
 
     return options;
 }
 
 /* Clean up configuration system */
 void polycall_config_cleanup(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx
 ) {
     config_node_t* current_node;
     config_node_t* next_node;
     config_provider_node_t* current_provider;
     config_provider_node_t* next_provider;
     change_handler_node_t* current_handler;
     change_handler_node_t* next_handler;
 
     if (!ctx || !config_ctx) {
         return;
     }
 
     /* Auto-save configuration if enabled */
     if (config_ctx->options.auto_save && config_ctx->options.config_path) {
         polycall_config_save(ctx, config_ctx, config_ctx->options.config_path);
     }
 
     /* Free nodes */
     current_node = config_ctx->nodes;
     while (current_node) {
         next_node = current_node->next;
 
         /* Free key */
         if (current_node->key) {
             polycall_core_free(ctx, current_node->key);
         }
 
         /* Free value */
         free_value(ctx, &current_node->value);
 
         /* Free node */
         polycall_core_free(ctx, current_node);
 
         current_node = next_node;
     }
 
     /* Clean up providers */
     current_provider = config_ctx->providers;
     while (current_provider) {
         next_provider = current_provider->next;
 
         /* Cleanup provider if it has a cleanup function */
         if (current_provider->provider.cleanup) {
             current_provider->provider.cleanup(ctx, current_provider->provider.user_data);
         }
 
         /* Free provider node */
         polycall_core_free(ctx, current_provider);
 
         current_provider = next_provider;
     }
 
     /* Free change handlers */
     current_handler = config_ctx->handlers;
     while (current_handler) {
         next_handler = current_handler->next;
 
         /* Free key if present */
         if (current_handler->key) {
             polycall_core_free(ctx, current_handler->key);
         }
 
         /* Free handler node */
         polycall_core_free(ctx, current_handler);
 
         current_handler = next_handler;
     }
 
     /* Free context */
     polycall_core_free(ctx, config_ctx);
 }
 
 /* Load configuration from file */
 polycall_core_error_t polycall_config_load(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     const char* file_path
 ) {
     const char* path;
     config_provider_node_t* current_provider;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Use provided path or default from options */
     path = file_path ? file_path : config_ctx->options.config_path;
     if (!path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Try loading from each provider */
     current_provider = config_ctx->providers;
     while (current_provider) {
         /* TODO: Implement loading from file using providers */
         /* For now, we'll just return success */
         current_provider = current_provider->next;
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Save configuration to file */
 polycall_core_error_t polycall_config_save(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     const char* file_path
 ) {
     const char* path;
     config_provider_node_t* current_provider;
     config_node_t* current_node;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Use provided path or default from options */
     path = file_path ? file_path : config_ctx->options.config_path;
     if (!path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Try saving to the first provider that supports persistence */
     current_provider = config_ctx->providers;
     while (current_provider) {
         /* TODO: Implement saving to file using providers */
         /* For now, we'll just return success */
         current_provider = current_provider->next;
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
/* Register configuration provider */
polycall_core_error_t polycall_config_register_provider(
    polycall_core_context_t* ctx,
    polycall_config_context_t* config_ctx,
    const polycall_config_provider_t* provider
) {
    config_provider_node_t* new_provider;
    polycall_core_error_t result;

    if (!ctx || !config_ctx || !provider ||
        !provider->initialize || !provider->load || !provider->save) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
 
     /* Allocate new provider node */
     new_provider = polycall_core_malloc(ctx, sizeof(config_provider_node_t));
     if (!new_provider) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     /* Initialize provider node */
     memcpy(&new_provider->provider, provider, sizeof(polycall_config_provider_t));
     new_provider->next = NULL;
 
     /* Initialize provider */
     result = provider->initialize(ctx, provider->user_data);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, new_provider);
         return result;
     }
 
     /* Add to list */
     if (!config_ctx->providers) {
         config_ctx->providers = new_provider;
     } else {
         config_provider_node_t* current = config_ctx->providers;
         while (current->next) {
             current = current->next;
         }
         current->next = new_provider;
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Register configuration change handler */
 polycall_core_error_t polycall_config_register_change_handler(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     polycall_config_change_handler_t handler,
     void* user_data,
     uint32_t* handler_id
 ) {
     change_handler_node_t* new_handler;
 
     if (!ctx || !config_ctx || !handler || !handler_id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Check if change notifications are enabled */
     if (!config_ctx->options.enable_change_notification) {
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
 
     /* Allocate new handler node */
     new_handler = polycall_core_malloc(ctx, sizeof(change_handler_node_t));
     if (!new_handler) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     /* Initialize handler node */
     new_handler->id = config_ctx->next_handler_id++;
     new_handler->section_id = section_id;
     new_handler->handler = handler;
     new_handler->user_data = user_data;
     new_handler->next = NULL;
 
     /* Copy key if provided */
     if (key) {
         new_handler->key = polycall_core_malloc(ctx, strlen(key) + 1);
         if (!new_handler->key) {
             polycall_core_free(ctx, new_handler);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(new_handler->key, key);
     } else {
         new_handler->key = NULL;
     }
 
     /* Add to list */
     if (!config_ctx->handlers) {
         config_ctx->handlers = new_handler;
     } else {
         change_handler_node_t* current = config_ctx->handlers;
         while (current->next) {
             current = current->next;
         }
         current->next = new_handler;
     }
 
     *handler_id = new_handler->id;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Unregister configuration change handler */
 polycall_core_error_t polycall_config_unregister_change_handler(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     uint32_t handler_id
 ) {
     change_handler_node_t* current;
     change_handler_node_t* prev = NULL;
 
     if (!ctx || !config_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find handler in list */
     current = config_ctx->handlers;
     while (current) {
         if (current->id == handler_id) {
             /* Remove from list */
             if (prev) {
                 prev->next = current->next;
             } else {
                 config_ctx->handlers = current->next;
             }
 
             /* Free key if present */
             if (current->key) {
                 polycall_core_free(ctx, current->key);
             }
 
             /* Free handler node */
             polycall_core_free(ctx, current);
             return POLYCALL_CORE_SUCCESS;
         }
 
         prev = current;
         current = current->next;
     }
 
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 /* Get boolean configuration value */
 bool polycall_config_get_bool(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     bool default_value
 ) {
     config_node_t* node;
     polycall_config_value_t value;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !key) {
         return default_value;
     }
 
     /* Try to find in local cache */
     node = find_node(config_ctx, section_id, key);
     if (node && node->value.type == POLYCALL_CONFIG_VALUE_BOOLEAN) {
         return node->value.value.bool_value;
     }
 
     /* Try to load from providers */
     /* TODO: Implement loading from providers */
 
     return default_value;
 }
 
 /* Set boolean configuration value */
 polycall_core_error_t polycall_config_set_bool(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     bool value
 ) {
     config_node_t* node;
     polycall_config_value_t old_value;
     polycall_config_value_t new_value;
 
     if (!ctx || !config_ctx || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find or create node */
     node = find_node(config_ctx, section_id, key);
     if (!node) {
         /* Create new node */
         node = polycall_core_malloc(ctx, sizeof(config_node_t));
         if (!node) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
 
         /* Initialize node */
         memset(node, 0, sizeof(config_node_t));
         node->section_id = section_id;
         node->key = polycall_core_malloc(ctx, strlen(key) + 1);
         if (!node->key) {
             polycall_core_free(ctx, node);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(node->key, key);
 
         /* Add to list */
         node->next = config_ctx->nodes;
         config_ctx->nodes = node;
     } else {
         /* Save old value for notification */
         memcpy(&old_value, &node->value, sizeof(polycall_config_value_t));
         free_value(ctx, &node->value);
     }
 
     /* Set value */
     node->value.type = POLYCALL_CONFIG_VALUE_BOOLEAN;
     node->value.value.bool_value = value;
 
     /* Prepare new value for notification */
     new_value.type = POLYCALL_CONFIG_VALUE_BOOLEAN;
     new_value.value.bool_value = value;
 
     /* Notify change handlers */
     if (config_ctx->options.enable_change_notification) {
         notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, &new_value);
     }
 
     /* Auto-save if enabled */
     if (config_ctx->options.auto_save && config_ctx->options.config_path) {
         polycall_config_save(ctx, config_ctx, NULL);
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Get integer configuration value */
 int64_t polycall_config_get_int(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     int64_t default_value
 ) {
     config_node_t* node;
     polycall_config_value_t value;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !key) {
         return default_value;
     }
 
     /* Try to find in local cache */
     node = find_node(config_ctx, section_id, key);
     if (node && node->value.type == POLYCALL_CONFIG_VALUE_INTEGER) {
         return node->value.value.int_value;
     }
 
     /* Try to load from providers */
     /* TODO: Implement loading from providers */
 
     return default_value;
 }
 
 /* Set integer configuration value */
 polycall_core_error_t polycall_config_set_int(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     int64_t value
 ) {
     config_node_t* node;
     polycall_config_value_t old_value;
     polycall_config_value_t new_value;
 
     if (!ctx || !config_ctx || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find or create node */
     node = find_node(config_ctx, section_id, key);
     if (!node) {
         /* Create new node */
         node = polycall_core_malloc(ctx, sizeof(config_node_t));
         if (!node) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
 
         /* Initialize node */
         memset(node, 0, sizeof(config_node_t));
         node->section_id = section_id;
         node->key = polycall_core_malloc(ctx, strlen(key) + 1);
         if (!node->key) {
             polycall_core_free(ctx, node);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(node->key, key);
 
         /* Add to list */
         node->next = config_ctx->nodes;
         config_ctx->nodes = node;
     } else {
         /* Save old value for notification */
         memcpy(&old_value, &node->value, sizeof(polycall_config_value_t));
         free_value(ctx, &node->value);
     }
 
     /* Set value */
     node->value.type = POLYCALL_CONFIG_VALUE_INTEGER;
     node->value.value.int_value = value;
 
     /* Prepare new value for notification */
     new_value.type = POLYCALL_CONFIG_VALUE_INTEGER;
     new_value.value.int_value = value;
 
     /* Notify change handlers */
     if (config_ctx->options.enable_change_notification) {
         notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, &new_value);
     }
 
     /* Auto-save if enabled */
     if (config_ctx->options.auto_save && config_ctx->options.config_path) {
         polycall_config_save(ctx, config_ctx, NULL);
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Get float configuration value */
 double polycall_config_get_float(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     double default_value
 ) {
     config_node_t* node;
     polycall_config_value_t value;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !key) {
         return default_value;
     }
 
     /* Try to find in local cache */
     node = find_node(config_ctx, section_id, key);
     if (node && node->value.type == POLYCALL_CONFIG_VALUE_FLOAT) {
         return node->value.value.float_value;
     }
 
     /* Try to load from providers */
     /* TODO: Implement loading from providers */
 
     return default_value;
 }
 
 /* Set float configuration value */
 polycall_core_error_t polycall_config_set_float(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     double value
 ) {
     config_node_t* node;
     polycall_config_value_t old_value;
     polycall_config_value_t new_value;
 
     if (!ctx || !config_ctx || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find or create node */
     node = find_node(config_ctx, section_id, key);
     if (!node) {
         /* Create new node */
         node = polycall_core_malloc(ctx, sizeof(config_node_t));
         if (!node) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
 
         /* Initialize node */
         memset(node, 0, sizeof(config_node_t));
         node->section_id = section_id;
         node->key = polycall_core_malloc(ctx, strlen(key) + 1);
         if (!node->key) {
             polycall_core_free(ctx, node);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(node->key, key);
 
         /* Add to list */
         node->next = config_ctx->nodes;
         config_ctx->nodes = node;
     } else {
         /* Save old value for notification */
         memcpy(&old_value, &node->value, sizeof(polycall_config_value_t));
         free_value(ctx, &node->value);
     }
 
     /* Set value */
     node->value.type = POLYCALL_CONFIG_VALUE_FLOAT;
     node->value.value.float_value = value;
 
     /* Prepare new value for notification */
     new_value.type = POLYCALL_CONFIG_VALUE_FLOAT;
     new_value.value.float_value = value;
 
     /* Notify change handlers */
     if (config_ctx->options.enable_change_notification) {
         notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, &new_value);
     }
 
     /* Auto-save if enabled */
     if (config_ctx->options.auto_save && config_ctx->options.config_path) {
         polycall_config_save(ctx, config_ctx, NULL);
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Get string configuration value */
 polycall_core_error_t polycall_config_get_string(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     char* buffer,
     size_t buffer_size,
     const char* default_value
 ) {
     config_node_t* node;
     polycall_config_value_t value;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !key || !buffer || buffer_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Try to find in local cache */
     node = find_node(config_ctx, section_id, key);
     if (node && node->value.type == POLYCALL_CONFIG_VALUE_STRING && node->value.value.string_value) {
         strncpy(buffer, node->value.value.string_value, buffer_size - 1);
         buffer[buffer_size - 1] = '\0';
         return POLYCALL_CORE_SUCCESS;
     }
 
     /* Try to load from providers */
     /* TODO: Implement loading from providers */
 
     /* Use default value */
     if (default_value) {
         strncpy(buffer, default_value, buffer_size - 1);
         buffer[buffer_size - 1] = '\0';
         return POLYCALL_CORE_SUCCESS;
     } else {
         buffer[0] = '\0';
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
 }
 
 /* Set string configuration value */
 polycall_core_error_t polycall_config_set_string(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     const char* value
 ) {
     config_node_t* node;
     polycall_config_value_t old_value;
     polycall_config_value_t new_value;
 
     if (!ctx || !config_ctx || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find or create node */
     node = find_node(config_ctx, section_id, key);
     if (!node) {
         /* Create new node */
         node = polycall_core_malloc(ctx, sizeof(config_node_t));
         if (!node) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
 
         /* Initialize node */
         memset(node, 0, sizeof(config_node_t));
         node->section_id = section_id;
         node->key = polycall_core_malloc(ctx, strlen(key) + 1);
         if (!node->key) {
             polycall_core_free(ctx, node);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(node->key, key);
 
         /* Add to list */
         node->next = config_ctx->nodes;
         config_ctx->nodes = node;
     } else {
         /* Save old value for notification */
         memcpy(&old_value, &node->value, sizeof(polycall_config_value_t));
         free_value(ctx, &node->value);
     }
 
     /* Set value */
     node->value.type = POLYCALL_CONFIG_VALUE_STRING;
     node->value.value.string_value = polycall_core_malloc(ctx, strlen(value) + 1);
     if (!node->value.value.string_value) {
         polycall_core_free(ctx, node->key);
         polycall_core_free(ctx, node);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     strcpy(node->value.value.string_value, value);
 
     /* Prepare new value for notification */
     new_value.type = POLYCALL_CONFIG_VALUE_STRING;
     new_value.value.string_value = node->value.value.string_value;
 
     /* Notify change handlers */
     if (config_ctx->options.enable_change_notification) {
         notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, &new_value);
     }
 
     /* Auto-save if enabled */
     if (config_ctx->options.auto_save && config_ctx->options.config_path) {
         polycall_config_save(ctx, config_ctx, NULL);
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Get object configuration value */
 polycall_core_error_t polycall_config_get_object(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     void** value
 ) {
     config_node_t* node;
     polycall_config_value_t config_value;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     *value = NULL;
 
     /* Try to find in local cache */
     node = find_node(config_ctx, section_id, key);
     if (node && node->value.type == POLYCALL_CONFIG_VALUE_OBJECT) {
         *value = node->value.value.object_value;
         return POLYCALL_CORE_SUCCESS;
     }
 
     /* Try to load from providers */
     /* TODO: Implement loading from providers */
 
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 /* Set object configuration value */
 polycall_core_error_t polycall_config_set_object(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key,
     void* value,
     void (*object_free)(void* object)
 ) {
     config_node_t* node;
     polycall_config_value_t old_value;
     polycall_config_value_t new_value;
 
     if (!ctx || !config_ctx || !key || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find or create node */
     node = find_node(config_ctx, section_id, key);
     if (!node) {
         /* Create new node */
         node = polycall_core_malloc(ctx, sizeof(config_node_t));
         if (!node) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
 
         /* Initialize node */
         memset(node, 0, sizeof(config_node_t));
         node->section_id = section_id;
         node->key = polycall_core_malloc(ctx, strlen(key) + 1);
         if (!node->key) {
             polycall_core_free(ctx, node);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(node->key, key);
 
         /* Add to list */
         node->next = config_ctx->nodes;
         config_ctx->nodes = node;
     } else {
         /* Save old value for notification */
         memcpy(&old_value, &node->value, sizeof(polycall_config_value_t));
         free_value(ctx, &node->value);
     }
 
     /* Set value */
     node->value.type = POLYCALL_CONFIG_VALUE_OBJECT;
     node->value.value.object_value = value;
     node->value.object_free = object_free;
 
     /* Prepare new value for notification */
     new_value.type = POLYCALL_CONFIG_VALUE_OBJECT;
     new_value.value.object_value = value;
     new_value.object_free = object_free;
 
     /* Notify change handlers */
     if (config_ctx->options.enable_change_notification) {
         notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, &new_value);
     }
 
     /* Auto-save if enabled */
     if (config_ctx->options.auto_save && config_ctx->options.config_path) {
         polycall_config_save(ctx, config_ctx, NULL);
     }
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Check if configuration key exists */
 bool polycall_config_exists(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key
 ) {
     config_node_t* node;
     bool exists = false;
     config_provider_node_t* current_provider;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !key) {
         return false;
     }
 
     /* Check local cache first */
     if (find_node(config_ctx, section_id, key)) {
         return true;
     }
 
     /* Try each provider */
     current_provider = config_ctx->providers;
     while (current_provider) {
         if (current_provider->provider.exists) {
             if (current_provider->provider.exists(ctx, current_provider->provider.user_data, 
                                                  section_id, key, &exists) == POLYCALL_CORE_SUCCESS && exists) {
                 return true;
             }
         }
         current_provider = current_provider->next;
     }
 
     return false;
 }
 
 /* Remove configuration key */
 polycall_core_error_t polycall_config_remove(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     const char* key
 ) {
     config_node_t* current;
     config_node_t* prev = NULL;
     polycall_config_value_t old_value;
     polycall_config_value_t new_value;
 
     if (!ctx || !config_ctx || !key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Find node in list */
     current = config_ctx->nodes;
     while (current) {
         if (current->section_id == section_id && strcmp(current->key, key) == 0) {
             /* Save old value for notification */
             memcpy(&old_value, &current->value, sizeof(polycall_config_value_t));
 
             /* Initialize empty new value for notification */
             memset(&new_value, 0, sizeof(polycall_config_value_t));
 
             /* Remove from list */
             if (prev) {
                 prev->next = current->next;
             } else {
                 config_ctx->nodes = current->next;
             }
 
             /* Free node data */
             free_value(ctx, &current->value);
             polycall_core_free(ctx, current->key);
             polycall_core_free(ctx, current);
 
             /* Notify change handlers */
             if (config_ctx->options.enable_change_notification) {
                 notify_change_handlers(ctx, config_ctx, section_id, key, &old_value, &new_value);
             }
 
             /* Auto-save if enabled */
             if (config_ctx->options.auto_save && config_ctx->options.config_path) {
                 polycall_config_save(ctx, config_ctx, NULL);
             }
 
             return POLYCALL_CORE_SUCCESS;
         }
 
         prev = current;
         current = current->next;
     }
 
     /* TODO: Try to remove from providers */
 
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 /* Enumerate configuration keys */
 polycall_core_error_t polycall_config_enumerate(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     polycall_config_section_t section_id,
     void (*callback)(const char* key, void* user_data),
     void* user_data
 ) {
     config_node_t* current;
     config_provider_node_t* current_provider;
     polycall_core_error_t result;
 
     if (!ctx || !config_ctx || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     /* Enumerate local cache */
     current = config_ctx->nodes;
     while (current) {
         if (current->section_id == section_id) {
             callback(current->key, user_data);
         }
         current = current->next;
     }
 
     /* TODO: Enumerate from providers */
 
     return POLYCALL_CORE_SUCCESS;
 }