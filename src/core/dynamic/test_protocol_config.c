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
 * @file test_protocol_config.c
 * @brief Unit tests for protocol configuration functionality
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 #ifdef USE_CHECK_FRAMEWORK
 #include <check.h>
 #else
 #include "unit_tests_framwork.h"
 #endif
 
 #include "polycall/core/protocol/protocol_config.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "mock_protocol_context.h"
 
 // Test fixture
 static polycall_core_context_t* test_ctx = NULL;
 static polycall_protocol_context_t* test_proto_ctx = NULL;
 
 // Setup function - runs before each test
 static void setup(void) {
     // Initialize test context
     test_ctx = polycall_core_create();
     test_proto_ctx = mock_protocol_context_create();
 }
 
 // Teardown function - runs after each test
 static void teardown(void) {
     // Free resources
     if (test_proto_ctx) {
         mock_protocol_context_destroy(test_proto_ctx);
         test_proto_ctx = NULL;
     }
     
     if (test_ctx) {
         polycall_core_destroy(test_ctx);
         test_ctx = NULL;
     }
 }
 
 // Test default configuration creation
 static int test_default_config(void) {
     protocol_config_t config = polycall_protocol_default_config();
     
     // Verify default values
     ASSERT_EQUAL_INT(PROTOCOL_TRANSPORT_TCP, config.core.transport_type);
     ASSERT_EQUAL_INT(PROTOCOL_ENCODING_JSON, config.core.encoding_format);
     ASSERT_EQUAL_INT(PROTOCOL_VALIDATION_STANDARD, config.core.validation_level);
     ASSERT_EQUAL_INT(30000, config.core.default_timeout_ms);
     ASSERT_EQUAL_INT(5000, config.core.handshake_timeout_ms);
     ASSERT_EQUAL_INT(60000, config.core.keep_alive_interval_ms);
     ASSERT_EQUAL_INT(8080, config.core.default_port);
     ASSERT_TRUE(config.core.enable_tls);
     ASSERT_TRUE(config.core.enable_compression);
     ASSERT_TRUE(config.core.enable_auto_reconnect);
     ASSERT_EQUAL_INT(PROTOCOL_RETRY_EXPONENTIAL, config.core.retry_policy);
     ASSERT_EQUAL_INT(5, config.core.max_retry_count);
     
     // Verify TLS defaults
     ASSERT_NULL(config.tls.cert_file);
     ASSERT_NULL(config.tls.key_file);
     ASSERT_NULL(config.tls.ca_file);
     ASSERT_TRUE(config.tls.verify_peer);
     ASSERT_FALSE(config.tls.allow_self_signed);
     ASSERT_EQUAL_STR("HIGH:!aNULL:!MD5:!RC4", config.tls.cipher_list);
     
     return 0;
 }
 
 // Test applying configuration
 static int test_apply_config(void) {
     protocol_config_t config = polycall_protocol_default_config();
     
     // Modify some configuration values
     config.core.transport_type = PROTOCOL_TRANSPORT_WEBSOCKET;
     config.core.default_port = 9090;
     config.core.enable_compression = false;
     
     // Apply configuration
     polycall_core_error_t result = polycall_protocol_apply_config(
         test_ctx, test_proto_ctx, &config);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify the mock protocol context was updated
     ASSERT_EQUAL_INT(PROTOCOL_TRANSPORT_WEBSOCKET, test_proto_ctx->transport_type);
     ASSERT_EQUAL_INT(9090, test_proto_ctx->default_port);
     ASSERT_FALSE(test_proto_ctx->enable_compression);
     
     return 0;
 }
 
 // Test configuration validation
 static int test_validate_config(void) {
     protocol_config_t config = polycall_protocol_default_config();
     char error_message[256] = {0};
     
     // Valid configuration should pass
     bool valid = polycall_protocol_validate_config(
         test_ctx, &config, error_message, sizeof(error_message));
     ASSERT_TRUE(valid);
     
     // Invalid configuration (invalid transport)
     config.core.transport_type = PROTOCOL_TRANSPORT_NONE;
     valid = polycall_protocol_validate_config(
         test_ctx, &config, error_message, sizeof(error_message));
     ASSERT_FALSE(valid);
     
     // Reset to valid configuration
     config = polycall_protocol_default_config();
     
     // Invalid configuration (TLS enabled but no cert/key)
     config.core.enable_tls = true;
     config.tls.cert_file = NULL;
     config.tls.key_file = NULL;
     valid = polycall_protocol_validate_config(
         test_ctx, &config, error_message, sizeof(error_message));
     ASSERT_FALSE(valid);
     
     return 0;
 }
 
 // Test configuration merging
 static int test_merge_config(void) {
     protocol_config_t base_config = polycall_protocol_default_config();
     protocol_config_t override_config = polycall_protocol_default_config();
     
     // Modify override configuration
     override_config.core.default_port = 9090;
     override_config.core.max_retry_count = 10;
     override_config.core.enable_compression = false;
     
     // Merge configurations
     polycall_core_error_t result = polycall_protocol_merge_config(
         test_ctx, &base_config, &override_config);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify merged values
     ASSERT_EQUAL_INT(9090, base_config.core.default_port);
     ASSERT_EQUAL_INT(10, base_config.core.max_retry_count);
     ASSERT_FALSE(base_config.core.enable_compression);
     
     // Verify unchanged values
     ASSERT_EQUAL_INT(PROTOCOL_TRANSPORT_TCP, base_config.core.transport_type);
     ASSERT_EQUAL_INT(30000, base_config.core.default_timeout_ms);
     
     return 0;
 }
 
 // Test configuration copying
 static int test_copy_config(void) {
     protocol_config_t src_config = polycall_protocol_default_config();
     protocol_config_t dest_config;
     
     // Set some custom values
     src_config.core.default_port = 9090;
     src_config.core.max_retry_count = 10;
     src_config.tls.cert_file = "/path/to/cert.pem";
     
     // Copy configuration
     polycall_core_error_t result = polycall_protocol_copy_config(
         test_ctx, &dest_config, &src_config);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify copied values
     ASSERT_EQUAL_INT(src_config.core.default_port, dest_config.core.default_port);
     ASSERT_EQUAL_INT(src_config.core.max_retry_count, dest_config.core.max_retry_count);
     ASSERT_EQUAL_STR(src_config.tls.cert_file, dest_config.tls.cert_file);
     
     // Clean up allocated strings
     polycall_protocol_cleanup_config_strings(test_ctx, &dest_config);
     
     return 0;
 }
 
 // Test configuration initialization
 static int test_config_init(void) {
     protocol_config_t config = polycall_protocol_default_config();
     
     // Initialize protocol with configuration
     polycall_core_error_t result = polycall_protocol_config_init(
         test_ctx, test_proto_ctx, &config);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify the mock protocol context was initialized with the configuration
     ASSERT_EQUAL_INT(PROTOCOL_TRANSPORT_TCP, test_proto_ctx->transport_type);
     ASSERT_EQUAL_INT(8080, test_proto_ctx->default_port);
     ASSERT_TRUE(test_proto_ctx->enable_compression);
     
     return 0;
 }
 
 // Main function to run all tests
 int main(void) {
     RESET_TESTS();
     
     // Run tests
     setup();
     RUN_TEST(test_default_config);
     teardown();
     
     setup();
     RUN_TEST(test_apply_config);
     teardown();
     
     setup();
     RUN_TEST(test_validate_config);
     teardown();
     
     setup();
     RUN_TEST(test_merge_config);
     teardown();
     
     setup();
     RUN_TEST(test_copy_config);
     teardown();
     
     setup();
     RUN_TEST(test_config_init);
     teardown();
     
     return TEST_REPORT();
 }
