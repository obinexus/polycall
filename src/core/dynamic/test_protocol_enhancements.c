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
 * @file test_protocol_enhancements.c
 * @brief Comprehensive test suite for LibPolyCall Protocol Enhancements
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements comprehensive tests for all protocol enhancement modules
 * including advanced security, connection pool, hierarchical state, message
 * optimization, and subscription systems. Tests cover initialization, core
 * functionality, error conditions, and integration between modules.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Core include files */
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/protocol/polycall_protocol_context.h"
#include "polycall/core/protocol/protocol_state_machine.h"

/* Enhancement include files */
#include "polycall/core/protocol/enhancements/advanced_security.h"
#include "polycall/core/protocol/enhancements/connection_pool.h"
#include "polycall/core/protocol/enhancements/hierarchical_state.h"
#include "polycall/core/protocol/enhancements/message_optimization.h"
#include "polycall/core/protocol/enhancements/protocol_enhacements_config.h"
#include "polycall/core/protocol/enhancements/subscription.h"

/* Test framework */
#include "unit_test_framework.h"

/* Mock implementations */
#include "mock_core_context.h"
#include "mock_network_endpoint.h"
#include "mock_protocol_context.h"

/* Global test variables */
static polycall_core_context_t *g_core_ctx = NULL;
static polycall_protocol_context_t *g_proto_ctx = NULL;
static polycall_protocol_enhancements_context_t *g_enh_ctx = NULL;
static NetworkEndpoint *g_endpoint = NULL;

/* Helper functions */
static void setup_test_environment(void);
static void teardown_test_environment(void);
static void security_event_callback(uint32_t event_id, void *event_data,
                                    void *user_data);
static void subscription_callback(const char *topic, const void *data,
                                  size_t data_size, void *user_data);

/*---------------------------------------------------------------------------*/
/* Test functions */
/*---------------------------------------------------------------------------*/

/**
 * @brief Test protocol enhancements initialization and cleanup
 */
