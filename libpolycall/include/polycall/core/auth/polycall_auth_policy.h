/**
 * @file polycall_auth_policy.h
 * @brief Policy management for LibPolyCall authentication
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the policy management interfaces for LibPolyCall authentication.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_POLICY_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_POLICY_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Role structure
  */
 typedef struct {
     char* name;                          /**< Role name */
     char* description;                   /**< Role description */
 } role_t;
 
 /**
  * @brief Policy structure
  */
 typedef struct {
     char* name;                          /**< Policy name */
     char* description;                   /**< Policy description */
     policy_statement_t** statements;     /**< Array of policy statements */
     size_t statement_count;              /**< Number of statements */
 } policy_t;
 
 /**
  * @brief Add a role
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param role Role to add
  * @return Error code
  */
 polycall_core_error_t polycall_auth_add_role(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const role_t* role
 );
 
 /**
  * @brief Assign a role to an identity
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param role_name Role name
  * @return Error code
  */
 polycall_core_error_t polycall_auth_assign_role(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* role_name
 );
 
 /**
  * @brief Remove a role from an identity
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param role_name Role name
  * @return Error code
  */
 polycall_core_error_t polycall_auth_remove_role(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* role_name
 );
 
 /**
  * @brief Add a policy
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param policy Policy to add
  * @return Error code
  */
 polycall_core_error_t polycall_auth_add_policy(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const policy_t* policy
 );
 
 /**
  * @brief Attach a policy to a role
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param role_name Role name
  * @param policy_name Policy name
  * @return Error code
  */
 polycall_core_error_t polycall_auth_attach_policy(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* role_name,
     const char* policy_name
 );
 
 /**
  * @brief Detach a policy from a role
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param role_name Role name
  * @param policy_name Policy name
  * @return Error code
  */
 polycall_core_error_t polycall_auth_detach_policy(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* role_name,
     const char* policy_name
 );
 
 /**
  * @brief Evaluate a permission request
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param resource Resource to access
  * @param action Action to perform
  * @param context Context in JSON format
  * @param allowed Pointer to receive whether the action is allowed
  * @return Error code
  */
 polycall_core_error_t polycall_auth_evaluate_permission(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* resource,
     const char* action,
     const char* context,
     bool* allowed
 );
 
 // Forward declarations for internal functions
 static bool policy_matches_resource(const char* policy_resource, const char* resource);
 static bool policy_matches_action(const char* policy_action, const char* action);
 static bool evaluate_policy_statement(policy_statement_t* statement, const char* resource, 
                                     const char* action, const char* context);
 static polycall_core_error_t get_identity_roles(polycall_core_context_t* core_ctx, 
                                               polycall_auth_context_t* auth_ctx,
                                               const char* identity_id, 
                                               char*** roles, size_t* role_count);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_POLICY_H_H */
 