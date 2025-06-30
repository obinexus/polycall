/**
 * @file auth_audit.c
 * @brief Implementation of audit logging for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the audit logging interfaces for LibPolyCall authentication,
 * providing functions to log, query, and export security-related events.
 */

 #include "polycall/core/auth/polycall_auth_audit.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 

 
 /**
  * @brief Log an audit event
  */
 polycall_core_error_t polycall_auth_log_audit_event(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const audit_event_t* event
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !event) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if audit logging is enabled
     if (!auth_ctx->config.enable_audit_logging) {
         return POLYCALL_CORE_SUCCESS; // Silently ignore if logging is disabled
     }
     
     // Create an audit entry from the event
     audit_entry_t* entry = create_audit_entry(core_ctx, event);
     if (!entry) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Lock the audit context
     pthread_mutex_lock(&auth_ctx->auth_audit->mutex);
     
     // Check if we need to resize the audit entries array
     if (auth_ctx->auth_audit->entry_count >= auth_ctx->auth_audit->entry_capacity) {
         size_t new_capacity = auth_ctx->auth_audit->entry_capacity * 2;
         if (new_capacity == 0) {
             new_capacity = 32; // Initial capacity if not yet initialized
         }
         
         audit_entry_t** new_entries = polycall_core_malloc(core_ctx, 
                                                           new_capacity * sizeof(audit_entry_t*));
         if (!new_entries) {
             free_audit_entry(core_ctx, entry);
             pthread_mutex_unlock(&auth_ctx->auth_audit->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing entries
         for (size_t i = 0; i < auth_ctx->auth_audit->entry_count; i++) {
             new_entries[i] = auth_ctx->auth_audit->entries[i];
         }
         
         // Free old array but not the entries themselves
         if (auth_ctx->auth_audit->entries) {
             polycall_core_free(core_ctx, auth_ctx->auth_audit->entries);
         }
         
         auth_ctx->auth_audit->entries = new_entries;
         auth_ctx->auth_audit->entry_capacity = new_capacity;
     }
     
     // Add the entry to the array
     auth_ctx->auth_audit->entries[auth_ctx->auth_audit->entry_count++] = entry;
     
     // Set the logging timestamp
     entry->log_timestamp = get_current_timestamp();
     
     // Unlock the audit context
     pthread_mutex_unlock(&auth_ctx->auth_audit->mutex);
     
     // Log to system log if configured
     // This would typically integrate with the global logging system
     // For this implementation, we'll just use a placeholder
     /*
     POLYCALL_LOG(core_ctx, POLYCALL_LOG_INFO, 
                 "Audit event [%s]: identity=%s, resource=%s, action=%s, success=%s", 
                 polycall_audit_event_type_to_string(event->type),
                 event->identity_id ? event->identity_id : "null",
                 event->resource ? event->resource : "null",
                 event->action ? event->action : "null",
                 event->success ? "true" : "false");
     */
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Query audit events
  */
 polycall_core_error_t polycall_auth_query_audit_events(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const audit_query_t* query,
     audit_event_t*** events,
     size_t* event_count
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !query || !events || !event_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize result
     *events = NULL;
     *event_count = 0;
     
     // Lock the audit context
     pthread_mutex_lock(&auth_ctx->auth_audit->mutex);
     
     // First pass: count matching entries
     size_t matching_count = 0;
     for (size_t i = 0; i < auth_ctx->auth_audit->entry_count; i++) {
         if (match_audit_event(auth_ctx->auth_audit->entries[i], query)) {
             matching_count++;
         }
     }
     
     // No matches found
     if (matching_count == 0) {
         pthread_mutex_unlock(&auth_ctx->auth_audit->mutex);
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Determine result count - in a real implementation we might add
     // pagination parameters to audit_query_t structure
     size_t result_count = matching_count;
     
     // Allocate result array
     audit_event_t** result = polycall_core_malloc(core_ctx, result_count * sizeof(audit_event_t*));
     if (!result) {
         pthread_mutex_unlock(&auth_ctx->auth_audit->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Second pass: collect matching entries
    size_t result_index = 0;
    for (size_t i = 0; i < auth_ctx->auth_audit->entry_count && result_index < result_count; i++) {
        audit_entry_t* entry = auth_ctx->auth_audit->entries[i];
        if (match_audit_event(entry, query)) {
           
           // Create a copy of the event
           audit_event_t* event_copy = polycall_auth_create_audit_event(
              core_ctx,
              entry->event.type,
              entry->event.identity_id,
              entry->event.resource,
              entry->event.action,
              entry->event.success,
              entry->event.error_message
           );
           
           if (!event_copy) {
              // Clean up on failure
              for (size_t j = 0; j < result_index; j++) {
                 polycall_auth_free_audit_event(core_ctx, result[j]);
              }
              polycall_core_free(core_ctx, result);
              pthread_mutex_unlock(&auth_ctx->auth_audit->mutex);
              return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
           }
           
           // Copy additional fields
           event_copy->timestamp = entry->event.timestamp;
           
           // Note: source_ip, user_agent, and details fields are not copied
           // because they are not present in the audit_event_t structure
           
           result[result_index++] = event_copy;
        }
    }
     
     // Unlock the audit context
     pthread_mutex_unlock(&auth_ctx->auth_audit->mutex);
     
     // Return results
     *events = result;
     *event_count = result_index;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Export audit events to a file
  */
 polycall_core_error_t polycall_auth_export_audit_events(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const audit_query_t* query,
     const char* filename,
     const char* format
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !query || !filename || !format) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Query matching events
     audit_event_t** events = NULL;
     size_t event_count = 0;
     
     polycall_core_error_t result = polycall_auth_query_audit_events(
         core_ctx,
         auth_ctx,
         query,
         &events,
         &event_count
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // No events found
     if (event_count == 0) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Generate export content based on format
     char* content = NULL;
     
     if (strcmp(format, "json") == 0) {
         content = export_events_to_json(core_ctx, events, event_count);
     } else if (strcmp(format, "csv") == 0) {
         content = export_events_to_csv(core_ctx, events, event_count);
     } else {
         // Unsupported format
         for (size_t i = 0; i < event_count; i++) {
             polycall_auth_free_audit_event(core_ctx, events[i]);
         }
         polycall_core_free(core_ctx, events);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!content) {
         // Failed to generate content
         for (size_t i = 0; i < event_count; i++) {
             polycall_auth_free_audit_event(core_ctx, events[i]);
         }
         polycall_core_free(core_ctx, events);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Open file for writing
     FILE* file = fopen(filename, "w");
     if (!file) {
         polycall_core_free(core_ctx, content);
         for (size_t i = 0; i < event_count; i++) {
             polycall_auth_free_audit_event(core_ctx, events[i]);
         }
         polycall_core_free(core_ctx, events);
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Write content to file
     size_t content_len = strlen(content);
     size_t written = fwrite(content, 1, content_len, file);
     
     // Close file
     fclose(file);
     
     // Check if write was successful
     if (written != content_len) {
         polycall_core_free(core_ctx, content);
         for (size_t i = 0; i < event_count; i++) {
             polycall_auth_free_audit_event(core_ctx, events[i]);
         }
         polycall_core_free(core_ctx, events);
         return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
     }
     
     // Clean up
     polycall_core_free(core_ctx, content);
     for (size_t i = 0; i < event_count; i++) {
         polycall_auth_free_audit_event(core_ctx, events[i]);
     }
     polycall_core_free(core_ctx, events);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create an audit event
  */
 audit_event_t* polycall_auth_create_audit_event(
     polycall_core_context_t* core_ctx,
     polycall_audit_event_type_t type,
     const char* identity_id,
     const char* resource,
     const char* action,
     bool success,
     const char* error_message
 ) {
     // Validate parameters
     if (!core_ctx) {
         return NULL;
     }
     
     // Allocate event structure
     audit_event_t* event = polycall_core_malloc(core_ctx, sizeof(audit_event_t));
     if (!event) {
         return NULL;
     }
     
     // Initialize event
     memset(event, 0, sizeof(audit_event_t));
     
     // Set basic fields
     event->type = type;
     event->timestamp = get_current_timestamp();
     event->success = success;
     
     // Copy identity ID if provided
     if (identity_id) {
         size_t len = strlen(identity_id) + 1;
         event->identity_id = polycall_core_malloc(core_ctx, len);
         if (event->identity_id) {
             memcpy(event->identity_id, identity_id, len);
         }
     }
     
     // Copy resource if provided
     if (resource) {
         size_t len = strlen(resource) + 1;
         event->resource = polycall_core_malloc(core_ctx, len);
         if (event->resource) {
             memcpy(event->resource, resource, len);
         }
     }
     
     // Copy action if provided
     if (action) {
         size_t len = strlen(action) + 1;
         event->action = polycall_core_malloc(core_ctx, len);
         if (event->action) {
             memcpy(event->action, action, len);
         }
     }
     
     // Copy error message if provided and event failed
     if (!success && error_message) {
         size_t len = strlen(error_message) + 1;
         event->error_message = polycall_core_malloc(core_ctx, len);
         if (event->error_message) {
             memcpy(event->error_message, error_message, len);
         }
     }
     
     return event;
 }
 
 /**
  * @brief Free an audit event
  */
 void polycall_auth_free_audit_event(
     polycall_core_context_t* core_ctx,
     audit_event_t* event
 ) {
     if (!core_ctx || !event) {
         return;
     }
     
     // Free all allocated fields
     if (event->identity_id) {
         polycall_core_free(core_ctx, event->identity_id);
     }
     
     if (event->resource) {
         polycall_core_free(core_ctx, event->resource);
     }
     
     if (event->action) {
         polycall_core_free(core_ctx, event->action);
     }
     
     if (event->error_message) {
         polycall_core_free(core_ctx, event->error_message);
     }
     
     if (event->user_data) {
         polycall_core_free(core_ctx, event->user_data);
     }
     
     // Free event structure
     polycall_core_free(core_ctx, event);
 }
 
 /**
  * @brief Free an array of audit events
  */
 void polycall_auth_free_audit_events(
     polycall_core_context_t* core_ctx,
     audit_event_t** events,
     size_t event_count
 ) {
     if (!core_ctx || !events) {
         return;
     }
     
     // Free each event
     for (size_t i = 0; i < event_count; i++) {
         if (events[i]) {
             polycall_auth_free_audit_event(core_ctx, events[i]);
         }
     }
     
     // Free the array
     polycall_core_free(core_ctx, events);
 }
 
 /**
  * @brief Get a string representation of an audit event type
  */
 const char* polycall_audit_event_type_to_string(polycall_audit_event_type_t type) {
     switch (type) {
         case POLYCALL_AUDIT_EVENT_LOGIN:
             return "LOGIN";
         case POLYCALL_AUDIT_EVENT_LOGOUT:
             return "LOGOUT";
         case POLYCALL_AUDIT_EVENT_TOKEN_ISSUE:
             return "TOKEN_ISSUE";
         case POLYCALL_AUDIT_EVENT_TOKEN_VALIDATE:
             return "TOKEN_VALIDATE";
         case POLYCALL_AUDIT_EVENT_TOKEN_REFRESH:
             return "TOKEN_REFRESH";
         case POLYCALL_AUDIT_EVENT_TOKEN_REVOKE:
             return "TOKEN_REVOKE";
         case POLYCALL_AUDIT_EVENT_ACCESS_DENIED:
             return "ACCESS_DENIED";
         case POLYCALL_AUDIT_EVENT_ACCESS_GRANTED:
             return "ACCESS_GRANTED";
         case POLYCALL_AUDIT_EVENT_IDENTITY_CREATE:
             return "IDENTITY_CREATE";
         case POLYCALL_AUDIT_EVENT_IDENTITY_UPDATE:
             return "IDENTITY_UPDATE";
         case POLYCALL_AUDIT_EVENT_IDENTITY_DELETE:
             return "IDENTITY_DELETE";
         case POLYCALL_AUDIT_EVENT_PASSWORD_CHANGE:
             return "PASSWORD_CHANGE";
         case POLYCALL_AUDIT_EVENT_PASSWORD_RESET:
             return "PASSWORD_RESET";
         case POLYCALL_AUDIT_EVENT_ROLE_ASSIGN:
             return "ROLE_ASSIGN";
         case POLYCALL_AUDIT_EVENT_ROLE_REMOVE:
             return "ROLE_REMOVE";
         case POLYCALL_AUDIT_EVENT_POLICY_CREATE:
             return "POLICY_CREATE";
         case POLYCALL_AUDIT_EVENT_POLICY_UPDATE:
             return "POLICY_UPDATE";
         case POLYCALL_AUDIT_EVENT_POLICY_DELETE:
             return "POLICY_DELETE";
         case POLYCALL_AUDIT_EVENT_CUSTOM:
             return "CUSTOM";
         default:
             return "UNKNOWN";
     }
 }
 
 /**
  * @brief Initialize audit service
  * Internal function called from auth_context.c
  */
 auth_audit_t* init_auth_audit(polycall_core_context_t* ctx, bool enable_logging) {
     if (!ctx) {
         return NULL;
     }
     
     // Allocate audit context
     auth_audit_t* audit = polycall_core_malloc(ctx, sizeof(auth_audit_t));
     if (!audit) {
         return NULL;
     }
     
     // Initialize audit context
     memset(audit, 0, sizeof(auth_audit_t));
     audit->core_ctx = ctx;
     audit->enable_logging = enable_logging;
     audit->entry_capacity = 32; // Initial capacity
     
     // Allocate entries array
     audit->entries = polycall_core_malloc(ctx, audit->entry_capacity * sizeof(audit_entry_t*));
     if (!audit->entries) {
         polycall_core_free(ctx, audit);
         return NULL;
     }
     
     // Initialize mutex
     if (pthread_mutex_init(&audit->mutex, NULL) != 0) {
         polycall_core_free(ctx, audit->entries);
         polycall_core_free(ctx, audit);
         return NULL;
     }
     
     return audit;
 }
 
 /**
  * @brief Cleanup audit service
  * Internal function called from auth_context.c
  */
 void cleanup_auth_audit(polycall_core_context_t* ctx, auth_audit_t* audit) {
     if (!ctx || !audit) {
         return;
     }
     
     // Lock mutex
     pthread_mutex_lock(&audit->mutex);
     
     // Free entries
     for (size_t i = 0; i < audit->entry_count; i++) {
         if (audit->entries[i]) {
             free_audit_entry(ctx, audit->entries[i]);
         }
     }
     
     // Free entries array
     if (audit->entries) {
         polycall_core_free(ctx, audit->entries);
     }
     
     // Unlock and destroy mutex
     pthread_mutex_unlock(&audit->mutex);
     pthread_mutex_destroy(&audit->mutex);

        // Free audit context
        polycall_core_free(ctx, audit);
        audit = NULL;
        

        return NULL;
        }

 /**
  * @brief Create an audit entry from an event
  */
 static audit_entry_t* create_audit_entry(polycall_core_context_t* core_ctx, const audit_event_t* event) {
     if (!core_ctx || !event) {
         return NULL;
     }
     
     // Allocate entry
     audit_entry_t* entry = polycall_core_malloc(core_ctx, sizeof(audit_entry_t));
     if (!entry) {
         return NULL;
     }
     
     // Initialize entry
     memset(entry, 0, sizeof(audit_entry_t));
     
     // Set timestamp
     entry->log_timestamp = get_current_timestamp();
     
     // Copy event type
     entry->event.type = event->type;
     entry->event.timestamp = event->timestamp;
     entry->event.success = event->success;
     
     // Copy identity ID if provided
     if (event->identity_id) {
         size_t len = strlen(event->identity_id) + 1;
         entry->event.identity_id = polycall_core_malloc(core_ctx, len);
         if (!entry->event.identity_id) {
             free_audit_entry(core_ctx, entry);
             return NULL;
         }
         memcpy(entry->event.identity_id, event->identity_id, len);
     }
     
     // Copy resource if provided
     if (event->resource) {
         size_t len = strlen(event->resource) + 1;
         entry->event.resource = polycall_core_malloc(core_ctx, len);
         if (!entry->event.resource) {
             free_audit_entry(core_ctx, entry);
             return NULL;
         }
         memcpy(entry->event.resource, event->resource, len);
     }
     
     // Copy action if provided
     if (event->action) {
         size_t len = strlen(event->action) + 1;
         entry->event.action = polycall_core_malloc(core_ctx, len);
         if (!entry->event.action) {
             free_audit_entry(core_ctx, entry);
             return NULL;
         }
         memcpy(entry->event.action, event->action, len);
     }
     
     // Copy error message if provided
     if (event->error_message) {
         size_t len = strlen(event->error_message) + 1;
         entry->event.error_message = polycall_core_malloc(core_ctx, len);
         if (!entry->event.error_message) {
             free_audit_entry(core_ctx, entry);
             return NULL;
         }
         memcpy(entry->event.error_message, event->error_message, len);
     }
     
     // Copy source IP if provided
     if (event->source_ip) {
         size_t len = strlen(event->source_ip) + 1;
         entry->event.source_ip = polycall_core_malloc(core_ctx, len);
         if (!entry->event.source_ip) {
             free_audit_entry(core_ctx, entry);
             return NULL;
         }
         memcpy(entry->event.source_ip, event->source_ip, len);
     }
     
     // Copy user agent if provided
     if (event->user_agent) {
         size_t len = strlen(event->user_agent) + 1;
         entry->event.user_agent = polycall_core_malloc(core_ctx, len);
         if (!entry->event.user_agent) {
             free_audit_entry(core_ctx, entry);
             return NULL;
         }
         memcpy(entry->event.user_agent, event->user_agent, len);
     }
     
     // Copy details if provided
     if (event->details) {
         size_t len = strlen(event->details) + 1;
         entry->event.details = polycall_core_malloc(core_ctx, len);
         if (!entry->event.details) {
             free_audit_entry(core_ctx, entry);

                return NULL;    
            }

        memcpy(entry->event.details, event->details, len);
    }
    // Set log timestamp
    entry->log_timestamp = get_current_timestamp();
    return entry;
}



 /**
    * @brief Free an audit entry
    */
 static void free_audit_entry(polycall_core_context_t* core_ctx, audit_entry_t* entry) {
         if (!core_ctx || !entry) {
                 return;
         }
         
         // Free all allocated fields
         if (entry->event.identity_id) {
                 polycall_core_free(core_ctx, entry->event.identity_id);
         }
         
         if (entry->event.resource) {
                 polycall_core_free(core_ctx, entry->event.resource);
         }
         
         if (entry->event.action) {
                 polycall_core_free(core_ctx, entry->event.action);
         }
         
         if (entry->event.error_message) {
                 polycall_core_free(core_ctx, entry->event.error_message);
         }
         
         if (entry->event.source_ip) {
                 polycall_core_free(core_ctx, entry->event.source_ip);
         }
         
         if (entry->event.user_agent) {
                 polycall_core_free(core_ctx, entry->event.user_agent);
         }
         
         if (entry->event.details) {
                 polycall_core_free(core_ctx, entry->event.details);
         }
         
         // Free the entry structure itself
         polycall_core_free(core_ctx, entry);
 }

 static bool match_audit_event(const audit_entry_t* entry, const audit_query_t* query) {
     if (!entry || !query) {
         return false;
     }
     
     // Check timestamp range
     if (query->start_time > 0 && entry->event.timestamp < query->start_time) {
         return false;
     }
     
     if (query->end_time > 0 && entry->event.timestamp > query->end_time) {
         return false;
     }
     
     // Check event type
     if (query->type != POLYCALL_AUDIT_EVENT_CUSTOM && entry->event.type != query->type) {
         return false;
     }
     
     // Check identity ID
     if (query->identity_id && entry->event.identity_id) {
         if (strcmp(query->identity_id, entry->event.identity_id) != 0) {
             return false;
         }
     } else if (query->identity_id) {
         return false; // Query specifies an identity but event has none
     }
     
     // Check success flag
     if (query->filter_by_success && entry->event.success != query->success) {
         return false;
     }
     // Check action
     if (query->action && entry->event.action) {
         if (strcmp(query->action, entry->event.action) != 0) {
             return false;
         }
     } else if (query->action) {
         return false; // Query specifies an action but event has none
     }
     
     // All checks passed
     return true;
 }
 
 /**
  * @brief Export events to JSON format
  */
 static char* export_events_to_json(polycall_core_context_t* core_ctx, audit_event_t** events, 
                                  size_t event_count) {
     if (!core_ctx || !events || event_count == 0) {
         return NULL;
     }
     
     // Calculate buffer size (rough estimate)
     size_t buffer_size = 256; // Initial size for JSON wrapper
     for (size_t i = 0; i < event_count; i++) {
         buffer_size += 512; // Base size per event
         
         if (events[i]->identity_id) {
             buffer_size += strlen(events[i]->identity_id) * 2; // Account for JSON escaping
         }
         
         if (events[i]->resource) {
             buffer_size += strlen(events[i]->resource) * 2;
         }
         
         if (events[i]->action) {
             buffer_size += strlen(events[i]->action) * 2;
         }
         
         if (events[i]->error_message) {
             buffer_size += strlen(events[i]->error_message) * 2;
         }
         
         if (events[i]->source_ip) {
             buffer_size += strlen(events[i]->source_ip) * 2;
         }
         
         if (events[i]->user_agent) {
             buffer_size += strlen(events[i]->user_agent) * 2;
         }
         
         if (events[i]->details) {
             buffer_size += strlen(events[i]->details) * 2;
         }
     }
     
     // Allocate buffer
     size_t buffer_size = 256; // Initial size for CSV header
     
     // Calculate additional space needed for each event
     for (size_t i = 0; i < event_count; i++) {
         buffer_size += 256; // Base size per event
         
         if (events[i]->identity_id) {
             buffer_size += strlen(events[i]->identity_id) * 2; // Account for CSV escaping
         }
         
         if (events[i]->resource) {
             buffer_size += strlen(events[i]->resource) * 2;
         }
         
         if (events[i]->action) {
             buffer_size += strlen(events[i]->action) * 2;
         }
         
         if (events[i]->error_message) {
             buffer_size += strlen(events[i]->error_message) * 2;
         }
         
         if (events[i]->source_ip) {
             buffer_size += strlen(events[i]->source_ip) * 2;
         }
         
         if (events[i]->user_agent) {
             buffer_size += strlen(events[i]->user_agent) * 2;
         }
         
         if (events[i]->details) {
             buffer_size += strlen(events[i]->details) * 2;
         }
     }
     
     char* buffer = polycall_core_malloc(core_ctx, buffer_size);
     if (!buffer) {
         return NULL;
     }
     
     
     // Write JSON header
     int pos = sprintf(buffer, "{\n  \"events\": [\n");
     
     // Write each event
     for (size_t i = 0; i < event_count; i++) {
         audit_event_t* event = events[i];
         
         // Start event object
         pos += sprintf(buffer + pos, "    {\n");
         
         // Write event type
         pos += sprintf(buffer + pos, "      \"type\": \"%s\",\n", 
                        polycall_audit_event_type_to_string(event->type));
         
         // Write timestamp
         pos += sprintf(buffer + pos, "      \"timestamp\": %llu,\n", 
                        (unsigned long long)event->timestamp);
         
         // Write identity ID if present
         if (event->identity_id) {
             pos += sprintf(buffer + pos, "      \"identity_id\": \"%s\",\n", event->identity_id);
         } else {
             pos += sprintf(buffer + pos, "      \"identity_id\": null,\n");
         }
         
         // Write resource if present
         if (event->resource) {
             pos += sprintf(buffer + pos, "      \"resource\": \"%s\",\n", event->resource);
         } else {
             pos += sprintf(buffer + pos, "      \"resource\": null,\n");
         }
         
         // Write action if present
         if (event->action) {
             pos += sprintf(buffer + pos, "      \"action\": \"%s\",\n", event->action);
         } else {
             pos += sprintf(buffer + pos, "      \"action\": null,\n");
         }
         
         // Write success flag
         pos += sprintf(buffer + pos, "      \"success\": %s,\n", event->success ? "true" : "false");
         
         // Write error message if present
         if (event->error_message) {
             pos += sprintf(buffer + pos, "      \"error_message\": \"%s\",\n", event->error_message);
         } else {
             pos += sprintf(buffer + pos, "      \"error_message\": null,\n");
         }
         
         // Write source IP if present
         if (event->source_ip) {
             pos += sprintf(buffer + pos, "      \"source_ip\": \"%s\",\n", event->source_ip);
         } else {
             pos += sprintf(buffer + pos, "      \"source_ip\": null,\n");
         }
         
         // Write user agent if present
         if (event->user_agent) {
             pos += sprintf(buffer + pos, "      \"user_agent\": \"%s\",\n", event->user_agent);
         } else {
             pos += sprintf(buffer + pos, "      \"user_agent\": null,\n");
         }
         
         // Write details if present
         if (event->details) {
             // Details may already be in JSON format
             if (event->details[0] == '{') {
                 pos += sprintf(buffer + pos, "      \"details\": %s\n", event->details);
             } else {
                 pos += sprintf(buffer + pos, "      \"details\": \"%s\"\n", event->details);
             }
         } else {
             pos += sprintf(buffer + pos, "      \"details\": null\n");
         }
         
         // End event object
         if (i < event_count - 1) {
             pos += sprintf(buffer + pos, "    },\n");
         } else {
             pos += sprintf(buffer + pos, "    }\n");
         }
     }
     
     // Write JSON footer
     pos += sprintf(buffer + pos, "  ]\n}");
     
     return buffer;
 }
 
 /**
  * @brief Helper function to escape CSV fields
  */
 static char* escape_csv(const char* str) {
     if (!str) return NULL;
     
     // Calculate escaped length
     size_t len = 0;
     for (const char* p = str; *p; p++) {
         if (*p == '"' || *p == ',') len++;
         len++;
     }
     
     // Allocate buffer
     char* result = malloc(len + 3); // +2 for quotes, +1 for null terminator
     if (!result) return NULL;
     
     // Write escaped string
     char* q = result;
     *q++ = '"';
     for (const char* p = str; *p; p++) {
         if (*p == '"') *q++ = '"'; // Double quotes
         *q++ = *p;
     }
     *q++ = '"';
     *q = '\0';
     
     return result;
 }
 
 /**
  * @brief Export events to CSV format
  */
 static char* export_events_to_csv(polycall_core_context_t* core_ctx, audit_event_t** events, 
                                 size_t event_count) {
     if (!core_ctx || !events || event_count == 0) {
         return NULL;
     }
     
     // Calculate buffer size (rough estimate)
     size_t buffer_size = 256; // Initial size for CSV header
     for (size_t i = 0; i < event_count; i++) {
         buffer_size += 256; // Base size per event
         
         if (events[i]->identity_id) {
             buffer_size += strlen(events[i]->identity_id) * 2; // Account for CSV escaping
         }
         
         if (events[i]->resource) {
             buffer_size += strlen(events[i]->resource) * 2;
         }
         
         if (events[i]->action) {
             buffer_size += strlen(events[i]->action) * 2;
         }
         
         if (events[i]->error_message) {
             buffer_size += strlen(events[i]->error_message) * 2;
         }
         
         if (events[i]->source_ip) {
             buffer_size += strlen(events[i]->source_ip) * 2;
         }
         
         if (events[i]->user_agent) {
             buffer_size += strlen(events[i]->user_agent) * 2;
         }
         
         if (events[i]->details) {
             buffer_size += strlen(events[i]->details) * 2;
         }
     }
     
     // Allocate buffer
     char* buffer = polycall_core_malloc(core_ctx, buffer_size);
     if (!buffer) {
         return NULL;
     }
     
     // Write CSV header
     int pos = sprintf(buffer, "type,timestamp,identity_id,resource,action,success,error_message,source_ip,user_agent,details\n");
     
     // Write each event
     for (size_t i = 0; i < event_count; i++) {
         audit_event_t* event = events[i];
          
          // Write event type
          pos += sprintf(buffer + pos, "%s,", polycall_audit_event_type_to_string(event->type));
          
          // Write timestamp
          pos += sprintf(buffer + pos, "%llu,", (unsigned long long)event->timestamp);
          
          // Write identity ID
          if (event->identity_id) {
              char* escaped = escape_csv(event->identity_id);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s,", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\",");
              }
          } else {
              pos += sprintf(buffer + pos, ",");
          }
          
          // Write resource
          if (event->resource) {
              char* escaped = escape_csv(event->resource);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s,", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\",");
              }
          } else {
              pos += sprintf(buffer + pos, ",");
          }
          
          // Write action
          if (event->action) {
              char* escaped = escape_csv(event->action);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s,", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\",");
              }
          } else {
              pos += sprintf(buffer + pos, ",");
          }
          
          // Write success
          pos += sprintf(buffer + pos, "%s,", event->success ? "true" : "false");
          
          // Write error message
          if (event->error_message) {
              char* escaped = escape_csv(event->error_message);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s,", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\",");
              }
          } else {
              pos += sprintf(buffer + pos, ",");
          }
          
          // Write source IP
          if (event->source_ip) {
              char* escaped = escape_csv(event->source_ip);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s,", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\",");
              }
          } else {
              pos += sprintf(buffer + pos, ",");
          }
          
          // Write user agent
          if (event->user_agent) {
              char* escaped = escape_csv(event->user_agent);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s,", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\",");
              }
          } else {
              pos += sprintf(buffer + pos, ",");
          }
          
          // Write details
          if (event->details) {
              char* escaped = escape_csv(event->details);
              if (escaped) {
                  pos += sprintf(buffer + pos, "%s", escaped);
                  free(escaped);
              } else {
                  pos += sprintf(buffer + pos, "\"\"");
              }
          }
          
          // End line
          pos += sprintf(buffer + pos, "\n");
      }
      
      return buffer;
 }