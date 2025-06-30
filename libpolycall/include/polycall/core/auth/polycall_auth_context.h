/**
 * @file polycall_auth_context.h
 * @brief Authentication and authorization context for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the authentication context that manages identity, credentials,
 * and authorization for LibPolyCall components.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_CONTEXT_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONTEXT_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <string.h>
 #include <time.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* Forward declarations */
 typedef struct polycall_auth_context polycall_auth_context_t;
 typedef struct identity_registry identity_registry_t;
 typedef struct credential_store credential_store_t;
 typedef struct token_service token_service_t;
 typedef struct auth_policy_manager auth_policy_manager_t;
 typedef struct auth_integrator auth_integrator_t;
 typedef struct auth_audit auth_audit_t;
 
 /* Type definitions used across auth modules */
 typedef struct identity_attributes {
     char* name;                          /**< Identity name */
     char* email;                         /**< Email address */
     char** roles;                        /**< Array of roles */
     size_t role_count;                   /**< Number of roles */
     char** groups;                       /**< Array of groups */
     size_t group_count;                  /**< Number of groups */
     uint64_t created_timestamp;          /**< Creation timestamp */
     uint64_t last_login_timestamp;       /**< Last login timestamp */
     bool is_active;                      /**< Whether the identity is active */
     char* metadata;                      /**< Additional metadata in JSON format */
 } identity_attributes_t;
 
 /**
  * @brief Token type enumeration
  */
 typedef enum {
     POLYCALL_TOKEN_TYPE_ACCESS = 0,      /**< Access token */
     POLYCALL_TOKEN_TYPE_REFRESH,         /**< Refresh token */
     POLYCALL_TOKEN_TYPE_API_KEY          /**< API key */
 } polycall_token_type_t;
 
 /**
  * @brief Policy effect enumeration
  */
 typedef enum {
     POLYCALL_POLICY_EFFECT_ALLOW = 0,    /**< Allow effect */
     POLYCALL_POLICY_EFFECT_DENY          /**< Deny effect */
 } polycall_policy_effect_t;
 
 /**
  * @brief Policy statement structure
  */
 typedef struct policy_statement {
     polycall_policy_effect_t effect;     /**< Policy effect */
     char** actions;                      /**< Array of actions */
     size_t action_count;                 /**< Number of actions */
     char** resources;                    /**< Array of resources */
     size_t resource_count;               /**< Number of resources */
     char* condition;                     /**< Condition in JSON format */
 } policy_statement_t;
 
 /**
  * @brief Audit event type enumeration
  */
 typedef enum {
     POLYCALL_AUDIT_EVENT_LOGIN = 0,          /**< Login event */
     POLYCALL_AUDIT_EVENT_LOGOUT,             /**< Logout event */
     POLYCALL_AUDIT_EVENT_TOKEN_ISSUE,        /**< Token issue event */
     POLYCALL_AUDIT_EVENT_TOKEN_VALIDATE,     /**< Token validation event */
     POLYCALL_AUDIT_EVENT_TOKEN_REFRESH,      /**< Token refresh event */
     POLYCALL_AUDIT_EVENT_TOKEN_REVOKE,       /**< Token revocation event */
     POLYCALL_AUDIT_EVENT_ACCESS_DENIED,      /**< Access denied event */
     POLYCALL_AUDIT_EVENT_ACCESS_GRANTED,     /**< Access granted event */
     POLYCALL_AUDIT_EVENT_IDENTITY_CREATE,    /**< Identity creation event */
     POLYCALL_AUDIT_EVENT_IDENTITY_UPDATE,    /**< Identity update event */
     POLYCALL_AUDIT_EVENT_IDENTITY_DELETE,    /**< Identity deletion event */
     POLYCALL_AUDIT_EVENT_PASSWORD_CHANGE,    /**< Password change event */
     POLYCALL_AUDIT_EVENT_PASSWORD_RESET,     /**< Password reset event */
     POLYCALL_AUDIT_EVENT_ROLE_ASSIGN,        /**< Role assignment event */
     POLYCALL_AUDIT_EVENT_ROLE_REMOVE,        /**< Role removal event */
     POLYCALL_AUDIT_EVENT_POLICY_CREATE,      /**< Policy creation event */
     POLYCALL_AUDIT_EVENT_POLICY_UPDATE,      /**< Policy update event */
     POLYCALL_AUDIT_EVENT_POLICY_DELETE,      /**< Policy deletion event */
     POLYCALL_AUDIT_EVENT_CUSTOM              /**< Custom event */
 } polycall_audit_event_type_t;
 
 /**
  * @brief Audit event structure
  */
 typedef struct audit_event {
     polycall_audit_event_type_t type;    /**< Event type */
     uint64_t timestamp;                  /**< Event timestamp */
     char* identity_id;                   /**< Identity ID */
     char* resource;                      /**< Resource */
     char* action;                        /**< Action */
     bool success;                        /**< Whether the event was successful */
     char* error_message;                 /**< Error message if unsuccessful */
     char* source_ip;                     /**< Source IP address */
     char* user_agent;                    /**< User agent */
     char* details;                       /**< Additional details in JSON format */
 } audit_event_t;
 
 /**
  * @brief Audit query structure
  */
 typedef struct audit_query {
     uint64_t start_timestamp;            /**< Start timestamp */
     uint64_t end_timestamp;              /**< End timestamp */
     char* identity_id;                   /**< Identity ID filter */
     polycall_audit_event_type_t event_type; /**< Event type filter */
     bool filter_success;                 /**< Whether to filter by success */
     bool success_value;                  /**< Success value to filter */
     char* resource;                      /**< Resource filter */
     char* action;                        /**< Action filter */
     size_t limit;                        /**< Maximum number of results */
     size_t offset;                       /**< Result offset */
 } audit_query_t;
 
 /**
  * @brief Token validation result
  */
 typedef struct token_validation_result {
     bool is_valid;                       /**< Whether the token is valid */
     void* claims;                        /**< Token claims if valid (opaque pointer) */
     char* error_message;                 /**< Error message if invalid */
 } token_validation_result_t;
 
 /**
  * @brief Audit entry
  */
 typedef struct audit_entry {
     audit_event_t event;                 /**< Audit event */
     uint64_t log_timestamp;              /**< Logging timestamp */
 } audit_entry_t;
 
 /**
  * @brief Configuration for the auth context
  */
 typedef struct {
     bool enable_token_validation;            /**< Enable token validation */
     bool enable_access_control;              /**< Enable access control */
     bool enable_audit_logging;               /**< Enable audit logging */
     uint32_t token_validity_period_sec;      /**< Token validity period in seconds */
     uint32_t refresh_token_validity_sec;     /**< Refresh token validity in seconds */
     bool enable_credential_hashing;          /**< Enable credential hashing */
     const char* token_signing_secret;        /**< Secret for token signing */
     uint32_t flags;                          /**< Additional configuration flags */
     void* user_data;                         /**< User data */
 } polycall_auth_config_t;
 
 /**
  * @brief Initialize the authentication context
  *
  * @param core_ctx Core context
  * @param auth_ctx Pointer to receive the auth context
  * @param config Configuration for the auth context
  * @return Error code
  */
 polycall_core_error_t polycall_auth_init(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t** auth_ctx,
     const polycall_auth_config_t* config
 );
 
 /**
  * @brief Clean up the authentication context
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context to clean up
  */
 void polycall_auth_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx
 );
 
 /**
  * @brief Get the identity for the current operation
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Pointer to receive the identity ID
  * @return Error code
  */
 polycall_core_error_t polycall_auth_get_current_identity(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     char** identity_id
 );
 
 /**
  * @brief Authenticate a user with username and password
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param username Username
  * @param password Password
  * @param access_token Pointer to receive the access token
  * @param refresh_token Pointer to receive the refresh token
  * @return Error code
  */
 polycall_core_error_t polycall_auth_authenticate(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* username,
     const char* password,
     char** access_token,
     char** refresh_token
 );
 
 /**
  * @brief Validate an access token
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param token Token to validate
  * @param identity_id Pointer to receive the identity ID associated with the token
  * @return Error code
  */
 polycall_core_error_t polycall_auth_validate_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token,
     char** identity_id
 );
 
 /**
  * @brief Refresh an access token using a refresh token
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param refresh_token Refresh token
  * @param access_token Pointer to receive the new access token
  * @return Error code
  */
 polycall_core_error_t polycall_auth_refresh_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* refresh_token,
     char** access_token
 );
 
 /**
  * @brief Revoke a token
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param token Token to revoke
  * @return Error code
  */
 polycall_core_error_t polycall_auth_revoke_token(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* token
 );
 
 /**
  * @brief Check if an identity has permission for a resource and action
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param identity_id Identity ID
  * @param resource Resource to access
  * @param action Action to perform
  * @param allowed Pointer to receive whether the action is allowed
  * @return Error code
  */
 polycall_core_error_t polycall_auth_check_permission(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const char* identity_id,
     const char* resource,
     const char* action,
     bool* allowed
 );
 
 /**
  * @brief Create a default auth configuration
  *
  * @return Default configuration
  */
 polycall_auth_config_t polycall_auth_create_default_config(void);
 
 // Define error sources
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONTEXT_H_H
 
 // Internal structures for context components
 
 /**
  * @brief Identity registry structure
  */
 struct identity_registry {
     polycall_core_context_t* core_ctx;  // Core context
     size_t capacity;                    // Registry capacity
     size_t count;                       // Number of identities
     char** identity_ids;                // Array of identity IDs
     identity_attributes_t** attributes; // Array of identity attributes
     char** hashed_passwords;            // Array of hashed passwords
     pthread_mutex_t mutex;              // Thread safety mutex
 };
 
 /**
  * @brief Credential store structure
  */
 struct credential_store {
     polycall_core_context_t* core_ctx;  // Core context
     bool enable_hashing;                // Whether to hash credentials
     char* salt;                         // Salt for hashing
     uint32_t hash_iterations;           // Hash iterations
     pthread_mutex_t mutex;              // Thread safety mutex
 };
 
 /**
  * @brief Token store entry
  */
 typedef struct {
     char* token;                        // Token string
     char* identity_id;                  // Associated identity ID
     polycall_token_type_t type;         // Token type
     uint64_t issued_at;                 // Issued at timestamp
     uint64_t expires_at;                // Expires at timestamp
     bool is_revoked;                    // Whether the token is revoked
 } token_entry_t;
 
 /**
  * @brief Token service structure
  */
 struct token_service {
     polycall_core_context_t* core_ctx;  // Core context
     char* signing_secret;               // Secret for token signing
     uint32_t access_token_validity;     // Access token validity in seconds
     uint32_t refresh_token_validity;    // Refresh token validity in seconds
     token_entry_t** tokens;             // Array of token entries
     size_t token_count;                 // Number of tokens
     size_t token_capacity;              // Token array capacity
     pthread_mutex_t mutex;              // Thread safety mutex
 };
 
 /**
  * @brief Role entry
  */
 typedef struct {
     char* name;                        // Role name
     char* description;                 // Role description
     char** policy_names;               // Array of attached policy names
     size_t policy_count;               // Number of attached policies
 } role_entry_t;
 
 /**
  * @brief Policy entry
  */
 typedef struct {
     char* name;                        // Policy name
     char* description;                 // Policy description
     policy_statement_t** statements;   // Array of policy statements
     size_t statement_count;            // Number of statements
 } policy_entry_t;
 
 /**
  * @brief Auth policy manager structure
  */
 struct auth_policy_manager {
     polycall_core_context_t* core_ctx;  // Core context
     role_entry_t** roles;               // Array of roles
     size_t role_count;                  // Number of roles
     size_t role_capacity;               // Role array capacity
     policy_entry_t** policies;          // Array of policies
     size_t policy_count;                // Number of policies
     size_t policy_capacity;             // Policy array capacity
     pthread_mutex_t mutex;              // Thread safety mutex
 };
 
 /**
  * @brief Auth audit structure
  */
 struct auth_audit {
     polycall_core_context_t* core_ctx;  // Core context
     audit_entry_t** entries;            // Array of audit entries
     size_t entry_count;                 // Number of entries
     size_t entry_capacity;              // Entry array capacity
     bool enable_logging;                // Whether logging is enabled
     pthread_mutex_t mutex;              // Thread safety mutex
 };
 
 /**
  * @brief Subsystem hook
  */
 typedef struct {
     void* context;                     // Subsystem context
     void* callback;                    // Callback function
 } subsystem_hook_t;
 
 /**
  * @brief Auth integrator structure
  */
 struct auth_integrator {
     polycall_core_context_t* core_ctx;  // Core context
     subsystem_hook_t protocol_hook;     // Protocol system hook
     subsystem_hook_t micro_hook;        // Micro command system hook
     subsystem_hook_t edge_hook;         // Edge command system hook
     subsystem_hook_t telemetry_hook;    // Telemetry system hook
 };
 
 /**
  * @brief Authentication context structure
  */
 struct polycall_auth_context {
     polycall_core_context_t* core_ctx;  // Core context
     identity_registry_t* identities;    // Identity registry
     credential_store_t* credentials;    // Credential store
     token_service_t* token_service;     // Token service
     auth_policy_manager_t* policies;    // Policy manager
     auth_integrator_t* integrator;      // Subsystem integrator
     auth_audit_t* auth_audit;           // Audit system
     polycall_auth_config_t config;      // Configuration
     char* current_identity;             // Current operation identity
 };
 
 // Forward declarations for internal functions
 static identity_registry_t* init_identity_registry(polycall_core_context_t* ctx, size_t capacity);
 static void cleanup_identity_registry(polycall_core_context_t* ctx, identity_registry_t* registry);
 static credential_store_t* init_credential_store(polycall_core_context_t* ctx, bool enable_hashing);
 static void cleanup_credential_store(polycall_core_context_t* ctx, credential_store_t* store);
 static token_service_t* init_token_service(polycall_core_context_t* ctx, const char* signing_secret,
                                          uint32_t access_validity, uint32_t refresh_validity);
 static void cleanup_token_service(polycall_core_context_t* ctx, token_service_t* service);
 static auth_policy_manager_t* init_policy_manager(polycall_core_context_t* ctx);
 static void cleanup_policy_manager(polycall_core_context_t* ctx, auth_policy_manager_t* manager);
 static auth_audit_t* init_auth_audit(polycall_core_context_t* ctx, bool enable_logging);
 static void cleanup_auth_audit(polycall_core_context_t* ctx, auth_audit_t* audit);
 static auth_integrator_t* init_auth_integrator(polycall_core_context_t* ctx);
 static void cleanup_auth_integrator(polycall_core_context_t* ctx, auth_integrator_t* integrator);
 static char* hash_password(credential_store_t* store, const char* password);
 static bool verify_password(credential_store_t* store, const char* password, const char* hash);
 static char* generate_token(token_service_t* service, const char* identity_id,
                           polycall_token_type_t type, uint64_t expiry_time);
 static token_validation_result_t* validate_token_internal(token_service_t* service, const char* token);
 static char* calculate_hmac(const char* key, const char* message);
 static uint64_t get_current_timestamp(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_CONTEXT_H_H */