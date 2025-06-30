/**
#include "polycall/core/ffi/security.h"

 * @file security.c
 * @brief Security layer implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the zero-trust security model for the LibPolyCall FFI,
 * providing access control, permission management, and audit logging for
 * cross-language function calls.
 */

 #include "polycall/core/ffi/security.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <time.h>
 

 /**
  * @brief Check if a security context is valid
  */
 static bool validate_security_context(security_context_t* ctx) {
     return ctx && ctx->magic == SECURITY_CONTEXT_MAGIC;
 }
 
 /**
  * @brief Get current timestamp
  */
 static uint64_t get_timestamp(void) {
     struct timespec ts;
     clock_gettime(CLOCK_REALTIME, &ts);
     return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
 }
 
 /**
  * @brief Initialize access control list
  */
 static polycall_core_error_t init_acl(
     polycall_core_context_t* ctx,
     access_control_list_t** acl,
     bool default_deny
 ) {
     *acl = polycall_core_malloc(ctx, sizeof(access_control_list_t));
     if (!*acl) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(*acl, 0, sizeof(access_control_list_t));
     (*acl)->default_deny = default_deny;
     
     if (pthread_mutex_init(&(*acl)->mutex, NULL) != 0) {
         polycall_core_free(ctx, *acl);
         *acl = NULL;
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up access control list
  */
 static void cleanup_acl(
     polycall_core_context_t* ctx,
     access_control_list_t* acl
 ) {
     if (!acl) {
         return;
     }
     
     pthread_mutex_destroy(&acl->mutex);
     polycall_core_free(ctx, acl);
 }
 
 /**
  * @brief Initialize permission registry
  */
 static polycall_core_error_t init_permission_registry(
     polycall_core_context_t* ctx,
     permission_registry_t** registry
 ) {
     *registry = polycall_core_malloc(ctx, sizeof(permission_registry_t));
     if (!*registry) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(*registry, 0, sizeof(permission_registry_t));
     
     if (pthread_mutex_init(&(*registry)->mutex, NULL) != 0) {
         polycall_core_free(ctx, *registry);
         *registry = NULL;
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Register standard permissions
     (*registry)->permissions[0].value = POLYCALL_PERM_EXECUTE;
     strcpy((*registry)->permissions[0].name, "execute");
     strcpy((*registry)->permissions[0].description, "Execute functions");
     
     (*registry)->permissions[1].value = POLYCALL_PERM_READ_MEMORY;
     strcpy((*registry)->permissions[1].name, "read_memory");
     strcpy((*registry)->permissions[1].description, "Read memory");
     
     (*registry)->permissions[2].value = POLYCALL_PERM_WRITE_MEMORY;
     strcpy((*registry)->permissions[2].name, "write_memory");
     strcpy((*registry)->permissions[2].description, "Write memory");
     
     (*registry)->permissions[3].value = POLYCALL_PERM_ALLOCATE_MEMORY;
     strcpy((*registry)->permissions[3].name, "allocate_memory");
     strcpy((*registry)->permissions[3].description, "Allocate memory");
     
     (*registry)->permissions[4].value = POLYCALL_PERM_SHARE_MEMORY;
     strcpy((*registry)->permissions[4].name, "share_memory");
     strcpy((*registry)->permissions[4].description, "Share memory");
     
     (*registry)->count = 5;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up permission registry
  */
 static void cleanup_permission_registry(
     polycall_core_context_t* ctx,
     permission_registry_t* registry
 ) {
     if (!registry) {
         return;
     }
     
     pthread_mutex_destroy(&registry->mutex);
     polycall_core_free(ctx, registry);
 }
 
 /**
  * @brief Initialize audit log
  */
 static polycall_core_error_t init_audit_log(
     polycall_core_context_t* ctx,
     audit_log_t** audit_log,
     const audit_policy_t* policy
 ) {
     *audit_log = polycall_core_malloc(ctx, sizeof(audit_log_t));
     if (!*audit_log) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(*audit_log, 0, sizeof(audit_log_t));
     
     if (policy) {
         (*audit_log)->policy = *policy;
     } else {
         // Default policy
         (*audit_log)->policy.level = POLYCALL_AUDIT_LEVEL_ERROR;
         (*audit_log)->policy.log_to_console = true;
         (*audit_log)->policy.log_to_file = false;
         (*audit_log)->policy.max_entries = MAX_AUDIT_ENTRIES;
     }
     
     // Open log file if needed
     if ((*audit_log)->policy.log_to_file && (*audit_log)->policy.log_file[0]) {
         (*audit_log)->log_file = fopen((*audit_log)->policy.log_file, "a");
         if (!(*audit_log)->log_file) {
             polycall_core_free(ctx, *audit_log);
             *audit_log = NULL;
             return POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED;
         }
     }
     
     if (pthread_mutex_init(&(*audit_log)->mutex, NULL) != 0) {
         if ((*audit_log)->log_file) {
             fclose((*audit_log)->log_file);
         }
         polycall_core_free(ctx, *audit_log);
         *audit_log = NULL;
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up audit log
  */
 static void cleanup_audit_log(
     polycall_core_context_t* ctx,
     audit_log_t* audit_log
 ) {
     if (!audit_log) {
         return;
     }
     
     pthread_mutex_destroy(&audit_log->mutex);
     
     if (audit_log->log_file) {
         fclose(audit_log->log_file);
     }
     
     // Free event details memory
     for (size_t i = 0; i < audit_log->count; i++) {
         size_t index = (audit_log->index + i) % MAX_AUDIT_ENTRIES;
         if (audit_log->events[index].details) {
             polycall_core_free(ctx, (void*)audit_log->events[index].details);
         }
     }
     
     polycall_core_free(ctx, audit_log);
 }
 
 /**
  * @brief Initialize security policy
  */
 static polycall_core_error_t init_security_policy(
     polycall_core_context_t* ctx,
     security_policy_t** policy,
     polycall_security_level_t security_level
 ) {
     *policy = polycall_core_malloc(ctx, sizeof(security_policy_t));
     if (!*policy) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(*policy, 0, sizeof(security_policy_t));
     
     // Configure based on security level
     (*policy)->security_level = security_level;
     
     switch (security_level) {
         case POLYCALL_SECURITY_LEVEL_NONE:
             (*policy)->isolation_level = POLYCALL_ISOLATION_LEVEL_NONE;
             (*policy)->enforce_call_validation = false;
             (*policy)->enforce_type_safety = false;
             (*policy)->enforce_memory_isolation = false;
             (*policy)->allow_dynamic_registration = true;
             break;
             
         case POLYCALL_SECURITY_LEVEL_LOW:
             (*policy)->isolation_level = POLYCALL_ISOLATION_LEVEL_SHARED;
             (*policy)->enforce_call_validation = true;
             (*policy)->enforce_type_safety = true;
             (*policy)->enforce_memory_isolation = false;
             (*policy)->allow_dynamic_registration = true;
             break;
             
         case POLYCALL_SECURITY_LEVEL_MEDIUM:
             (*policy)->isolation_level = POLYCALL_ISOLATION_LEVEL_FUNCTION;
             (*policy)->enforce_call_validation = true;
             (*policy)->enforce_type_safety = true;
             (*policy)->enforce_memory_isolation = true;
             (*policy)->allow_dynamic_registration = true;
             break;
             
         case POLYCALL_SECURITY_LEVEL_HIGH:
             (*policy)->isolation_level = POLYCALL_ISOLATION_LEVEL_MODULE;
             (*policy)->enforce_call_validation = true;
             (*policy)->enforce_type_safety = true;
             (*policy)->enforce_memory_isolation = true;
             (*policy)->allow_dynamic_registration = false;
             break;
             
         case POLYCALL_SECURITY_LEVEL_MAXIMUM:
             (*policy)->isolation_level = POLYCALL_ISOLATION_LEVEL_PROCESS;
             (*policy)->enforce_call_validation = true;
             (*policy)->enforce_type_safety = true;
             (*policy)->enforce_memory_isolation = true;
             (*policy)->allow_dynamic_registration = false;
             break;
             
         default:
             (*policy)->security_level = POLYCALL_SECURITY_LEVEL_MEDIUM;
             (*policy)->isolation_level = POLYCALL_ISOLATION_LEVEL_FUNCTION;
             (*policy)->enforce_call_validation = true;
             (*policy)->enforce_type_safety = true;
             (*policy)->enforce_memory_isolation = true;
             (*policy)->allow_dynamic_registration = true;
             break;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up security policy
  */
 static void cleanup_security_policy(
     polycall_core_context_t* ctx,
     security_policy_t* policy
 ) {
     if (!policy) {
         return;
     }
     
     polycall_core_free(ctx, policy);
 }
 
 /**
  * @brief Initialize isolation manager
  */
 static polycall_core_error_t init_isolation_manager(
     polycall_core_context_t* ctx,
     isolation_manager_t** isolation
 ) {
     *isolation = polycall_core_malloc(ctx, sizeof(isolation_manager_t));
     if (!*isolation) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(*isolation, 0, sizeof(isolation_manager_t));
     
     if (pthread_mutex_init(&(*isolation)->mutex, NULL) != 0) {
         polycall_core_free(ctx, *isolation);
         *isolation = NULL;
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up isolation manager
  */
 static void cleanup_isolation_manager(
     polycall_core_context_t* ctx,
     isolation_manager_t* isolation
 ) {
     if (!isolation) {
         return;
     }
     
     pthread_mutex_destroy(&isolation->mutex);
     polycall_core_free(ctx, isolation);
 }
 
 /**
  * @brief Log an audit event
  */
 static void log_audit_event(
     polycall_core_context_t* ctx,
     audit_log_t* audit_log,
     const audit_event_t* event
 ) {
     if (!audit_log || !event) {
         return;
     }
     
     pthread_mutex_lock(&audit_log->mutex);
     
     // Determine if event should be logged based on audit level
     bool should_log = false;
     
     switch (audit_log->policy.level) {
         case POLYCALL_AUDIT_LEVEL_NONE:
             should_log = false;
             break;
             
         case POLYCALL_AUDIT_LEVEL_ERROR:
             should_log = (event->result.allowed == false);
             break;
             
         case POLYCALL_AUDIT_LEVEL_WARNING:
             should_log = (event->result.allowed == false || 
                           strstr(event->action, "warning") != NULL);
             break;
             
         case POLYCALL_AUDIT_LEVEL_INFO:
         case POLYCALL_AUDIT_LEVEL_DEBUG:
         case POLYCALL_AUDIT_LEVEL_TRACE:
             should_log = true;
             break;
     }
     
     if (!should_log) {
         pthread_mutex_unlock(&audit_log->mutex);
         return;
     }
     
     // Add event to circular buffer
     size_t index = audit_log->index;
     
     // Free previous event details if it exists
     if (audit_log->events[index].details) {
         polycall_core_free(ctx, (void*)audit_log->events[index].details);
     }
     
     // Copy event data
     audit_log->events[index] = *event;
     
     // Make a copy of the details string
     if (event->details) {
         char* details_copy = polycall_core_malloc(ctx, strlen(event->details) + 1);
         if (details_copy) {
             strcpy(details_copy, event->details);
             audit_log->events[index].details = details_copy;
         } else {
             audit_log->events[index].details = NULL;
         }
     }
     
     // Update index and count
     audit_log->index = (audit_log->index + 1) % MAX_AUDIT_ENTRIES;
     if (audit_log->count < MAX_AUDIT_ENTRIES) {
         audit_log->count++;
     }
     
     // Log to console if enabled
     if (audit_log->policy.log_to_console) {
         fprintf(stderr, "[SECURITY] %s -> %s | %s | %s | %s\n",
                 event->source_language,
                 event->target_language,
                 event->function_name,
                 event->action,
                 event->result.allowed ? "ALLOWED" : "DENIED");
         
         if (event->details) {
             fprintf(stderr, "           Details: %s\n", event->details);
         }
     }
     
     // Log to file if enabled
     if (audit_log->policy.log_to_file && audit_log->log_file) {
         char timestamp_str[32];
         time_t now = time(NULL);
         struct tm* tm_info = localtime(&now);
         strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
         
         fprintf(audit_log->log_file, "[%s] %s -> %s | %s | %s | %s\n",
                 timestamp_str,
                 event->source_language,
                 event->target_language,
                 event->function_name,
                 event->action,
                 event->result.allowed ? "ALLOWED" : "DENIED");
         
         if (event->details) {
             fprintf(audit_log->log_file, "           Details: %s\n", event->details);
         }
         
         fflush(audit_log->log_file);
     }
     
     // Call callback if registered
     if (audit_log->callback) {
         pthread_mutex_unlock(&audit_log->mutex);
         audit_log->callback(ctx, event, audit_log->user_data);
         pthread_mutex_lock(&audit_log->mutex);
     }
     
     pthread_mutex_unlock(&audit_log->mutex);
 }
 
 /**
  * @brief Match a string against a pattern (simple wildcards)
  */
 static bool match_pattern(const char* pattern, const char* str) {
     // Empty pattern matches only empty string
     if (!pattern || !*pattern) {
         return !str || !*str;
     }
     
     // Wildcard matches everything
     if (strcmp(pattern, "*") == 0) {
         return true;
     }
     
     // Exact match
     return strcmp(pattern, str) == 0;
 }
 
 /**
  * @brief Check if function access is allowed
  */
 static bool check_function_access(
     access_control_list_t* acl,
     const char* function_name,
     const char* source_language,
     const char* source_context,
     permission_set_t* required_permissions,
     char* error_message,
     size_t message_size
 ) {
     if (!acl || !function_name || !source_language) {
         if (error_message && message_size > 0) {
             strncpy(error_message, "Invalid parameters for access check", message_size - 1);
             error_message[message_size - 1] = '\0';
         }
         return false;
     }
     
     pthread_mutex_lock(&acl->mutex);
     
     // Check ACL entries for matches
     bool found_match = false;
     bool access_allowed = false;
     
     for (size_t i = 0; i < acl->count; i++) {
         acl_entry_t* entry = &acl->entries[i];
         
         // Skip disabled entries
         if (!entry->enabled) {
             continue;
         }
         
         // Check if entry matches
         if (match_pattern(entry->function_id, function_name) &&
             match_pattern(entry->caller_language, source_language) &&
             (source_context == NULL || match_pattern(entry->caller_context, source_context))) {
             
             // Found matching entry
             found_match = true;
             
             // Set required permissions
             if (required_permissions) {
                 *required_permissions = entry->required_permissions;
             }
             
             // This entry allows access
             access_allowed = true;
             break;
         }
     }
     
     // If no matching entry was found, apply default policy
     if (!found_match) {
         if (acl->default_deny) {
             if (error_message && message_size > 0) {
                 snprintf(error_message, message_size - 1, 
                          "No matching ACL entry for %s called by %s, default deny policy applied",
                          function_name, source_language);
                 error_message[message_size - 1] = '\0';
             }
             pthread_mutex_unlock(&acl->mutex);
             return false;
         } else {
             // Default allow
             if (required_permissions) {
                 *required_permissions = POLYCALL_PERM_EXECUTE;
             }
             pthread_mutex_unlock(&acl->mutex);
             return true;
         }
     }
     
     pthread_mutex_unlock(&acl->mutex);
     
     if (!access_allowed && error_message && message_size > 0) {
         snprintf(error_message, message_size - 1, 
                  "Access denied for %s called by %s",
                  function_name, source_language);
         error_message[message_size - 1] = '\0';
     }
     
     return access_allowed;
 }
 
 /**
  * @brief Initialize security context
  */
 polycall_core_error_t polycall_security_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t** security_ctx,
     const security_config_t* config
 ) {
     if (!ctx || !security_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate security context
     security_context_t* new_ctx = polycall_core_malloc(ctx, sizeof(security_context_t));
     if (!new_ctx) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate security context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(security_context_t));
     new_ctx->magic = SECURITY_CONTEXT_MAGIC;
     new_ctx->core_ctx = ctx;
     
     // Use provided configuration or defaults
     polycall_security_level_t security_level = POLYCALL_SECURITY_LEVEL_MEDIUM;
     bool default_deny = true;
     audit_policy_t audit_policy = {0};
     
     if (config) {
         security_level = config->security_level;
         default_deny = config->default_deny;
         audit_policy.level = config->audit_level;
         audit_policy.max_entries = MAX_AUDIT_ENTRIES;
     }
     
     // Initialize ACL
     polycall_core_error_t result = init_acl(ctx, &new_ctx->acl, default_deny);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, new_ctx);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize access control list");
         return result;
     }
     
     // Initialize permission registry
     result = init_permission_registry(ctx, &new_ctx->permissions);
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_acl(ctx, new_ctx->acl);
         polycall_core_free(ctx, new_ctx);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize permission registry");
         return result;
     }
     
     // Initialize audit log
     result = init_audit_log(ctx, &new_ctx->audit_log, &audit_policy);
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_permission_registry(ctx, new_ctx->permissions);
         cleanup_acl(ctx, new_ctx->acl);
         polycall_core_free(ctx, new_ctx);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize audit log");
         return result;
     }
     
     // Initialize security policy
     result = init_security_policy(ctx, &new_ctx->policy, security_level);
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_audit_log(ctx, new_ctx->audit_log);
         cleanup_permission_registry(ctx, new_ctx->permissions);
         cleanup_acl(ctx, new_ctx->acl);
         polycall_core_free(ctx, new_ctx);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize security policy");
         return result;
     }
     
     // Initialize isolation manager
     result = init_isolation_manager(ctx, &new_ctx->isolation);
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_security_policy(ctx, new_ctx->policy);
         cleanup_audit_log(ctx, new_ctx->audit_log);
         cleanup_permission_registry(ctx, new_ctx->permissions);
         cleanup_acl(ctx, new_ctx->acl);
         polycall_core_free(ctx, new_ctx);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize isolation manager");
         return result;
     }
     
     // Load policy file if specified
     if (config && config->policy_file && config->policy_file[0]) {
         result = polycall_security_load_policy(ctx, ffi_ctx, new_ctx, config->policy_file);
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               result,
                               POLYCALL_ERROR_SEVERITY_WARNING, 
                               "Failed to load security policy file: %s", config->policy_file);
             // Continue without policy file
         }
     }
     
     *security_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up security context
  */
 void polycall_security_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx
 ) {
     if (!ctx || !validate_security_context(security_ctx)) {
         return;
     }
     
     // Clean up components in reverse order
     cleanup_isolation_manager(ctx, security_ctx->isolation);
     cleanup_security_policy(ctx, security_ctx->policy);
     cleanup_audit_log(ctx, security_ctx->audit_log);
     cleanup_permission_registry(ctx, security_ctx->permissions);
     cleanup_acl(ctx, security_ctx->acl);
     
     // Clear magic number and free context
     security_ctx->magic = 0;
     polycall_core_free(ctx, security_ctx);
 }
 
 /**
  * @brief Verify function access
  */
 polycall_core_error_t polycall_security_verify_access(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* function_name,
     const char* source_language,
     const char* source_context,
     security_result_t* result
 ) {
     if (!ctx || !validate_security_context(security_ctx) || 
         !function_name || !source_language || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize result
     memset(result, 0, sizeof(security_result_t));
     
     // Skip validation if security policy doesn't require it
     if (!security_ctx->policy->enforce_call_validation) {
         result->allowed = true;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Check function access permission
     permission_set_t required_permissions = 0;
     char error_message[MAX_ERROR_MSG_LEN] = {0};
     
     bool access_allowed = check_function_access(
         security_ctx->acl,
         function_name,
         source_language,
         source_context,
         &required_permissions,
         error_message,
         sizeof(error_message)
     );
     
     // Set result
     result->allowed = access_allowed;
     result->missing_permissions = required_permissions;
     
     if (!access_allowed && error_message[0]) {
         strncpy(result->error_message, error_message, sizeof(result->error_message) - 1);
         result->error_message[sizeof(result->error_message) - 1] = '\0';
     }
     
     // Log audit event
     audit_event_t event = {0};
     event.timestamp = get_timestamp();
     event.source_language = source_language;
     event.target_language = "";  // Unknown at this point
     event.function_name = function_name;
     event.action = "access_check";
     event.result = *result;
     event.details = error_message[0] ? error_message : NULL;
     
     log_audit_event(ctx, security_ctx->audit_log, &event);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register a function with security attributes
  */
 polycall_core_error_t polycall_security_register_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     security_context_t* security_ctx,
     const char* function_name,
     const char* source_language,
     permission_set_t required_permissions,
     polycall_isolation_level_t isolation_level
 ) {
     if (!ctx || !validate_security_context(security_ctx) || 
         !function_name || !source_language) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if dynamic registration is allowed
     if (!security_ctx->policy->allow_dynamic_registration) {

    
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                               POLYCALL_ERROR_SEVERITY_WARNING, 
                               "Dynamic function registration is disabled by security policy");
             return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
         }
         
         pthread_mutex_lock(&security_ctx->acl->mutex);
         
         // Check if we have room for a new entry
         if (security_ctx->acl->count >= MAX_ACL_ENTRIES) {
             pthread_mutex_unlock(&security_ctx->acl->mutex);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "ACL capacity exceeded");
             return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
         }
         
         // Check if function already registered
         for (size_t i = 0; i < security_ctx->acl->count; i++) {
             if (strcmp(security_ctx->acl->entries[i].function_id, function_name) == 0 &&
                 strcmp(security_ctx->acl->entries[i].caller_language, source_language) == 0) {