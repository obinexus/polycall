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
 * @file test_network.c
 * @brief Unit tests for the network module of LibPolyCall
 * @author Unit tests for LibPolyCall
 *
 * This file contains unit tests for the main network interface in LibPolyCall.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "polycall/core/network/network.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/protocol/polycall_protocol_context.h"

// Mock protocol context for testing
typedef struct {
  int dummy;
} mock_protocol_context_t;

// Test event handler callback
static int test_event_count = 0;
static void test_event_handler(polycall_network_context_t *ctx,
                               polycall_endpoint_t *endpoint, void *event_data,
                               void *user_data) {
  test_event_count++;
}

void test_network_init_cleanup(void) {
  printf("Testing network_init and network_cleanup functions...\n");

  // Initialize core context (usually done by the application)
  polycall_core_context_t *core_ctx = malloc(sizeof(polycall_core_context_t));
  assert(core_ctx != NULL);

  // Test with NULL parameters
  polycall_network_context_t *net_ctx = NULL;
  polycall_core_error_t result = polycall_network_init(NULL, &net_ctx, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_init(core_ctx, NULL, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters but default config
  result = polycall_network_init(core_ctx, &net_ctx, NULL);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(net_ctx != NULL);

  // Verify the network context was initialized properly
  assert(net_ctx->core_ctx == core_ctx);
  assert(net_ctx->flags & POLYCALL_NETWORK_FLAG_INITIALIZED);
  assert(net_ctx->flags & POLYCALL_NETWORK_FLAG_SUBSYSTEM_INITIALIZED);

  // Clean up
  polycall_network_cleanup(core_ctx, net_ctx);

  // Test with custom config
  polycall_network_config_t config = polycall_network_create_default_config();
  config.thread_pool_size = 2;
  config.max_connections = 50;
  config.enable_compression = true;

  result = polycall_network_init(core_ctx, &net_ctx, &config);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(net_ctx != NULL);

  // Verify config was applied
  assert(net_ctx->worker_thread_count == 2);
  assert(net_ctx->config.max_connections == 50);
  assert(net_ctx->flags & POLYCALL_NETWORK_FLAG_COMPRESSED);

  // Clean up
  polycall_network_cleanup(core_ctx, net_ctx);
  free(core_ctx);

  printf("network_init and network_cleanup tests passed!\n");
}

void test_network_create_client(void) {
  printf("Testing network_create_client function...\n");

  // Initialize core and network contexts
  polycall_core_context_t *core_ctx = malloc(sizeof(polycall_core_context_t));
  polycall_network_context_t *net_ctx = NULL;
  polycall_network_init(core_ctx, &net_ctx, NULL);

  // Create a mock protocol context
  mock_protocol_context_t mock_proto;
  polycall_protocol_context_t *proto_ctx =
      (polycall_protocol_context_t *)&mock_proto;

  // Test with NULL parameters
  polycall_network_client_t *client = NULL;
  polycall_core_error_t result =
      polycall_network_create_client(NULL, net_ctx, proto_ctx, &client, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_create_client(core_ctx, NULL, proto_ctx, &client, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_create_client(core_ctx, net_ctx, NULL, &client, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_create_client(core_ctx, net_ctx, proto_ctx, NULL, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters but default config
  result = polycall_network_create_client(core_ctx, net_ctx, proto_ctx, &client,
                                          NULL);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(client != NULL);

  // Create client with custom config
  polycall_network_client_config_t config =
      polycall_network_client_create_default_config();
  config.connect_timeout_ms = 5000;
  config.enable_auto_reconnect = false;

  polycall_network_client_t *client2 = NULL;
  result = polycall_network_create_client(core_ctx, net_ctx, proto_ctx,
                                          &client2, &config);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(client2 != NULL);

  // Clean up
  polycall_network_client_cleanup(core_ctx, client);
  polycall_network_client_cleanup(core_ctx, client2);
  polycall_network_cleanup(core_ctx, net_ctx);
  free(core_ctx);

  printf("network_create_client tests passed!\n");
}

void test_network_create_server(void) {
  printf("Testing network_create_server function...\n");

  // Initialize core and network contexts
  polycall_core_context_t *core_ctx = malloc(sizeof(polycall_core_context_t));
  polycall_network_context_t *net_ctx = NULL;
  polycall_network_init(core_ctx, &net_ctx, NULL);

  // Create a mock protocol context
  mock_protocol_context_t mock_proto;
  polycall_protocol_context_t *proto_ctx =
      (polycall_protocol_context_t *)&mock_proto;

  // Test with NULL parameters
  polycall_network_server_t *server = NULL;
  polycall_core_error_t result =
      polycall_network_create_server(NULL, net_ctx, proto_ctx, &server, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_create_server(core_ctx, NULL, proto_ctx, &server, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_create_server(core_ctx, net_ctx, NULL, &server, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_create_server(core_ctx, net_ctx, proto_ctx, NULL, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters but default config
  result = polycall_network_create_server(core_ctx, net_ctx, proto_ctx, &server,
                                          NULL);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(server != NULL);

  // Create server with custom config
  polycall_network_server_config_t config =
      polycall_network_server_create_default_config();
  config.port = 8888;
  config.backlog = 20;
  config.max_connections = 50;

  polycall_network_server_t *server2 = NULL;
  result = polycall_network_create_server(core_ctx, net_ctx, proto_ctx,
                                          &server2, &config);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(server2 != NULL);

  // Clean up
  polycall_network_server_cleanup(core_ctx, server);
  polycall_network_server_cleanup(core_ctx, server2);
  polycall_network_cleanup(core_ctx, net_ctx);
  free(core_ctx);

  printf("network_create_server tests passed!\n");
}

void test_network_register_event_handler(void) {
  printf("Testing network_register_event_handler function...\n");

  // Initialize core and network contexts
  polycall_core_context_t *core_ctx = malloc(sizeof(polycall_core_context_t));
  polycall_network_context_t *net_ctx = NULL;
  polycall_network_init(core_ctx, &net_ctx, NULL);

  // Test with NULL parameters
  polycall_core_error_t result = polycall_network_register_event_handler(
      NULL, net_ctx, POLYCALL_NETWORK_EVENT_CONNECT, test_event_handler, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_register_event_handler(
      core_ctx, NULL, POLYCALL_NETWORK_EVENT_CONNECT, test_event_handler, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_register_event_handler(
      core_ctx, net_ctx, POLYCALL_NETWORK_EVENT_CONNECT, NULL, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  test_event_count = 0;
  result = polycall_network_register_event_handler(
      core_ctx, net_ctx, POLYCALL_NETWORK_EVENT_CONNECT, test_event_handler,
      NULL);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Register for another event type
  result = polycall_network_register_event_handler(
      core_ctx, net_ctx, POLYCALL_NETWORK_EVENT_DISCONNECT, test_event_handler,
      NULL);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Try to trigger events (without actual connection, just for checking
  // registration)
  trigger_event(net_ctx, NULL, POLYCALL_NETWORK_EVENT_CONNECT, NULL);
  assert(test_event_count == 1);

  trigger_event(net_ctx, NULL, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
  assert(test_event_count == 2);

  // Clean up
  polycall_network_cleanup(core_ctx, net_ctx);
  free(core_ctx);

  printf("network_register_event_handler tests passed!\n");
}

void test_network_options(void) {
  printf("Testing network options functions...\n");

  // Initialize core and network contexts
  polycall_core_context_t *core_ctx = malloc(sizeof(polycall_core_context_t));
  polycall_network_context_t *net_ctx = NULL;
  polycall_network_init(core_ctx, &net_ctx, NULL);

  // Test set_option with NULL parameters
  bool enable_compression = true;
  polycall_core_error_t result = polycall_network_set_option(
      NULL, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &enable_compression, sizeof(enable_compression));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_set_option(
      core_ctx, NULL, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &enable_compression, sizeof(enable_compression));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_set_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED, NULL,
      sizeof(enable_compression));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  result = polycall_network_set_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &enable_compression, sizeof(enable_compression));
  assert(result == POLYCALL_CORE_SUCCESS);

  // Test get_option with NULL parameters
  bool compression_enabled = false;
  size_t size = sizeof(compression_enabled);
  result = polycall_network_get_option(
      NULL, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &compression_enabled, &size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_get_option(
      core_ctx, NULL, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &compression_enabled, &size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_get_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED, NULL,
      &size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_get_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &compression_enabled, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  result = polycall_network_get_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED,
      &compression_enabled, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(compression_enabled == true);

  // Test setting and getting timeout option
  uint32_t timeout = 5000;
  result = polycall_network_set_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_OPERATION_TIMEOUT, &timeout,
      sizeof(timeout));
  assert(result == POLYCALL_CORE_SUCCESS);

  uint32_t retrieved_timeout = 0;
  size = sizeof(retrieved_timeout);
  result = polycall_network_get_option(
      core_ctx, net_ctx, POLYCALL_NETWORK_OPTION_OPERATION_TIMEOUT,
      &retrieved_timeout, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(retrieved_timeout == 5000);

  // Clean up
  polycall_network_cleanup(core_ctx, net_ctx);
  free(core_ctx);

  printf("network options tests passed!\n");
}

void test_network_stats(void) {
  printf("Testing network_get_stats function...\n");

  // Initialize core and network contexts
  polycall_core_context_t *core_ctx = malloc(sizeof(polycall_core_context_t));
  polycall_network_context_t *net_ctx = NULL;
  polycall_network_init(core_ctx, &net_ctx, NULL);

  // Test with NULL parameters
  polycall_network_stats_t stats;
  polycall_core_error_t result =
      polycall_network_get_stats(NULL, net_ctx, &stats);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_get_stats(core_ctx, NULL, &stats);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_get_stats(core_ctx, net_ctx, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  result = polycall_network_get_stats(core_ctx, net_ctx, &stats);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Initial stats should be at their defaults
  assert(stats.active_clients == 0);
  assert(stats.active_servers == 0);
  assert(stats.active_connections == 0);

  // Clean up
  polycall_network_cleanup(core_ctx, net_ctx);
  free(core_ctx);

  printf("network_get_stats tests passed!\n");
}

void test_network_subsystem(void) {
  printf("Testing network subsystem functions...\n");

  // Test initialization and cleanup
  polycall_core_error_t result = polycall_network_subsystem_init();
  assert(result == POLYCALL_CORE_SUCCESS);

  polycall_network_subsystem_cleanup();

  // Test version function
  const char *version = polycall_network_get_version();
  assert(version != NULL);
  assert(strcmp(version, POLYCALL_NETWORK_VERSION) == 0);

  printf("network subsystem tests passed!\n");
}

int main(void) {
  printf("Running network module unit tests...\n");

  // Run all test functions
  test_network_init_cleanup();
  test_network_create_client();
  test_network_create_server();
  test_network_register_event_handler();
  test_network_options();
  test_network_stats();
  test_network_subsystem();

  printf("All network module tests passed!\n");
  return 0;
}
