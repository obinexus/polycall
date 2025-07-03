/**
 * @file polycall_auth_identity.h
 * @brief Identity management for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the identity management interfaces for LibPolyCall authentication.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_IDENTITY_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_IDENTITY_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Register a new identity
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param attributes Identity attributes
  * @param initial_password Initial password (will be hashed)
  * @return Error code
  */
 polycall_core_error_t polycall_auth_register_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const identity_attributes_t* attributes,
     const char* initial_password
 );
 
 /**
  * @brief Get identity attributes
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param attributes Pointer to receive identity attributes
  * @return Error code
  */
 polycall_core_error_t polycall_auth_get_identity_attributes(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     identity_attributes_t** attributes
 );
 
 /**
  * @brief Update identity attributes
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param attributes New identity attributes
  * @return Error code
  */
 polycall_core_error_t polycall_auth_update_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const identity_attributes_t* attributes
 );
 
 /**
  * @brief Change identity password
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param current_password Current password
  * @param new_password New password
  * @return Error code
  */
 polycall_core_error_t polycall_auth_change_password(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* current_password,
     const char* new_password
 );
 
 /**
  * @brief Reset identity password (administrative function)
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param new_password New password
  * @return Error code
  */
 polycall_core_error_t polycall_auth_reset_password(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* new_password
 );
 
 /**
  * @brief Deactivate an identity
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @return Error code
  */
 polycall_core_error_t polycall_auth_deactivate_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id
 );
 
 /**
  * @brief Reactivate an identity
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @return Error code
  */
 polycall_core_error_t polycall_auth_reactivate_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id
 );
 
 /**
  * @brief Free identity attributes
  *
  * @param core_ctx Core context
  * @param attributes Attributes to free
  */
 void polycall_auth_free_identity_attributes(
     polycall_core_context_t* core_ctx,
     identity_attributes_t* attributes
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_IDENTITY_H_H */