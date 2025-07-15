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
 * @file polycall_hierarchical_error.c
 * @brief Hierarchical Error Handling implementation for LibPolyCall
 * @author Based on design by Nnamdi Okpala (OBINexusComputing)
 *
 * Implements advanced error handling with inheritance, component-specific
 * error reporting, and error propagation for complex protocol interactions.
 */

 #include "polycall/core/polycall/polycall_hierarchical_error.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_logger.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdarg.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 /**
  * @brief Component error handler structure
  */
 typedef struct component_error_handler {
     char component_name[POLYCALL_MAX_COMPONENT_NAME_LENGTH];
     polycall_error_source_t source;
     polycall_hierarchical_error_handler_fn handler;
     void* user_data;
     polycall_error_propagation_mode_t propagation_mode;
     char parent_component[POLYCALL_MAX_COMPONENT_NAME_LENGTH];
     polycall_error_record_t last_error;
     struct component_error_handler* next;
 } component_error_handler_t;
 
 /**
  * @brief Hierarchical error context implementation
  */
 struct polycall_hierarchical_error_context {
     polycall_core_context_t* core_ctx;
     component_error_handler_t* handlers;
     uint32_t handler_count;
 };
 
 /**
  * @brief Find a component handler by name
  *
  * @param error_ctx Hierarchical error context
  * @param component_name Component name
  * @return Found handler or NULL
  */
 static component_error_handler_t* find_handler(
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name
 ) {
     component_error_handler_t* current = error_ctx->handlers;
     
     while (current) {
         if (strcmp(current->component_name, component_name) == 0) {
             return current;
         }
         current = current->next;
     }
     
     return NULL;
 }
 
 /**
  * @brief Initialize hierarchical error system
  */
 polycall_core_error_t polycall_hierarchical_error_init(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t** error_ctx
 ) {
     if (!core_ctx || !error_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_hierarchical_error_context_t* new_ctx = polycall_core_malloc(core_ctx, 
         sizeof(polycall_hierarchical_error_context_t));
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_hierarchical_error_context_t));
     new_ctx->core_ctx = core_ctx;
     new_ctx->handlers = NULL;
     new_ctx->handler_count = 0;
     
     *error_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up hierarchical error system
  */
 void polycall_hierarchical_error_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx
 ) {
     if (!core_ctx || !error_ctx) {
         return;
     }
     
     // Free all handlers
     component_error_handler_t* current = error_ctx->handlers;
     component_error_handler_t* next = NULL;
     
     while (current) {
         next = current->next;
         polycall_core_free(core_ctx, current);
         current = next;
     }
     
     // Free context
     polycall_core_free(core_ctx, error_ctx);
 }
 
 /**
  * @brief Register a component-specific error handler
  */
 polycall_core_error_t polycall_hierarchical_error_register_handler(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const polycall_hierarchical_error_handler_config_t* config
 ) {
     if (!core_ctx || !error_ctx || !config || !config->component_name[0]) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we already have a handler for this component
     if (find_handler(error_ctx, config->component_name)) {
         return POLYCALL_CORE_ERROR_ALREADY_EXISTS;
     }
     
     // Check if parent exists if specified
     if (config->parent_component[0] && !find_handler(error_ctx, config->parent_component)) {
         // Parent specified but not found
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Create new handler
     component_error_handler_t* handler = polycall_core_malloc(core_ctx, 
         sizeof(component_error_handler_t));
     if (!handler) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize handler
     memset(handler, 0, sizeof(component_error_handler_t));
     strncpy(handler->component_name, config->component_name, POLYCALL_MAX_COMPONENT_NAME_LENGTH - 1);
     handler->source = config->source;
     handler->handler = config->handler;
     handler->user_data = config->user_data;
     handler->propagation_mode = config->propagation_mode;
     strncpy(handler->parent_component, config->parent_component, POLYCALL_MAX_COMPONENT_NAME_LENGTH - 1);
     
     // Add to list
     handler->next = error_ctx->handlers;
     error_ctx->handlers = handler;
     error_ctx->handler_count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Unregister a component-specific error handler
  */
 polycall_core_error_t polycall_hierarchical_error_unregister_handler(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name
 ) {
     if (!core_ctx || !error_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     component_error_handler_t* current = error_ctx->handlers;
     component_error_handler_t* prev = NULL;
     
     // Find the handler
     while (current) {
         if (strcmp(current->component_name, component_name) == 0) {
             // Check if any components have this as parent
             component_error_handler_t* check = error_ctx->handlers;
             while (check) {
                 if (strcmp(check->parent_component, component_name) == 0) {
                     // Can't remove, it's a parent
                     return POLYCALL_CORE_ERROR_INVALID_STATE;
                 }
                 check = check->next;
             }
             
             // Remove from list
             if (prev) {
                 prev->next = current->next;
             } else {
                 error_ctx->handlers = current->next;
             }
             
             // Free handler
             polycall_core_free(core_ctx, current);
             error_ctx->handler_count--;
             
             return POLYCALL_CORE_SUCCESS;
         }
         
         prev = current;
         current = current->next;
     }
     
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 /**
  * @brief Propagate error to other components
  *
  * @param core_ctx Core context
  * @param error_ctx Hierarchical error context
  * @param handler Handler triggering propagation
  * @param record Error record to propagate
  */
 static void propagate_error(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     component_error_handler_t* handler,
     const polycall_error_record_t* record
 ) {
     if (!handler || !record) {
         return;
     }
     
     // Check propagation mode
     if (handler->propagation_mode == POLYCALL_ERROR_PROPAGATE_NONE) {
         return;
     }
     
     // Propagate upward if needed
     if (handler->propagation_mode == POLYCALL_ERROR_PROPAGATE_UPWARD || 
         handler->propagation_mode == POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL) {
         
         if (handler->parent_component[0]) {
             component_error_handler_t* parent = find_handler(error_ctx, handler->parent_component);
             if (parent && parent->handler) {
                 // Call parent handler
                 parent->handler(core_ctx, handler->component_name, record->source, 
                     record->code, record->severity, record->message, parent->user_data);
                 
                 // Update parent's last error
                 memcpy(&parent->last_error, record, sizeof(polycall_error_record_t));
                 
                 // Continue propagation
                 propagate_error(core_ctx, error_ctx, parent, record);
             }
         }
     }
     
     // Propagate downward if needed
     if (handler->propagation_mode == POLYCALL_ERROR_PROPAGATE_DOWNWARD || 
         handler->propagation_mode == POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL) {
         
         // Find all children
         component_error_handler_t* current = error_ctx->handlers;
         while (current) {
             if (strcmp(current->parent_component, handler->component_name) == 0 && current->handler) {
                 // Call child handler
                 current->handler(core_ctx, handler->component_name, record->source, 
                     record->code, record->severity, record->message, current->user_data);
                 
                 // Update child's last error
                 memcpy(&current->last_error, record, sizeof(polycall_error_record_t));
                 
                 // Continue propagation
                 propagate_error(core_ctx, error_ctx, current, record);
             }
             current = current->next;
         }
     }
 }
 
 /**
  * @brief Set an error with propagation
  */
 polycall_core_error_t polycall_hierarchical_error_set(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name,
     polycall_error_source_t source,
     int32_t code,
     polycall_error_severity_t severity,
     const char* file,
     int line,
     const char* message,
     ...
 ) {
     if (!core_ctx || !error_ctx || !component_name || !file || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the component handler
     component_error_handler_t* handler = find_handler(error_ctx, component_name);
     if (!handler) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Format the error message
     va_list args;
     va_start(args, message);
     vsnprintf(handler->last_error.message, POLYCALL_ERROR_MAX_MESSAGE_LENGTH - 1, 
         message, args);
     va_end(args);
     
     // Fill in error record
     handler->last_error.source = source;
     handler->last_error.code = code;
     handler->last_error.severity = severity;
     handler->last_error.file = file;
     handler->last_error.line = line;
     handler->last_error.timestamp = time(NULL);
     
     // Call the handler if available
     if (handler->handler) {
         handler->handler(core_ctx, component_name, source, code, severity,
             handler->last_error.message, handler->user_data);
     }
     
     // Add to core error system as well
     polycall_error_set_full(core_ctx, source, code, severity, file, line,
         handler->last_error.message);
     
     // Propagate to related components
     propagate_error(core_ctx, error_ctx, handler, &handler->last_error);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get the parent component
  */
 polycall_core_error_t polycall_hierarchical_error_get_parent(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name,
     char* parent_buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !error_ctx || !component_name || !parent_buffer || buffer_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the component handler
     component_error_handler_t* handler = find_handler(error_ctx, component_name);
     if (!handler) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check if parent exists
     if (!handler->parent_component[0]) {
         parent_buffer[0] = '\0'; // No parent
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Copy parent name
     strncpy(parent_buffer, handler->parent_component, buffer_size - 1);
     parent_buffer[buffer_size - 1] = '\0';
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get child components
  */
 polycall_core_error_t polycall_hierarchical_error_get_children(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name,
     char children[][POLYCALL_MAX_COMPONENT_NAME_LENGTH],
     uint32_t max_children,
     uint32_t* child_count
 ) {
     if (!core_ctx || !error_ctx || !component_name || !children || !child_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the component handler
     component_error_handler_t* handler = find_handler(error_ctx, component_name);
     if (!handler) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find all children
     *child_count = 0;
     component_error_handler_t* current = error_ctx->handlers;
     
     while (current && *child_count < max_children) {
         if (strcmp(current->parent_component, component_name) == 0) {
             strncpy(children[*child_count], current->component_name, 
                 POLYCALL_MAX_COMPONENT_NAME_LENGTH - 1);
             children[*child_count][POLYCALL_MAX_COMPONENT_NAME_LENGTH - 1] = '\0';
             (*child_count)++;
         }
         current = current->next;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set error propagation mode
  */
 polycall_core_error_t polycall_hierarchical_error_set_propagation(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name,
     polycall_error_propagation_mode_t mode
 ) {
     if (!core_ctx || !error_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the component handler
     component_error_handler_t* handler = find_handler(error_ctx, component_name);
     if (!handler) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Update propagation mode
     handler->propagation_mode = mode;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get last error for a component
  */
 bool polycall_hierarchical_error_get_last(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name,
     polycall_error_record_t* record
 ) {
     if (!core_ctx || !error_ctx || !component_name || !record) {
         return false;
     }
     
     // Find the component handler
     component_error_handler_t* handler = find_handler(error_ctx, component_name);
     if (!handler) {
         return false;
     }
     
     // Check if there's an error
     if (handler->last_error.code == 0) {
         return false; // No error
     }
     
     // Copy error record
     memcpy(record, &handler->last_error, sizeof(polycall_error_record_t));
     
     return true;
 }
 
 /**
  * @brief Clear last error for a component
  */
 polycall_core_error_t polycall_hierarchical_error_clear(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_error_context_t* error_ctx,
     const char* component_name
 ) {
     if (!core_ctx || !error_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the component handler
     component_error_handler_t* handler = find_handler(error_ctx, component_name);
     if (!handler) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Clear error
     memset(&handler->last_error, 0, sizeof(polycall_error_record_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
