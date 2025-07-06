/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file test_hierarchical_error.c
 * @brief Unit tests for hierarchical error handling using AAA pattern
 * @author OBINexusComputing Implementation Team
 */

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_hierarchical_error.h"
#include "tests/polycall_test_framework.h"

/**
 * @brief Test fixture context for hierarchical error tests
 */
typedef struct {
  polycall_core_context_t *core_ctx;
  polycall_hierarchical_error_ctx_t *error_ctx;
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
static void mock_error_handler(polycall_core_context_t *ctx,
                               const char *component_name,
                               polycall_error_source_t source, int32_t code,
                               polycall_error_severity_t severity,
                               const char *message, void *user_data) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)user_data;

  // Store call information
  fixture->handler_call_count++;

  if (component_name) {
    strncpy(fixture->component_name, component_name,
            sizeof(fixture->component_name) - 1);
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

  POLYCALL_TEST_INFO("Mock handler called: component=%s, source=%d, code=%d, "
                     "severity=%d, message=%s",
                     component_name ? component_name : "NULL", source, code,
                     severity, message ? message : "NULL");
}

POLYCALL_TEST_SUITE_BEGIN(hierarchical_error) {
  // Global setup for all tests in this suite
  POLYCALL_TEST_INFO("Setting up hierarchical error test suite");
  return NULL; // No global context needed
}

POLYCALL_TEST_FIXTURE(hierarchical_error, basic_fixture) {
  // Setup code for the fixture
  POLYCALL_TEST_INFO("Setting up basic_fixture");

  hierarchical_error_fixture_t *fixture =
      malloc(sizeof(hierarchical_error_fixture_t));
  POLYCALL_ASSERT_NOT_NULL(fixture, "Failed to allocate fixture");

  memset(fixture, 0, sizeof(hierarchical_error_fixture_t));

  // Initialize core context
  polycall_core_error_t core_result = polycall_core_init(&fixture->core_ctx);
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, core_result,
                            "Failed to initialize core context");

  // Initialize hierarchical error context
  polycall_core_error_t error_result =
      polycall_hierarchical_error_init(fixture->core_ctx, &fixture->error_ctx);
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, error_result,
                            "Failed to initialize hierarchical error context");

  return fixture;
}

