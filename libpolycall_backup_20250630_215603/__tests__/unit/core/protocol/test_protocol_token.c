/**
 * @file test_protocol_token.c
 * @brief Unit test for protocol token functionality using AAA pattern
 * @author LibPolyCall Implementation Team
 */

#include "polycall/core/protocol/polycall_protocol.h"
#include "polycall/core/protocol/polycall_token.h"
#include "__tests__/test_framework.h"

POLYCALL_TEST_SUITE_BEGIN(protocol_token) {
    // Global setup for test suite
    polycall_test_log_info("Setting up protocol_token test suite");
    
    // Initialize component stubs
    const char* components[] = {"protocol"};
    if (!test_stub_manager_init(components, 1)) {
        return NULL;
    }
    
    // Create a global context that will be shared across all tests
    polycall_core_context_t* core_ctx = NULL;
    if (polycall_core_context_create(&core_ctx) != POLYCALL_CORE_ERROR_NONE) {
        test_stub_manager_cleanup();
        return NULL;
    }
    
    return (void*)core_ctx;
}

POLYCALL_TEST_FIXTURE(protocol_token, token_fixture) {
    // ARRANGE phase - setup test fixture
    POLYCALL_ARRANGE_PHASE("Setting up token fixture for protocol tests");
    
    // Get the global core context
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)global_context;
    
    // Create a token context for the test
    polycall_token_context_t* token_ctx = NULL;
    polycall_token_context_create(core_ctx, &token_ctx);
    
    POLYCALL_ASSERT_NOT_NULL(token_ctx, "Token context should be created successfully");
    
    // Return the token context as the fixture context
    return (void*)token_ctx;
}

POLYCALL_TEST_FIXTURE_END(protocol_token, token_fixture) {
    // Clean up fixture
    if (fixture_context) {
        polycall_token_context_t* token_ctx = (polycall_token_context_t*)fixture_context;
        polycall_token_context_destroy(token_ctx);
    }
}

POLYCALL_TEST_CASE_WITH_FIXTURE(protocol_token, create_token, token_fixture) {
    // Get the token context from fixture
    polycall_token_context_t* token_ctx = (polycall_token_context_t*)fixture_context;
    
    // ARRANGE phase - prepare test data
    POLYCALL_ARRANGE_PHASE("Prepare data for token creation test");
    const char* token_content = "test-token-1234";
    polycall_token_t* token = NULL;
    
    // ACT phase - perform the action being tested
    POLYCALL_ACT_PHASE("Create a new token");
    polycall_token_error_t result = polycall_token_create(token_ctx, token_content, &token);
    
    // ASSERT phase - verify the expected outcome
    POLYCALL_ASSERT_PHASE("Verify token was created successfully");
    POLYCALL_ASSERT_TRUE(result == POLYCALL_TOKEN_ERROR_NONE, 
                        "Token creation should succeed");
    POLYCALL_ASSERT_NOT_NULL(token, "Token should not be NULL");
    
    const char* retrieved_content = polycall_token_get_content(token);
    POLYCALL_ASSERT_STRING_EQUAL(token_content, retrieved_content, 
                               "Token content should match");
    
    // Clean up
    polycall_token_destroy(token);
}

