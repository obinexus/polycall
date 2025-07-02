/**
 * @file test_message.c
 * @brief Unit tests for protocol message handling functionality
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 #ifdef USE_CHECK_FRAMEWORK
 #include <check.h>
 #else
 #include "unit_tests_framwork.h"
 #endif
 
 #include "polycall/core/protocol/message.h"
 #include "polycall/core/polycall/polycall_core.h"
 
 // Test fixture
 static polycall_core_context_t* test_ctx = NULL;
 
 // Setup function - runs before each test
 static void setup(void) {
     // Initialize test context
     test_ctx = polycall_core_create();
 }
 
 // Teardown function - runs after each test
 static void teardown(void) {
     // Clean up message pool
     polycall_message_cleanup_pool(test_ctx);
     
     // Free resources
     if (test_ctx) {
         polycall_core_destroy(test_ctx);
         test_ctx = NULL;
     }
 }
 
 // Test callback function for message handlers
 static polycall_core_error_t test_message_handler(
     polycall_core_context_t* ctx,
     polycall_message_t* message,
     void* user_data
 ) {
     int* counter = (int*)user_data;
     if (counter) {
         (*counter)++;
     }
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Test message creation
 static int test_message_create(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_core_error_t result = polycall_message_create(
         test_ctx, &message, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(message);
     
     // Verify message type
     ASSERT_EQUAL_INT(POLYCALL_MESSAGE_TYPE_HANDSHAKE, message->header.type);
     
     // Verify message properties
     ASSERT_EQUAL_INT(0, message->header.flags);
     ASSERT_EQUAL_INT(0, message->header.sequence);
     ASSERT_EQUAL_INT(0, message->header.payload_size);
     ASSERT_EQUAL_INT(0, message->header.metadata_size);
     ASSERT_NULL(message->payload);
     ASSERT_NULL(message->metadata);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Test setting payload
 static int test_set_payload(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_COMMAND);
     
     // Set payload
     const char* test_payload = "Test payload data";
     size_t payload_size = strlen(test_payload) + 1;
     
     polycall_core_error_t result = polycall_message_set_payload(
         test_ctx, message, test_payload, payload_size);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(message->payload);
     ASSERT_EQUAL_INT(payload_size, message->payload_size);
     ASSERT_EQUAL_INT(payload_size, message->header.payload_size);
     
     // Verify payload content
     ASSERT_MEMORY_EQUAL(test_payload, message->payload, payload_size);
     
     // Change payload
     const char* new_payload = "New payload";
     size_t new_size = strlen(new_payload) + 1;
     
     result = polycall_message_set_payload(test_ctx, message, new_payload, new_size);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(message->payload);
     ASSERT_EQUAL_INT(new_size, message->payload_size);
     ASSERT_EQUAL_INT(new_size, message->header.payload_size);
     
     // Verify new payload content
     ASSERT_MEMORY_EQUAL(new_payload, message->payload, new_size);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Test setting metadata
 static int test_set_metadata(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_RESPONSE);
     
     // Set metadata
     const char* test_metadata = "Test metadata";
     size_t metadata_size = strlen(test_metadata) + 1;
     
     polycall_core_error_t result = polycall_message_set_metadata(
         test_ctx, message, test_metadata, metadata_size);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(message->metadata);
     ASSERT_EQUAL_INT(metadata_size, message->metadata_size);
     ASSERT_EQUAL_INT(metadata_size, message->header.metadata_size);
     
     // Verify metadata content
     ASSERT_MEMORY_EQUAL(test_metadata, message->metadata, metadata_size);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Test setting flags
 static int test_set_flags(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
     
     // Set flags
     polycall_message_flags_t flags = POLYCALL_MESSAGE_FLAG_ENCRYPTED | POLYCALL_MESSAGE_FLAG_REQUIRES_ACK;
     
     polycall_core_error_t result = polycall_message_set_flags(test_ctx, message, flags);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(flags, message->header.flags);
     
     // Get flags
     polycall_message_flags_t retrieved_flags = polycall_message_get_flags(message);
     ASSERT_EQUAL_INT(flags, retrieved_flags);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Test serialization and deserialization
 static int test_serialization(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_COMMAND);
     
     // Set payload and metadata
     const char* test_payload = "Test payload for serialization";
     const char* test_metadata = "Test metadata";
     
     polycall_message_set_payload(test_ctx, message, test_payload, strlen(test_payload) + 1);
     polycall_message_set_metadata(test_ctx, message, test_metadata, strlen(test_metadata) + 1);
     polycall_message_set_flags(test_ctx, message, POLYCALL_MESSAGE_FLAG_REQUIRES_ACK);
     
     // Serialize message
     void* buffer = NULL;
     size_t buffer_size = 0;
     
     polycall_core_error_t result = polycall_message_serialize(
         test_ctx, message, &buffer, &buffer_size);
     
     // Verify serialization result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(buffer);
     ASSERT_TRUE(buffer_size > 0);
     
     // Deserialize message
     polycall_message_t* deserialized = NULL;
     result = polycall_message_deserialize(test_ctx, buffer, buffer_size, &deserialized);
     
     // Verify deserialization result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(deserialized);
     
     // Verify deserialized message properties
     ASSERT_EQUAL_INT(message->header.type, deserialized->header.type);
     ASSERT_EQUAL_INT(message->header.flags, deserialized->header.flags);
     ASSERT_EQUAL_INT(message->header.payload_size, deserialized->header.payload_size);
     ASSERT_EQUAL_INT(message->header.metadata_size, deserialized->header.metadata_size);
     
     // Verify payload and metadata
     ASSERT_MEMORY_EQUAL(test_payload, deserialized->payload, strlen(test_payload) + 1);
     ASSERT_MEMORY_EQUAL(test_metadata, deserialized->metadata, strlen(test_metadata) + 1);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     polycall_message_destroy(test_ctx, deserialized);
     
     // Buffer was allocated by the message_serialize function
     polycall_core_free(test_ctx, buffer);
     
     return 0;
 }
 
 // Test string payload helpers
 static int test_string_payload(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_COMMAND);
     
     // Set string payload
     const char* test_string = "Test string payload";
     
     polycall_core_error_t result = polycall_message_set_string_payload(
         test_ctx, message, test_string);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(message->payload);
     ASSERT_EQUAL_INT(strlen(test_string) + 1, message->payload_size);
     
     // Get string payload
     const char* retrieved_string = polycall_message_get_string_payload(message);
     ASSERT_NOT_NULL(retrieved_string);
     ASSERT_EQUAL_STR(test_string, retrieved_string);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Test message accessors
 static int test_message_accessors(void) {
     polycall_message_t* message = NULL;
     
     // Create message
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_RESPONSE);
     
     // Set payload and metadata
     const char* test_payload = "Test payload";
     const char* test_metadata = "Test metadata";
     
     polycall_message_set_payload(test_ctx, message, test_payload, strlen(test_payload) + 1);
     polycall_message_set_metadata(test_ctx, message, test_metadata, strlen(test_metadata) + 1);
     
     // Get payload
     size_t payload_size = 0;
     const void* payload = polycall_message_get_payload(message, &payload_size);
     
     ASSERT_NOT_NULL(payload);
     ASSERT_EQUAL_INT(strlen(test_payload) + 1, payload_size);
     ASSERT_MEMORY_EQUAL(test_payload, payload, payload_size);
     
     // Get metadata
     size_t metadata_size = 0;
     const void* metadata = polycall_message_get_metadata(message, &metadata_size);
     
     ASSERT_NOT_NULL(metadata);
     ASSERT_EQUAL_INT(strlen(test_metadata) + 1, metadata_size);
     ASSERT_MEMORY_EQUAL(test_metadata, metadata, metadata_size);
     
     // Get message type
     polycall_message_type_t message_type = polycall_message_get_type(message);
     ASSERT_EQUAL_INT(POLYCALL_MESSAGE_TYPE_RESPONSE, message_type);
     
     // Get sequence number
     uint32_t sequence = polycall_message_get_sequence(message);
     ASSERT_EQUAL_INT(message->header.sequence, sequence);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Test message handlers
 static int test_message_handlers(void) {
     // This is a partial test since the implementation is marked as placeholder
     
     int handler_counter = 0;
     
     // Register a handler
     polycall_core_error_t result = polycall_message_register_handler(
         test_ctx,
         POLYCALL_MESSAGE_TYPE_HANDSHAKE,
         test_message_handler,
         &handler_counter);
     
     // The implementation returns UNSUPPORTED_OPERATION
     ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION, result);
     
     // Create a message
     polycall_message_t* message = NULL;
     polycall_message_create(test_ctx, &message, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
     
     // Dispatch message (should also return UNSUPPORTED_OPERATION)
     result = polycall_message_dispatch(test_ctx, message);
     ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION, result);
     
     // Clean up
     polycall_message_destroy(test_ctx, message);
     
     return 0;
 }
 
 // Main function to run all tests
 int main(void) {
     RESET_TESTS();
     
     // Run tests
     setup();
     RUN_TEST(test_message_create);
     teardown();
     
     setup();
     RUN_TEST(test_set_payload);
     teardown();
     
     setup();
     RUN_TEST(test_set_metadata);
     teardown();
     
     setup();
     RUN_TEST(test_set_flags);
     teardown();
     
     setup();
     RUN_TEST(test_serialization);
     teardown();
     
     setup();
     RUN_TEST(test_string_payload);
     teardown();
     
     setup();
     RUN_TEST(test_message_accessors);
     teardown();
     
     setup();
     RUN_TEST(test_message_handlers);
     teardown();
     
     return TEST_REPORT();
 }