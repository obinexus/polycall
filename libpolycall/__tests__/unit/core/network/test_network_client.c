/**
 * @file test_network_client.c
 * @brief Unit tests for the network client module of LibPolyCall
 * @author Unit tests for LibPolyCall
 *
 * This file contains unit tests for the client-side networking interface in LibPolyCall.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>
 
 #include "polycall/core/network/network_client.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_memory.h"
 
 // Mock protocol context for testing
 typedef struct {
     int dummy;
 } mock_protocol_context_t;
 
 // Test connection callback
 static int test_connection_count = 0;
 static void test_connection_callback(
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint,
     bool connected,
     void* user_data
 ) {
     test_connection_count++;
 }
 
 // Test error callback
 static int test_error_count = 0;
 static void test_error_callback(
     polycall_network_client_t* client,
     polycall_core_error_t error,
     const char* message,
     void* user_data
 ) {
     test_error_count++;
 }
 
 // Event handler for client events
 static int test_event_count = 0;
 static void test_event_handler(
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint,
     void* event_data,
     void* user_data
 ) {
     test_event_count++;
 }
 
 // Helper function to create a core context for testing
 static polycall_core_context_t* create_test_core_context(void) {
     polycall_core_context_t* ctx = malloc(sizeof(polycall_core_context_t));
     assert(ctx != NULL);
     memset(ctx, 0, sizeof(polycall_core_context_t));
     
     // Initialize memory functions
     ctx->memory_allocate = malloc;
     ctx->memory_free = free;
     ctx->memory_realloc = realloc;
     
     return ctx;
 }
 
 void test_client_create_cleanup(void) {
     printf("Testing client_create and client_cleanup functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Test with NULL parameters
     polycall_network_client_t* client = NULL;
     polycall_core_error_t result = polycall_network_client_create(NULL, proto_ctx, &client, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_create(core_ctx, NULL, &client, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_create(core_ctx, proto_ctx, NULL, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters but default config
     result = polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(client != NULL);
     
     // Verify the client was initialized properly
     assert(client->core_ctx == core_ctx);
     assert(client->proto_ctx == proto_ctx);
     assert(client->initialized == true);
     assert(client->endpoints == NULL);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     
     // Test with custom config
     polycall_network_client_config_t config = polycall_network_client_create_default_config();
     config.connect_timeout_ms = 5000;
     config.enable_auto_reconnect = false;
     config.connection_callback = test_connection_callback;
     config.error_callback = test_error_callback;
     
     result = polycall_network_client_create(core_ctx, proto_ctx, &client, &config);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(client != NULL);
     
     // Verify config was applied
     assert(client->config.connect_timeout_ms == 5000);
     assert(client->config.enable_auto_reconnect == false);
     assert(client->connection_callback == test_connection_callback);
     assert(client->error_callback == test_error_callback);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(core_ctx);
     
     printf("client_create and client_cleanup tests passed!\n");
 }
 
 void test_client_connect_disconnect(void) {
     printf("Testing client_connect and client_disconnect functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client with callbacks
     polycall_network_client_config_t config = polycall_network_client_create_default_config();
     config.connection_callback = test_connection_callback;
     config.error_callback = test_error_callback;
     
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, &config);
     
     // NOTE: Due to limitations of unit testing without a network,
     // we can't actually connect to a real endpoint. Instead, we'll
     // test the parameter validation of these functions.
     
     // Test connect with NULL parameters
     polycall_endpoint_t* endpoint = NULL;
     polycall_core_error_t result = polycall_network_client_connect(NULL, client, "127.0.0.1", 8080, 1000, &endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_connect(core_ctx, NULL, "127.0.0.1", 8080, 1000, &endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_connect(core_ctx, client, NULL, 8080, 1000, &endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_connect(core_ctx, client, "127.0.0.1", 8080, 1000, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test disconnect with NULL parameters
     result = polycall_network_client_disconnect(NULL, client, endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_disconnect(core_ctx, NULL, endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_disconnect(core_ctx, client, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(core_ctx);
     
     printf("client_connect and client_disconnect tests passed!\n");
 }
 
 void test_client_send_receive(void) {
     printf("Testing client_send and client_receive functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     
     // Create a mock packet
     polycall_network_packet_t* packet = malloc(sizeof(polycall_network_packet_t));
     assert(packet != NULL);
     memset(packet, 0, sizeof(polycall_network_packet_t));
     
     // Mock endpoint
     polycall_endpoint_t* endpoint = malloc(sizeof(polycall_endpoint_t));
     assert(endpoint != NULL);
     memset(endpoint, 0, sizeof(polycall_endpoint_t));
     
     // NOTE: Again, due to limitations of unit testing without a network,
     // we can only test parameter validation.
     
     // Test send with NULL parameters
     polycall_core_error_t result = polycall_network_client_send(NULL, client, endpoint, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send(core_ctx, NULL, endpoint, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send(core_ctx, client, NULL, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send(core_ctx, client, endpoint, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test receive with NULL parameters
     polycall_network_packet_t* recv_packet = NULL;
     result = polycall_network_client_receive(NULL, client, endpoint, &recv_packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_receive(core_ctx, NULL, endpoint, &recv_packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_receive(core_ctx, client, NULL, &recv_packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_receive(core_ctx, client, endpoint, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(endpoint);
     free(packet);
     free(core_ctx);
     
     printf("client_send and client_receive tests passed!\n");
 }
 
 void test_client_send_message(void) {
     printf("Testing client_send_message function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     
     // Mock endpoint
     polycall_endpoint_t* endpoint = malloc(sizeof(polycall_endpoint_t));
     assert(endpoint != NULL);
     memset(endpoint, 0, sizeof(polycall_endpoint_t));
     
     // Mock message
     polycall_message_t* message = malloc(sizeof(polycall_message_t));
     assert(message != NULL);
     memset(message, 0, sizeof(polycall_message_t));
     
     // Test with NULL parameters
     polycall_message_t* response = NULL;
     polycall_core_error_t result = polycall_network_client_send_message(
         NULL, client, proto_ctx, endpoint, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send_message(
         core_ctx, NULL, proto_ctx, endpoint, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send_message(
         core_ctx, client, NULL, endpoint, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send_message(
         core_ctx, client, proto_ctx, NULL, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_send_message(
         core_ctx, client, proto_ctx, endpoint, NULL, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(endpoint);
     free(message);
     free(core_ctx);
     
     printf("client_send_message tests passed!\n");
 }
 
 void test_client_set_event_callback(void) {
     printf("Testing client_set_event_callback function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     
     // Test with NULL parameters
     test_event_count = 0;
     polycall_core_error_t result = polycall_network_client_set_event_callback(
         NULL, client, POLYCALL_NETWORK_EVENT_CONNECT, test_event_handler, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_set_event_callback(
         core_ctx, NULL, POLYCALL_NETWORK_EVENT_CONNECT, test_event_handler, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_set_event_callback(
         core_ctx, client, POLYCALL_NETWORK_EVENT_CONNECT, NULL, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters
     result = polycall_network_client_set_event_callback(
         core_ctx, client, POLYCALL_NETWORK_EVENT_CONNECT, test_event_handler, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Register for another event type
     result = polycall_network_client_set_event_callback(
         core_ctx, client, POLYCALL_NETWORK_EVENT_DISCONNECT, test_event_handler, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(core_ctx);
     
     printf("client_set_event_callback tests passed!\n");
 }
 
 void test_client_process_events(void) {
     printf("Testing client_process_events function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     
     // Test with NULL parameters
     polycall_core_error_t result = polycall_network_client_process_events(NULL, client, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_process_events(core_ctx, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters (no active endpoints, should timeout)
     result = polycall_network_client_process_events(core_ctx, client, 100);
     assert(result == POLYCALL_CORE_ERROR_TIMEOUT);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(core_ctx);
     
     printf("client_process_events tests passed!\n");
 }
 
 void test_client_get_stats(void) {
     printf("Testing client_get_stats function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     
     // Test with NULL parameters
     polycall_network_stats_t stats;
     polycall_core_error_t result = polycall_network_client_get_stats(NULL, client, &stats);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_get_stats(core_ctx, NULL, &stats);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_get_stats(core_ctx, client, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters
     result = polycall_network_client_get_stats(core_ctx, client, &stats);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Initial stats should all be zero
     assert(stats.active_connections == 0);
     assert(stats.connection_attempts == 0);
     assert(stats.bytes_sent == 0);
     assert(stats.bytes_received == 0);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(core_ctx);
     
     printf("client_get_stats tests passed!\n");
 }
 
 void test_client_options(void) {
     printf("Testing client options functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create client
     polycall_network_client_t* client = NULL;
     polycall_network_client_create(core_ctx, proto_ctx, &client, NULL);
     
     // Test set_option with NULL parameters
     int buf_size = 16384;
     polycall_core_error_t result = polycall_network_client_set_option(
         NULL, client, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, &buf_size, sizeof(buf_size));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_set_option(
         core_ctx, NULL, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, &buf_size, sizeof(buf_size));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_set_option(
         core_ctx, client, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, NULL, sizeof(buf_size));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters - need a mock endpoint for this to succeed
     // In a real test environment, we would create a connected endpoint
     
     // Test get_option with NULL parameters
     int retrieved_buf_size = 0;
     size_t size = sizeof(retrieved_buf_size);
     result = polycall_network_client_get_option(
         NULL, client, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, &retrieved_buf_size, &size);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_get_option(
         core_ctx, NULL, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, &retrieved_buf_size, &size);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_get_option(
         core_ctx, client, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, NULL, &size);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_client_get_option(
         core_ctx, client, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, &retrieved_buf_size, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters - will likely fail without a real endpoint
     // We expect POLYCALL_CORE_ERROR_INVALID_STATE here
     result = polycall_network_client_get_option(
         core_ctx, client, POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE, &retrieved_buf_size, &size);
     assert(result == POLYCALL_CORE_ERROR_INVALID_STATE);
     
     // Clean up
     polycall_network_client_cleanup(core_ctx, client);
     free(core_ctx);
     
     printf("client options tests passed!\n");
 }
 
 void test_client_default_config(void) {
     printf("Testing client_create_default_config function...\n");
     
     // Test default config creation
     polycall_network_client_config_t config = polycall_network_client_create_default_config();
     
     // Verify default values
     assert(config.connect_timeout_ms == 30000);
     assert(config.operation_timeout_ms == 30000);
     assert(config.keep_alive_interval_ms == 60000);
     assert(config.max_reconnect_attempts == 5);
     assert(config.reconnect_delay_ms == 5000);
     assert(config.enable_auto_reconnect == true);
     assert(config.enable_tls == false);
     assert(config.tls_cert_file == NULL);
     assert(config.tls_key_file == NULL);
     assert(config.tls_ca_file == NULL);
     assert(config.max_pending_requests == DEFAULT_MAX_PENDING_REQUESTS);
     assert(config.max_message_size == 1024 * 1024);
     assert(config.user_data == NULL);
     assert(config.connection_callback == NULL);
     assert(config.error_callback == NULL);
     
     printf("client_create_default_config tests passed!\n");
 }
 
 int main(void) {
     printf("Running network client module unit tests...\n");
     
     // Run all test functions
     test_client_create_cleanup();
     test_client_connect_disconnect();
     test_client_send_receive();
     test_client_send_message();
     test_client_set_event_callback();
     test_client_process_events();
     test_client_get_stats();
     test_client_options();
     test_client_default_config();
     
     printf("All network client module tests passed!\n");
     return 0;
 }