POLYCALL_TEST_FIXTURE_END(hierarchical_error, basic_fixture) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)context;

  if (fixture) {
    // Clean up error context
    if (fixture->error_ctx) {
      polycall_hierarchical_error_cleanup(fixture->core_ctx,
                                          fixture->error_ctx);
    }

    // Clean up core context
    if (fixture->core_ctx) {
      polycall_core_cleanup(fixture->core_ctx);
    }

    free(fixture);
  }
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, initialization,
                                basic_fixture) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)fixture_context;

  // ARRANGE phase - already done in fixture setup
  POLYCALL_ARRANGE_PHASE("Verify initialization through fixture setup");

  // ACT phase - no action needed, already initialized in fixture
  POLYCALL_ACT_PHASE("No additional action needed");

  // ASSERT phase - verify contexts are properly initialized
  POLYCALL_ASSERT_PHASE("Verify contexts are properly initialized");
  POLYCALL_ASSERT_NOT_NULL(fixture->core_ctx,
                           "Core context should be initialized");
  POLYCALL_ASSERT_NOT_NULL(fixture->error_ctx,
                           "Error context should be initialized");
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, register_handler,
                                basic_fixture) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)fixture_context;

  // ARRANGE phase
  POLYCALL_ARRANGE_PHASE("Prepare handler configuration");
  polycall_hierarchical_error_handler_config_t config = {
      .component_name = "test_component",
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = mock_error_handler,
      .user_data = fixture,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_UPWARD,
      .parent_component = "core"};

  // ACT phase
  POLYCALL_ACT_PHASE("Register error handler");
  polycall_core_error_t result = polycall_hierarchical_error_register_handler(
      fixture->core_ctx, fixture->error_ctx, &config);

  // ASSERT phase
  POLYCALL_ASSERT_PHASE("Verify handler registration");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, result,
                            "Handler registration should succeed");

  // Verify handler was registered by checking if component exists
  bool has_handler = polycall_hierarchical_error_has_handler(
      fixture->core_ctx, fixture->error_ctx, "test_component");
  POLYCALL_ASSERT_TRUE(has_handler,
                       "Component should have a registered handler");
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, set_error, basic_fixture) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)fixture_context;

  // ARRANGE phase
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
      .parent_component = "core"};

  polycall_hierarchical_error_register_handler(fixture->core_ctx,
                                               fixture->error_ctx, &config);

  // ACT phase
  POLYCALL_ACT_PHASE("Set hierarchical error");
  polycall_core_error_t result = polycall_hierarchical_error_set(
      fixture->core_ctx, fixture->error_ctx, "test_component",
      POLYCALL_ERROR_SOURCE_CORE, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
      POLYCALL_ERROR_SEVERITY_ERROR, "Test error message");

  // ASSERT phase
  POLYCALL_ASSERT_PHASE("Verify error handling");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, result,
                            "Error setting should succeed");
  POLYCALL_ASSERT_INT_EQUAL(1, fixture->handler_call_count,
                            "Handler should be called exactly once");
  POLYCALL_ASSERT_STRING_EQUAL("test_component", fixture->component_name,
                               "Component name should match");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_ERROR_SOURCE_CORE, fixture->source,
                            "Error source should match");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                            fixture->code, "Error code should match");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_ERROR_SEVERITY_ERROR, fixture->severity,
                            "Error severity should match");
  POLYCALL_ASSERT_STRING_EQUAL("Test error message", fixture->message,
                               "Error message should match");
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, error_propagation,
                                basic_fixture) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)fixture_context;

  // ARRANGE phase
  POLYCALL_ARRANGE_PHASE("Set up component hierarchy for propagation testing");

  // Reset call count
  fixture->handler_call_count = 0;

  // Register parent handler
  polycall_hierarchical_error_handler_config_t parent_config = {
      .component_name = "parent_component",
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = mock_error_handler,
      .user_data = fixture,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_DOWNWARD,
      .parent_component = NULL // No parent
  };

  polycall_hierarchical_error_register_handler(
      fixture->core_ctx, fixture->error_ctx, &parent_config);

  // Register child handler
  polycall_hierarchical_error_handler_config_t child_config = {
      .component_name = "child_component",
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = mock_error_handler,
      .user_data = fixture,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_UPWARD,
      .parent_component = "parent_component"};

  polycall_hierarchical_error_register_handler(
      fixture->core_ctx, fixture->error_ctx, &child_config);

  // ACT phase
  POLYCALL_ACT_PHASE("Set error in child component");
  polycall_core_error_t result = polycall_hierarchical_error_set(
      fixture->core_ctx, fixture->error_ctx, "child_component",
      POLYCALL_ERROR_SOURCE_CORE, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
      POLYCALL_ERROR_SEVERITY_ERROR, "Error in child component");

  // ASSERT phase
  POLYCALL_ASSERT_PHASE("Verify error propagation");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, result,
                            "Error setting should succeed");
  POLYCALL_ASSERT_INT_EQUAL(
      2, fixture->handler_call_count,
      "Handler should be called twice (once for child, once for parent)");

  // Verify parent relationship
  char parent_name[64] = {0};
  polycall_core_error_t parent_result = polycall_hierarchical_error_get_parent(
      fixture->core_ctx, fixture->error_ctx, "child_component", parent_name,
      sizeof(parent_name));

  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, parent_result,
                            "Getting parent should succeed");
  POLYCALL_ASSERT_STRING_EQUAL("parent_component", parent_name,
                               "Parent name should match");
}

