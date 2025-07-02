/**
 * @file polycall_auth_audit.h
 * @brief Audit logging for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the audit logging interfaces for LibPolyCall authentication.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_AUDIT_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_AUDIT_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
/* 
 * Using polycall_audit_event_type_t from polycall_auth_context.h
 * No need to redefine here
 */
 
/* 
 * Use types from polycall_auth_context.h to avoid type conflicts
 */

/**
 * Forward reference to audit_event_t from polycall_auth_context.h
 */
typedef struct audit_event audit_event_t;

/**
 * Forward reference to audit_query_t from polycall_auth_context.h
 */
typedef struct audit_query audit_query_t;
/**
 * @brief Audit event structure
 */
typedef struct audit_event {
    uint64_t timestamp;                      // Event timestamp
    polycall_audit_event_type_t type;        // Event type
    char* identity_id;                       // Identity ID
    char* resource;                          // Resource accessed
    char* action;                            // Action performed
    bool success;                            // Whether the action succeeded
    char* error_message;                     // Error message if unsuccessful
    void* user_data;                         // User-defined data
    char* user_agent;                        // User agent information
    char* source_ip;                         // Source IP address
    char* details;                           // Additional details
    
} audit_event_t;

/**
 * @brief Audit query parameters
 */
typedef struct audit_query {
    uint64_t start_time;                     // Start timestamp for query range
    uint64_t end_time;                       // End timestamp for query range
    polycall_audit_event_type_t type;        // Event type to filter
    char* identity_id;                       // Identity ID to filter
    char* resource;                          // Resource to filter
    char* action;                            // Action to filter
    bool filter_by_success;                  // Whether to filter by success
    bool success;                            // Success value to filter
} audit_query_t;
 /**
  * @brief Log an audit event
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param event Event to log
  * @return Error code
  */
 polycall_core_error_t polycall_auth_log_audit_event(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const audit_event_t* event
 );
 
 /**
  * @brief Query audit events
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param query Query parameters
  * @param events Returned events
  * @param event_count Number of returned events
  * @return Error code
  */
 polycall_core_error_t polycall_auth_query_audit_events(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const audit_query_t* query,
     audit_event_t*** events,
     size_t* event_count
 );
 
 /**
  * @brief Export audit events to a file
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param query Query parameters
  * @param filename File path
  * @param format Export format ("json", "csv")
  * @return Error code
  */
 polycall_core_error_t polycall_auth_export_audit_events(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const audit_query_t* query,
     const char* filename,
     const char* format
 );
 
 /**
  * @brief Create an audit event
  *
  * @param core_ctx Core context
  * @param type Event type
  * @param identity_id Identity ID
  * @param resource Resource
  * @param action Action
  * @param success Whether the event was successful
  * @param error_message Error message if unsuccessful
  * @return Audit event or NULL on failure
  */
 audit_event_t* polycall_auth_create_audit_event(
     polycall_core_context_t* core_ctx,
     polycall_audit_event_type_t type,
     const char* identity_id,
     const char* resource,
     const char* action,
     bool success,
     const char* error_message
 );
 
 /**
  * @brief Free an audit event
  *
  * @param core_ctx Core context
  * @param event Event to free
  */
 void polycall_auth_free_audit_event(
     polycall_core_context_t* core_ctx,
     audit_event_t* event
 );
 
 /**
  * @brief Free an array of audit events
  *
  * @param core_ctx Core context
  * @param events Events to free
  * @param event_count Number of events
  */
 void polycall_auth_free_audit_events(
     polycall_core_context_t* core_ctx,
     audit_event_t** events,
     size_t event_count
 );
 
 /**
  * @brief Get a string representation of an audit event type
  *
  * @param type Audit event type
  * @return String representation
  */
 const char* polycall_audit_event_type_to_string(polycall_audit_event_type_t type);
 
 /* Forward declarations for internal functions used in implementation files */
 #if defined(POLYCALL_INTERNAL_IMPLEMENTATION)
 static audit_entry_t* create_audit_entry(polycall_core_context_t* core_ctx, const audit_event_t* event);
 static void free_audit_entry(polycall_core_context_t* core_ctx, audit_entry_t* entry);
 static bool match_audit_event(const audit_entry_t* entry, const audit_query_t* query);
 static char* export_events_to_json(polycall_core_context_t* core_ctx, audit_event_t** events, 
                                   size_t event_count);
 static char* export_events_to_csv(polycall_core_context_t* core_ctx, audit_event_t** events, 
                                  size_t event_count);
 #endif
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_AUDIT_H_H */
#if defined(POLYCALL_INTERNAL_IMPLEMENTATION)
static audit_entry_t* create_audit_entry(polycall_core_context_t* core_ctx, const audit_event_t* event);
static void free_audit_entry(polycall_core_context_t* core_ctx, audit_entry_t* entry);
static bool match_audit_event(const audit_entry_t* entry, const audit_query_t* query);
static char* export_events_to_json(polycall_core_context_t* core_ctx, audit_event_t** events, 
                                   size_t event_count);
static char* export_events_to_csv(polycall_core_context_t* core_ctx, audit_event_t** events, 
                                  size_t event_count);
static uint64_t get_current_timestamp(void);
#endif
