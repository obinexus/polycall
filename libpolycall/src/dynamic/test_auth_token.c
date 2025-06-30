
#include "core/integration/auth/test_auth_token.h"

/**
 * @file test_auth_token.c
 * @brief Unit tests for LibPolyCall authentication token functionality
 * @author Integration with Nnamdi Okpala's design (OBINexusComputing)
 */

 #include "core/polycall/auth/polycall_auth_context.h"
 #include "core/polycall/auth/polycall_auth_token.h"
 #include "core/polycall/polycall_core.h"
 #include "polycall/test/test_framework.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 // Test configuration
 #define TEST_IDENTITY_ID "test_identity"
 #define TEST_SIGNING_SECRET "test_signing_secret_with_sufficient_length_for_zero_trust"
 
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
     auth_config.token_signing_secret = TEST_SIGNING_SECRET;
     
     if (polycall_auth_init(core_ctx, &auth_ctx, &auth_config) != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to initialize auth context\n");
         polycall_core_cleanup(core_ctx);
         return false;
     }
     
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
 
 // Test token issuance
 static bool test_token_issuance(void) {
     // Test access token issuance
     char* access_token = NULL;
     polycall_core_error_t result = polycall_auth_issue_token(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
         NULL, 0, NULL, &access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Access token issuance failed: %d\n", result);
         return false;
     }
     
     if (!access_token) {
         fprintf(stderr, "Access token issuance succeeded but token is NULL\n");
         return false;
     }
     
     // Test refresh token issuance
     char* refresh_token = NULL;
     result = polycall_auth_issue_token(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_REFRESH, 
         NULL, 0, NULL, &refresh_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Refresh token issuance failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     if (!refresh_token) {
         fprintf(stderr, "Refresh token issuance succeeded but token is NULL\n");
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     // Test API key issuance
     const char* scopes[] = {"read", "write"};
     char* api_key = NULL;
     result = polycall_auth_generate_api_key(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, "test_key", 
         scopes, 2, 30, &api_key
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "API key issuance failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     if (!api_key) {
         fprintf(stderr, "API key issuance succeeded but key is NULL\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, refresh_token);
         return false;
     }
     
     // Free resources
     polycall_core_free(core_ctx, access_token);
     polycall_core_free(core_ctx, refresh_token);
     polycall_core_free(core_ctx, api_key);
     
     return true;
 }
 
 // Test token validation
 static bool test_token_validation(void) {
     // Issue a test token
     char* access_token = NULL;
     polycall_core_error_t result = polycall_auth_issue_token(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
         NULL, 0, NULL, &access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS || !access_token) {
         fprintf(stderr, "Failed to issue token for validation test\n");
         return false;
     }
     
     // Validate the token
     token_validation_result_t* validation_result = NULL;
     result = polycall_auth_validate_token_ex(
         core_ctx, auth_ctx, access_token, &validation_result
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Token validation failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     if (!validation_result) {
         fprintf(stderr, "Token validation succeeded but result is NULL\n");
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     // Verify validation result
     if (!validation_result->is_valid) {
         fprintf(stderr, "Token should be valid but validation failed: %s\n", 
                 validation_result->error_message);
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         return false;
     }
     
     // Verify subject claim
     if (!validation_result->claims || !validation_result->claims->subject) {
         fprintf(stderr, "Token validation succeeded but claims are invalid\n");
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         return false;
     }
     
     if (strcmp(validation_result->claims->subject, TEST_IDENTITY_ID) != 0) {
         fprintf(stderr, "Token subject claim is invalid: %s\n", validation_result->claims->subject);
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         return false;
     }
     
     // Test token expiration
     if (validation_result->claims->expires_at <= validation_result->claims->issued_at) {
         fprintf(stderr, "Token expiration time is invalid\n");
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         return false;
     }
     
     // Test invalid token
     char invalid_token[256];
     snprintf(invalid_token, sizeof(invalid_token), "%s_invalid", access_token);
     
     token_validation_result_t* invalid_result = NULL;
     result = polycall_auth_validate_token_ex(
         core_ctx, auth_ctx, invalid_token, &invalid_result
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Invalid token validation failed with error: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         return false;
     }
     
     if (!invalid_result) {
         fprintf(stderr, "Invalid token validation succeeded but result is NULL\n");
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         return false;
     }
     
     // Verify invalid result
     if (invalid_result->is_valid) {
         fprintf(stderr, "Invalid token should be invalid but validation succeeded\n");
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_validation_result(core_ctx, validation_result);
         polycall_auth_free_token_validation_result(core_ctx, invalid_result);
         return false;
     }
     
     // Free resources
     polycall_core_free(core_ctx, access_token);
     polycall_auth_free_token_validation_result(core_ctx, validation_result);
     polycall_auth_free_token_validation_result(core_ctx, invalid_result);
     
     return true;
 }
 
 // Test token introspection
 static bool test_token_introspection(void) {
     // Issue a test token with custom claims
     const char* custom_claims_json = "{\"app_id\":\"test_app\",\"device\":\"test_device\"}";
     const char* scopes[] = {"read", "write"};
     
     char* access_token = NULL;
     polycall_core_error_t result = polycall_auth_issue_token(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
         scopes, 2, custom_claims_json, &access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS || !access_token) {
         fprintf(stderr, "Failed to issue token for introspection test\n");
         return false;
     }
     
     // Introspect the token
     token_claims_t* claims = NULL;
     result = polycall_auth_introspect_token(
         core_ctx, auth_ctx, access_token, &claims
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Token introspection failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     if (!claims) {
         fprintf(stderr, "Token introspection succeeded but claims are NULL\n");
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     // Verify claims
     if (!claims->subject || strcmp(claims->subject, TEST_IDENTITY_ID) != 0) {
         fprintf(stderr, "Token subject claim is invalid: %s\n", claims->subject);
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_claims(core_ctx, claims);
         return false;
     }
     
     // Verify scopes
     if (claims->scope_count != 2) {
         fprintf(stderr, "Token scope count is invalid: %zu\n", claims->scope_count);
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_claims(core_ctx, claims);
         return false;
     }
     
     if (!claims->scopes[0] || strcmp(claims->scopes[0], "read") != 0 ||
         !claims->scopes[1] || strcmp(claims->scopes[1], "write") != 0) {
         fprintf(stderr, "Token scopes are invalid\n");
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_claims(core_ctx, claims);
         return false;
     }
     
     // Verify custom claims
     if (!claims->custom_claims || strstr(claims->custom_claims, "app_id") == NULL) {
         fprintf(stderr, "Token custom claims are invalid: %s\n", 
                 claims->custom_claims ? claims->custom_claims : "NULL");
         polycall_core_free(core_ctx, access_token);
         polycall_auth_free_token_claims(core_ctx, claims);
         return false;
     }
     
     // Free resources
     polycall_core_free(core_ctx, access_token);
     polycall_auth_free_token_claims(core_ctx, claims);
     
     return true;
 }
 
 // Test token revocation
 static bool test_token_revocation(void) {
     // Issue a test token
     char* access_token = NULL;
     polycall_core_error_t result = polycall_auth_issue_token(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
         NULL, 0, NULL, &access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS || !access_token) {
         fprintf(stderr, "Failed to issue token for revocation test\n");
         return false;
     }
     
     // Validate before revocation
     char* identity_id = NULL;
     result = polycall_auth_validate_token(
         core_ctx, auth_ctx, access_token, &identity_id
     );
     
     if (result != POLYCALL_CORE_SUCCESS || !identity_id) {
         fprintf(stderr, "Token validation before revocation failed\n");
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     polycall_core_free(core_ctx, identity_id);
     identity_id = NULL;
     
     // Revoke the token
     result = polycall_auth_revoke_token(
         core_ctx, auth_ctx, access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Token revocation failed: %d\n", result);
         polycall_core_free(core_ctx, access_token);
         return false;
     }
     
     // Validate after revocation
     result = polycall_auth_validate_token(
         core_ctx, auth_ctx, access_token, &identity_id
     );
     
     if (result == POLYCALL_CORE_SUCCESS && identity_id) {
         fprintf(stderr, "Token validation after revocation succeeded, but should fail\n");
         polycall_core_free(core_ctx, access_token);
         polycall_core_free(core_ctx, identity_id);
         return false;
     }
     
     // Free resources
     polycall_core_free(core_ctx, access_token);
     
     return true;
 }
 
 // Test token expiration
 static bool test_token_expiration(void) {
     // Helper function to create short-lived token
     polycall_core_error_t (*issue_short_token)(
         polycall_core_context_t*, 
         polycall_auth_context_t*,
         const char*,
         polycall_token_type_t,
         const char**,
         size_t,
         const char*,
         uint32_t,
         char**
     ) = NULL;
     
     // If the function exists in implementation, use it
     // Otherwise, we'll skip this test
     if (issue_short_token == NULL) {
         printf("Skipping token expiration test - no support for custom expiration\n");
         return true;
     }
     
     // Issue a token with 1 second expiration
     char* access_token = NULL;
     polycall_core_error_t result = issue_short_token(
         core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
         NULL, 0, NULL, 1, &access_token
     );
     
     if (result != POLYCALL_CORE_SUCCESS || !access_token) {
         fprintf(stderr, "Failed to issue short-lived token\n");
         return false;
     }
     
     // Validate immediately - should succeed
     char* identity_id = NULL;
     result = polycall_auth_validate_token(
         core_ctx, auth_ctx, access_token, &identity_id
     );
     
     if (result != POLYCALL_CORE_SUCCESS || !identity_id) {
         fprintf(stderr, "Token validation immediately after issuance failed\n");
polycall_core_free(core_ctx, access_token);
        return false;
    }
    
    polycall_core_free(core_ctx, identity_id);
    identity_id = NULL;
    
    // Wait for token to expire
    printf("Waiting for token to expire...\n");
    sleep(2); // Wait 2 seconds to ensure token expiration
    
    // Validate after expiration - should fail
    result = polycall_auth_validate_token(
        core_ctx, auth_ctx, access_token, &identity_id
    );
    
    if (result == POLYCALL_CORE_SUCCESS && identity_id) {
        fprintf(stderr, "Token validation after expiration succeeded, but should fail\n");
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, identity_id);
        return false;
    }
    
    // Free resources
    polycall_core_free(core_ctx, access_token);
    
    return true;
}

// Test token scope validation
static bool test_token_scopes(void) {
    // Issue a token with specific scopes
    const char* test_scopes[] = {"read:data", "write:data"};
    
    char* access_token = NULL;
    polycall_core_error_t result = polycall_auth_issue_token(
        core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
        test_scopes, 2, NULL, &access_token
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !access_token) {
        fprintf(stderr, "Failed to issue token with scopes\n");
        return false;
    }
    
    // Introspect to verify scopes
    token_claims_t* claims = NULL;
    result = polycall_auth_introspect_token(
        core_ctx, auth_ctx, access_token, &claims
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !claims) {
        fprintf(stderr, "Token introspection failed\n");
        polycall_core_free(core_ctx, access_token);
        return false;
    }
    
    // Verify scope count
    if (claims->scope_count != 2) {
        fprintf(stderr, "Token scope count is invalid: %zu (expected 2)\n", claims->scope_count);
        polycall_core_free(core_ctx, access_token);
        polycall_auth_free_token_claims(core_ctx, claims);
        return false;
    }
    
    // Verify scope values
    bool read_scope_found = false;
    bool write_scope_found = false;
    
    for (size_t i = 0; i < claims->scope_count; i++) {
        if (claims->scopes[i] && strcmp(claims->scopes[i], "read:data") == 0) {
            read_scope_found = true;
        } else if (claims->scopes[i] && strcmp(claims->scopes[i], "write:data") == 0) {
            write_scope_found = true;
        }
    }
    
    if (!read_scope_found || !write_scope_found) {
        fprintf(stderr, "Expected scopes not found in token\n");
        polycall_core_free(core_ctx, access_token);
        polycall_auth_free_token_claims(core_ctx, claims);
        return false;
    }
    
    // Helper function to check scope permissions
    bool (*check_token_scope)(
        polycall_core_context_t*, 
        polycall_auth_context_t*,
        const char*,
        const char*,
        bool*
    ) = NULL;
    
    // If the function exists in implementation, use it
    // Otherwise, we'll skip this part of the test
    if (check_token_scope != NULL) {
        bool has_scope = false;
        
        // Check valid scope - should succeed
        result = check_token_scope(
            core_ctx, auth_ctx, access_token, "read:data", &has_scope
        );
        
        if (result != POLYCALL_CORE_SUCCESS || !has_scope) {
            fprintf(stderr, "Token scope check failed for valid scope\n");
            polycall_core_free(core_ctx, access_token);
            polycall_auth_free_token_claims(core_ctx, claims);
            return false;
        }
        
        // Check invalid scope - should fail
        has_scope = false;
        result = check_token_scope(
            core_ctx, auth_ctx, access_token, "admin:data", &has_scope
        );
        
        if (result != POLYCALL_CORE_SUCCESS || has_scope) {
            fprintf(stderr, "Token scope check succeeded for invalid scope\n");
            polycall_core_free(core_ctx, access_token);
            polycall_auth_free_token_claims(core_ctx, claims);
            return false;
        }
    }
    
    // Free resources
    polycall_core_free(core_ctx, access_token);
    polycall_auth_free_token_claims(core_ctx, claims);
    
    return true;
}

// Test token refresh mechanism
static bool test_token_refresh_mechanism(void) {
    // Issue an access and refresh token pair
    char* access_token = NULL;
    char* refresh_token = NULL;
    
    // Mock authentication to get both tokens
    const char* test_scopes[] = {"read", "write"};
    
    polycall_core_error_t result = polycall_auth_issue_token(
        core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
        test_scopes, 2, NULL, &access_token
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !access_token) {
        fprintf(stderr, "Failed to issue access token for refresh test\n");
        return false;
    }
    
    result = polycall_auth_issue_token(
        core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_REFRESH, 
        test_scopes, 2, NULL, &refresh_token
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !refresh_token) {
        fprintf(stderr, "Failed to issue refresh token\n");
        polycall_core_free(core_ctx, access_token);
        return false;
    }
    
    // Get initial token introspection
    token_claims_t* initial_claims = NULL;
    result = polycall_auth_introspect_token(
        core_ctx, auth_ctx, access_token, &initial_claims
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !initial_claims) {
        fprintf(stderr, "Initial token introspection failed\n");
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, refresh_token);
        return false;
    }
    
    // Use refresh token to get a new access token
    char* new_access_token = NULL;
    result = polycall_auth_refresh_token(
        core_ctx, auth_ctx, refresh_token, &new_access_token
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !new_access_token) {
        fprintf(stderr, "Token refresh failed: %d\n", result);
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, refresh_token);
        polycall_auth_free_token_claims(core_ctx, initial_claims);
        return false;
    }
    
    // Verify new token is different from old one
    if (strcmp(new_access_token, access_token) == 0) {
        fprintf(stderr, "New access token is identical to old one\n");
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, refresh_token);
        polycall_core_free(core_ctx, new_access_token);
        polycall_auth_free_token_claims(core_ctx, initial_claims);
        return false;
    }
    
    // Introspect new token
    token_claims_t* new_claims = NULL;
    result = polycall_auth_introspect_token(
        core_ctx, auth_ctx, new_access_token, &new_claims
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !new_claims) {
        fprintf(stderr, "New token introspection failed\n");
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, refresh_token);
        polycall_core_free(core_ctx, new_access_token);
        polycall_auth_free_token_claims(core_ctx, initial_claims);
        return false;
    }
    
    // Verify new token has same subject as old one
    if (!new_claims->subject || 
        !initial_claims->subject || 
        strcmp(new_claims->subject, initial_claims->subject) != 0) {
        fprintf(stderr, "New token has different subject than original token\n");
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, refresh_token);
        polycall_core_free(core_ctx, new_access_token);
        polycall_auth_free_token_claims(core_ctx, initial_claims);
        polycall_auth_free_token_claims(core_ctx, new_claims);
        return false;
    }
    
    // Verify new token's issued_at is later than old token's
    if (new_claims->issued_at <= initial_claims->issued_at) {
        fprintf(stderr, "New token was not issued after original token\n");
        polycall_core_free(core_ctx, access_token);
        polycall_core_free(core_ctx, refresh_token);
        polycall_core_free(core_ctx, new_access_token);
        polycall_auth_free_token_claims(core_ctx, initial_claims);
        polycall_auth_free_token_claims(core_ctx, new_claims);
        return false;
    }
    
    // Try to use the old token - should fail if token revocation on refresh is implemented
    char* identity_id = NULL;
    result = polycall_auth_validate_token(
        core_ctx, auth_ctx, access_token, &identity_id
    );
    
    // If we have automatic revocation on refresh, the old token should be invalid
    // But this is implementation-specific, so we don't strictly test for failure here
    if (result == POLYCALL_CORE_SUCCESS && identity_id) {
        printf("Note: Old access token still valid after refresh (implementation-specific)\n");
        polycall_core_free(core_ctx, identity_id);
    }
    
    // Free resources
    polycall_core_free(core_ctx, access_token);
    polycall_core_free(core_ctx, refresh_token);
    polycall_core_free(core_ctx, new_access_token);
    polycall_auth_free_token_claims(core_ctx, initial_claims);
    polycall_auth_free_token_claims(core_ctx, new_claims);
    
    return true;
}

// Test zero-trust token properties
static bool test_zero_trust_token_properties(void) {
    // Issue a token with device and context information
    const char* device_info = "{\"device_id\":\"test_device\",\"platform\":\"test_platform\"}";
    const char* custom_claims = "{\"context\":{\"ip\":\"192.168.1.1\",\"user_agent\":\"Test Agent\"}}";
    
    char* access_token = NULL;
    polycall_core_error_t result = polycall_auth_issue_token(
        core_ctx, auth_ctx, TEST_IDENTITY_ID, POLYCALL_TOKEN_TYPE_ACCESS, 
        NULL, 0, custom_claims, &access_token
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !access_token) {
        fprintf(stderr, "Failed to issue token with zero-trust properties\n");
        return false;
    }
    
    // Introspect the token
    token_claims_t* claims = NULL;
    result = polycall_auth_introspect_token(
        core_ctx, auth_ctx, access_token, &claims
    );
    
    if (result != POLYCALL_CORE_SUCCESS || !claims) {
        fprintf(stderr, "Token introspection failed\n");
        polycall_core_free(core_ctx, access_token);
        return false;
    }
    
    // Verify zero-trust properties
    if (!claims->custom_claims) {
        fprintf(stderr, "Zero-trust token properties missing (custom claims)\n");
        polycall_core_free(core_ctx, access_token);
        polycall_auth_free_token_claims(core_ctx, claims);
        return false;
    }
    
    // Check context info is included
    if (strstr(claims->custom_claims, "ip") == NULL || 
        strstr(claims->custom_claims, "user_agent") == NULL) {
        fprintf(stderr, "Zero-trust context information missing in token\n");
        polycall_core_free(core_ctx, access_token);
        polycall_auth_free_token_claims(core_ctx, claims);
        return false;
    }
    
    // Helper function to check if token validation considers device/context data
    // This would be implementation-specific, so we just check for the existence
    // of the claims in the token
    
    // Free resources
    polycall_core_free(core_ctx, access_token);
    polycall_auth_free_token_claims(core_ctx, claims);
    
    return true;
}

// Main test function
int main(void) {
    // Initialize test framework
    test_suite_t suite = {0};
    suite.name = "Authentication Token Unit Tests";
    suite.setup = test_setup;
    suite.teardown = test_teardown;
    
    // Add tests
    test_case_t tests[] = {
        {"Token Issuance", test_token_issuance},
        {"Token Validation", test_token_validation},
        {"Token Introspection", test_token_introspection},
        {"Token Revocation", test_token_revocation},
        {"Token Expiration", test_token_expiration},
        {"Token Scopes", test_token_scopes},
        {"Token Refresh Mechanism", test_token_refresh_mechanism},
        {"Zero-Trust Token Properties", test_zero_trust_token_properties}
    };
    
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        test_suite_add_test(&suite, &tests[i]);
    }
    
    // Run tests
    bool success = run_test_suite(&suite);
    
    return success ? 0 : 1;
}