POLYCALL_TEST_CASE_WITH_FIXTURE(hierarchical_error, bidirectional_propagation,
                                basic_fixture) {
  hierarchical_error_fixture_t *fixture =
      (hierarchical_error_fixture_t *)fixture_context;

  // ARRANGE phase
  POLYCALL_ARRANGE_PHASE(
      "Set up three-level component hierarchy with bidirectional propagation");

  // Reset call count
  fixture->handler_call_count = 0;

  // Register root handler
  polycall_hierarchical_error_handler_config_t root_config = {
      .component_name = "root_component",
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = mock_error_handler,
      .user_data = fixture,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
      .parent_component = NULL // No parent
  };

  polycall_hierarchical_error_register_handler(
      fixture->core_ctx, fixture->error_ctx, &root_config);

  // Register middle handler
  polycall_hierarchical_error_handler_config_t middle_config = {
      .component_name = "middle_component",
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = mock_error_handler,
      .user_data = fixture,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
      .parent_component = "root_component"};

  polycall_hierarchical_error_register_handler(
      fixture->core_ctx, fixture->error_ctx, &middle_config);

  // Register leaf handler
  polycall_hierarchical_error_handler_config_t leaf_config = {
      .component_name = "leaf_component",
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = mock_error_handler,
      .user_data = fixture,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
      .parent_component = "middle_component"};

  polycall_hierarchical_error_register_handler(
      fixture->core_ctx, fixture->error_ctx, &leaf_config);

  // ACT phase
  POLYCALL_ACT_PHASE("Set error in middle component");
  polycall_core_error_t result = polycall_hierarchical_error_set(
      fixture->core_ctx, fixture->error_ctx, "middle_component",
      POLYCALL_ERROR_SOURCE_CORE, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
      POLYCALL_ERROR_SEVERITY_ERROR, "Error in middle component");

  // ASSERT phase
  POLYCALL_ASSERT_PHASE("Verify bidirectional error propagation");
  POLYCALL_ASSERT_INT_EQUAL(POLYCALL_CORE_SUCCESS, result,
                            "Error setting should succeed");
  POLYCALL_ASSERT_INT_EQUAL(
      3, fixture->handler_call_count,
      "Handler should be called three times (middle, root, and leaf)");
}

POLYCALL_TEST_SUITE_END(hierarchical_error) {
  // Global cleanup for the suite
  POLYCALL_TEST_INFO("Cleaning up hierarchical error test suite");
}

// Define test components
#define COMPONENT_CORE "core"
#define COMPONENT_NETWORK "network"
#define COMPONENT_PROTOCOL "protocol"
#define COMPONENT_CLIENT "client"
#define COMPONENT_SERVER "server"

// Error counter for verification
static int error_count = 0;

/**
 * @brief Core component error handler
 */
static void core_error_handler(polycall_core_context_t *ctx,
                               const char *component_name,
                               polycall_error_source_t source, int32_t code,
                               polycall_error_severity_t severity,
                               const char *message, void *user_data) {
  printf("[CORE] Received error from %s: [%d] %s (Severity: %d)\n",
         component_name, code, message, severity);
  error_count++;
}

/**
 * @brief Network component error handler
 */
static void network_error_handler(polycall_core_context_t *ctx,
                                  const char *component_name,
                                  polycall_error_source_t source, int32_t code,
                                  polycall_error_severity_t severity,
                                  const char *message, void *user_data) {
  printf("[NETWORK] Received error from %s: [%d] %s (Severity: %d)\n",
         component_name, code, message, severity);
  error_count++;
}

/**
 * @brief Protocol component error handler
 */
static void protocol_error_handler(polycall_core_context_t *ctx,
                                   const char *component_name,
                                   polycall_error_source_t source, int32_t code,
                                   polycall_error_severity_t severity,
                                   const char *message, void *user_data) {
  printf("[PROTOCOL] Received error from %s: [%d] %s (Severity: %d)\n",
         component_name, code, message, severity);
  error_count++;
}

/**
 * @brief Client component error handler
 */
