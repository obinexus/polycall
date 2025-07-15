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
#include "polycall/core/auth/auth_policy.h"

 * @file auth_policy.c
 * @brief Implementation of policy management for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the policy management interfaces for LibPolyCall authentication,
 * providing functions to manage roles, policies, and permissions.
 */

 #include "polycall/core/auth/polycall_auth_policy.h"

 
 /* Define missing error code */
 #ifndef POLYCALL_CORE_ERROR_ALREADY_EXISTS
 #define POLYCALL_CORE_ERROR_ALREADY_EXISTS (POLYCALL_CORE_ERROR_BASE + 7)
 #endif
 
 /**
  * @brief Add a role
  */
 polycall_core_error_t polycall_auth_add_role(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const role_t* role
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !role || !role->name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock policy manager
     pthread_mutex_lock(&auth_ctx->policies->mutex);
     
     // Check if role already exists
     for (size_t i = 0; i < auth_ctx->policies->role_count; i++) {
         if (strcmp(auth_ctx->policies->roles[i]->name, role->name) == 0) {
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_ALREADY_EXISTS;
         }
     }
     
     // Check if we need to expand the role array
     if (auth_ctx->policies->role_count >= auth_ctx->policies->role_capacity) {
         size_t new_capacity = auth_ctx->policies->role_capacity * 2;
         role_entry_t** new_roles = polycall_core_malloc(core_ctx, new_capacity * sizeof(role_entry_t*));
         if (!new_roles) {
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_roles, auth_ctx->policies->roles, 
                auth_ctx->policies->role_count * sizeof(role_entry_t*));
         polycall_core_free(core_ctx, auth_ctx->policies->roles);
         auth_ctx->policies->roles = new_roles;
         auth_ctx->policies->role_capacity = new_capacity;
     }
     
     // Create a new role entry
     role_entry_t* new_role = polycall_core_malloc(core_ctx, sizeof(role_entry_t));
     if (!new_role) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(new_role, 0, sizeof(role_entry_t));
     
     // Copy role name
     size_t name_len = strlen(role->name) + 1;
     new_role->name = polycall_core_malloc(core_ctx, name_len);
     if (!new_role->name) {
         polycall_core_free(core_ctx, new_role);
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(new_role->name, role->name, name_len);
     
     // Copy role description if provided
     if (role->description) {
         size_t desc_len = strlen(role->description) + 1;
         new_role->description = polycall_core_malloc(core_ctx, desc_len);
         if (!new_role->description) {
             polycall_core_free(core_ctx, new_role->name);
             polycall_core_free(core_ctx, new_role);
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(new_role->description, role->description, desc_len);
     }
     
     // Initialize policy arrays
     new_role->policy_names = NULL;
     new_role->policy_count = 0;
     
     // Add to role array
     auth_ctx->policies->roles[auth_ctx->policies->role_count++] = new_role;
     
     pthread_mutex_unlock(&auth_ctx->policies->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_ROLE_ASSIGN,
         auth_ctx->current_identity,
         NULL,
         "create_role",
         true,
         NULL
     );
     
     if (event) {
         // Add role name to details
         char details[256] = {0};
         snprintf(details, sizeof(details), "{\"role_name\":\"%s\"}", role->name);
         event->details = strdup(details);
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Assign a role to an identity
  */
 polycall_core_error_t polycall_auth_assign_role(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* role_name
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !role_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock policy manager to check if role exists
     pthread_mutex_lock(&auth_ctx->policies->mutex);
     
     // Check if role exists
     bool role_exists = false;
     for (size_t i = 0; i < auth_ctx->policies->role_count; i++) {
         if (strcmp(auth_ctx->policies->roles[i]->name, role_name) == 0) {
             role_exists = true;
             break;
         }
     }
     
     pthread_mutex_unlock(&auth_ctx->policies->mutex);
     
     if (!role_exists) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Lock identity registry
     pthread_mutex_lock(&auth_ctx->identities->mutex);
     
     // Find identity
     int identity_index = -1;
     for (size_t i = 0; i < auth_ctx->identities->count; i++) {
         if (strcmp(auth_ctx->identities->identity_ids[i], identity_id) == 0) {
             identity_index = (int)i;
             break;
         }
     }
     
     // Identity not found
     if (identity_index == -1) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get attributes
     identity_attributes_t* attrs = auth_ctx->identities->attributes[identity_index];
     
     // Check if role is already assigned
     for (size_t i = 0; i < attrs->role_count; i++) {
         if (attrs->roles[i] && strcmp(attrs->roles[i], role_name) == 0) {
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_SUCCESS; // Role already assigned
         }
     }
     
     // Add role to identity
     char** new_roles = polycall_core_malloc(core_ctx, (attrs->role_count + 1) * sizeof(char*));
     if (!new_roles) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Copy existing roles
     for (size_t i = 0; i < attrs->role_count; i++) {
         new_roles[i] = attrs->roles[i];
     }
     
     // Add new role
     size_t role_len = strlen(role_name) + 1;
     new_roles[attrs->role_count] = polycall_core_malloc(core_ctx, role_len);
     if (!new_roles[attrs->role_count]) {
         polycall_core_free(core_ctx, new_roles);
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(new_roles[attrs->role_count], role_name, role_len);
     
     // Update identity attributes
     char** old_roles = attrs->roles;
     attrs->roles = new_roles;
     attrs->role_count++;
     
     // Free old role array (but not the strings, as they were copied by reference)
     if (old_roles) {
         polycall_core_free(core_ctx, old_roles);
     }
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_ROLE_ASSIGN,
         identity_id,
         NULL,
         "assign_role",
         true,
         NULL
     );
     
     if (event) {
         // Add role name to details
         char details[256] = {0};
         snprintf(details, sizeof(details), "{\"role_name\":\"%s\"}", role_name);
         event->details = strdup(details);
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Remove a role from an identity
  */
 polycall_core_error_t polycall_auth_remove_role(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* role_name
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !role_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock identity registry
     pthread_mutex_lock(&auth_ctx->identities->mutex);
     
     // Find identity
     int identity_index = -1;
     for (size_t i = 0; i < auth_ctx->identities->count; i++) {
         if (strcmp(auth_ctx->identities->identity_ids[i], identity_id) == 0) {
             identity_index = (int)i;
             break;
         }
     }
     
     // Identity not found
     if (identity_index == -1) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get attributes
     identity_attributes_t* attrs = auth_ctx->identities->attributes[identity_index];
     
     // Find role index
     int role_index = -1;
     for (size_t i = 0; i < attrs->role_count; i++) {
         if (attrs->roles[i] && strcmp(attrs->roles[i], role_name) == 0) {
             role_index = (int)i;
             break;
         }
     }
     
     // Role not found
     if (role_index == -1) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Remove role
     polycall_core_free(core_ctx, attrs->roles[role_index]);
     
     // Shift remaining roles
     for (size_t i = role_index; i < attrs->role_count - 1; i++) {
         attrs->roles[i] = attrs->roles[i + 1];
     }
     
     attrs->role_count--;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_ROLE_REMOVE,
         identity_id,
         NULL,
         "remove_role",
         true,
         NULL
     );
     
     if (event) {
         // Add role name to details
         char details[256] = {0};
         snprintf(details, sizeof(details), "{\"role_name\":\"%s\"}", role_name);
         event->details = strdup(details);
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Add a policy
  */
 polycall_core_error_t polycall_auth_add_policy(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const policy_t* policy
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !policy || !policy->name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock policy manager
     pthread_mutex_lock(&auth_ctx->policies->mutex);
     
     // Check if policy already exists
     for (size_t i = 0; i < auth_ctx->policies->policy_count; i++) {
         if (strcmp(auth_ctx->policies->policies[i]->name, policy->name) == 0) {
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_ALREADY_EXISTS;
         }
     }
     
     // Check if we need to expand the policy array
     if (auth_ctx->policies->policy_count >= auth_ctx->policies->policy_capacity) {
         size_t new_capacity = auth_ctx->policies->policy_capacity * 2;
         policy_entry_t** new_policies = polycall_core_malloc(core_ctx, new_capacity * sizeof(policy_entry_t*));
         if (!new_policies) {
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_policies, auth_ctx->policies->policies, 
                auth_ctx->policies->policy_count * sizeof(policy_entry_t*));
         polycall_core_free(core_ctx, auth_ctx->policies->policies);
         auth_ctx->policies->policies = new_policies;
         auth_ctx->policies->policy_capacity = new_capacity;
     }
     
     // Create a new policy entry
     policy_entry_t* new_policy = polycall_core_malloc(core_ctx, sizeof(policy_entry_t));
     if (!new_policy) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memset(new_policy, 0, sizeof(policy_entry_t));
     
     // Copy policy name
     size_t name_len = strlen(policy->name) + 1;
     new_policy->name = polycall_core_malloc(core_ctx, name_len);
     if (!new_policy->name) {
         polycall_core_free(core_ctx, new_policy);
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(new_policy->name, policy->name, name_len);
     
     // Copy policy description if provided
     if (policy->description) {
         size_t desc_len = strlen(policy->description) + 1;
         new_policy->description = polycall_core_malloc(core_ctx, desc_len);
         if (!new_policy->description) {
             polycall_core_free(core_ctx, new_policy->name);
             polycall_core_free(core_ctx, new_policy);
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(new_policy->description, policy->description, desc_len);
     }
     
     // Copy statements if provided
     if (policy->statements && policy->statement_count > 0) {
         new_policy->statements = polycall_core_malloc(core_ctx, 
                                                     policy->statement_count * sizeof(policy_statement_t*));
         if (!new_policy->statements) {
             if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
             polycall_core_free(core_ctx, new_policy->name);
             polycall_core_free(core_ctx, new_policy);
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         new_policy->statement_count = policy->statement_count;
         
         for (size_t i = 0; i < policy->statement_count; i++) {
             policy_statement_t* src_stmt = policy->statements[i];
             if (!src_stmt) {
                 new_policy->statements[i] = NULL;
                 continue;
             }
             
             policy_statement_t* new_stmt = polycall_core_malloc(core_ctx, sizeof(policy_statement_t));
             if (!new_stmt) {
                 for (size_t j = 0; j < i; j++) {
                     if (new_policy->statements[j]) {
                         // Clean up each statement
                         if (new_policy->statements[j]->actions) {
                             for (size_t k = 0; k < new_policy->statements[j]->action_count; k++) {
                                 if (new_policy->statements[j]->actions[k]) {
                                     polycall_core_free(core_ctx, new_policy->statements[j]->actions[k]);
                                 }
                             }
                             polycall_core_free(core_ctx, new_policy->statements[j]->actions);
                         }
                         
                         if (new_policy->statements[j]->resources) {
                             for (size_t k = 0; k < new_policy->statements[j]->resource_count; k++) {
                                 if (new_policy->statements[j]->resources[k]) {
                                     polycall_core_free(core_ctx, new_policy->statements[j]->resources[k]);
                                 }
                             }
                             polycall_core_free(core_ctx, new_policy->statements[j]->resources);
                         }
                         
                         if (new_policy->statements[j]->condition) {
                             polycall_core_free(core_ctx, new_policy->statements[j]->condition);
                         }
                         
                         polycall_core_free(core_ctx, new_policy->statements[j]);
                     }
                 }
                 
                 polycall_core_free(core_ctx, new_policy->statements);
                 if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
                 polycall_core_free(core_ctx, new_policy->name);
                 polycall_core_free(core_ctx, new_policy);
                 pthread_mutex_unlock(&auth_ctx->policies->mutex);
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             memset(new_stmt, 0, sizeof(policy_statement_t));
             
             // Copy effect
             new_stmt->effect = src_stmt->effect;
             
             // Copy actions
             if (src_stmt->actions && src_stmt->action_count > 0) {
                 new_stmt->actions = polycall_core_malloc(core_ctx, src_stmt->action_count * sizeof(char*));
                 if (!new_stmt->actions) {
                     polycall_core_free(core_ctx, new_stmt);
                     
                     for (size_t j = 0; j < i; j++) {
                         if (new_policy->statements[j]) {
                             // Clean up each statement
                             if (new_policy->statements[j]->actions) {
                                 for (size_t k = 0; k < new_policy->statements[j]->action_count; k++) {
                                     if (new_policy->statements[j]->actions[k]) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->actions[k]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_policy->statements[j]->actions);
                             }
                             
                             if (new_policy->statements[j]->resources) {
                                 for (size_t k = 0; k < new_policy->statements[j]->resource_count; k++) {
                                     if (new_policy->statements[j]->resources[k]) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->resources[k]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_policy->statements[j]->resources);
                             }
                             
                             if (new_policy->statements[j]->condition) {
                                 polycall_core_free(core_ctx, new_policy->statements[j]->condition);
                             }
                             
                             polycall_core_free(core_ctx, new_policy->statements[j]);
                         }
                     }
                     
                     polycall_core_free(core_ctx, new_policy->statements);
                     if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
                     polycall_core_free(core_ctx, new_policy->name);
                     polycall_core_free(core_ctx, new_policy);
                     pthread_mutex_unlock(&auth_ctx->policies->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 new_stmt->action_count = src_stmt->action_count;
                 
                 for (size_t k = 0; k < src_stmt->action_count; k++) {
                     if (src_stmt->actions[k]) {
                         size_t action_len = strlen(src_stmt->actions[k]) + 1;
                         new_stmt->actions[k] = polycall_core_malloc(core_ctx, action_len);
                         if (!new_stmt->actions[k]) {
                             // Clean up on failure
                             for (size_t l = 0; l < k; l++) {
                                 if (new_stmt->actions[l]) {
                                     polycall_core_free(core_ctx, new_stmt->actions[l]);
                                 }
                             }
                             polycall_core_free(core_ctx, new_stmt->actions);
                             polycall_core_free(core_ctx, new_stmt);
                             
                             // Clean up previous statements
                             for (size_t j = 0; j < i; j++) {
                                 if (new_policy->statements[j]) {
                                     if (new_policy->statements[j]->actions) {
                                         for (size_t m = 0; m < new_policy->statements[j]->action_count; m++) {
                                             if (new_policy->statements[j]->actions[m]) {
                                                 polycall_core_free(core_ctx, new_policy->statements[j]->actions[m]);
                                             }
                                         }
                                         polycall_core_free(core_ctx, new_policy->statements[j]->actions);
                                     }
                                     
                                     if (new_policy->statements[j]->resources) {
                                         for (size_t m = 0; m < new_policy->statements[j]->resource_count; m++) {
                                             if (new_policy->statements[j]->resources[m]) {
                                                 polycall_core_free(core_ctx, new_policy->statements[j]->resources[m]);
                                             }
                                         }
                                         polycall_core_free(core_ctx, new_policy->statements[j]->resources);
                                     }
                                     
                                     if (new_policy->statements[j]->condition) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->condition);
                                     }
                                     
                                     polycall_core_free(core_ctx, new_policy->statements[j]);
                                 }
                             }
                             
                             polycall_core_free(core_ctx, new_policy->statements);
                             if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
                             polycall_core_free(core_ctx, new_policy->name);
                             polycall_core_free(core_ctx, new_policy);
                             pthread_mutex_unlock(&auth_ctx->policies->mutex);
                             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                         }
                         
                         memcpy(new_stmt->actions[k], src_stmt->actions[k], action_len);
                     } else {
                         new_stmt->actions[k] = NULL;
                     }
                 }
             }
             
             // Copy resources
             if (src_stmt->resources && src_stmt->resource_count > 0) {
                 new_stmt->resources = polycall_core_malloc(core_ctx, src_stmt->resource_count * sizeof(char*));
                 if (!new_stmt->resources) {
                     // Clean up on failure
                     if (new_stmt->actions) {
                         for (size_t k = 0; k < new_stmt->action_count; k++) {
                             if (new_stmt->actions[k]) {
                                 polycall_core_free(core_ctx, new_stmt->actions[k]);
                             }
                         }
                         polycall_core_free(core_ctx, new_stmt->actions);
                     }
                     polycall_core_free(core_ctx, new_stmt);
                     
                     // Clean up previous statements
                     for (size_t j = 0; j < i; j++) {
                         if (new_policy->statements[j]) {
                             if (new_policy->statements[j]->actions) {
                                 for (size_t k = 0; k < new_policy->statements[j]->action_count; k++) {
                                     if (new_policy->statements[j]->actions[k]) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->actions[k]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_policy->statements[j]->actions);
                             }
                             
                             if (new_policy->statements[j]->resources) {
                                 for (size_t k = 0; k < new_policy->statements[j]->resource_count; k++) {
                                     if (new_policy->statements[j]->resources[k]) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->resources[k]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_policy->statements[j]->resources);
                             }
                             
                             if (new_policy->statements[j]->condition) {
                                 polycall_core_free(core_ctx, new_policy->statements[j]->condition);
                             }
                             
                             polycall_core_free(core_ctx, new_policy->statements[j]);
                         }
                     }
                     
                     polycall_core_free(core_ctx, new_policy->statements);
                     if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
                     polycall_core_free(core_ctx, new_policy->name);
                     polycall_core_free(core_ctx, new_policy);
                     pthread_mutex_unlock(&auth_ctx->policies->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 new_stmt->resource_count = src_stmt->resource_count;
                 
                 for (size_t k = 0; k < src_stmt->resource_count; k++) {
                     if (src_stmt->resources[k]) {
                         size_t resource_len = strlen(src_stmt->resources[k]) + 1;
                         new_stmt->resources[k] = polycall_core_malloc(core_ctx, resource_len);
                         if (!new_stmt->resources[k]) {
                             // Clean up on failure
                             for (size_t l = 0; l < k; l++) {
                                 if (new_stmt->resources[l]) {
                                     polycall_core_free(core_ctx, new_stmt->resources[l]);
                                 }
                             }
                             polycall_core_free(core_ctx, new_stmt->resources);
                             
                             if (new_stmt->actions) {
                                 for (size_t l = 0; l < new_stmt->action_count; l++) {
                                     if (new_stmt->actions[l]) {
                                         polycall_core_free(core_ctx, new_stmt->actions[l]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_stmt->actions);
                             }
                             polycall_core_free(core_ctx, new_stmt);
                             
                             // Clean up previous statements
                             for (size_t j = 0; j < i; j++) {
                                 if (new_policy->statements[j]) {
                                     if (new_policy->statements[j]->actions) {
                                         for (size_t m = 0; m < new_policy->statements[j]->action_count; m++) {
                                             if (new_policy->statements[j]->actions[m]) {
                                                 polycall_core_free(core_ctx, new_policy->statements[j]->actions[m]);
                                             }
                                         }
                                         polycall_core_free(core_ctx, new_policy->statements[j]->actions);
                                     }
                                     
                                     if (new_policy->statements[j]->resources) {
                                         for (size_t m = 0; m < new_policy->statements[j]->resource_count; m++) {
                                             if (new_policy->statements[j]->resources[m]) {
                                                 polycall_core_free(core_ctx, new_policy->statements[j]->resources[m]);
                                             }
                                         }
                                         polycall_core_free(core_ctx, new_policy->statements[j]->resources);
                                     }
                                     
                                     if (new_policy->statements[j]->condition) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->condition);
                                     }
                                     
                                     polycall_core_free(core_ctx, new_policy->statements[j]);
                                 }
                             }
                             
                             polycall_core_free(core_ctx, new_policy->statements);
                             if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
                             polycall_core_free(core_ctx, new_policy->name);
                             polycall_core_free(core_ctx, new_policy);
                             pthread_mutex_unlock(&auth_ctx->policies->mutex);
                             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                         }
                         
                         memcpy(new_stmt->resources[k], src_stmt->resources[k], resource_len);
                     } else {
                         new_stmt->resources[k] = NULL;
                     }
                 }
             }
             
             // Copy condition if present
             if (src_stmt->condition) {
                 size_t condition_len = strlen(src_stmt->condition) + 1;
                 new_stmt->condition = polycall_core_malloc(core_ctx, condition_len);
                 if (!new_stmt->condition) {
                     // Clean up on failure
                     if (new_stmt->resources) {
                         for (size_t k = 0; k < new_stmt->resource_count; k++) {
                             if (new_stmt->resources[k]) {
                                 polycall_core_free(core_ctx, new_stmt->resources[k]);
                             }
                         }
                         polycall_core_free(core_ctx, new_stmt->resources);
                     }
                     
                     if (new_stmt->actions) {
                         for (size_t k = 0; k < new_stmt->action_count; k++) {
                             if (new_stmt->actions[k]) {
                                 polycall_core_free(core_ctx, new_stmt->actions[k]);
                             }
                         }
                         polycall_core_free(core_ctx, new_stmt->actions);
                     }
                     polycall_core_free(core_ctx, new_stmt);
                     
                     // Clean up previous statements
                     for (size_t j = 0; j < i; j++) {
                         if (new_policy->statements[j]) {
                             if (new_policy->statements[j]->actions) {
                                 for (size_t k = 0; k < new_policy->statements[j]->action_count; k++) {
                                     if (new_policy->statements[j]->actions[k]) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->actions[k]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_policy->statements[j]->actions);
                             }
                             
                             if (new_policy->statements[j]->resources) {
                                 for (size_t k = 0; k < new_policy->statements[j]->resource_count; k++) {
                                     if (new_policy->statements[j]->resources[k]) {
                                         polycall_core_free(core_ctx, new_policy->statements[j]->resources[k]);
                                     }
                                 }
                                 polycall_core_free(core_ctx, new_policy->statements[j]->resources);
                             }
                             
                             if (new_policy->statements[j]->condition) {
                                 polycall_core_free(core_ctx, new_policy->statements[j]->condition);
                             }
                             
                             polycall_core_free(core_ctx, new_policy->statements[j]);
                         }
                     }
                     
                     polycall_core_free(core_ctx, new_policy->statements);
                     if (new_policy->description) polycall_core_free(core_ctx, new_policy->description);
                     polycall_core_free(core_ctx, new_policy->name);
                     polycall_core_free(core_ctx, new_policy);
                     pthread_mutex_unlock(&auth_ctx->policies->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(new_stmt->condition, src_stmt->condition, condition_len);
             }
             
             // Add statement to policy
             new_policy->statements[i] = new_stmt;
         }
     }
     
     // Add policy to registry
     auth_ctx->policies->policies[auth_ctx->policies->policy_count++] = new_policy;
     
     pthread_mutex_unlock(&auth_ctx->policies->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_POLICY_CREATE,
         auth_ctx->current_identity,
         NULL,
         NULL,
         true,
         NULL
     );
     
     if (event) {
         // Add policy name to details
         char details[256] = {0};
         snprintf(details, sizeof(details), "{\"policy_name\":\"%s\"}", policy->name);
         event->details = strdup(details);
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Attach a policy to a role
  */
 polycall_core_error_t polycall_auth_attach_policy(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* role_name,
     const char* policy_name
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !role_name || !policy_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock policy manager
     pthread_mutex_lock(&auth_ctx->policies->mutex);
     
     // Find role
     int role_index = -1;
     for (size_t i = 0; i < auth_ctx->policies->role_count; i++) {
         if (strcmp(auth_ctx->policies->roles[i]->name, role_name) == 0) {
             role_index = (int)i;
             break;
         }
     }
     
     // Role not found
     if (role_index == -1) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find policy
     bool policy_exists = false;
     for (size_t i = 0; i < auth_ctx->policies->policy_count; i++) {
         if (strcmp(auth_ctx->policies->policies[i]->name, policy_name) == 0) {
             policy_exists = true;
             break;
         }
     }
     
     // Policy not found
     if (!policy_exists) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get role entry
     role_entry_t* role = auth_ctx->policies->roles[role_index];
     
     // Check if policy is already attached
     for (size_t i = 0; i < role->policy_count; i++) {
         if (role->policy_names[i] && strcmp(role->policy_names[i], policy_name) == 0) {
             pthread_mutex_unlock(&auth_ctx->policies->mutex);
             return POLYCALL_CORE_SUCCESS; // Policy already attached
         }
     }
     
     // Add policy to role
     char** new_policies = polycall_core_malloc(core_ctx, (role->policy_count + 1) * sizeof(char*));
     if (!new_policies) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Copy existing policies
     for (size_t i = 0; i < role->policy_count; i++) {
         new_policies[i] = role->policy_names[i];
     }
     
     // Add new policy
     size_t policy_len = strlen(policy_name) + 1;
     new_policies[role->policy_count] = polycall_core_malloc(core_ctx, policy_len);
     if (!new_policies[role->policy_count]) {
         polycall_core_free(core_ctx, new_policies);
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(new_policies[role->policy_count], policy_name, policy_len);
     
     // Update role
     char** old_policies = role->policy_names;
     role->policy_names = new_policies;
     role->policy_count++;
     
     // Free old policy array (but not the strings, as they were copied by reference)
     if (old_policies) {
         polycall_core_free(core_ctx, old_policies);
     }
     
     pthread_mutex_unlock(&auth_ctx->policies->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_POLICY_UPDATE,
         auth_ctx->current_identity,
         NULL,
         "attach_policy",
         true,
         NULL
     );
     
     if (event) {
         // Add details
         char details[512] = {0};
         snprintf(details, sizeof(details), "{\"role_name\":\"%s\",\"policy_name\":\"%s\"}", 
                  role_name, policy_name);
         event->details = strdup(details);
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Detach a policy from a role
  */
 polycall_core_error_t polycall_auth_detach_policy(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* role_name,
     const char* policy_name
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !role_name || !policy_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock policy manager
     pthread_mutex_lock(&auth_ctx->policies->mutex);
     
     // Find role
     int role_index = -1;
     for (size_t i = 0; i < auth_ctx->policies->role_count; i++) {
         if (strcmp(auth_ctx->policies->roles[i]->name, role_name) == 0) {
             role_index = (int)i;
             break;
         }
     }
     
     // Role not found
     if (role_index == -1) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get role entry
     role_entry_t* role = auth_ctx->policies->roles[role_index];
     
     // Find policy index
     int policy_index = -1;
     for (size_t i = 0; i < role->policy_count; i++) {
         if (role->policy_names[i] && strcmp(role->policy_names[i], policy_name) == 0) {
             policy_index = (int)i;
             break;
         }
     }
     
     // Policy not found
     if (policy_index == -1) {
         pthread_mutex_unlock(&auth_ctx->policies->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Remove policy
     polycall_core_free(core_ctx, role->policy_names[policy_index]);
     
     // Shift remaining policies
     for (size_t i = policy_index; i < role->policy_count - 1; i++) {
         role->policy_names[i] = role->policy_names[i + 1];
     }
     
     role->policy_count--;
     
     pthread_mutex_unlock(&auth_ctx->policies->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_POLICY_UPDATE,
         auth_ctx->current_identity,
         NULL,
         "detach_policy",
         true,
         NULL
     );
     
     if (event) {
         // Add details
         char details[512] = {0};
         snprintf(details, sizeof(details), "{\"role_name\":\"%s\",\"policy_name\":\"%s\"}", 
                  role_name, policy_name);
         event->details = strdup(details);
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Evaluate a permission request
  */
 polycall_core_error_t polycall_auth_evaluate_permission(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* resource,
     const char* action,
     const char* context,
     bool* allowed
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !resource || !action || !allowed) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Default to denied
     *allowed = false;
     
     // Check if access control is enabled
     if (!auth_ctx->config.enable_access_control) {
         // If access control is disabled, all access is allowed
         *allowed = true;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Get identity roles
     char** roles = NULL;
     size_t role_count = 0;
     
     polycall_core_error_t result = get_identity_roles(core_ctx, auth_ctx, identity_id, &roles, &role_count);
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // If identity has no roles, access is denied
     if (role_count == 0) {
         *allowed = false;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Lock policy manager
     pthread_mutex_lock(&auth_ctx->policies->mutex);
     
     // Track effect
     bool has_explicit_deny = false;
     bool has_explicit_allow = false;
     
     // Check each role's policies
     for (size_t i = 0; i < role_count; i++) {
         const char* role_name = roles[i];
         if (!role_name) continue;
         
         // Find role
         int role_index = -1;
         for (size_t j = 0; j < auth_ctx->policies->role_count; j++) {
             if (strcmp(auth_ctx->policies->roles[j]->name, role_name) == 0) {
                 role_index = (int)j;
                 break;
             }
         }
         
         if (role_index == -1) continue; // Role not found
         
         role_entry_t* role = auth_ctx->policies->roles[role_index];
         
         // Check each policy attached to the role
         for (size_t j = 0; j < role->policy_count; j++) {
             const char* policy_name = role->policy_names[j];
             if (!policy_name) continue;
             
             // Find policy
             int policy_index = -1;
             for (size_t k = 0; k < auth_ctx->policies->policy_count; k++) {
                 if (strcmp(auth_ctx->policies->policies[k]->name, policy_name) == 0) {
                     policy_index = (int)k;
                     break;
                 }
             }
             
             if (policy_index == -1) continue; // Policy not found
             
             policy_entry_t* policy = auth_ctx->policies->policies[policy_index];
             
             // Check each statement in the policy
             for (size_t k = 0; k < policy->statement_count; k++) {
                 policy_statement_t* statement = policy->statements[k];
                 if (!statement) continue;
                 
                 // Evaluate statement
                 bool statement_match = evaluate_policy_statement(statement, resource, action, context);
                 
                 if (statement_match) {
                     if (statement->effect == POLYCALL_POLICY_EFFECT_DENY) {
                         has_explicit_deny = true;
                     } else if (statement->effect == POLYCALL_POLICY_EFFECT_ALLOW) {
                         has_explicit_allow = true;
                     }
                 }
             }
         }
     }
     
     // Unlock policy manager
     pthread_mutex_unlock(&auth_ctx->policies->mutex);
     
     // Free roles
     for (size_t i = 0; i < role_count; i++) {
         if (roles[i]) {
             polycall_core_free(core_ctx, roles[i]);
         }
     }
     polycall_core_free(core_ctx, roles);
     
     // In zero-trust model, explicit deny overrides any allow
     if (has_explicit_deny) {
         *allowed = false;
     } else if (has_explicit_allow) {
         *allowed = true;
     } else {
         *allowed = false; // Default deny if no explicit allow
     }
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         *allowed ? POLYCALL_AUDIT_EVENT_ACCESS_GRANTED : POLYCALL_AUDIT_EVENT_ACCESS_DENIED,
         identity_id,
         resource,
         action,
         *allowed,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Check if an identity has permission for a resource and action
  */
 polycall_core_error_t polycall_auth_check_permission(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* resource,
     const char* action,
     bool* allowed
 ) {
     // This is a simplified wrapper around evaluate_permission
     return polycall_auth_evaluate_permission(core_ctx, auth_ctx, identity_id, resource, action, NULL, allowed);
 }
 
 /**
  * @brief Check if a policy resource pattern matches a resource
  */
 static bool policy_matches_resource(const char* policy_resource, const char* resource) {
     if (!policy_resource || !resource) {
         return false;
     }
     
     // Exact match
     if (strcmp(policy_resource, resource) == 0) {
         return true;
     }
     
     // Wildcard match (e.g., "function:*" matches "function:test_function")
     size_t policy_len = strlen(policy_resource);
     if (policy_len > 0 && policy_resource[policy_len - 1] == '*') {
         // Check if the prefix matches
         if (policy_len > 1 && strncmp(policy_resource, resource, policy_len - 1) == 0) {
             return true;
         }
     }
     
     return false;
 }
 
 /**
  * @brief Check if a policy action pattern matches an action
  */
 static bool policy_matches_action(const char* policy_action, const char* action) {
     if (!policy_action || !action) {
         return false;
     }
     
     // Exact match
     if (strcmp(policy_action, action) == 0) {
         return true;
     }
     
     // Wildcard match (e.g., "*" matches any action)
     if (strcmp(policy_action, "*") == 0) {
         return true;
     }
     
     return false;
 }
 
 /**
  * @brief Evaluate a policy statement against a resource and action
  */
 static bool evaluate_policy_statement(
     policy_statement_t* statement,
     const char* resource,
     const char* action,
     const char* context
 ) {
     if (!statement || !resource || !action) {
         return false;
     }
     
     // Check if any resource matches
     bool resource_match = false;
     for (size_t i = 0; i < statement->resource_count; i++) {
         if (statement->resources[i] && policy_matches_resource(statement->resources[i], resource)) {
             resource_match = true;
             break;
         }
     }
     
     if (!resource_match) {
         return false;
     }
     
     // Check if any action matches
     bool action_match = false;
     for (size_t i = 0; i < statement->action_count; i++) {
         if (statement->actions[i] && policy_matches_action(statement->actions[i], action)) {
             action_match = true;
             break;
         }
     }
     
     if (!action_match) {
         return false;
     }
     
     // Check condition if present
     if (statement->condition && context) {
         // In a real implementation, this would evaluate the condition expression
         // against the context. For this implementation, we'll just check if the
         // condition is present in the context.
         if (strstr(context, statement->condition) == NULL) {
             return false;
         }
     }
     
     // All checks passed
     return true;
 }
 
 /**
  * @brief Get all roles assigned to an identity
  */
 static polycall_core_error_t get_identity_roles(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     char*** roles,
     size_t* role_count
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !roles || !role_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize result
     *roles = NULL;
     *role_count = 0;
     
     // Lock identity registry
     pthread_mutex_lock(&auth_ctx->identities->mutex);
     
     // Find identity
     int identity_index = -1;
     for (size_t i = 0; i < auth_ctx->identities->count; i++) {
         if (strcmp(auth_ctx->identities->identity_ids[i], identity_id) == 0) {
             identity_index = (int)i;
             break;
         }
     }
     
     // Identity not found
     if (identity_index == -1) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get roles
     identity_attributes_t* attrs = auth_ctx->identities->attributes[identity_index];
     if (!attrs->roles || attrs->role_count == 0) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_SUCCESS; // No roles
     }
     
     // Create result array
     *roles = polycall_core_malloc(core_ctx, attrs->role_count * sizeof(char*));
     if (!*roles) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     *role_count = attrs->role_count;
     
     // Copy roles
     for (size_t i = 0; i < attrs->role_count; i++) {
         if (attrs->roles[i]) {
             size_t role_len = strlen(attrs->roles[i]) + 1;
             (*roles)[i] = polycall_core_malloc(core_ctx, role_len);
             if (!(*roles)[i]) {
                 // Clean up on failure
                 for (size_t j = 0; j < i; j++) {
                     if ((*roles)[j]) {
                         polycall_core_free(core_ctx, (*roles)[j]);
                     }
                 }
                 polycall_core_free(core_ctx, *roles);
                 *roles = NULL;
                 *role_count = 0;
                 pthread_mutex_unlock(&auth_ctx->identities->mutex);
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             memcpy((*roles)[i], attrs->roles[i], role_len);
         } else {
             (*roles)[i] = NULL;
         }
     }
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
