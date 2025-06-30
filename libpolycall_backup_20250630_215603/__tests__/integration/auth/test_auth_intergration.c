/**
#include "core/integration/auth/test_auth_intergration.h"

 * @file test_auth_integration.c
 * @brief Integration tests for LibPolyCall authentication module
 * @author Integration with Nnamdi Okpala's design (OBINexusComputing)
 */

 #include "core/polycall/auth/polycall_auth_context.h"
 #include "core/polycall/auth/polycall_auth_identity.h"
 #include "core/polycall/auth/polycall_auth_token.h"
 #include "core/polycall/auth/polycall_auth_policy.h"
 #include "core/polycall/polycall_core.h"
 #include "polycall/test/test_framework.h"
 #include <stdio.h>
 #include <string.h>
 #include <pthread.h>
 
 // Test configuration
 #define TEST_USERNAME "test_user"
 #define TEST_PASSWORD "test_password"
 #define TEST_RESOURCE "function:test_function"
 #define TEST_ACTION "execute"
 
 // Test framework setup
 static polycall_core_context_t* core_ctx = NULL;
 static polycall_auth_context_t* auth_ctx = NULL;
 
 // Test setup function
 static bool test_setup(void) {
     // Initialize core context
     polycall_core_config_t core_config = polycall_core_create_default_config();
     if (polycall_core_init(&core_ctx, &core_config) != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to initialize core context\n");
         return false;
     }
     
     // Initialize auth context
     polycall_auth_config_t auth_config = polycall_auth_create_default_config();
     auth_config.token_signing_secret = "test_signing_secret_with_sufficient_length_for_zero_trust";
     
     if (polycall_auth_init(core_ctx, &auth_ctx, &auth_config) != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to initialize auth context\n");
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
     // Register test identity
     identity_attributes_t attributes = {0};
     attributes.name = TEST_USERNAME;
     attributes.email = "test@example.com";
     attributes.is_active = true;
     
     if (polycall_auth_register_identity(core_ctx, auth_ctx, "test_identity", 
                                        &attributes, TEST_PASSWORD) != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to register test identity\n");
         polycall_auth_cleanup(core_ctx, auth_ctx);
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
     // Create test role
     role_t role = {0};
     role.name = "test_role";
     role.description = "Test role for integration tests";
     
     if (polycall_auth_add_role(core_ctx, auth_ctx, &role) != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to add test role\n");
         polycall_auth_cleanup(core_ctx, auth_ctx);
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
     // Assign role to identity
     if (polycall_auth_assign_role(core_ctx, auth_ctx, 
                                 "test_identity", "test_role") != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to assign role to identity\n");
         polycall_auth_cleanup(core_ctx, auth_ctx);
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
     // Create test policy
     policy_statement_t statement = {0};
     statement.effect = POLYCALL_POLICY_EFFECT_ALLOW;
     statement.actions = (char**)malloc(sizeof(char*));
     statement.actions[0] = strdup(TEST_ACTION);
     statement.action_count = 1;
     statement.resources = (char**)malloc(sizeof(char*));
     statement.resources[0] = strdup(TEST_RESOURCE);
     statement.resource_count = 1;
     
     policy_statement_t* statements[1] = { &statement };
     
     policy_t policy = {0};
     policy.name = "test_policy";
     policy.description = "Test policy for integration tests";
     policy.statements = statements;
     policy.statement_count = 1;
     
     if (polycall_auth_add_policy(core_ctx, auth_ctx, &policy) != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to add test policy\n");
         free(statement.actions[0]);
         free(statement.actions);
         free(statement.resources[0]);
         free(statement.resources);
         polycall_auth_cleanup(core_ctx, auth_ctx);
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
     // Attach policy to role
     if (polycall_auth_attach_policy(core_ctx, auth_ctx, 
                                   "test_role", "test_policy") != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to attach policy to role\n");
         free(statement.actions[0]);
         free(statement.actions);
         free(statement.resources[0]);
         free(statement.resources);
         polycall_auth_cleanup(core_ctx, auth_ctx);
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
     // Clean up resources from policy creation
     free(statement.actions[0]);
     free(statement.actions);
     free(statement.resources[0]);
     free(statement.resources);
     
     return true;
 }
 
 // Test teardown function
 static void test_teardown(void) {
     if (auth_ctx) {
         polycall_auth_cleanup(core_ctx, auth_ctx);
         auth_ctx = NULL;
     }
     
     if (core_ctx) {
         polycall_core_cleanup(core_ctx);
         core_ctx = NULL;
     }
 }
 
 // Test authentication
 static bool test_authentication(void) {
     char* access_token = NULL;
     char* refresh_token = NULL;
     
     // Test successful authentication
     polycall_core_error_t result = polycall_auth_authenticate(
         core_ctx, auth_ctx, TEST_USERNAME, TEST_PASSWORD, &access_token, &refresh_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication failed: %d\n", result);
         return false;
     }
     
     if (!access_token || !refresh_token) {
         fprintf(stderr, "Authentication succeeded but tokens are NULL\n");
         return false;
     }
     
     printf("Access token: %s\n", access_token);
     printf("Refresh token: %s\n", refresh_token);
     
     // Test failed authentication with wrong password
     result = polycall_auth_authenticate(
         core_ctx, auth_ctx, TEST_USERNAME, "wrong_password", &access_token, &refresh_token
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication succeeded with wrong password\n");
         return false;
     }
     
     // Test failed authentication with non-existent user
     result = polycall_auth_authenticate(
         core_ctx, auth_ctx, "nonexistent_user", TEST_PASSWORD, &access_token, &refresh_token
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication succeeded with non-existent user\n");
         return false;
     }
     
     // Free allocated memory
     if (access_token) {
         polycall_core_free(core_ctx, access_token);
     }
     
     if (refresh_token) {
         polycall_core_free(core_ctx, refresh_token);
     }
     
     return true;
 }
 
 // Test token validation
 static bool test_token_validation(void) {
     char* access_token = NULL;
     char* refresh_token = NULL;
     
     // Authenticate to get tokens
     polycall_core_error_t result = polycall_auth_authenticate(
         core_ctx, auth_ctx, TEST_USERNAME, TEST_PASSWORD, &access_token, &refresh_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication failed in token validation test: %d\n", result);
         return false;
     }
     
     // Test successful access token validation
     char* identity_id = NULL;
     result = polycall_auth_validate_token(
         core_ctx, auth_ctx, access_token, &identity_id
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Access token validation failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     if (!identity_id) {
         fprintf(stderr, "Token validation succeeded but identity_id is NULL\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     printf("Validated identity: %s\n", identity_id);
     
     // Test failed validation with invalid token
     char invalid_token[256];
     snprintf(invalid_token, sizeof(invalid_token), "%s_invalid", access_token);
     
     result = polycall_auth_validate_token(
         core_ctx, auth_ctx, invalid_token, &identity_id
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Validation succeeded with invalid token\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         polycall_core_free(core_ctx, identity_id);
         return false;
     }
     
     // Free allocated memory
     polycall_core_free(core_ctx, access_token);
     polycall_core_free(core_ctx, refresh_token);
     if (identity_id) {
         polycall_core_free(core_ctx, identity_id);
     }
     
     return true;
 }
 
 // Test token refresh
 static bool test_token_refresh(void) {
     char* access_token = NULL;
     char* refresh_token = NULL;
     
     // Authenticate to get tokens
     polycall_core_error_t result = polycall_auth_authenticate(
         core_ctx, auth_ctx, TEST_USERNAME, TEST_PASSWORD, &access_token, &refresh_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication failed in token refresh test: %d\n", result);
         return false;
     }
     
     // Test successful token refresh
     char* new_access_token = NULL;
     result = polycall_auth_refresh_token(
         core_ctx, auth_ctx, refresh_token, &new_access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Token refresh failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     if (!new_access_token) {
         fprintf(stderr, "Token refresh succeeded but new_access_token is NULL\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     printf("New access token: %s\n", new_access_token);
     
     // Test failed refresh with access token instead of refresh token
     result = polycall_auth_refresh_token(
         core_ctx, auth_ctx, access_token, &new_access_token
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Refresh succeeded with access token\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         polycall_core_free(core_ctx, new_access_token);
         return false;
     }
     
     // Free allocated memory
     polycall_core_free(core_ctx, access_token);
     polycall_core_free(core_ctx, refresh_token);
     polycall_core_free(core_ctx, new_access_token);
     
     return true;
 }
 
 // Test permission checking
 static bool test_permission_checking(void) {
     char* access_token = NULL;
     char* refresh_token = NULL;
     
     // Authenticate to get tokens
     polycall_core_error_t result = polycall_auth_authenticate(
         core_ctx, auth_ctx, TEST_USERNAME, TEST_PASSWORD, &access_token, &refresh_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication failed in permission check test: %d\n", result);
         return false;
     }
     
     // Validate token to get identity ID
     char* identity_id = NULL;
     result = polycall_auth_validate_token(
         core_ctx, auth_ctx, access_token, &identity_id
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Token validation failed in permission check test: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     // Test allowed permission
     bool allowed = false;
     result = polycall_auth_check_permission(
         core_ctx, auth_ctx, identity_id, TEST_RESOURCE, TEST_ACTION, &allowed
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Permission check failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         polycall_core_free(core_ctx, identity_id);
         return false;
     }
     
     if (!allowed) {
         fprintf(stderr, "Permission check should allow access but denied\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         polycall_core_free(core_ctx, identity_id);
         return false;
     }
     
     // Test denied permission
     allowed = false;
     result = polycall_auth_check_permission(
         core_ctx, auth_ctx, identity_id, TEST_RESOURCE, "unauthorized_action", &allowed
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Permission check for unauthorized action failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         polycall_core_free(core_ctx, identity_id);
         return false;
     }
     
     if (allowed) {
         fprintf(stderr, "Permission check should deny access but allowed\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         polycall_core_free(core_ctx, identity_id);
         return false;
     }
     
     // Free allocated memory
     polycall_core_free(core_ctx, access_token);
     polycall_core_free(core_ctx, refresh_token);
     polycall_core_free(core_ctx, identity_id);
     
     return true;
 }
 
 // Test zero-trust security constraints
 static bool test_zero_trust_security(void) {
     // Create a configuration with unsafe settings
     polycall_auth_config_t unsafe_config = polycall_auth_create_default_config();
     unsafe_config.enable_token_validation = false;
     unsafe_config.enable_access_control = false;
     unsafe_config.enable_audit_logging = false;
     unsafe_config.enable_credential_hashing = false;
     unsafe_config.token_validity_period_sec = 86400 * 7; // 7 days, too long
     unsafe_config.token_signing_secret = "short"; // Too short
     
     // Apply zero-trust constraints
     polycall_core_error_t result = polycall_auth_apply_zero_trust_constraints(
         core_ctx, &unsafe_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to apply zero-trust constraints: %d\n", result);
         return false;
     }
     
     // Verify that unsafe settings were corrected
     if (!unsafe_config.enable_token_validation) {
         fprintf(stderr, "Zero-trust failed to enforce token validation\n");
         return false;
     }
     
     if (!unsafe_config.enable_access_control) {
         fprintf(stderr, "Zero-trust failed to enforce access control\n");
         return false;
     }
     
     if (!unsafe_config.enable_audit_logging) {
         fprintf(stderr, "Zero-trust failed to enforce audit logging\n");
         return false;
     }
     
     if (!unsafe_config.enable_credential_hashing) {
         fprintf(stderr, "Zero-trust failed to enforce credential hashing\n");
         return false;
     }
     
     if (unsafe_config.token_validity_period_sec > 3600) {
         fprintf(stderr, "Zero-trust failed to limit token validity period\n");
         return false;
     }
     
     // Clean up
     polycall_auth_cleanup_config(core_ctx, &unsafe_config);
     
     return true;
 }
 
 // Test configuration loading
 static bool test_config_loading(void) {
     // This would typically use a test configuration file
     // For this test, we'll just test the merge functionality
     
     polycall_auth_config_t base_config = polycall_auth_create_default_config();
     base_config.token_signing_secret = "base_signing_secret_with_sufficient_length";
     base_config.token_validity_period_sec = 1800; // 30 minutes
     
     polycall_auth_config_t override_config = polycall_auth_create_default_config();
     override_config.token_validity_period_sec = 900; // 15 minutes
     override_config.token_signing_secret = "override_signing_secret_with_sufficient_length";
     
     polycall_auth_config_t merged_config = {0};
     
     // Test merging configurations
     polycall_core_error_t result = polycall_auth_merge_configs(
         core_ctx, &base_config, &override_config, &merged_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to merge configurations: %d\n", result);
         polycall_auth_cleanup_config(core_ctx, &base_config);
         polycall_auth_cleanup_config(core_ctx, &override_config);
         return false;
     }
     
     // Verify merged configuration
     if (merged_config.token_validity_period_sec != override_config.token_validity_period_sec) {
         fprintf(stderr, "Merged config did not properly override token validity\n");
         polycall_auth_cleanup_config(core_ctx, &base_config);
         polycall_auth_cleanup_config(core_ctx, &override_config);
         polycall_auth_cleanup_config(core_ctx, &merged_config);
         return false;
     }
     
     if (strcmp(merged_config.token_signing_secret, override_config.token_signing_secret) != 0) {
         fprintf(stderr, "Merged config did not properly override token signing secret\n");
         polycall_auth_cleanup_config(core_ctx, &base_config);
         polycall_auth_cleanup_config(core_ctx, &override_config);
         polycall_auth_cleanup_config(core_ctx, &merged_config);
         return false;
     }
     
     // Clean up
     polycall_auth_cleanup_config(core_ctx, &base_config);
     polycall_auth_cleanup_config(core_ctx, &override_config);
     polycall_auth_cleanup_config(core_ctx, &merged_config);
     
     return true;
 }
 
 // Test concurrent token validation
 static bool test_concurrent_validation(void) {
     // Set up for concurrent testing
     static const int THREAD_COUNT = 4;
     static const int ITERATIONS_PER_THREAD = 100;
     
     // Get initial tokens
     char* access_token = NULL;
     char* refresh_token = NULL;
     
     polycall_core_error_t result = polycall_auth_authenticate(
         core_ctx, auth_ctx, TEST_USERNAME, TEST_PASSWORD, &access_token, &refresh_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Authentication failed in concurrent test: %d\n", result);
         return false;
     }
     
     // Thread context structure
     typedef struct {
         int thread_id;
         const char* token;
         int success_count;
         int failure_count;
     } thread_context_t;
     
     // Thread function
     void* validation_thread(void* arg) {
         thread_context_t* ctx = (thread_context_t*)arg;
         
         for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
             char* identity_id = NULL;
             polycall_core_error_t result = polycall_auth_validate_token(
                 core_ctx, auth_ctx, ctx->token, &identity_id
             );
             
             if (result == POLYCALL_CORE_SUCCESS) {
                 ctx->success_count++;
                 if (identity_id) {
                     polycall_core_free(core_ctx, identity_id);
                 }
             } else {
                 ctx->failure_count++;
             }
         }
         
         return NULL;
     }
     
     // Create and start threads
     pthread_t threads[THREAD_COUNT];
     thread_context_t contexts[THREAD_COUNT];
     
     for (int i = 0; i < THREAD_COUNT; i++) {
         contexts[i].thread_id = i;
         contexts[i].token = access_token;
         contexts[i].success_count = 0;
         contexts[i].failure_count = 0;
         
         if (pthread_create(&threads[i], NULL, validation_thread, &contexts[i]) != 0) {
             fprintf(stderr, "Failed to create thread %d\n", i);
             polycall_core_free(core_ctx, access_token);
             polycall_core_free(core_ctx, refresh_token);
             return false;
         }
     }
     
     // Wait for threads to complete
     for (int i = 0; i < THREAD_COUNT; i++) {
         pthread_join(threads[i], NULL);
     }
     
     // Verify results
     int total_success = 0;
     int total_failure = 0;
     
     for (int i = 0; i < THREAD_COUNT; i++) {
         total_success += contexts[i].success_count;
         total_failure += contexts[i].failure_count;
     }
     
     printf("Concurrent validation results: %d successes, %d failures\n", 
            total_success, total_failure);
     
     // Clean up
     polycall_core_free(core_ctx, access_token);
     polycall_core_free(core_ctx, refresh_token);
     
     // All validations should succeed
     return total_success == THREAD_COUNT * ITERATIONS_PER_THREAD && total_failure == 0;
 }
 
 // Main test function
 int main(void) {
     // Initialize test framework
     test_suite_t suite = {0};
     suite.name = "Authentication Integration Tests";
     suite.setup = test_setup;
     suite.teardown = test_teardown;
     
     // Add tests
     test_case_t tests[] = {
         {"Authentication", test_authentication},
         {"Token Validation", test_token_validation},
         {"Token Refresh", test_token_refresh},
         {"Permission Checking", test_permission_checking},
         {"Zero-Trust Security", test_zero_trust_security},
         {"Configuration Loading", test_config_loading},
         {"Concurrent Validation", test_concurrent_validation}
     };
     
     for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
         test_suite_add_test(&suite, &tests[i]);
     }
     
     // Run tests
     bool success = run_test_suite(&suite);
     
     return success ? 0 : 1;
 }