static void client_error_handler(polycall_core_context_t *ctx,
                                 const char *component_name,
                                 polycall_error_source_t source, int32_t code,
                                 polycall_error_severity_t severity,
                                 const char *message, void *user_data) {
  printf("[CLIENT] Received error from %s: [%d] %s (Severity: %d)\n",
         component_name, code, message, severity);
  error_count++;
}

/**
 * @brief Server component error handler
 */
static void server_error_handler(polycall_core_context_t *ctx,
                                 const char *component_name,
                                 polycall_error_source_t source, int32_t code,
                                 polycall_error_severity_t severity,
                                 const char *message, void *user_data) {
  printf("[SERVER] Received error from %s: [%d] %s (Severity: %d)\n",
         component_name, code, message, severity);
  error_count++;
}

/**
 * @brief Setup the test environment
 *
 * @param core_ctx Core context
 * @param error_ctx Pointer to receive hierarchical error context
 * @return Error code
 */
static polycall_core_error_t
setup_test(polycall_core_context_t *core_ctx,
           polycall_hierarchical_error_context_t **error_ctx) {
  polycall_core_error_t result;

  // Initialize hierarchical error system
  result = polycall_hierarchical_error_init(core_ctx, error_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to initialize hierarchical error system: %d\n", result);
    return result;
  }

  // Register core component
  polycall_hierarchical_error_handler_config_t core_config = {
      .component_name = COMPONENT_CORE,
      .source = POLYCALL_ERROR_SOURCE_CORE,
      .handler = core_error_handler,
      .user_data = NULL,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_DOWNWARD,
      .parent_component = ""};

  result = polycall_hierarchical_error_register_handler(core_ctx, *error_ctx,
                                                        &core_config);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to register core component: %d\n", result);
    return result;
  }

  // Register network component
  polycall_hierarchical_error_handler_config_t network_config = {
      .component_name = COMPONENT_NETWORK,
      .source = POLYCALL_ERROR_SOURCE_NETWORK,
      .handler = network_error_handler,
      .user_data = NULL,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
      .parent_component = COMPONENT_CORE};

  result = polycall_hierarchical_error_register_handler(core_ctx, *error_ctx,
                                                        &network_config);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to register network component: %d\n", result);
    return result;
  }

  // Register protocol component
  polycall_hierarchical_error_handler_config_t protocol_config = {
      .component_name = COMPONENT_PROTOCOL,
      .source = POLYCALL_ERROR_SOURCE_PROTOCOL,
      .handler = protocol_error_handler,
      .user_data = NULL,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
      .parent_component = COMPONENT_CORE};

  result = polycall_hierarchical_error_register_handler(core_ctx, *error_ctx,
                                                        &protocol_config);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to register protocol component: %d\n", result);
    return result;
  }

  // Register client component
  polycall_hierarchical_error_handler_config_t client_config = {
      .component_name = COMPONENT_CLIENT,
      .source = POLYCALL_ERROR_SOURCE_NETWORK,
      .handler = client_error_handler,
      .user_data = NULL,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_UPWARD,
      .parent_component = COMPONENT_NETWORK};

  result = polycall_hierarchical_error_register_handler(core_ctx, *error_ctx,
                                                        &client_config);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to register client component: %d\n", result);
    return result;
  }

  // Register server component
  polycall_hierarchical_error_handler_config_t server_config = {
      .component_name = COMPONENT_SERVER,
      .source = POLYCALL_ERROR_SOURCE_NETWORK,
      .handler = server_error_handler,
      .user_data = NULL,
      .propagation_mode = POLYCALL_ERROR_PROPAGATE_UPWARD,
      .parent_component = COMPONENT_NETWORK};

  result = polycall_hierarchical_error_register_handler(core_ctx, *error_ctx,
                                                        &server_config);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to register server component: %d\n", result);
    return result;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Run propagation tests
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @return Error code
 */
