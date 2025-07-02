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
#include "polycall/core/auth/auth_identity.h"

 * @file auth_identity.c
 * @brief Implementation of identity management for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the identity management interfaces for LibPolyCall authentication,
 * providing functions to create, update, and manage user identities.
 */

    #include "polycall/core/auth/polycall_auth_identity.h"

 /**
  * @brief Register a new identity
  */
 polycall_core_error_t polycall_auth_register_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const identity_attributes_t* attributes,
     const char* initial_password
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !attributes || !initial_password) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check identity ID format
     if (strlen(identity_id) == 0 || strlen(identity_id) > 128) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if attributes contain required fields
     if (!attributes->name || strlen(attributes->name) == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock identity registry
     pthread_mutex_lock(&auth_ctx->identities->mutex);
     
     // Check if identity already exists
     for (size_t i = 0; i < auth_ctx->identities->count; i++) {
         if (strcmp(auth_ctx->identities->identity_ids[i], identity_id) == 0) {
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_ALREADY_EXISTS;
         }
         
         // Also check by name
         identity_attributes_t* existing = auth_ctx->identities->attributes[i];
         if (existing && existing->name && strcmp(existing->name, attributes->name) == 0) {
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_RESOURCE_EXISTS;
         }
     }
     
     // Check if registry has capacity
     if (auth_ctx->identities->count >= auth_ctx->identities->capacity) {
         // Grow the registry
         size_t new_capacity = auth_ctx->identities->capacity * 2;
         
         char** new_ids = polycall_core_malloc(core_ctx, new_capacity * sizeof(char*));
         identity_attributes_t** new_attrs = polycall_core_malloc(core_ctx, new_capacity * sizeof(identity_attributes_t*));
         char** new_passwords = polycall_core_malloc(core_ctx, new_capacity * sizeof(char*));
         
         if (!new_ids || !new_attrs || !new_passwords) {
             if (new_ids) polycall_core_free(core_ctx, new_ids);
             if (new_attrs) polycall_core_free(core_ctx, new_attrs);
             if (new_passwords) polycall_core_free(core_ctx, new_passwords);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing entries
         memcpy(new_ids, auth_ctx->identities->identity_ids, auth_ctx->identities->count * sizeof(char*));
         memcpy(new_attrs, auth_ctx->identities->attributes, auth_ctx->identities->count * sizeof(identity_attributes_t*));
         memcpy(new_passwords, auth_ctx->identities->hashed_passwords, auth_ctx->identities->count * sizeof(char*));
         
         // Free old arrays
         polycall_core_free(core_ctx, auth_ctx->identities->identity_ids);
         polycall_core_free(core_ctx, auth_ctx->identities->attributes);
         polycall_core_free(core_ctx, auth_ctx->identities->hashed_passwords);
         
         // Update registry
         auth_ctx->identities->identity_ids = new_ids;
         auth_ctx->identities->attributes = new_attrs;
         auth_ctx->identities->hashed_passwords = new_passwords;
         auth_ctx->identities->capacity = new_capacity;
     }
     
     // Hash the password
     char* hashed_password = hash_password(auth_ctx->credentials, initial_password);
     if (!hashed_password) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Create a copy of the identity ID
     size_t id_len = strlen(identity_id) + 1;
     char* id_copy = polycall_core_malloc(core_ctx, id_len);
     if (!id_copy) {
         polycall_core_free(core_ctx, hashed_password);
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(id_copy, identity_id, id_len);
     
     // Create a copy of the attributes
     identity_attributes_t* attr_copy = polycall_core_malloc(core_ctx, sizeof(identity_attributes_t));
     if (!attr_copy) {
         polycall_core_free(core_ctx, hashed_password);
         polycall_core_free(core_ctx, id_copy);
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize attributes
     memset(attr_copy, 0, sizeof(identity_attributes_t));
     
     // Copy name
     if (attributes->name) {
         size_t name_len = strlen(attributes->name) + 1;
         attr_copy->name = polycall_core_malloc(core_ctx, name_len);
         if (!attr_copy->name) {
             polycall_core_free(core_ctx, hashed_password);
             polycall_core_free(core_ctx, id_copy);
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(attr_copy->name, attributes->name, name_len);
     }
     
     // Copy email
     if (attributes->email) {
         size_t email_len = strlen(attributes->email) + 1;
         attr_copy->email = polycall_core_malloc(core_ctx, email_len);
         if (!attr_copy->email) {
             polycall_core_free(core_ctx, hashed_password);
             polycall_core_free(core_ctx, id_copy);
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(attr_copy->email, attributes->email, email_len);
     }
     
     // Copy roles if present
     if (attributes->roles && attributes->role_count > 0) {
         attr_copy->roles = polycall_core_malloc(core_ctx, attributes->role_count * sizeof(char*));
         if (!attr_copy->roles) {
             polycall_core_free(core_ctx, hashed_password);
             polycall_core_free(core_ctx, id_copy);
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         attr_copy->role_count = attributes->role_count;
         
         for (size_t i = 0; i < attributes->role_count; i++) {
             if (attributes->roles[i]) {
                 size_t role_len = strlen(attributes->roles[i]) + 1;
                 attr_copy->roles[i] = polycall_core_malloc(core_ctx, role_len);
                 if (!attr_copy->roles[i]) {
                     polycall_core_free(core_ctx, hashed_password);
                     polycall_core_free(core_ctx, id_copy);
                     if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
                     if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
                     
                     for (size_t j = 0; j < i; j++) {
                         polycall_core_free(core_ctx, attr_copy->roles[j]);
                     }
                     
                     polycall_core_free(core_ctx, attr_copy->roles);
                     polycall_core_free(core_ctx, attr_copy);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(attr_copy->roles[i], attributes->roles[i], role_len);
             } else {
                 attr_copy->roles[i] = NULL;
             }
         }
     }
     
     // Copy groups if present
     if (attributes->groups && attributes->group_count > 0) {
         attr_copy->groups = polycall_core_malloc(core_ctx, attributes->group_count * sizeof(char*));
         if (!attr_copy->groups) {
             polycall_core_free(core_ctx, hashed_password);
             polycall_core_free(core_ctx, id_copy);
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
             
             if (attr_copy->roles) {
                 for (size_t i = 0; i < attr_copy->role_count; i++) {
                     if (attr_copy->roles[i]) polycall_core_free(core_ctx, attr_copy->roles[i]);
                 }
                 polycall_core_free(core_ctx, attr_copy->roles);
             }
             
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         attr_copy->group_count = attributes->group_count;
         
         for (size_t i = 0; i < attributes->group_count; i++) {
             if (attributes->groups[i]) {
                 size_t group_len = strlen(attributes->groups[i]) + 1;
                 attr_copy->groups[i] = polycall_core_malloc(core_ctx, group_len);
                 if (!attr_copy->groups[i]) {
                     polycall_core_free(core_ctx, hashed_password);
                     polycall_core_free(core_ctx, id_copy);
                     if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
                     if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
                     
                     if (attr_copy->roles) {
                         for (size_t j = 0; j < attr_copy->role_count; j++) {
                             if (attr_copy->roles[j]) polycall_core_free(core_ctx, attr_copy->roles[j]);
                         }
                         polycall_core_free(core_ctx, attr_copy->roles);
                     }
                     
                     for (size_t j = 0; j < i; j++) {
                         polycall_core_free(core_ctx, attr_copy->groups[j]);
                     }
                     
                     polycall_core_free(core_ctx, attr_copy->groups);
                     polycall_core_free(core_ctx, attr_copy);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(attr_copy->groups[i], attributes->groups[i], group_len);
             } else {
                 attr_copy->groups[i] = NULL;
             }
         }
     }
     
     // Copy metadata if present
     if (attributes->metadata) {
         size_t metadata_len = strlen(attributes->metadata) + 1;
         attr_copy->metadata = polycall_core_malloc(core_ctx, metadata_len);
         if (!attr_copy->metadata) {
             polycall_core_free(core_ctx, hashed_password);
             polycall_core_free(core_ctx, id_copy);
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
             
             if (attr_copy->roles) {
                 for (size_t i = 0; i < attr_copy->role_count; i++) {
                     if (attr_copy->roles[i]) polycall_core_free(core_ctx, attr_copy->roles[i]);
                 }
                 polycall_core_free(core_ctx, attr_copy->roles);
             }
             
             if (attr_copy->groups) {
                 for (size_t i = 0; i < attr_copy->group_count; i++) {
                     if (attr_copy->groups[i]) polycall_core_free(core_ctx, attr_copy->groups[i]);
                 }
                 polycall_core_free(core_ctx, attr_copy->groups);
             }
             
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(attr_copy->metadata, attributes->metadata, metadata_len);
     }
     
     // Set timestamps
     attr_copy->created_timestamp = get_current_timestamp();
     attr_copy->last_login_timestamp = 0; // No login yet
     
     // Set active status (default to active)
     attr_copy->is_active = attributes->is_active || true;
     
     // Add to registry
     auth_ctx->identities->identity_ids[auth_ctx->identities->count] = id_copy;
     auth_ctx->identities->attributes[auth_ctx->identities->count] = attr_copy;
     auth_ctx->identities->hashed_passwords[auth_ctx->identities->count] = hashed_password;
     auth_ctx->identities->count++;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_IDENTITY_CREATE,
         identity_id,
         NULL,
         NULL,
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get identity attributes
  */
 polycall_core_error_t polycall_auth_get_identity_attributes(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     identity_attributes_t** attributes
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !attributes) {
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
     
     // Get existing attributes
     identity_attributes_t* src_attrs = auth_ctx->identities->attributes[identity_index];
     
     // Create a copy of the attributes
     identity_attributes_t* attr_copy = polycall_core_malloc(core_ctx, sizeof(identity_attributes_t));
     if (!attr_copy) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize attributes
     memset(attr_copy, 0, sizeof(identity_attributes_t));
     
     // Copy name
     if (src_attrs->name) {
         size_t name_len = strlen(src_attrs->name) + 1;
         attr_copy->name = polycall_core_malloc(core_ctx, name_len);
         if (!attr_copy->name) {
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(attr_copy->name, src_attrs->name, name_len);
     }
     
     // Copy email
     if (src_attrs->email) {
         size_t email_len = strlen(src_attrs->email) + 1;
         attr_copy->email = polycall_core_malloc(core_ctx, email_len);
         if (!attr_copy->email) {
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(attr_copy->email, src_attrs->email, email_len);
     }
     
     // Copy roles if present
     if (src_attrs->roles && src_attrs->role_count > 0) {
         attr_copy->roles = polycall_core_malloc(core_ctx, src_attrs->role_count * sizeof(char*));
         if (!attr_copy->roles) {
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         attr_copy->role_count = src_attrs->role_count;
         
         for (size_t i = 0; i < src_attrs->role_count; i++) {
             if (src_attrs->roles[i]) {
                 size_t role_len = strlen(src_attrs->roles[i]) + 1;
                 attr_copy->roles[i] = polycall_core_malloc(core_ctx, role_len);
                 if (!attr_copy->roles[i]) {
                     if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
                     if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
                     
                     for (size_t j = 0; j < i; j++) {
                         if (attr_copy->roles[j]) polycall_core_free(core_ctx, attr_copy->roles[j]);
                     }
                     
                     polycall_core_free(core_ctx, attr_copy->roles);
                     polycall_core_free(core_ctx, attr_copy);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(attr_copy->roles[i], src_attrs->roles[i], role_len);
             } else {
                 attr_copy->roles[i] = NULL;
             }
         }
     }
     
     // Copy groups if present
     if (src_attrs->groups && src_attrs->group_count > 0) {
         attr_copy->groups = polycall_core_malloc(core_ctx, src_attrs->group_count * sizeof(char*));
         if (!attr_copy->groups) {
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
             
             if (attr_copy->roles) {
                 for (size_t i = 0; i < attr_copy->role_count; i++) {
                     if (attr_copy->roles[i]) polycall_core_free(core_ctx, attr_copy->roles[i]);
                 }
                 polycall_core_free(core_ctx, attr_copy->roles);
             }
             
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         attr_copy->group_count = src_attrs->group_count;
         
         for (size_t i = 0; i < src_attrs->group_count; i++) {
             if (src_attrs->groups[i]) {
                 size_t group_len = strlen(src_attrs->groups[i]) + 1;
                 attr_copy->groups[i] = polycall_core_malloc(core_ctx, group_len);
                 if (!attr_copy->groups[i]) {
                     if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
                     if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
                     
                     if (attr_copy->roles) {
                         for (size_t j = 0; j < attr_copy->role_count; j++) {
                             if (attr_copy->roles[j]) polycall_core_free(core_ctx, attr_copy->roles[j]);
                         }
                         polycall_core_free(core_ctx, attr_copy->roles);
                     }
                     
                     for (size_t j = 0; j < i; j++) {
                         if (attr_copy->groups[j]) polycall_core_free(core_ctx, attr_copy->groups[j]);
                     }
                     
                     polycall_core_free(core_ctx, attr_copy->groups);
                     polycall_core_free(core_ctx, attr_copy);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(attr_copy->groups[i], src_attrs->groups[i], group_len);
             } else {
                 attr_copy->groups[i] = NULL;
             }
         }
     }
     
     // Copy metadata if present
     if (src_attrs->metadata) {
         size_t metadata_len = strlen(src_attrs->metadata) + 1;
         attr_copy->metadata = polycall_core_malloc(core_ctx, metadata_len);
         if (!attr_copy->metadata) {
             if (attr_copy->name) polycall_core_free(core_ctx, attr_copy->name);
             if (attr_copy->email) polycall_core_free(core_ctx, attr_copy->email);
             
             if (attr_copy->roles) {
                 for (size_t i = 0; i < attr_copy->role_count; i++) {
                     if (attr_copy->roles[i]) polycall_core_free(core_ctx, attr_copy->roles[i]);
                 }
                 polycall_core_free(core_ctx, attr_copy->roles);
             }
             
             if (attr_copy->groups) {
                 for (size_t i = 0; i < attr_copy->group_count; i++) {
                     if (attr_copy->groups[i]) polycall_core_free(core_ctx, attr_copy->groups[i]);
                 }
                 polycall_core_free(core_ctx, attr_copy->groups);
             }
             
             polycall_core_free(core_ctx, attr_copy);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(attr_copy->metadata, src_attrs->metadata, metadata_len);
     }
     
     // Copy timestamps
     attr_copy->created_timestamp = src_attrs->created_timestamp;
     attr_copy->last_login_timestamp = src_attrs->last_login_timestamp;
     
     // Copy active status
     attr_copy->is_active = src_attrs->is_active;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     *attributes = attr_copy;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Update identity attributes
  */
 polycall_core_error_t polycall_auth_update_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const identity_attributes_t* attributes
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !attributes) {
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
     
     // Get existing attributes
     identity_attributes_t* existing = auth_ctx->identities->attributes[identity_index];
     
     // Create a new attributes structure
     identity_attributes_t* new_attrs = polycall_core_malloc(core_ctx, sizeof(identity_attributes_t));
     if (!new_attrs) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize attributes
     memset(new_attrs, 0, sizeof(identity_attributes_t));
     
     // Update name if provided
     if (attributes->name) {
         size_t name_len = strlen(attributes->name) + 1;
         new_attrs->name = polycall_core_malloc(core_ctx, name_len);
         if (!new_attrs->name) {
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(new_attrs->name, attributes->name, name_len);
     } else if (existing->name) {
         // Keep existing name
         size_t name_len = strlen(existing->name) + 1;
         new_attrs->name = polycall_core_malloc(core_ctx, name_len);
         if (!new_attrs->name) {
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(new_attrs->name, existing->name, name_len);
     }
     
     // Update email if provided
     if (attributes->email) {
         size_t email_len = strlen(attributes->email) + 1;
         new_attrs->email = polycall_core_malloc(core_ctx, email_len);
         if (!new_attrs->email) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(new_attrs->email, attributes->email, email_len);
     } else if (existing->email) {
         // Keep existing email
         size_t email_len = strlen(existing->email) + 1;
         new_attrs->email = polycall_core_malloc(core_ctx, email_len);
         if (!new_attrs->email) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(new_attrs->email, existing->email, email_len);
     }
     
     // Update roles if provided
     if (attributes->roles && attributes->role_count > 0) {
         new_attrs->roles = polycall_core_malloc(core_ctx, attributes->role_count * sizeof(char*));
         if (!new_attrs->roles) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         new_attrs->role_count = attributes->role_count;
         
         for (size_t i = 0; i < attributes->role_count; i++) {
             if (attributes->roles[i]) {
                 size_t role_len = strlen(attributes->roles[i]) + 1;
                 new_attrs->roles[i] = polycall_core_malloc(core_ctx, role_len);
                 if (!new_attrs->roles[i]) {
                     if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
                     if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
                     
                     for (size_t j = 0; j < i; j++) {
                         if (new_attrs->roles[j]) polycall_core_free(core_ctx, new_attrs->roles[j]);
                     }
                     
                     polycall_core_free(core_ctx, new_attrs->roles);
                     polycall_core_free(core_ctx, new_attrs);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(new_attrs->roles[i], attributes->roles[i], role_len);
             } else {
                 new_attrs->roles[i] = NULL;
             }
         }
     } else if (existing->roles && existing->role_count > 0) {
         // Keep existing roles
         new_attrs->roles = polycall_core_malloc(core_ctx, existing->role_count * sizeof(char*));
         if (!new_attrs->roles) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         new_attrs->role_count = existing->role_count;
         
         for (size_t i = 0; i < existing->role_count; i++) {
             if (existing->roles[i]) {
                 size_t role_len = strlen(existing->roles[i]) + 1;
                 new_attrs->roles[i] = polycall_core_malloc(core_ctx, role_len);
                 if (!new_attrs->roles[i]) {
                     if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
                     if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
                     
                     for (size_t j = 0; j < i; j++) {
                         if (new_attrs->roles[j]) polycall_core_free(core_ctx, new_attrs->roles[j]);
                     }
                     
                     polycall_core_free(core_ctx, new_attrs->roles);
                     polycall_core_free(core_ctx, new_attrs);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(new_attrs->roles[i], existing->roles[i], role_len);
             } else {
                 new_attrs->roles[i] = NULL;
             }
         }
     }
     
     // Update groups if provided
     if (attributes->groups && attributes->group_count > 0) {
         new_attrs->groups = polycall_core_malloc(core_ctx, attributes->group_count * sizeof(char*));
         if (!new_attrs->groups) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
             
             if (new_attrs->roles) {
                 for (size_t i = 0; i < new_attrs->role_count; i++) {
                     if (new_attrs->roles[i]) polycall_core_free(core_ctx, new_attrs->roles[i]);
                 }
                 polycall_core_free(core_ctx, new_attrs->roles);
             }
             
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         new_attrs->group_count = attributes->group_count;
         
         for (size_t i = 0; i < attributes->group_count; i++) {
             if (attributes->groups[i]) {
                 size_t group_len = strlen(attributes->groups[i]) + 1;
                 new_attrs->groups[i] = polycall_core_malloc(core_ctx, group_len);
                 if (!new_attrs->groups[i]) {
                     if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
                     if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
                     
                     if (new_attrs->roles) {
                         for (size_t j = 0; j < new_attrs->role_count; j++) {
                             if (new_attrs->roles[j]) polycall_core_free(core_ctx, new_attrs->roles[j]);
                         }
                         polycall_core_free(core_ctx, new_attrs->roles);
                     }
                     
                     for (size_t j = 0; j < i; j++) {
                         if (new_attrs->groups[j]) polycall_core_free(core_ctx, new_attrs->groups[j]);
                     }
                     
                     polycall_core_free(core_ctx, new_attrs->groups);
                     polycall_core_free(core_ctx, new_attrs);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(new_attrs->groups[i], attributes->groups[i], group_len);
             } else {
                 new_attrs->groups[i] = NULL;
             }
         }
     } else if (existing->groups && existing->group_count > 0) {
         // Keep existing groups
         new_attrs->groups = polycall_core_malloc(core_ctx, existing->group_count * sizeof(char*));
         if (!new_attrs->groups) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
             
             if (new_attrs->roles) {
                 for (size_t i = 0; i < new_attrs->role_count; i++) {
                     if (new_attrs->roles[i]) polycall_core_free(core_ctx, new_attrs->roles[i]);
                 }
                 polycall_core_free(core_ctx, new_attrs->roles);
             }
             
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         new_attrs->group_count = existing->group_count;
         
         for (size_t i = 0; i < existing->group_count; i++) {
             if (existing->groups[i]) {
                 size_t group_len = strlen(existing->groups[i]) + 1;
                 new_attrs->groups[i] = polycall_core_malloc(core_ctx, group_len);
                 if (!new_attrs->groups[i]) {
                     if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
                     if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
                     
                     if (new_attrs->roles) {
                         for (size_t j = 0; j < new_attrs->role_count; j++) {
                             if (new_attrs->roles[j]) polycall_core_free(core_ctx, new_attrs->roles[j]);
                         }
                         polycall_core_free(core_ctx, new_attrs->roles);
                     }
                     
                     for (size_t j = 0; j < i; j++) {
                         if (new_attrs->groups[j]) polycall_core_free(core_ctx, new_attrs->groups[j]);
                     }
                     
                     polycall_core_free(core_ctx, new_attrs->groups);
                     polycall_core_free(core_ctx, new_attrs);
                     pthread_mutex_unlock(&auth_ctx->identities->mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(new_attrs->groups[i], existing->groups[i], group_len);
             } else {
                 new_attrs->groups[i] = NULL;
             }
         }
     }
     
     // Update metadata if provided
     if (attributes->metadata) {
         size_t metadata_len = strlen(attributes->metadata) + 1;
         new_attrs->metadata = polycall_core_malloc(core_ctx, metadata_len);
         if (!new_attrs->metadata) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
             
             if (new_attrs->roles) {
                 for (size_t i = 0; i < new_attrs->role_count; i++) {
                     if (new_attrs->roles[i]) polycall_core_free(core_ctx, new_attrs->roles[i]);
                 }
                 polycall_core_free(core_ctx, new_attrs->roles);
             }
             
             if (new_attrs->groups) {
                 for (size_t i = 0; i < new_attrs->group_count; i++) {
                     if (new_attrs->groups[i]) polycall_core_free(core_ctx, new_attrs->groups[i]);
                 }
                 polycall_core_free(core_ctx, new_attrs->groups);
             }
             
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_attrs->metadata, attributes->metadata, metadata_len);
     } else if (existing->metadata) {
         // Keep existing metadata
         size_t metadata_len = strlen(existing->metadata) + 1;
         new_attrs->metadata = polycall_core_malloc(core_ctx, metadata_len);
         if (!new_attrs->metadata) {
             if (new_attrs->name) polycall_core_free(core_ctx, new_attrs->name);
             if (new_attrs->email) polycall_core_free(core_ctx, new_attrs->email);
             
             if (new_attrs->roles) {
                 for (size_t i = 0; i < new_attrs->role_count; i++) {
                     if (new_attrs->roles[i]) polycall_core_free(core_ctx, new_attrs->roles[i]);
                 }
                 polycall_core_free(core_ctx, new_attrs->roles);
             }
             
             if (new_attrs->groups) {
                 for (size_t i = 0; i < new_attrs->group_count; i++) {
                     if (new_attrs->groups[i]) polycall_core_free(core_ctx, new_attrs->groups[i]);
                 }
                 polycall_core_free(core_ctx, new_attrs->groups);
             }
             
             polycall_core_free(core_ctx, new_attrs);
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_attrs->metadata, existing->metadata, metadata_len);
     }
     
     // Keep timestamps
     new_attrs->created_timestamp = existing->created_timestamp;
     new_attrs->last_login_timestamp = existing->last_login_timestamp;
     
     // Update active status
     new_attrs->is_active = attributes->is_active;
     
     // Clean up old attributes
     polycall_auth_free_identity_attributes(core_ctx, existing);
     
     // Update registry
     auth_ctx->identities->attributes[identity_index] = new_attrs;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_IDENTITY_UPDATE,
         identity_id,
         NULL,
         NULL,
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Change identity password
  */
 polycall_core_error_t polycall_auth_change_password(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* current_password,
     const char* new_password
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !current_password || !new_password) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if new password meets complexity requirements
     if (strlen(new_password) < 8) {
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
     
     // Verify current password
     char* stored_hash = auth_ctx->identities->hashed_passwords[identity_index];
     
     // Make a copy of the current password hash for verification
     // This allows us to release the mutex while we verify
     char* hash_copy = NULL;
     if (stored_hash) {
         size_t hash_len = strlen(stored_hash) + 1;
         hash_copy = polycall_core_malloc(core_ctx, hash_len);
         if (!hash_copy) {
             pthread_mutex_unlock(&auth_ctx->identities->mutex);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memcpy(hash_copy, stored_hash, hash_len);
     }
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Verify current password
     bool password_valid = verify_password(auth_ctx->credentials, current_password, hash_copy);
     
     // Free the hash copy
     if (hash_copy) {
         polycall_core_free(core_ctx, hash_copy);
     }
     
     if (!password_valid) {
         // Create audit event for failed password change
         audit_event_t* event = polycall_auth_create_audit_event(
             core_ctx,
             POLYCALL_AUDIT_EVENT_PASSWORD_CHANGE,
             identity_id,
             NULL,
             NULL,
             false,
             "Invalid current password"
         );
         
         if (event) {
             polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
             polycall_auth_free_audit_event(core_ctx, event);
         }
         
         return POLYCALL_CORE_ERROR_ACCESS_DENIED;
     }
     
     // Hash the new password
     char* new_hash = hash_password(auth_ctx->credentials, new_password);
     if (!new_hash) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Lock identity registry again
     pthread_mutex_lock(&auth_ctx->identities->mutex);
     
     // Check if identity still exists
     if (identity_index >= (int)auth_ctx->identities->count ||
         strcmp(auth_ctx->identities->identity_ids[identity_index], identity_id) != 0) {
         polycall_core_free(core_ctx, new_hash);
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Free the old hash
     if (auth_ctx->identities->hashed_passwords[identity_index]) {
         polycall_core_free(core_ctx, auth_ctx->identities->hashed_passwords[identity_index]);
     }
     
     // Update password
     auth_ctx->identities->hashed_passwords[identity_index] = new_hash;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_PASSWORD_CHANGE,
         identity_id,
         NULL,
         NULL,
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Reset identity password (administrative function)
  */
 polycall_core_error_t polycall_auth_reset_password(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* new_password
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !new_password) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if new password meets complexity requirements
     if (strlen(new_password) < 8) {
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
     
     // Hash the new password
     char* new_hash = hash_password(auth_ctx->credentials, new_password);
     if (!new_hash) {
         pthread_mutex_unlock(&auth_ctx->identities->mutex);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Free the old hash
     if (auth_ctx->identities->hashed_passwords[identity_index]) {
         polycall_core_free(core_ctx, auth_ctx->identities->hashed_passwords[identity_index]);
     }
     
     // Update password
     auth_ctx->identities->hashed_passwords[identity_index] = new_hash;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_PASSWORD_RESET,
         identity_id,
         NULL,
         NULL,
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Deactivate an identity
  */
 polycall_core_error_t polycall_auth_deactivate_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id) {
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
     
     // Set identity as inactive
     auth_ctx->identities->attributes[identity_index]->is_active = false;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_IDENTITY_UPDATE,
         identity_id,
         NULL,
         "deactivate",
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Reactivate an identity
  */
 polycall_core_error_t polycall_auth_reactivate_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id) {
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
     
     // Set identity as active
     auth_ctx->identities->attributes[identity_index]->is_active = true;
     
     pthread_mutex_unlock(&auth_ctx->identities->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_IDENTITY_UPDATE,
         identity_id,
         NULL,
         "reactivate",
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Free identity attributes
  */
 void polycall_auth_free_identity_attributes(
     polycall_core_context_t* core_ctx,
     identity_attributes_t* attributes
 ) {
     if (!core_ctx || !attributes) {
         return;
     }
     
     // Free name
     if (attributes->name) {
         polycall_core_free(core_ctx, attributes->name);
     }
     
     // Free email
     if (attributes->email) {
         polycall_core_free(core_ctx, attributes->email);
     }
     
     // Free roles
     if (attributes->roles) {
         for (size_t i = 0; i < attributes->role_count; i++) {
             if (attributes->roles[i]) {
                 polycall_core_free(core_ctx, attributes->roles[i]);
             }
         }
         polycall_core_free(core_ctx, attributes->roles);
     }
     
     // Free groups
     if (attributes->groups) {
         for (size_t i = 0; i < attributes->group_count; i++) {
             if (attributes->groups[i]) {
                 polycall_core_free(core_ctx, attributes->groups[i]);
             }
         }
         polycall_core_free(core_ctx, attributes->groups);
     }
     
     // Free metadata
     if (attributes->metadata) {
         polycall_core_free(core_ctx, attributes->metadata);
     }
     
     // Free attributes structure
     polycall_core_free(core_ctx, attributes);
 }