POLYCALL_TEST_CASE_WITH_FIXTURE(protocol_token, validate_token, token_fixture) {
    // Get the token context from fixture
    polycall_token_context_t* token_ctx = (polycall_token_context_t*)fixture_context;
    
    // ARRANGE phase - prepare test data
    POLYCALL_ARRANGE_PHASE("Prepare data for token validation test");
    const char* valid_content = "valid-token-5678";
    const char* invalid_content = "invalid-token-9012";
    polycall_token_t* token = NULL;
    
    // Create a token for testing
    polycall_token_error_t create_result = polycall_token_create(token_ctx, valid_content, &token);
    POLYCALL_ASSERT_TRUE(create_result == POLYCALL_TOKEN_ERROR_NONE, 
                        "Token should be created successfully for the test");
    
    // Register token for validation
    polycall_token_error_t register_result = polycall_token_register(token_ctx, token);
    POLYCALL_ASSERT_TRUE(register_result == POLYCALL_TOKEN_ERROR_NONE, 
                        "Token registration should succeed");
    
    // ACT & ASSERT phase 1 - validate with correct content
    POLYCALL_ACT_PHASE("Validate token with valid content");
    bool valid_result = polycall_token_validate(token_ctx, valid_content);
    
    POLYCALL_ASSERT_PHASE("Verify validation succeeds with valid content");
    POLYCALL_ASSERT_TRUE(valid_result, "Token validation should succeed with valid content");
    
    // ACT & ASSERT phase 2 - validate with incorrect content
    POLYCALL_ACT_PHASE("Validate token with invalid content");
    bool invalid_result = polycall_token_validate(token_ctx, invalid_content);
    
    POLYCALL_ASSERT_PHASE("Verify validation fails with invalid content");
    POLYCALL_ASSERT_FALSE(invalid_result, "Token validation should fail with invalid content");
    
    // Clean up
    polycall_token_unregister(token_ctx, token);
    polycall_token_destroy(token);
}

POLYCALL_TEST_CASE_WITH_FIXTURE(protocol_token, token_expiration, token_fixture) {
    // Get the token context from fixture
    polycall_token_context_t* token_ctx = (polycall_token_context_t*)fixture_context;
    
    // ARRANGE phase - prepare test data
    POLYCALL_ARRANGE_PHASE("Prepare data for token expiration test");
    const char* token_content = "expirable-token-1234";
    polycall_token_t* token = NULL;
    
    // Create a token with a short expiration time (100ms)
    polycall_token_error_t create_result = polycall_token_create(token_ctx, token_content, &token);
    POLYCALL_ASSERT_TRUE(create_result == POLYCALL_TOKEN_ERROR_NONE, 
                        "Token should be created successfully");
    
    // Set token expiration
    polycall_token_error_t exp_result = polycall_token_set_expiration(token, 100); // 100ms
    POLYCALL_ASSERT_TRUE(exp_result == POLYCALL_TOKEN_ERROR_NONE, 
                        "Token expiration should be set successfully");
    
    // Register token
    polycall_token_error_t reg_result = polycall_token_register(token_ctx, token);
    POLYCALL_ASSERT_TRUE(reg_result == POLYCALL_TOKEN_ERROR_NONE, 
                        "Token registration should succeed");
    
    // ACT & ASSERT phase 1 - check token is valid before expiration
    POLYCALL_ACT_PHASE("Validate token before expiration");
    bool valid_before = polycall_token_validate(token_ctx, token_content);
    
    POLYCALL_ASSERT_PHASE("Verify token is valid before expiration");
    POLYCALL_ASSERT_TRUE(valid_before, "Token should be valid before expiration");
    
    // ACT & ASSERT phase 2 - wait for expiration and check again
    POLYCALL_ACT_PHASE("Wait for token expiration and validate again");
    
    // Sleep for 200ms to ensure token expires
    struct timespec sleep_time = {0, 200 * 1000 * 1000}; // 200ms
    nanosleep(&sleep_time, NULL);
    
    bool valid_after = polycall_token_validate(token_ctx, token_content);
    
    POLYCALL_ASSERT_PHASE("Verify token is invalid after expiration");
    POLYCALL_ASSERT_FALSE(valid_after, "Token should be invalid after expiration");
    
    // Clean up
    polycall_token_destroy(token);
}

POLYCALL_TEST_SUITE_END(protocol_token) {
    // Global cleanup for test suite
    polycall_test_log_info("Cleaning up protocol_token test suite");
    
    // Clean up core context
    if (global_context) {
        polycall_core_context_t* core_ctx = (polycall_core_context_t*)global_context;
        polycall_core_context_destroy(core_ctx);
    }
    
    // Clean up component stubs
    test_stub_manager_cleanup();
}

// Main function that runs all the tests
POLYCALL_TEST_MAIN(
    protocol_token_create_token_register,
    protocol_token_validate_token_register,
    protocol_token_token_expiration_register
)