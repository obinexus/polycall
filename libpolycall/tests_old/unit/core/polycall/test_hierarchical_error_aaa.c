/**
 * @file test_hierarchical_error_aaa.c
 * @brief Unit tests for hierarchical error handling using AAA pattern
 * @author LibPolyCall Implementation Team
 */

#include "polycall/core/polycall/polycall_hierarchical_error.h"
#include "polycall/core/polycall/polycall_core.h"
#include "__tests__/test_framework.h"

/**
 * @brief Test fixture context for hierarchical error tests
 */
typedef struct {
    polycall_core_context_t* core_ctx;
    polycall_hierarchical_error_ctx_t* error_ctx;
    int handler_call_count;
    char component_name[64];
    polycall_error_source_t source;
    int32_t code;
    polycall_error_severity_t severity;
    char message[256];
} hierarchical_error_fixture_t;

/**
 * @brief Mock error handler for testing
 */
static void mock_error_handler(
    polycall_core_context_t* ctx,
    const char* component_name,
    polycall_error_source_t source,
    int32_t code,
    polycall_error_severity_t severity,
    const char* message,
    void* user_data
) {
    hierarchical_error_fixture_t* fixture = (hierarchical_error_fixture_t*)user_data;
    
    // Store call information
    fixture->handler_call_count++;
    
    if (component_name) {
        strncpy(fixture->component_name, component_name, sizeof(fixture->component_name) - 1);
        fixture->component_name[sizeof(fixture->component_name) - 1] = '\0';
    } else {
        fixture->component_name[0] = '\0';
    }
    
    fixture->source = source;
    fixture->code = code;
    fixture->severity = severity;
    
    if (message) {
        strncpy(fixture->message, message, sizeof(fixture->message) - 1);
        fixture->message[sizeof(fixture->message) - 1] = '\0';
    } else {
        fixture->message[0] = '\0';
    }
    
    polycall_test_log_info("Mock handler called: component=%s, source=%d, code=%d, severity=%d, message=%s",
                           component_name ? component_name : "NULL", 
                           source, code, severity, 
                           message ? message : "NULL");
}

POLYCALL_TEST_SUITE_BEGIN(hierarchical_error) {
    // Global setup for all tests in this suite
    polycall_test_log_info("Setting up hierarchical error test suite");
    
    // Initialize component stubs
    const char* components[] = {"polycall"};
    if (!test_stub_manager_init(components, 1)) {
        return NULL;
    }
    
    return NULL; // No global context needed
}

POLYCALL_TEST_FIXTURE(hierarchical_error, basic_fixture) {
    // Setup code for the fixture
    polycall_test_log_info("Setting up basic_fixture");
    
    hierarchical_error_fixture_t* fixture = malloc(sizeof(hierarchical_error_fixture_t));
    POLYCALL_ASSERT_NOT_NULL(fixture, "Failed to allocate fixture");
    
    memset(fixture, 0, sizeof(hierarchical_error_fixture_t));
    
    // Initialize core context
    polycall_core_error_t core_result = polycall_core_init(&fixture->core_ctx);
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, core_result, 
                             "Failed to initialize core context");
    
    // Initialize hierarchical error context
    polycall_core_error_t error_result = polycall_hierarchical_error_init(
        fixture->core_ctx, &fixture->error_ctx);
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, error_result, 
                             "Failed to initialize hierarchical error context");
    
    return fixture;
}

POLYCALL_TEST_FIXTURE_END(hierarchical_error, basic_fixture) {
    hierarchical_error_fixture_t* fixture = (hierarchical_error_fixture_t*)context;
    
    if (fixture) {
        // Clean up error context
        if (fixture->error_ctx) {
            polycall_hierarchical_error_cleanup(fixture->core_ctx, fixture->error_ctx);
        }
        
        // Clean up core context
        if (fixture->core_ctx) {
            polycall_core_cleanup(fixture->core_ctx);
        }
        
        free(fixture);
    }
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, register_handler, basic_fixture) {
    hierarchical_error_fixture_t* fixture = (hierarchical_error_fixture_t*)fixture_context;
    
    // ARRANGE phase - setup handler configuration
    POLYCALL_ARRANGE_PHASE("Prepare handler configuration");
    polycall_hierarchical_error_handler_config_t config = {
        .component_name = "test_component",
        .source = POLYCALL_ERROR_SOURCE_CORE,
        .handler = mock_error_handler,
        .user_data = fixture,
        .propagation_mode = POLYCALL_ERROR_PROPAGATE_UPWARD,
        .parent_component = "core"
    };
    
    // ACT phase - register the handler
    POLYCALL_ACT_PHASE("Register error handler");
    polycall_core_error_t result = polycall_hierarchical_error_register_handler(
        fixture->core_ctx,
        fixture->error_ctx,
        &config
    );
    
    // ASSERT phase - verify registration succeeded
    POLYCALL_ASSERT_PHASE("Verify handler registration");
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, result, 
                             "Handler registration should succeed");
    
    // Verify handler was registered by checking if component exists
    bool has_handler = polycall_hierarchical_error_has_handler(
        fixture->core_ctx,
        fixture->error_ctx,
        "test_component"
    );
    POLYCALL_ASSERT_TRUE(has_handler, "Component should have a registered handler");
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, set_error, basic_fixture) {
    hierarchical_error_fixture_t* fixture = (hierarchical_error_fixture_t*)fixture_context;
    
    // ARRANGE phase - register a handler
    POLYCALL_ARRANGE_PHASE("Register error handler");
    
    // Reset call count
    fixture->handler_call_count = 0;
    
    // Register handler
    polycall_hierarchical_error_handler_config_t config = {
        .component_name = "test_component",
        .source = POLYCALL_ERROR_SOURCE_CORE,
        .handler = mock_error_handler,
        .user_data = fixture,
        .propagation_mode = POLYCALL_ERROR_PROPAGATE_UPWARD,
        .parent_component = "core"
    };
    
    polycall_hierarchical_error_register_handler(
        fixture->core_ctx,
        fixture->error_ctx,
        &config
    );
    
    // ACT phase - set an error
    POLYCALL_ACT_PHASE("Set hierarchical error");
    polycall_core_error_t result = polycall_hierarchical_error_set(
        fixture->core_ctx,
        fixture->error_ctx,
        "test_component",
        POLYCALL_ERROR_SOURCE_CORE,
        POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
        POLYCALL_ERROR_SEVERITY_ERROR,
        "Test error message"
    );
    
    // ASSERT phase - verify error handling
    POLYCALL_ASSERT_PHASE("Verify error handling");
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, result, 
                             "Error setting should succeed");
    POLYCALL_ASSERT_INT_EQUAL(1, fixture->handler_call_count, 
                             "Handler should be called exactly once");
    POLYCALL_ASSERT_STRING_EQUAL("test_component", fixture->component_name, 
                                "Component name should match");
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_ERROR_SOURCE_CORE, fixture->source, 
                             "Error source should match");
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, fixture->code, 
                             "Error code should match");
    POLYCALL_ASSERT_INT_EQUAL(POLYCALL_ERROR_SEVERITY_ERROR, fixture->severity, 
                             "Error severity should match");
    POLYCALL_ASSERT_STRING_EQUAL("Test error message", fixture->message, 
                                "Error message should match");
}

POLYCALL_TEST_SUITE_END(hierarchical_error) {
    // Global cleanup for the suite
    polycall_test_log_info("Cleaning up hierarchical error test suite");
    test_stub_manager_cleanup();
}

// Main function that runs all the tests
POLYCALL_TEST_MAIN(
    hierarchical_error_register_handler_register,
    hierarchical_error_set_error_register
)