static polycall_core_error_t
run_propagation_tests(polycall_core_context_t *core_ctx,
                      polycall_hierarchical_error_context_t *error_ctx) {
  polycall_core_error_t result;
  error_count = 0;

  printf("\n=== Testing Error Propagation ===\n\n");

  // Test client error (should propagate up to network and core)
  printf("Setting client error (should propagate upward)...\n");
  result = POLYCALL_HIERARCHICAL_ERROR_SET(
      core_ctx, error_ctx, COMPONENT_CLIENT, POLYCALL_ERROR_SOURCE_NETWORK,
      POLYCALL_CORE_ERROR_NETWORK, POLYCALL_ERROR_SEVERITY_ERROR,
      "Connection failed to %s", "example.com");

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to set client error: %d\n", result);
    return result;
  }

  printf("Client error propagation triggered %d handlers\n", error_count);
  error_count = 0;

  // Test server error (should propagate up to network and core)
  printf("\nSetting server error (should propagate upward)...\n");
  result = POLYCALL_HIERARCHICAL_ERROR_SET(
      core_ctx, error_ctx, COMPONENT_SERVER, POLYCALL_ERROR_SOURCE_NETWORK,
      POLYCALL_CORE_ERROR_ACCESS_DENIED, POLYCALL_ERROR_SEVERITY_ERROR,
      "Authentication failed for client %s", "192.168.1.10");

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to set server error: %d\n", result);
    return result;
  }

  printf("Server error propagation triggered %d handlers\n", error_count);
  error_count = 0;

  // Test network error (should propagate both ways)
  printf("\nSetting network error (should propagate bidirectionally)...\n");
  result = POLYCALL_HIERARCHICAL_ERROR_SET(
      core_ctx, error_ctx, COMPONENT_NETWORK, POLYCALL_ERROR_SOURCE_NETWORK,
      POLYCALL_CORE_ERROR_TIMEOUT, POLYCALL_ERROR_SEVERITY_WARNING,
      "Network timeout after %d ms", 5000);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to set network error: %d\n", result);
    return result;
  }

  printf("Network error propagation triggered %d handlers\n", error_count);
  error_count = 0;

  // Test core error (should propagate down to all)
  printf("\nSetting core error (should propagate downward)...\n");
  result = POLYCALL_HIERARCHICAL_ERROR_SET(
      core_ctx, error_ctx, COMPONENT_CORE, POLYCALL_ERROR_SOURCE_CORE,
      POLYCALL_CORE_ERROR_INTERNAL, POLYCALL_ERROR_SEVERITY_FATAL,
      "Critical system failure: %s", "memory corruption");

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to set core error: %d\n", result);
    return result;
  }

  printf("Core error propagation triggered %d handlers\n", error_count);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Test error retrieval and clearing
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @return Error code
 */
