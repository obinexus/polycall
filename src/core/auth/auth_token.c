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
#include "polycall/core/auth/auth_token.h"

 * @file auth_token.c
 * @brief Implementation of token management for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the token management interfaces for LibPolyCall authentication,
 * providing functions to generate, validate, and revoke authentication tokens.
 */

 #include "polycall/core/auth/polycall_auth_context.h"
 #include "polycall/core/auth/polycall_auth_token.h"
 #include "polycall/core/auth/polycall_auth_audit.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 
 // Forward declaration for internal functions
 static char* create_token_id(polycall_token_type_t type);
 static char* build_token_payload(token_claims_t* claims);
 static token_claims_t* parse_token_payload(const char* payload);
 static char* hmac_sha256(const char* key, const char* message);
 static char* base64_encode(const unsigned char* data, size_t input_length);
 static unsigned char* base64_decode(const char* data, size_t *output_length);
 static void free_token_entry(polycall_core_context_t* core_ctx, token_entry_t* entry);
 
 /**
  * @brief Issue a token
  */
 polycall_core_error_t polycall_auth_issue_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     polycall_token_type_t token_type,
     const char** scopes,
     size_t scope_count,
     const char* custom_claims,
     char** token
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !token) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create token claims
     token_claims_t claims = {0};
     
     // Set subject (identity ID)
     size_t subject_len = strlen(identity_id) + 1;
     claims.subject = polycall_core_malloc(core_ctx, subject_len);
     if (!claims.subject) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(claims.subject, identity_id, subject_len);
     
     // Set issuer (hostname/service name)
     const char* issuer = "libpolycall_auth"; // Default issuer
     size_t issuer_len = strlen(issuer) + 1;
     claims.issuer = polycall_core_malloc(core_ctx, issuer_len);
     if (!claims.issuer) {
         polycall_core_free(core_ctx, claims.subject);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(claims.issuer, issuer, issuer_len);
     
     // Set audience
     const char* audience = "*"; // Default audience (all services)
     size_t audience_len = strlen(audience) + 1;
     claims.audience = polycall_core_malloc(core_ctx, audience_len);
     if (!claims.audience) {
         polycall_core_free(core_ctx, claims.subject);
         polycall_core_free(core_ctx, claims.issuer);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(claims.audience, audience, audience_len);
     
     // Set token ID
     char* token_id = create_token_id(token_type);
     if (!token_id) {
         polycall_core_free(core_ctx, claims.subject);
         polycall_core_free(core_ctx, claims.issuer);
         polycall_core_free(core_ctx, claims.audience);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     size_t token_id_len = strlen(token_id) + 1;
     claims.token_id = polycall_core_malloc(core_ctx, token_id_len);
     if (!claims.token_id) {
         free(token_id); // Using standard free because create_token_id uses malloc
         polycall_core_free(core_ctx, claims.subject);
         polycall_core_free(core_ctx, claims.issuer);
         polycall_core_free(core_ctx, claims.audience);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memcpy(claims.token_id, token_id, token_id_len);
     free(token_id);
     
     // Set timestamps
     uint64_t now = get_current_timestamp();
     claims.issued_at = now;
     
     // Set expiry time based on token type
     if (token_type == POLYCALL_TOKEN_TYPE_ACCESS) {
         claims.expires_at = now + auth_ctx->token_service->access_token_validity;
     } else if (token_type == POLYCALL_TOKEN_TYPE_REFRESH) {
         claims.expires_at = now + auth_ctx->token_service->refresh_token_validity;
     } else {
         // API keys - use a long validity (1 year) by default
         claims.expires_at = now + (365 * 24 * 60 * 60);
     }
     
     // Copy scopes if provided
     if (scopes && scope_count > 0) {
         claims.scopes = polycall_core_malloc(core_ctx, scope_count * sizeof(char*));
         if (!claims.scopes) {
             polycall_core_free(core_ctx, claims.subject);
             polycall_core_free(core_ctx, claims.issuer);
             polycall_core_free(core_ctx, claims.audience);
             polycall_core_free(core_ctx, claims.token_id);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         claims.scope_count = scope_count;
         
         for (size_t i = 0; i < scope_count; i++) {
             if (scopes[i]) {
                 size_t scope_len = strlen(scopes[i]) + 1;
                 claims.scopes[i] = polycall_core_malloc(core_ctx, scope_len);
                 if (!claims.scopes[i]) {
                     polycall_core_free(core_ctx, claims.subject);
                     polycall_core_free(core_ctx, claims.issuer);
                     polycall_core_free(core_ctx, claims.audience);
                     polycall_core_free(core_ctx, claims.token_id);
                     
                     for (size_t j = 0; j < i; j++) {
                         if (claims.scopes[j]) {
                             polycall_core_free(core_ctx, claims.scopes[j]);
                         }
                     }
                     
                     polycall_core_free(core_ctx, claims.scopes);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 memcpy(claims.scopes[i], scopes[i], scope_len);
             } else {
                 claims.scopes[i] = NULL;
             }
         }
     }
     
     // Copy custom claims if provided
     if (custom_claims) {
         size_t custom_claims_len = strlen(custom_claims) + 1;
         claims.custom_claims = polycall_core_malloc(core_ctx, custom_claims_len);
         if (!claims.custom_claims) {
             polycall_core_free(core_ctx, claims.subject);
             polycall_core_free(core_ctx, claims.issuer);
             polycall_core_free(core_ctx, claims.audience);
             polycall_core_free(core_ctx, claims.token_id);
             
             if (claims.scopes) {
                 for (size_t i = 0; i < claims.scope_count; i++) {
                     if (claims.scopes[i]) {
                         polycall_core_free(core_ctx, claims.scopes[i]);
                     }
                 }
                 polycall_core_free(core_ctx, claims.scopes);
             }
             
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(claims.custom_claims, custom_claims, custom_claims_len);
     }
     
     // Generate token
     char* new_token = generate_token(auth_ctx->token_service, identity_id, token_type, claims.expires_at);
     if (!new_token) {
         polycall_core_free(core_ctx, claims.subject);
         polycall_core_free(core_ctx, claims.issuer);
         polycall_core_free(core_ctx, claims.audience);
         polycall_core_free(core_ctx, claims.token_id);
         
         if (claims.scopes) {
             for (size_t i = 0; i < claims.scope_count; i++) {
                 if (claims.scopes[i]) {
                     polycall_core_free(core_ctx, claims.scopes[i]);
                 }
             }
             polycall_core_free(core_ctx, claims.scopes);
         }
         
         if (claims.custom_claims) {
             polycall_core_free(core_ctx, claims.custom_claims);
         }
         
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Cleanup claims
     polycall_core_free(core_ctx, claims.subject);
     polycall_core_free(core_ctx, claims.issuer);
     polycall_core_free(core_ctx, claims.audience);
     polycall_core_free(core_ctx, claims.token_id);
     
     if (claims.scopes) {
         for (size_t i = 0; i < claims.scope_count; i++) {
             if (claims.scopes[i]) {
                 polycall_core_free(core_ctx, claims.scopes[i]);
             }
         }
         polycall_core_free(core_ctx, claims.scopes);
     }
     
     if (claims.custom_claims) {
         polycall_core_free(core_ctx, claims.custom_claims);
     }
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_TOKEN_ISSUE,
         identity_id,
         NULL,
         token_type == POLYCALL_TOKEN_TYPE_ACCESS ? "access" :
         (token_type == POLYCALL_TOKEN_TYPE_REFRESH ? "refresh" : "api_key"),
         true,
         NULL
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     *token = new_token;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate a token
  */
 polycall_core_error_t polycall_auth_validate_token_ex(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token,
     token_validation_result_t** result
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !token || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create validation result
     token_validation_result_t* validation_result = polycall_core_malloc(core_ctx, sizeof(token_validation_result_t));
     if (!validation_result) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize result
     memset(validation_result, 0, sizeof(token_validation_result_t));
     validation_result->is_valid = false;
     
     // Validate token
     token_validation_result_t* internal_result = validate_token_internal(auth_ctx->token_service, token);
     if (!internal_result) {
         // Create error message
         const char* error_message = "Memory allocation failed during validation";
         size_t error_len = strlen(error_message) + 1;
         validation_result->error_message = polycall_core_malloc(core_ctx, error_len);
         if (validation_result->error_message) {
             memcpy(validation_result->error_message, error_message, error_len);
         }
         
         *result = validation_result;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Check validation result
     if (!internal_result->is_valid) {
         // Copy error message if present
         if (internal_result->error_message) {
             size_t error_len = strlen(internal_result->error_message) + 1;
             validation_result->error_message = polycall_core_malloc(core_ctx, error_len);
             if (validation_result->error_message) {
                 memcpy(validation_result->error_message, internal_result->error_message, error_len);
             }
         } else {
             const char* error_message = "Token validation failed";
             size_t error_len = strlen(error_message) + 1;
             validation_result->error_message = polycall_core_malloc(core_ctx, error_len);
             if (validation_result->error_message) {
                 memcpy(validation_result->error_message, error_message, error_len);
             }
         }
         
         free(internal_result->error_message);
         free(internal_result);
         
         *result = validation_result;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Token is valid, copy claims
     validation_result->is_valid = true;
     validation_result->claims = polycall_core_malloc(core_ctx, sizeof(token_claims_t));
     if (!validation_result->claims) {
         if (internal_result->claims) {
             free(internal_result->claims->subject);
             free(internal_result->claims->issuer);
             free(internal_result->claims->audience);
             free(internal_result->claims->token_id);
             
             if (internal_result->claims->roles) {
                 for (size_t i = 0; i < internal_result->claims->role_count; i++) {
                     free(internal_result->claims->roles[i]);
                 }
                 free(internal_result->claims->roles);
             }
             
             if (internal_result->claims->scopes) {
                 for (size_t i = 0; i < internal_result->claims->scope_count; i++) {
                     free(internal_result->claims->scopes[i]);
                 }
                 free(internal_result->claims->scopes);
             }
             
             free(internal_result->claims->device_info);
             free(internal_result->claims->custom_claims);
             free(internal_result->claims);
         }
         
         free(internal_result);
         polycall_core_free(core_ctx, validation_result);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize claims
     memset(validation_result->claims, 0, sizeof(token_claims_t));
     
     // Copy subject
     if (internal_result->claims->subject) {
         size_t subject_len = strlen(internal_result->claims->subject) + 1;
         validation_result->claims->subject = polycall_core_malloc(core_ctx, subject_len);
         if (validation_result->claims->subject) {
             memcpy(validation_result->claims->subject, internal_result->claims->subject, subject_len);
         }
     }
     
     // Copy issuer
     if (internal_result->claims->issuer) {
         size_t issuer_len = strlen(internal_result->claims->issuer) + 1;
         validation_result->claims->issuer = polycall_core_malloc(core_ctx, issuer_len);
         if (validation_result->claims->issuer) {
             memcpy(validation_result->claims->issuer, internal_result->claims->issuer, issuer_len);
         }
     }
     
     // Copy audience
     if (internal_result->claims->audience) {
         size_t audience_len = strlen(internal_result->claims->audience) + 1;
         validation_result->claims->audience = polycall_core_malloc(core_ctx, audience_len);
         if (validation_result->claims->audience) {
             memcpy(validation_result->claims->audience, internal_result->claims->audience, audience_len);
         }
     }
     
     // Copy token ID
     if (internal_result->claims->token_id) {
         size_t token_id_len = strlen(internal_result->claims->token_id) + 1;
         validation_result->claims->token_id = polycall_core_malloc(core_ctx, token_id_len);
         if (validation_result->claims->token_id) {
             memcpy(validation_result->claims->token_id, internal_result->claims->token_id, token_id_len);
         }
     }
     
     // Copy timestamps
     validation_result->claims->issued_at = internal_result->claims->issued_at;
     validation_result->claims->expires_at = internal_result->claims->expires_at;
     
     // Copy roles if present
     if (internal_result->claims->roles && internal_result->claims->role_count > 0) {
         validation_result->claims->roles = polycall_core_malloc(core_ctx, 
                                           internal_result->claims->role_count * sizeof(char*));
         if (validation_result->claims->roles) {
             validation_result->claims->role_count = internal_result->claims->role_count;
             
             for (size_t i = 0; i < internal_result->claims->role_count; i++) {
                 if (internal_result->claims->roles[i]) {
                     size_t role_len = strlen(internal_result->claims->roles[i]) + 1;
                     validation_result->claims->roles[i] = polycall_core_malloc(core_ctx, role_len);
                     if (validation_result->claims->roles[i]) {
                         memcpy(validation_result->claims->roles[i], 
                                internal_result->claims->roles[i], role_len);
                     }
                 } else {
                     validation_result->claims->roles[i] = NULL;
                 }
             }
         }
     }
     
     // Copy scopes if present
     if (internal_result->claims->scopes && internal_result->claims->scope_count > 0) {
         validation_result->claims->scopes = polycall_core_malloc(core_ctx, 
                                            internal_result->claims->scope_count * sizeof(char*));
         if (validation_result->claims->scopes) {
             validation_result->claims->scope_count = internal_result->claims->scope_count;
             
             for (size_t i = 0; i < internal_result->claims->scope_count; i++) {
                 if (internal_result->claims->scopes[i]) {
                     size_t scope_len = strlen(internal_result->claims->scopes[i]) + 1;
                     validation_result->claims->scopes[i] = polycall_core_malloc(core_ctx, scope_len);
                     if (validation_result->claims->scopes[i]) {
                         memcpy(validation_result->claims->scopes[i], 
                                internal_result->claims->scopes[i], scope_len);
                     }
                 } else {
                     validation_result->claims->scopes[i] = NULL;
                 }
             }
         }
     }
     
     // Copy device info if present
     if (internal_result->claims->device_info) {
         size_t device_info_len = strlen(internal_result->claims->device_info) + 1;
         validation_result->claims->device_info = polycall_core_malloc(core_ctx, device_info_len);
         if (validation_result->claims->device_info) {
             memcpy(validation_result->claims->device_info, internal_result->claims->device_info, 
                    device_info_len);
         }
     }
     
     // Copy custom claims if present
     if (internal_result->claims->custom_claims) {
         size_t custom_claims_len = strlen(internal_result->claims->custom_claims) + 1;
         validation_result->claims->custom_claims = polycall_core_malloc(core_ctx, custom_claims_len);
         if (validation_result->claims->custom_claims) {
             memcpy(validation_result->claims->custom_claims, internal_result->claims->custom_claims, 
                    custom_claims_len);
         }
     }
     
     // Clean up internal result
     if (internal_result->claims) {
         free(internal_result->claims->subject);
         free(internal_result->claims->issuer);
         free(internal_result->claims->audience);
         free(internal_result->claims->token_id);
         
         if (internal_result->claims->roles) {
             for (size_t i = 0; i < internal_result->claims->role_count; i++) {
                 free(internal_result->claims->roles[i]);
             }
             free(internal_result->claims->roles);
         }
         
         if (internal_result->claims->scopes) {
             for (size_t i = 0; i < internal_result->claims->scope_count; i++) {
                 free(internal_result->claims->scopes[i]);
             }
             free(internal_result->claims->scopes);
         }
         
         free(internal_result->claims->device_info);
         free(internal_result->claims->custom_claims);
         free(internal_result->claims);
     }
     
     free(internal_result->error_message);
     free(internal_result);
     
     // Create audit event
     if (validation_result->claims && validation_result->claims->subject) {
         audit_event_t* event = polycall_auth_create_audit_event(
             core_ctx,
             POLYCALL_AUDIT_EVENT_TOKEN_VALIDATE,
             validation_result->claims->subject,
             NULL,
             NULL,
             true,
             NULL
         );
         
         if (event) {
             polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
             polycall_auth_free_audit_event(core_ctx, event);
         }
     }
     
     *result = validation_result;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Introspect a token (get detailed information)
  */
 polycall_core_error_t polycall_auth_introspect_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token,
     token_claims_t** claims
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !token || !claims) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Validate token first
     token_validation_result_t* result = NULL;
     polycall_core_error_t error = polycall_auth_validate_token_ex(core_ctx, auth_ctx, token, &result);
     if (error != POLYCALL_CORE_SUCCESS) {
         return error;
     }
     
     // Check if token is valid
     if (!result || !result->is_valid || !result->claims) {
         if (result) {
             polycall_auth_free_token_validation_result(core_ctx, result);
         }
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Return claims (ownership is transferred to caller)
     *claims = result->claims;
     result->claims = NULL;
     
     // Free validation result
     polycall_auth_free_token_validation_result(core_ctx, result);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Generate an API key
  */
 polycall_core_error_t polycall_auth_generate_api_key(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* name,
     const char** scopes,
     size_t scope_count,
     uint32_t expiry_days,
     char** api_key
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !identity_id || !api_key) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Generate custom claims with name
     char custom_claims[256] = {0};
     if (name) {
         snprintf(custom_claims, sizeof(custom_claims), "{\"name\":\"%s\",\"type\":\"api_key\"}", name);
     } else {
         snprintf(custom_claims, sizeof(custom_claims), "{\"type\":\"api_key\"}");
     }
     
     // Calculate expiry time
     uint64_t expiry_time = 0;
     uint64_t now = get_current_timestamp();
     
     if (expiry_days > 0) {
         expiry_time = now + (expiry_days * 24 * 60 * 60);
     } else {
         // Default to 365 days
         expiry_time = now + (365 * 24 * 60 * 60);
     }
     
     // Generate token
     char* token = generate_token(auth_ctx->token_service, identity_id, POLYCALL_TOKEN_TYPE_API_KEY, expiry_time);
     if (!token) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_TOKEN_ISSUE,
         identity_id,
         NULL,
         "api_key",
         true,
         NULL
     );
     
     if (event) {
         // Add API key name to details if available
         if (name) {
             char details[256] = {0};
             snprintf(details, sizeof(details), "{\"name\":\"%s\"}", name);
             event->details = strdup(details);
         }
         
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     *api_key = token;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Revoke a token
  */
 polycall_core_error_t polycall_auth_revoke_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token
 ) {
     // Validate parameters
     if (!core_ctx || !auth_ctx || !token) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Validate token first to get identity
     token_validation_result_t* result = NULL;
     polycall_core_error_t error = polycall_auth_validate_token_ex(core_ctx, auth_ctx, token, &result);
     if (error != POLYCALL_CORE_SUCCESS) {
         return error;
     }
     
     // Check if token is valid
     if (!result || !result->is_valid || !result->claims) {
         if (result) {
             polycall_auth_free_token_validation_result(core_ctx, result);
         }
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get identity ID from claims
     char* identity_id = NULL;
     if (result->claims->subject) {
         identity_id = result->claims->subject;
     }
     
     // Lock token service
     pthread_mutex_lock(&auth_ctx->token_service->mutex);
     
     // Find token in registry
     bool found = false;
     for (size_t i = 0; i < auth_ctx->token_service->token_count; i++) {
         token_entry_t* entry = auth_ctx->token_service->tokens[i];
         if (entry && entry->token && strcmp(entry->token, token) == 0) {
             // Mark token as revoked
             entry->is_revoked = true;
             found = true;
             break;
         }
     }
     
     pthread_mutex_unlock(&auth_ctx->token_service->mutex);
     
     // Create audit event
     audit_event_t* event = polycall_auth_create_audit_event(
         core_ctx,
         POLYCALL_AUDIT_EVENT_TOKEN_REVOKE,
         identity_id,
         NULL,
         NULL,
         found,
         found ? NULL : "Token not found in registry"
     );
     
     if (event) {
         polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
         polycall_auth_free_audit_event(core_ctx, event);
     }
     
     // Free validation result
     polycall_auth_free_token_validation_result(core_ctx, result);
     
     return found ? POLYCALL_CORE_SUCCESS : POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 /**
  * @brief Free token validation result
  */
 void polycall_auth_free_token_validation_result(
     polycall_core_context_t* core_ctx,
     token_validation_result_t* result
 ) {
     if (!core_ctx || !result) {
         return;
     }
     
     // Free error message
     if (result->error_message) {
         polycall_core_free(core_ctx, result->error_message);
     }
     
     // Free claims if present
     if (result->claims) {
         polycall_auth_free_token_claims(core_ctx, result->claims);
     }
     
     // Free result itself
     polycall_core_free(core_ctx, result);
 }
 
 /**
  * @brief Free token claims
  */
 void polycall_auth_free_token_claims(
     polycall_core_context_t* core_ctx,
     token_claims_t* claims
 ) {
     if (!core_ctx || !claims) {
         return;
     }
     
     // Free strings
     if (claims->subject) {
         polycall_core_free(core_ctx, claims->subject);
     }
     
     if (claims->issuer) {
         polycall_core_free(core_ctx, claims->issuer);
     }
     
     if (claims->audience) {
         polycall_core_free(core_ctx, claims->audience);
     }
     
     if (claims->token_id) {
         polycall_core_free(core_ctx, claims->token_id);
     }
     
     // Free roles
     if (claims->roles) {
         for (size_t i = 0; i < claims->role_count; i++) {
             if (claims->roles[i]) {
                 polycall_core_free(core_ctx, claims->roles[i]);
             }
         }
         polycall_core_free(core_ctx, claims->roles);
     }
     
     // Free scopes
     if (claims->scopes) {
         for (size_t i = 0; i < claims->scope_count; i++) {
             if (claims->scopes[i]) {
                 polycall_core_free(core_ctx, claims->scopes[i]);
             }
         }
         polycall_core_free(core_ctx, claims->scopes);
     }
     
     // Free other fields
     if (claims->device_info) {
         polycall_core_free(core_ctx, claims->device_info);
     }
     
     if (claims->custom_claims) {
         polycall_core_free(core_ctx, claims->custom_claims);
     }
     
     // Free claims structure
     polycall_core_free(core_ctx, claims);
 }
 
 /**
  * @brief Create a token ID
  */
 static char* create_token_id(polycall_token_type_t type) {
     // Create a token ID with prefix based on type and random suffix
     char prefix = 'X'; // Default prefix
     
     switch (type) {
         case POLYCALL_TOKEN_TYPE_ACCESS:
             prefix = 'A';
             break;
         case POLYCALL_TOKEN_TYPE_REFRESH:
             prefix = 'R';
             break;
         case POLYCALL_TOKEN_TYPE_API_KEY:
             prefix = 'K';
             break;
         default:
             prefix = 'X';
             break;
     }
     
     // Generate random bytes for suffix
     unsigned char random_bytes[16];
     for (int i = 0; i < 16; i++) {
         random_bytes[i] = rand() % 256;
     }
     
     // Convert to hex
     char* token_id = malloc(34); // 1 char prefix + 32 hex chars + null terminator
     if (!token_id) {
         return NULL;
     }
     
     token_id[0] = prefix;
     
     // Convert bytes to hex
     for (int i = 0; i < 16; i++) {
         sprintf(token_id + 1 + i * 2, "%02x", random_bytes[i]);
     }
     
     token_id[33] = '\0';
     
     return token_id;
 }
 
 /**
  * @brief HMAC-SHA256 implementation
  * Note: In production, use a proper cryptographic library like OpenSSL
  */
 static char* hmac_sha256(const char* key, const char* message) {
     // TODO: Replace with real HMAC-SHA256 using OpenSSL
     // This is a placeholder that simply concatenates key and message
     // For the sake of implementation structure
     
     size_t key_len = strlen(key);
     size_t message_len = strlen(message);
     size_t result_len = key_len + message_len + 1;
     
     char* result = malloc(result_len);
     if (!result) {
         return NULL;
     }
     
     strcpy(result, key);
     strcat(result, message);
     
     return result;
 }
 
 /**
  * @brief Base64 encoding implementation
  * Note: In production, use a proper Base64 library
  */
 static char* base64_encode(const unsigned char* data, size_t input_length) {
     // TODO: Replace with real Base64 encoding
     // This is a placeholder that simply returns a copy of the data
     
     char* result = malloc(input_length + 1);
     if (!result) {
         return NULL;
     }
     
     memcpy(result, data, input_length);
     result[input_length] = '\0';
     
     return result;
 }
 
 /**
  * @brief Base64 decoding implementation
  * Note: In production, use a proper Base64 library
  */
 static unsigned char* base64_decode(const char* data, size_t* output_length) {
     // TODO: Replace with real Base64 decoding
     // This is a placeholder that simply returns a copy of the data
     
     size_t input_length = strlen(data);
     unsigned char* result = malloc(input_length + 1);
     if (!result) {
         return NULL;
     }
     
     memcpy(result, data, input_length);
     result[input_length] = '\0';
     
     if (output_length) {
         *output_length = input_length;
     }
     
     return result;
 }
 
 /**
  * @brief Free token entry
  */
 static void free_token_entry(polycall_core_context_t* core_ctx, token_entry_t* entry) {
     if (!core_ctx || !entry) {
         return;
     }
     
     if (entry->token) {
         polycall_core_free(core_ctx, entry->token);
     }
     
     if (entry->identity_id) {
         polycall_core_free(core_ctx, entry->identity_id);
     }
     
     polycall_core_free(core_ctx, entry);
 }