int test_enhancement_init_cleanup(void) {
  // Setup test environment
  setup_test_environment();

  // Create default configuration
  polycall_protocol_enhancements_config_t config =
      polycall_protocol_enhancements_default_config();

  // Initialize enhancements
  polycall_core_error_t result = polycall_protocol_enhancements_init(
      g_core_ctx, g_proto_ctx, &g_enh_ctx, &config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(g_enh_ctx);

  // Cleanup enhancements
  polycall_protocol_enhancements_cleanup(g_core_ctx, g_enh_ctx);
  g_enh_ctx = NULL;

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test advanced security module initialization and functions
 */
int test_advanced_security(void) {
  // Setup test environment
  setup_test_environment();

  // Initialize security context
  polycall_advanced_security_context_t *security_ctx = NULL;
  polycall_advanced_security_config_t security_config = {
      .initial_strategy = POLYCALL_AUTH_STRATEGY_SINGLE_FACTOR,
      .default_auth_method = POLYCALL_AUTH_METHOD_PASSWORD,
      .max_permissions = 32,
      .event_callback = security_event_callback,
      .user_data = NULL};

  polycall_core_error_t result = polycall_advanced_security_init(
      g_core_ctx, &security_ctx, &security_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(security_ctx);

  // Test permission functions
  result = polycall_advanced_security_grant_permission(g_core_ctx, security_ctx,
                                                       1 // Permission ID
  );
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  bool has_permission =
      polycall_advanced_security_check_permission(security_ctx,
                                                  1 // Permission ID
      );
  ASSERT_TRUE(has_permission);

  result =
      polycall_advanced_security_revoke_permission(g_core_ctx, security_ctx,
                                                   1 // Permission ID
      );
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  has_permission =
      polycall_advanced_security_check_permission(security_ctx,
                                                  1 // Permission ID
      );
  ASSERT_FALSE(has_permission);

  // Test authentication (with mock credentials)
  char mock_credentials[] = "test:password123";
  result = polycall_advanced_security_authenticate(
      g_core_ctx, security_ctx, mock_credentials, strlen(mock_credentials));

  // In this test environment with mocks, we expect authentication to fail
  // since we haven't properly set up the credential validation
  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_UNAUTHORIZED, result);

  // Test key rotation
  result = polycall_advanced_security_rotate_keys(g_core_ctx, security_ctx);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Cleanup
  polycall_advanced_security_cleanup(g_core_ctx, security_ctx);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test connection pool module initialization and functions
 */
int test_connection_pool(void) {
  // Setup test environment
  setup_test_environment();

  // Initialize connection pool
  polycall_connection_pool_context_t *pool_ctx = NULL;
  polycall_connection_pool_config_t pool_config =
      polycall_connection_pool_default_config();

  polycall_core_error_t result =
      polycall_connection_pool_init(g_core_ctx, &pool_ctx, &pool_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(pool_ctx);

  // Test pool statistics retrieval
  polycall_connection_pool_stats_t stats;
  result = polycall_connection_pool_get_stats(g_core_ctx, pool_ctx, &stats);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test pool resize
  result = polycall_connection_pool_resize(g_core_ctx, pool_ctx,
                                           8 // New size
  );

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test pool validation
  result = polycall_connection_pool_validate(g_core_ctx, pool_ctx,
                                             true // Close invalid connections
  );

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test strategy setting
  result = polycall_connection_pool_set_strategy(g_core_ctx, pool_ctx,
                                                 POLYCALL_POOL_STRATEGY_LRU);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test warm-up functionality
  result =
      polycall_connection_pool_warm_up(g_core_ctx, pool_ctx,
                                       4 // Number of connections to warm up
      );

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // We can't effectively test acquire/release without real connections,
  // but we can validate the API doesn't crash with proper parameters
  polycall_protocol_context_t *conn_proto_ctx = NULL;
  result = polycall_connection_pool_acquire(g_core_ctx, pool_ctx,
                                            0, // Non-blocking
                                            &conn_proto_ctx);

  // Expect timeout since we have mock connections
  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_TIMEOUT, result);

  // Cleanup
  polycall_connection_pool_cleanup(g_core_ctx, pool_ctx);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test hierarchical state module initialization and functions
 */
int test_hierarchical_state(void) {
  // Setup test environment
  setup_test_environment();

  // Create a state machine first
  polycall_state_machine_t *state_machine = NULL;
  polycall_sm_status_t sm_status =
      polycall_sm_create(g_core_ctx, &state_machine);

  ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, sm_status);
  ASSERT_NOT_NULL(state_machine);

  // Initialize hierarchical state context
  polycall_hierarchical_state_context_t *hsm_ctx = NULL;
  polycall_core_error_t result =
      polycall_hierarchical_state_init(g_core_ctx, &hsm_ctx, state_machine);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(hsm_ctx);

  // Add root state
  polycall_hierarchical_state_config_t root_config = {
      .name = "root",
      .relationship = POLYCALL_STATE_RELATIONSHIP_PARENT,
      .parent_state = "",
      .on_enter = NULL,
      .on_exit = NULL,
      .inheritance_model = POLYCALL_PERMISSION_INHERIT_NONE,
      .permission_count = 1};
  root_config.permissions[0] = 1; // Root permission

  result = polycall_hierarchical_state_add(g_core_ctx, hsm_ctx, &root_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Add child state
  polycall_hierarchical_state_config_t child_config = {
      .name = "child",
      .relationship = POLYCALL_STATE_RELATIONSHIP_PARENT,
      .parent_state = "root",
      .on_enter = NULL,
      .on_exit = NULL,
      .inheritance_model = POLYCALL_PERMISSION_INHERIT_ADDITIVE,
      .permission_count = 1};
  child_config.permissions[0] = 2; // Child permission

  result = polycall_hierarchical_state_add(g_core_ctx, hsm_ctx, &child_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Add transition
  polycall_hierarchical_transition_config_t transition_config = {
      .name = "root_to_child",
      .from_state = "root",
      .to_state = "child",
      .type = POLYCALL_HTRANSITION_EXTERNAL,
      .guard = NULL};

  result = polycall_hierarchical_state_add_transition(g_core_ctx, hsm_ctx,
                                                      &transition_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test permission functions
  bool has_permission =
      polycall_hierarchical_state_has_permission(g_core_ctx, hsm_ctx, "root",
                                                 1 // Permission ID
      );

  ASSERT_TRUE(has_permission);

  has_permission =
      polycall_hierarchical_state_has_permission(g_core_ctx, hsm_ctx, "child",
                                                 1 // Should inherit from parent
      );

  ASSERT_TRUE(has_permission);

  // Get parent state
  char parent_buffer[POLYCALL_SM_MAX_NAME_LENGTH];
  result = polycall_hierarchical_state_get_parent(
      g_core_ctx, hsm_ctx, "child", parent_buffer, POLYCALL_SM_MAX_NAME_LENGTH);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_EQUAL_STR("root", parent_buffer);

  // Add permission
  result =
      polycall_hierarchical_state_add_permission(g_core_ctx, hsm_ctx, "root",
                                                 3 // New permission ID
      );

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Check new permission
  has_permission = polycall_hierarchical_state_has_permission(
      g_core_ctx, hsm_ctx, "root", 3);

  ASSERT_TRUE(has_permission);

  // Remove permission
  result = polycall_hierarchical_state_remove_permission(g_core_ctx, hsm_ctx,
                                                         "root", 3);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  has_permission = polycall_hierarchical_state_has_permission(
      g_core_ctx, hsm_ctx, "root", 3);

  ASSERT_FALSE(has_permission);

  // Cleanup
  polycall_hierarchical_state_cleanup(g_core_ctx, hsm_ctx);

  polycall_sm_destroy(state_machine);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test message optimization module initialization and functions
 */
int test_message_optimization(void) {
  // Setup test environment
  setup_test_environment();

  // Initialize message optimization
  polycall_message_optimization_context_t *opt_ctx = NULL;
  polycall_message_optimization_config_t opt_config =
      polycall_message_default_config();

  polycall_core_error_t result = polycall_message_optimization_init(
      g_core_ctx, g_proto_ctx, &opt_ctx, &opt_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(opt_ctx);

  // Test message optimization
  const char test_message[] = "This is a test message for optimization.";
  char optimized_buffer[256];
  size_t optimized_size = 0;

  result = polycall_message_optimize(
      g_core_ctx, opt_ctx, test_message, strlen(test_message), optimized_buffer,
      sizeof(optimized_buffer), &optimized_size, POLYCALL_MSG_PRIORITY_NORMAL);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_TRUE(optimized_size > 0);

  // Test message restoration
  char restored_buffer[256];
  size_t restored_size = 0;

  result = polycall_message_restore(g_core_ctx, opt_ctx, optimized_buffer,
                                    optimized_size, restored_buffer,
                                    sizeof(restored_buffer), &restored_size);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_EQUAL_INT(strlen(test_message), restored_size);
  ASSERT_MEMORY_EQUAL(test_message, restored_buffer, restored_size);

  // Test batching (add messages to batch)
  for (int i = 0; i < 5; i++) {
    char msg_buffer[64];
    snprintf(msg_buffer, sizeof(msg_buffer), "Batch message %d", i);

    result = polycall_message_batch_add(g_core_ctx, opt_ctx, msg_buffer,
                                        strlen(msg_buffer),
                                        POLYCALL_MSG_PRIORITY_NORMAL,
                                        0 // Generic message type
    );

    ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  }

  // Process batch
  char batch_buffer[1024];
  size_t batch_size = 0;

  result = polycall_message_batch_process(g_core_ctx, opt_ctx,
                                          true, // Force flush
                                          batch_buffer, sizeof(batch_buffer),
                                          &batch_size);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_TRUE(batch_size > 0);

  // Test statistics
  polycall_message_optimization_stats_t stats;
  result = polycall_message_get_stats(g_core_ctx, opt_ctx, &stats);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_TRUE(stats.total_messages > 0);

  // Reset statistics
  result = polycall_message_reset_stats(g_core_ctx, opt_ctx);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Change compression level
  result = polycall_message_set_compression(g_core_ctx, opt_ctx,
                                            POLYCALL_MSG_COMPRESSION_MAX);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Change batch strategy
  result = polycall_message_set_batch_strategy(
      g_core_ctx, opt_ctx, POLYCALL_BATCH_STRATEGY_TIME, NULL);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Cleanup
  polycall_message_optimization_cleanup(g_core_ctx, opt_ctx);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test subscription module initialization and functions
 */
int test_subscription(void) {
  // Setup test environment
  setup_test_environment();

  // Initialize subscription system
  polycall_subscription_context_t *sub_ctx = NULL;
  protocol_enhancement_subscription_config_t sub_config = {
      .max_subscriptions = 100,
      .enable_wildcards = true,
      .max_subscribers_per_topic = 10,
      .delivery_attempt_count = 3};

  polycall_core_error_t result = polycall_subscription_init(
      g_core_ctx, g_proto_ctx, &sub_ctx, &sub_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(sub_ctx);

  // Test subscription
  uint32_t subscription_id = 0;
  result = polycall_subscription_subscribe(g_core_ctx, sub_ctx, "test/topic",
                                           subscription_callback,
                                           NULL, // User data
                                           &subscription_id);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_TRUE(subscription_id > 0);

  // Test message publishing
  const char test_data[] = "Test message data";
  result = polycall_subscription_publish(g_core_ctx, sub_ctx, "test/topic",
                                         test_data, strlen(test_data));

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test wildcard subscription
  uint32_t wildcard_subscription_id = 0;
  result = polycall_subscription_subscribe(g_core_ctx, sub_ctx, "test/*",
                                           subscription_callback,
                                           NULL, // User data
                                           &wildcard_subscription_id);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_TRUE(wildcard_subscription_id > 0);

  // Test message publishing to match wildcard
  result = polycall_subscription_publish(g_core_ctx, sub_ctx, "test/other",
                                         test_data, strlen(test_data));

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Test unsubscribe
  result =
      polycall_subscription_unsubscribe(g_core_ctx, sub_ctx, subscription_id);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Cleanup
  polycall_subscription_cleanup(g_core_ctx, sub_ctx);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test protocol enhancements integration
 */
int test_enhancement_integration(void) {
  // Setup test environment
  setup_test_environment();

  // Create comprehensive configuration with all enhancements enabled
  polycall_protocol_enhancements_config_t config =
      polycall_protocol_enhancements_default_config();

  // Initialize enhancements
  polycall_core_error_t result = polycall_protocol_enhancements_init(
      g_core_ctx, g_proto_ctx, &g_enh_ctx, &config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(g_enh_ctx);

  // Test that all modules are initialized
  ASSERT_NOT_NULL(g_enh_ctx->security_ctx);
  ASSERT_NOT_NULL(g_enh_ctx->connection_pool_ctx);
  ASSERT_NOT_NULL(g_enh_ctx->hierarchical_ctx);
  ASSERT_NOT_NULL(g_enh_ctx->optimization_ctx);
  ASSERT_NOT_NULL(g_enh_ctx->subscription_ctx);

  // Verify callback registration
  // We can't directly test this, but we can check for lack of errors

  // Test integration between modules
  // For example, test secure publish/subscribe

  // Publish a message with optimization and security
  const char test_data[] = "Secure and optimized message";
  result = polycall_subscription_publish(
      g_core_ctx, g_enh_ctx->subscription_ctx, "secure/topic", test_data,
      strlen(test_data));

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Cleanup
  polycall_protocol_enhancements_cleanup(g_core_ctx, g_enh_ctx);
  g_enh_ctx = NULL;

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test error conditions and edge cases
 */
int test_error_conditions(void) {
  // Setup test environment
  setup_test_environment();

  // Test NULL parameter handling
  polycall_core_error_t result = polycall_protocol_enhancements_init(
      NULL, // NULL core context
      g_proto_ctx, &g_enh_ctx, polycall_protocol_enhancements_default_config());

  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);

  // Test invalid security parameters
  polycall_advanced_security_context_t *security_ctx = NULL;
  result = polycall_advanced_security_init(g_core_ctx, &security_ctx,
                                           NULL // NULL config
  );

  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);

  // Test connection pool with invalid max connections
  polycall_connection_pool_context_t *pool_ctx = NULL;
  polycall_connection_pool_config_t pool_config =
      polycall_connection_pool_default_config();
  pool_config.max_pool_size =
      POLYCALL_MAX_POOL_CONNECTIONS + 1; // Exceeds maximum

  result = polycall_connection_pool_init(g_core_ctx, &pool_ctx, &pool_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);

  // Test message optimization with zero-sized message
  polycall_message_optimization_context_t *opt_ctx = NULL;
  result = polycall_message_optimization_init(
      g_core_ctx, g_proto_ctx, &opt_ctx, polycall_message_default_config());

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(opt_ctx);

  char buffer[256];
  size_t size = 0;

  result = polycall_message_optimize(g_core_ctx, opt_ctx, "test",
                                     0, // Zero size
                                     buffer, sizeof(buffer), &size,
                                     POLYCALL_MSG_PRIORITY_NORMAL);

  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);

  // Cleanup
  polycall_message_optimization_cleanup(g_core_ctx, opt_ctx);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/**
 * @brief Test memory management and resource cleanup
 */
int test_memory_management(void) {
  // This test would ideally use a memory tracking framework to ensure
  // no leaks occur. For simplicity, we'll just verify proper initialization
  // and cleanup sequences.

  // Setup test environment
  setup_test_environment();

  // Test initialization and immediate cleanup of each module

  // 1. Advanced Security
  polycall_advanced_security_context_t *security_ctx = NULL;
  polycall_advanced_security_config_t security_config = {
      .initial_strategy = POLYCALL_AUTH_STRATEGY_SINGLE_FACTOR,
      .default_auth_method = POLYCALL_AUTH_METHOD_PASSWORD,
      .max_permissions = 32,
      .event_callback = security_event_callback,
      .user_data = NULL};

  polycall_core_error_t result = polycall_advanced_security_init(
      g_core_ctx, &security_ctx, &security_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(security_ctx);

  polycall_advanced_security_cleanup(g_core_ctx, security_ctx);

  // 2. Connection Pool
  polycall_connection_pool_context_t *pool_ctx = NULL;
  result = polycall_connection_pool_init(
      g_core_ctx, &pool_ctx, polycall_connection_pool_default_config());

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(pool_ctx);

  polycall_connection_pool_cleanup(g_core_ctx, pool_ctx);

  // 3. Message Optimization
  polycall_message_optimization_context_t *opt_ctx = NULL;
  result = polycall_message_optimization_init(
      g_core_ctx, g_proto_ctx, &opt_ctx, polycall_message_default_config());

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(opt_ctx);

  polycall_message_optimization_cleanup(g_core_ctx, opt_ctx);

  // 4. Subscription
  polycall_subscription_context_t *sub_ctx = NULL;
  protocol_enhancement_subscription_config_t sub_config = {
      .max_subscriptions = 100,
      .enable_wildcards = true,
      .max_subscribers_per_topic = 10,
      .delivery_attempt_count = 3};

  result = polycall_subscription_init(g_core_ctx, g_proto_ctx, &sub_ctx,
                                      &sub_config);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(sub_ctx);

  polycall_subscription_cleanup(g_core_ctx, sub_ctx);

  // Teardown test environment
  teardown_test_environment();

  return 0;
}

/*---------------------------------------------------------------------------*/
/* Helper implementation */
/*---------------------------------------------------------------------------*/

/**
 * @brief Set up test environment with mock contexts
 */
static void setup_test_environment(void) {
  // Initialize mock core context
  g_core_ctx = mock_core_context_create();
  assert(g_core_ctx != NULL);

  // Initialize mock endpoint
  g_endpoint = mock_network_endpoint_create();
  assert(g_endpoint != NULL);

  // Initialize mock protocol context
  g_proto_ctx = mock_protocol_context_create(g_core_ctx, g_endpoint);
  assert(g_proto_ctx != NULL);
}

/**
 * @brief Tear down test environment
 */
static void teardown_test_environment(void) {
  // Clean up in reverse order
  if (g_proto_ctx) {
    mock_protocol_context_destroy(g_proto_ctx);
    g_proto_ctx = NULL;
  }

  if (g_endpoint) {
    mock_network_endpoint_destroy(g_endpoint);
    g_endpoint = NULL;
  }

  if (g_core_ctx) {
    mock_core_context_destroy(g_core_ctx);
    g_core_ctx = NULL;
  }
}

/**
 * @brief Security event callback for testing
 */
static void security_event_callback(uint32_t event_id, void *event_data,
                                    void *user_data) {
  // For testing, just print event ID
  printf("Security event received: %u\n", event_id);
}

/**
 * @brief Subscription callback for testing
 */
static void subscription_callback(const char *topic, const void *data,
                                  size_t data_size, void *user_data) {
  // For testing, just print message info
  printf("Received message on topic '%s' with %zu bytes\n", topic, data_size);
}

/*---------------------------------------------------------------------------*/
/* Main function */
/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {
  // Reset test counters
  RESET_TESTS();

  // Run tests
  printf("Running LibPolyCall Protocol Enhancements tests...\n");

  RUN_TEST(test_enhancement_init_cleanup);
  RUN_TEST(test_advanced_security);
  RUN_TEST(test_connection_pool);
  RUN_TEST(test_hierarchical_state);
  RUN_TEST(test_message_optimization);
  RUN_TEST(test_subscription);
  RUN_TEST(test_enhancement_integration);
  RUN_TEST(test_error_conditions);
  RUN_TEST(test_memory_management);

  // Report results and return appropriate exit code
  return TEST_REPORT();
}
