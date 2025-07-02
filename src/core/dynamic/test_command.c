/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file test_command.c
 * @brief Unit tests for protocol command handling functionality
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 #ifdef USE_CHECK_FRAMEWORK
 #include <check.h>
 #else
 #include "unit_tests_framwork.h"
 #endif
 
 #include "polycall/core/protocol/command.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "mock_protocol_context.h"
 
 // Test fixture
 static polycall_core_context_t* test_ctx = NULL;
 static polycall_protocol_context_t* test_proto_ctx = NULL;
 static polycall_command_registry_t* test_registry = NULL;
 
 // Test command handler execution counter
 static int test_handler_counter = 0;
 
 // Command handler and validator for testing
 static polycall_command_response_t* test_command_handler(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const polycall_command_message_t* message,
     void* user_data
 ) {
     // Increment counter
     test_handler_counter++;
     
     // Create success response
     polycall_command_response_t* response = NULL;
     const char* response_data = "Command executed successfully";
     polycall_command_create_response(
         ctx, POLYCALL_COMMAND_STATUS_SUCCESS, 
         response_data, strlen(response_data) + 1, 
         &response
     );
     
     return response;
 }
 
 static polycall_command_validation_t test_command_validator(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const polycall_command_message_t* message,
     void* user_data
 ) {
     polycall_command_validation_t result = {
         .status = POLYCALL_COMMAND_STATUS_SUCCESS,
         .error_code = 0,
         .error_message = ""
     };
     
     // Check if command has required parameter (ID 1)
     bool has_param = false;
     
     for (uint32_t i = 0; i < message->header.param_count; i++) {
         if (message->parameters[i].param_id == 1) {
             has_param = true;
             break;
         }
     }
     
     if (!has_param) {
         result.status = POLYCALL_COMMAND_STATUS_ERROR;
         result.error_code = POLYCALL_COMMAND_ERROR_INVALID_PARAMETERS;
         strncpy(result.error_message, "Missing required parameter (ID 1)", 
                 sizeof(result.error_message) - 1);
     }
     
     return result;
 }
 
 // Setup function - runs before each test
 static void setup(void) {
     // Initialize test context
     test_ctx = polycall_core_create();
     test_proto_ctx = mock_protocol_context_create();
     
     // Reset handler counter
     test_handler_counter = 0;
     
     // Create command registry
     polycall_command_config_t config = {
         .flags = 0,
         .initial_command_capacity = 10,
         .memory_pool_size = 4096,
         .user_data = NULL
     };
     
     polycall_command_init(test_ctx, test_proto_ctx, &test_registry, &config);
 }
 
 // Teardown function - runs after each test
 static void teardown(void) {
     // Free resources
     if (test_registry) {
         polycall_command_cleanup(test_ctx, test_registry);
         test_registry = NULL;
     }
     
     if (test_proto_ctx) {
         mock_protocol_context_destroy(test_proto_ctx);
         test_proto_ctx = NULL;
     }
     
     if (test_ctx) {
         polycall_core_destroy(test_ctx);
         test_ctx = NULL;
     }
 }
 
 // Test command registry creation
 static int test_registry_creation(void) {
     ASSERT_NOT_NULL(test_registry);
     return 0;
 }
 
 // Test command registration
 static int test_command_registration(void) {
     // Register a command
     polycall_command_info_t command_info = {
         .command_id = 0,  // Auto-generate ID
         .name = "test_command",
         .handler = test_command_handler,
         .validator = test_command_validator,
         .permissions = 0,
         .flags = POLYCALL_COMMAND_FLAG_NONE,
         .user_data = NULL
     };
     
     uint32_t command_id = 0;
     polycall_core_error_t result = polycall_command_register(
         test_ctx, test_registry, &command_info, &command_id);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_TRUE(command_id > 0);
     
     // Try to find the registered command by ID
     polycall_command_info_t retrieved_info;
     result = polycall_command_find_by_id(test_ctx, test_registry, command_id, &retrieved_info);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_STR("test_command", retrieved_info.name);
     ASSERT_EQUAL_INT(command_id, retrieved_info.command_id);
     
     // Try to find the registered command by name
     result = polycall_command_find_by_name(test_ctx, test_registry, "test_command", &retrieved_info);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(command_id, retrieved_info.command_id);
     
     // Try to register a duplicate command (same name)
     result = polycall_command_register(test_ctx, test_registry, &command_info, NULL);
     ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);
     
     return 0;
 }
 
 // Test command message creation
 static int test_command_message_creation(void) {
     // Register a command first
     polycall_command_info_t command_info = {
         .command_id = 1001,
         .name = "test_command",
         .handler = test_command_handler,
         .validator = NULL,
         .permissions = 0,
         .flags = POLYCALL_COMMAND_FLAG_NONE,
         .user_data = NULL
     };
     
     polycall_command_register(test_ctx, test_registry, &command_info, NULL);
     
     // Create a command message
     polycall_command_message_t* message = NULL;
     polycall_core_error_t result = polycall_command_create_message(
         test_ctx, &message, 1001);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(message);
     ASSERT_EQUAL_INT(1001, message->header.command_id);
     ASSERT_EQUAL_INT(0, message->header.param_count);
     
     // Clean up
     polycall_command_destroy_message(test_ctx, message);
     
     return 0;
 }
 
 // Test adding and getting parameters
 static int test_command_parameters(void) {
     // Create a command message
     polycall_command_message_t* message = NULL;
     polycall_command_create_message(test_ctx, &message, 1000);
     
     // Add string parameter
     const char* string_param = "Test parameter";
     polycall_core_error_t result = polycall_command_add_parameter(
         test_ctx, message, 1, POLYCALL_PARAM_TYPE_STRING,
         string_param, strlen(string_param) + 1, 0);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(1, message->header.param_count);
     
     // Add integer parameter
     int32_t int_param = 42;
     result = polycall_command_add_parameter(
         test_ctx, message, 2, POLYCALL_PARAM_TYPE_INT32,
         &int_param, sizeof(int_param), 0);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(2, message->header.param_count);
     
     // Add boolean parameter
     bool bool_param = true;
     result = polycall_command_add_parameter(
         test_ctx, message, 3, POLYCALL_PARAM_TYPE_BOOL,
         &bool_param, sizeof(bool_param), 0);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(3, message->header.param_count);
     
     // Get string parameter
     void* param_data;
     uint32_t param_size;
     result = polycall_command_get_parameter(
         test_ctx, message, 1, POLYCALL_PARAM_TYPE_STRING,
         &param_data, &param_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(strlen(string_param) + 1, param_size);
     ASSERT_EQUAL_STR(string_param, (char*)param_data);
     
     // Get integer parameter
     int32_t retrieved_int;
     param_size = sizeof(retrieved_int);
     result = polycall_command_get_parameter(
         test_ctx, message, 2, POLYCALL_PARAM_TYPE_INT32,
         &retrieved_int, &param_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(sizeof(int32_t), param_size);
     ASSERT_EQUAL_INT(int_param, retrieved_int);
     
     // Get boolean parameter
     bool retrieved_bool;
     param_size = sizeof(retrieved_bool);
     result = polycall_command_get_parameter(
         test_ctx, message, 3, POLYCALL_PARAM_TYPE_BOOL,
         &retrieved_bool, &param_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(sizeof(bool), param_size);
     ASSERT_EQUAL_INT(bool_param, retrieved_bool);
     
     // Try to get non-existent parameter
     result = polycall_command_get_parameter(
         test_ctx, message, 999, POLYCALL_PARAM_TYPE_ANY,
         &param_data, &param_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);
     
     // Clean up
     polycall_command_destroy_message(test_ctx, message);
     
     return 0;
 }
 
 // Test command serialization and deserialization
 static int test_command_serialization(void) {
     // Create a command message
     polycall_command_message_t* message = NULL;
     polycall_command_create_message(test_ctx, &message, 1234);
     
     // Add parameters
     const char* string_param = "Test string parameter";
     int32_t int_param = 42;
     bool bool_param = true;
     
     polycall_command_add_parameter(
         test_ctx, message, 1, POLYCALL_PARAM_TYPE_STRING,
         string_param, strlen(string_param) + 1, 0);
     
     polycall_command_add_parameter(
         test_ctx, message, 2, POLYCALL_PARAM_TYPE_INT32,
         &int_param, sizeof(int_param), 0);
     
     polycall_command_add_parameter(
         test_ctx, message, 3, POLYCALL_PARAM_TYPE_BOOL,
         &bool_param, sizeof(bool_param), 0);
     
     // Serialize message
     void* buffer = NULL;
     size_t buffer_size = 0;
     
     polycall_core_error_t result = polycall_command_serialize(
         test_ctx, message, &buffer, &buffer_size);
     
     // Verify serialization result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(buffer);
     ASSERT_TRUE(buffer_size > 0);
     
     // Deserialize message
     polycall_command_message_t* deserialized = NULL;
     result = polycall_command_deserialize(
         test_ctx, buffer, buffer_size, &deserialized);
     
     // Verify deserialization result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(deserialized);
     
     // Verify message properties
     ASSERT_EQUAL_INT(message->header.command_id, deserialized->header.command_id);
     ASSERT_EQUAL_INT(message->header.param_count, deserialized->header.param_count);
     
     // Verify parameters
     char* string_value;
     uint32_t string_size = 256;
     result = polycall_command_get_parameter(
         test_ctx, deserialized, 1, POLYCALL_PARAM_TYPE_STRING,
         &string_value, &string_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_STR(string_param, string_value);
     
     int32_t int_value;
     uint32_t int_size = sizeof(int_value);
     result = polycall_command_get_parameter(
         test_ctx, deserialized, 2, POLYCALL_PARAM_TYPE_INT32,
         &int_value, &int_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(int_param, int_value);
     
     bool bool_value;
     uint32_t bool_size = sizeof(bool_value);
     result = polycall_command_get_parameter(
         test_ctx, deserialized, 3, POLYCALL_PARAM_TYPE_BOOL,
         &bool_value, &bool_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_EQUAL_INT(bool_param, bool_value);
     
     // Clean up
     polycall_command_destroy_message(test_ctx, message);
     polycall_command_destroy_message(test_ctx, deserialized);
     polycall_core_free(test_ctx, buffer);
     
     return 0;
 }
 
 // Test command execution
 static int test_command_execution(void) {
     // Register a command
     polycall_command_info_t command_info = {
         .command_id = 2000,
         .name = "test_execution",
         .handler = test_command_handler,
         .validator = NULL,
         .permissions = 0,
         .flags = POLYCALL_COMMAND_FLAG_NONE,
         .user_data = NULL
     };
     
     polycall_command_register(test_ctx, test_registry, &command_info, NULL);
     
     // Create a command message
     polycall_command_message_t* message = NULL;
     polycall_command_create_message(test_ctx, &message, 2000);
     
     // Add a parameter
     const char* param = "Execute test";
     polycall_command_add_parameter(
         test_ctx, message, 1, POLYCALL_PARAM_TYPE_STRING,
         param, strlen(param) + 1, 0);
     
     // Reset handler counter
     test_handler_counter = 0;
     
     // Execute command
     polycall_command_response_t* response = NULL;
     polycall_core_error_t result = polycall_command_execute(
         test_ctx, test_registry, test_proto_ctx, message, &response);
     
     // Verify execution result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(response);
     ASSERT_EQUAL_INT(1, test_handler_counter);
     ASSERT_EQUAL_INT(POLYCALL_COMMAND_STATUS_SUCCESS, response->status);
     
     // Clean up
     polycall_command_destroy_message(test_ctx, message);
     polycall_command_destroy_response(test_ctx, response);
     
     return 0;
 }
 
 // Test command validation
 static int test_command_validation(void) {
     // Register a command with validator
     polycall_command_info_t command_info = {
         .command_id = 3000,
         .name = "test_validation",
         .handler = test_command_handler,
         .validator = test_command_validator,
         .permissions = 0,
         .flags = POLYCALL_COMMAND_FLAG_NONE,
         .user_data = NULL
     };
     
     polycall_command_register(test_ctx, test_registry, &command_info, NULL);
     
     // Create a command message without required parameter
     polycall_command_message_t* invalid_message = NULL;
     polycall_command_create_message(test_ctx, &invalid_message, 3000);
     
     // Execute command (should fail validation)
     polycall_command_response_t* invalid_response = NULL;
     polycall_core_error_t result = polycall_command_execute(
         test_ctx, test_registry, test_proto_ctx, invalid_message, &invalid_response);
     
     // Verify validation failure
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(invalid_response);
     ASSERT_EQUAL_INT(0, test_handler_counter); // Handler shouldn't be called
     ASSERT_EQUAL_INT(POLYCALL_COMMAND_STATUS_ERROR, invalid_response->status);
     
     // Create a valid command message with required parameter
     polycall_command_message_t* valid_message = NULL;
     polycall_command_create_message(test_ctx, &valid_message, 3000);
     
     // Add required parameter
     const char* param = "Required parameter";
     polycall_command_add_parameter(
         test_ctx, valid_message, 1, POLYCALL_PARAM_TYPE_STRING,
         param, strlen(param) + 1, 0);
     
     // Reset handler counter
     test_handler_counter = 0;
     
     // Execute command (should pass validation)
     polycall_command_response_t* valid_response = NULL;
     result = polycall_command_execute(
         test_ctx, test_registry, test_proto_ctx, valid_message, &valid_response);
     
     // Verify validation success
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(valid_response);
     ASSERT_EQUAL_INT(1, test_handler_counter); // Handler should be called
     ASSERT_EQUAL_INT(POLYCALL_COMMAND_STATUS_SUCCESS, valid_response->status);
     
     // Clean up
     polycall_command_destroy_message(test_ctx, invalid_message);
     polycall_command_destroy_response(test_ctx, invalid_response);
     polycall_command_destroy_message(test_ctx, valid_message);
     polycall_command_destroy_response(test_ctx, valid_response);
     
     return 0;
 }
 
 // Test command unregistration
 static int test_command_unregistration(void) {
     // Register a command
     polycall_command_info_t command_info = {
         .command_id = 4000,
         .name = "temporary_command",
         .handler = test_command_handler,
         .validator = NULL,
         .permissions = 0,
         .flags = POLYCALL_COMMAND_FLAG_NONE,
         .user_data = NULL
     };
     
     polycall_command_register(test_ctx, test_registry, &command_info, NULL);
     
     // Verify command exists
     polycall_command_info_t retrieved_info;
     polycall_core_error_t result = polycall_command_find_by_id(
         test_ctx, test_registry, 4000, &retrieved_info);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Unregister the command
     result = polycall_command_unregister(test_ctx, test_registry, 4000);
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify command no longer exists
     result = polycall_command_find_by_id(test_ctx, test_registry, 4000, &retrieved_info);
     ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, result);
     
     return 0;
 }
 
 // Test command response creation
 static int test_command_response_creation(void) {
     // Create a success response
     const char* success_data = "Success response data";
     polycall_command_response_t* success_response = NULL;
     
     polycall_core_error_t result = polycall_command_create_response(
         test_ctx, POLYCALL_COMMAND_STATUS_SUCCESS, 
         success_data, strlen(success_data) + 1,
         &success_response);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(success_response);
     ASSERT_EQUAL_INT(POLYCALL_COMMAND_STATUS_SUCCESS, success_response->status);
     ASSERT_NOT_NULL(success_response->response_data);
     ASSERT_EQUAL_STR(success_data, (char*)success_response->response_data);
     
     // Create an error response
     polycall_command_response_t* error_response = NULL;
     result = polycall_command_create_error_response(
         test_ctx, POLYCALL_COMMAND_ERROR_INVALID_PARAMETERS,
         "Invalid parameters",
         &error_response);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(error_response);
     ASSERT_EQUAL_INT(POLYCALL_COMMAND_STATUS_ERROR, error_response->status);
     ASSERT_EQUAL_INT(POLYCALL_COMMAND_ERROR_INVALID_PARAMETERS, error_response->error_code);
     ASSERT_EQUAL_STR("Invalid parameters", error_response->error_message);
     
     // Clean up
     polycall_command_destroy_response(test_ctx, success_response);
     polycall_command_destroy_response(test_ctx, error_response);
     
     return 0;
 }
 
 // Test response serialization and deserialization
 static int test_response_serialization(void) {
     // Create a response
     const char* response_data = "Response data for serialization test";
     polycall_command_response_t* response = NULL;
     
     polycall_command_create_response(
         test_ctx, POLYCALL_COMMAND_STATUS_SUCCESS,
         response_data, strlen(response_data) + 1,
         &response);
     
     // Serialize response
     void* buffer = NULL;
     size_t buffer_size = 0;
     
     polycall_core_error_t result = polycall_command_serialize_response(
         test_ctx, response, &buffer, &buffer_size);
     
     // Verify serialization result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(buffer);
     ASSERT_TRUE(buffer_size > 0);
     
     // Deserialize response
     polycall_command_response_t* deserialized = NULL;
     result = polycall_command_deserialize_response(
         test_ctx, buffer, buffer_size, &deserialized);
     
     // Verify deserialization result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(deserialized);
     
     // Verify response properties
     ASSERT_EQUAL_INT(response->status, deserialized->status);
     ASSERT_EQUAL_INT(response->data_size, deserialized->data_size);
     
     // Verify response data
     ASSERT_MEMORY_EQUAL(response_data, deserialized->response_data, strlen(response_data) + 1);
     
     // Clean up
     polycall_command_destroy_response(test_ctx, response);
     polycall_command_destroy_response(test_ctx, deserialized);
     polycall_core_free(test_ctx, buffer);
     
     return 0;
 }
 
 // Main function to run all tests
 int main(void) {
     RESET_TESTS();
     
     // Run tests
     setup();
     RUN_TEST(test_registry_creation);
     teardown();
     
     setup();
     RUN_TEST(test_command_registration);
     teardown();
     
     setup();
     RUN_TEST(test_command_message_creation);
     teardown();
     
     setup();
     RUN_TEST(test_command_parameters);
     teardown();
     
     setup();
     RUN_TEST(test_command_serialization);
     teardown();
     
     setup();
     RUN_TEST(test_command_execution);
     teardown();
     
     setup();
     RUN_TEST(test_command_validation);
     teardown();
     
     setup();
     RUN_TEST(test_command_unregistration);
     teardown();
     
     setup();
     RUN_TEST(test_command_response_creation);
     teardown();
     
     setup();
     RUN_TEST(test_response_serialization);
     teardown();
     
     return TEST_REPORT();
 }
