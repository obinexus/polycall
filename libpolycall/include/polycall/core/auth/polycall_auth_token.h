/**
 * @file polycall_auth_token.h
 * @brief Token management for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the token management interfaces for LibPolyCall authentication.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_TOKEN_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_TOKEN_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Token claims structure
  */
 typedef struct {
     char* subject;                       /**< Subject (identity ID) */
     char* issuer;                        /**< Issuer */
     uint64_t issued_at;                  /**< Issued at timestamp */
     uint64_t expires_at;                 /**< Expires at timestamp */
     char* audience;                      /**< Audience */
     char* token_id;                      /**< Token ID */
     char** roles;                        /**< Array of roles */
     size_t role_count;                   /**< Number of roles */
     char** scopes;                       /**< Array of scopes */
     size_t scope_count;                  /**< Number of scopes */
     char* device_info;                   /**< Device information */
     char* custom_claims;                 /**< Custom claims in JSON format */
 } token_claims_t;
 
 /**
  * @brief Issue a token
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param token_type Token type
  * @param scopes Array of scopes
  * @param scope_count Number of scopes
  * @param custom_claims Custom claims in JSON format
  * @param token Pointer to receive the token
  * @return Error code
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
 );
 
 /**
  * @brief Validate a token
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param token Token to validate
  * @param result Pointer to receive validation result
  * @return Error code
  */
 polycall_core_error_t polycall_auth_validate_token_ex(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token,
     token_validation_result_t** result
 );
 
 /**
  * @brief Introspect a token (get detailed information)
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param token Token to introspect
  * @param claims Pointer to receive token claims
  * @return Error code
  */
 polycall_core_error_t polycall_auth_introspect_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token,
     token_claims_t** claims
 );
 
 /**
  * @brief Generate an API key
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param name Key name
  * @param scopes Array of scopes
  * @param scope_count Number of scopes
  * @param expiry_days Expiry in days (0 for non-expiring)
  * @param api_key Pointer to receive the API key
  * @return Error code
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
 );
 
 /**
  * @brief Free token validation result
  *
  * @param core_ctx Core context
  * @param result Result to free
  */
 void polycall_auth_free_token_validation_result(
     polycall_core_context_t* core_ctx,
     token_validation_result_t* result
 );
 
 /**
  * @brief Free token claims
  *
  * @param core_ctx Core context
  * @param claims Claims to free
  */
 void polycall_auth_free_token_claims(
     polycall_core_context_t* core_ctx,
     token_claims_t* claims
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_TOKEN_H_H */