static polycall_core_error_t
test_error_management(polycall_core_context_t *core_ctx,
                      polycall_hierarchical_error_context_t *error_ctx) {
  polycall_core_error_t result;
  polycall_error_record_t record;

  printf("\n=== Testing Error Management ===\n\n");

  // Set an error on the protocol component
  printf("Setting protocol error...\n");
  result = POLYCALL_HIERARCHICAL_ERROR_SET(
      core_ctx, error_ctx, COMPONENT_PROTOCOL, POLYCALL_ERROR_SOURCE_PROTOCOL,
      POLYCALL_CORE_ERROR_INVALID_PARAMETERS, POLYCALL_ERROR_SEVERITY_ERROR,
      "Invalid protocol version: %d", 3);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to set protocol error: %d\n", result);
    return result;
  }

  // Get the error back
  bool has_error = polycall_hierarchical_error_get_last(
      core_ctx, error_ctx, COMPONENT_PROTOCOL, &record);

  if (has_error) {
    printf("Retrieved protocol error: [%d] %s (Severity: %d)\n", record.code,
           record.message, record.severity);
  } else {
    printf("Failed to retrieve protocol error\n");
    return POLYCALL_CORE_ERROR_NOT_FOUND;
  }

  // Clear the error
  printf("\nClearing protocol error...\n");
  result = polycall_hierarchical_error_clear(core_ctx, error_ctx,
                                             COMPONENT_PROTOCOL);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to clear protocol error: %d\n", result);
    return result;
  }

  // Verify it's gone
  has_error = polycall_hierarchical_error_get_last(core_ctx, error_ctx,
                                                   COMPONENT_PROTOCOL, &record);

  if (!has_error) {
    printf("Protocol error was successfully cleared\n");
  } else {
    printf("Protocol error still exists after clearing\n");
    return POLYCALL_CORE_ERROR_INTERNAL;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Test hierarchy relationships
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @return Error code
 */
static polycall_core_error_t
test_hierarchy_relationships(polycall_core_context_t *core_ctx,
                             polycall_hierarchical_error_context_t *error_ctx) {
  polycall_core_error_t result;
  char parent[POLYCALL_MAX_COMPONENT_NAME_LENGTH];
  char children[POLYCALL_MAX_CHILD_STATES][POLYCALL_MAX_COMPONENT_NAME_LENGTH];
  uint32_t child_count = 0;

  printf("\n=== Testing Hierarchy Relationships ===\n\n");

  // Get client's parent
  printf("Getting client's parent...\n");
  result = polycall_hierarchical_error_get_parent(
      core_ctx, error_ctx, COMPONENT_CLIENT, parent, sizeof(parent));

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to get client's parent: %d\n", result);
    return result;
  }

  printf("Client's parent is: %s\n", parent);

  // Get network's children
  printf("\nGetting network's children...\n");
  result = polycall_hierarchical_error_get_children(
      core_ctx, error_ctx, COMPONENT_NETWORK, children,
      POLYCALL_MAX_CHILD_STATES, &child_count);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to get network's children: %d\n", result);
    return result;
  }

  printf("Network has %d children:\n", child_count);
  for (uint32_t i = 0; i < child_count; i++) {
    printf("  - %s\n", children[i]);
  }

  // Get core's children
  printf("\nGetting core's children...\n");
  result = polycall_hierarchical_error_get_children(
      core_ctx, error_ctx, COMPONENT_CORE, children, POLYCALL_MAX_CHILD_STATES,
      &child_count);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to get core's children: %d\n", result);
    return result;
  }

  printf("Core has %d children:\n", child_count);
  for (uint32_t i = 0; i < child_count; i++) {
    printf("  - %s\n", children[i]);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Run the test suite
 *
 * @param core_ctx Core context
 * @return Error code
 */
polycall_core_error_t
run_hierarchical_error_tests(polycall_core_context_t *core_ctx) {
  polycall_hierarchical_error_context_t *error_ctx = NULL;
  polycall_core_error_t result;

  printf("=== Hierarchical Error Handling Tests ===\n\n");

  // Setup test environment
  result = setup_test(core_ctx, &error_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Run propagation tests
  result = run_propagation_tests(core_ctx, error_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_hierarchical_error_cleanup(core_ctx, error_ctx);
    return result;
  }

  // Test error management
  result = test_error_management(core_ctx, error_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_hierarchical_error_cleanup(core_ctx, error_ctx);
    return result;
  }

  // Test hierarchy relationships
  result = test_hierarchy_relationships(core_ctx, error_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_hierarchical_error_cleanup(core_ctx, error_ctx);
    return result;
  }

  // Clean up
  polycall_hierarchical_error_cleanup(core_ctx, error_ctx);

  printf("\n=== All Tests Completed Successfully ===\n");

  return POLYCALL_CORE_SUCCESS;
}
/**
 * @brief Register and run all test cases
 */
POLYCALL_TEST_MAIN(hierarchical_error_initialization_register,
                   hierarchical_error_register_handler_register,
                   hierarchical_error_set_error_register,
                   hierarchical_error_error_propagation_register,
                   hierarchical_error_bidirectional_propagation_register);
