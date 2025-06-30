/**
 * @file test_network_server.c
 * @brief Unit tests for the network server module of LibPolyCall
 * @author Unit tests for LibPolyCall
 *
 * This file contains unit tests for the server-side networking interface in LibPolyCall.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>
 
 #include "polycall/core/network/network_server.h"
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
     polycall_network_server_t* server,
     polycall_endpoint_t* endpoint,
     bool connected,
     void* user_data
 ) {
     test_connection_count++;
 }
 
 // Test error callback
 static int test_error_count = 0;
 static void test_error_callback(
     polycall_network_server_t* server,
     polycall_core_error_t error,
     const char* message,
     void* user_data
 ) {
     test_error_count++;
 }
 
 // Message handler for server message handling
 static int test_message_count = 0;
 static polycall_core_error_t test_message_handler(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_endpoint_t* endpoint,
     polycall_message_t* message,
     polycall_message_t** response,
     void* user_data
 ) {
     test_message_count++;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Event handler for server events
 static int test_event_count = 0;
 static void test_event_handler(
     polycall_network_server_t* server,
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
 
 void test_server_create_cleanup(void) {
     printf("Testing server_create and server_cleanup functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Test with NULL parameters
     polycall_network_server_t* server = NULL;
     polycall_core_error_t result = polycall_network_server_create(NULL, proto_ctx, &server, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_create(core_ctx, NULL, &server, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_create(core_ctx, proto_ctx, NULL, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters but default config
     result = polycall_network_server_create(core_ctx, proto_ctx, &server, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(server != NULL);
     
     // Verify the server was initialized properly
     assert(server->core_ctx == core_ctx);
     assert(server->proto_ctx == proto_ctx);
     assert(server->initialized == true);
     assert(server->running == false);
     assert(server->endpoints == NULL);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server);
     free(packet);
     free(core_ctx);
     
     printf("server_broadcast tests passed!\n");
 });
     
     // Test with custom config
     polycall_network_server_config_t config = polycall_network_server_create_default_config();
     config.port = 9090;
     config.backlog = 20;
     config.max_connections = 100;
     config.connection_callback = test_connection_callback;
     config.error_callback = test_error_callback;
     config.message_handler = test_message_handler;
     
     result = polycall_network_server_create(core_ctx, proto_ctx, &server, &config);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(server != NULL);
     
     // Verify config was applied
     assert(server->config.port == 9090);
     assert(server->config.backlog == 20);
     assert(server->config.max_connections == 100);
     assert(server->config.connection_callback == test_connection_callback);
     assert(server->config.error_callback == test_error_callback);
     assert(server->config.message_handler == test_message_handler);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server);
     free(core_ctx);
     
     printf("server_create and server_cleanup tests passed!\n");
 }
 
 void test_server_start_stop(void) {
     printf("Testing server_start and server_stop functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create server with callbacks
     polycall_network_server_config_t config = polycall_network_server_create_default_config();
     config.connection_callback = test_connection_callback;
     config.error_callback = test_error_callback;
     
     polycall_network_server_t* server = NULL;
     polycall_network_server_create(core_ctx, proto_ctx, &server, &config);
     
     // NOTE: Since we can't actually start the server in a unit test environment
     // (it would try to bind to a real port), we'll just test the parameter validation.
     
     // Test start with NULL parameters
     polycall_core_error_t result = polycall_network_server_start(NULL, server);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_start(core_ctx, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test stop with NULL parameters
     result = polycall_network_server_stop(NULL, server);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_stop(core_ctx, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server);
     free(core_ctx);
     
     printf("server_start and server_stop tests passed!\n");
 }
 
 void test_server_accept_disconnect(void) {
     printf("Testing server_accept and server_disconnect functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create server
     polycall_network_server_t* server = NULL;
     polycall_network_server_create(core_ctx, proto_ctx, &server, NULL);
     
     // Test accept with NULL parameters
     polycall_endpoint_t* endpoint = NULL;
     polycall_core_error_t result = polycall_network_server_accept(NULL, server, &endpoint, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_accept(core_ctx, NULL, &endpoint, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_accept(core_ctx, server, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test disconnect with NULL parameters
     result = polycall_network_server_disconnect(NULL, server, endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_disconnect(core_ctx, NULL, endpoint);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_disconnect(core_ctx, server, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server);
     free(core_ctx);
     
     printf("server_accept and server_disconnect tests passed!\n");
 }
 
 void test_server_send_receive(void) {
     printf("Testing server_send and server_receive functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create server
     polycall_network_server_t* server = NULL;
     polycall_network_server_create(core_ctx, proto_ctx, &server, NULL);
     
     // Create a mock packet
     polycall_network_packet_t* packet = malloc(sizeof(polycall_network_packet_t));
     assert(packet != NULL);
     memset(packet, 0, sizeof(polycall_network_packet_t));
     
     // Mock endpoint
     polycall_endpoint_t* endpoint = malloc(sizeof(polycall_endpoint_t));
     assert(endpoint != NULL);
     memset(endpoint, 0, sizeof(polycall_endpoint_t));
     
     // Test send with NULL parameters
     polycall_core_error_t result = polycall_network_server_send(NULL, server, endpoint, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send(core_ctx, NULL, endpoint, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send(core_ctx, server, NULL, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send(core_ctx, server, endpoint, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test receive with NULL parameters
     polycall_network_packet_t* recv_packet = NULL;
     result = polycall_network_server_receive(NULL, server, endpoint, &recv_packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_receive(core_ctx, NULL, endpoint, &recv_packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_receive(core_ctx, server, NULL, &recv_packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_receive(core_ctx, server, endpoint, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server);
     free(endpoint);
     free(packet);
     free(core_ctx);
     
     printf("server_send and server_receive tests passed!\n");
 }
 
 void test_server_send_message(void) {
     printf("Testing server_send_message function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create server
     polycall_network_server_t* server = NULL;
     polycall_network_server_create(core_ctx, proto_ctx, &server, NULL);
     
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
     polycall_core_error_t result = polycall_network_server_send_message(
         NULL, server, proto_ctx, endpoint, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send_message(
         core_ctx, NULL, proto_ctx, endpoint, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send_message(
         core_ctx, server, NULL, endpoint, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send_message(
         core_ctx, server, proto_ctx, NULL, message, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_send_message(
         core_ctx, server, proto_ctx, endpoint, NULL, 1000, &response);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server);
     free(endpoint);
     free(message);
     free(core_ctx);
     
     printf("server_send_message tests passed!\n");
 }
 
 void test_server_broadcast(void) {
     printf("Testing server_broadcast function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create a mock protocol context
     mock_protocol_context_t mock_proto;
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)&mock_proto;
     
     // Create server
     polycall_network_server_t* server = NULL;
     polycall_network_server_create(core_ctx, proto_ctx, &server, NULL);
     
     // Create a mock packet
     polycall_network_packet_t* packet = malloc(sizeof(polycall_network_packet_t));
     assert(packet != NULL);
     memset(packet, 0, sizeof(polycall_network_packet_t));
     
     // Test with NULL parameters
     polycall_core_error_t result = polycall_network_server_broadcast(NULL, server, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_broadcast(core_ctx, NULL, packet, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_server_broadcast(core_ctx, server, NULL, 1000);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_server_cleanup(core_ctx, server