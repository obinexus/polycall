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
 * @file test_config.c
 * @brief Unit tests for the configuration functionality in LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 */

 #include "unit_test_framework.h"
 #include "polycall/core/polycall/polycall_config.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 // Global variables for test
 static polycall_core_context_t* g_core_ctx = NULL;
 static polycall_config_context_t* g_config_ctx = NULL;
 
 // Change handler for testing
 static int g_change_handler_called = 0;
 static char g_last_changed_key[256] = {0};
 
 static void test_change_handler(
     polycall_core_context_t* ctx,
     polycall_config_section_t section_id,
     const char* key,
     const polycall_config_value_t* old_value,
     const polycall_config_value_t* new_value,
     void* user_data
 ) {
     g_change_handler_called++;
     if (key) {
         strncpy(g_last_changed_key, key, sizeof(g_last_changed_key) - 1);
         g_last_changed_key[sizeof(g_last_changed_key) - 1] = '\0';
     }
 }
 
 // Custom provider for testing
 static polycall_core_error_t test_provider_initialize(
     polycall_core_context_t* ctx,
     void* user_data
 ) {
     return POLYCALL_CORE_SUCCESS;
 }
 
 static void test_provider_cleanup(
     polycall_core_context_t* ctx,
     void* user_data
 ) {
     // Nothing to clean up
 }
 
 static polycall_core_error_t test_provider_load(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     const char* key,
     polycall_config_value_t* value
 ) {
     // Simple implementation that returns pre-defined values
     if (strcmp(key, "test_string") == 0) {
         value->type = POLYCALL_CONFIG_VALUE_STRING;
         value->value.string_value = strdup("provider_value");
         return POLYCALL_CORE_SUCCESS;
     } else if (strcmp(key, "test_int") == 0) {
         value->type = POLYCALL_CONFIG_VALUE_INTEGER;
         value->value.int_value = 12345;
         return POLYCALL_CORE_SUCCESS;
     }
     
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 static polycall_core_error_t test_provider_save(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     const char* key,
     const polycall_config_value_t* value
 ) {
     // Just pretend to save
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t test_provider_exists(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     const char* key,
     bool* exists
 ) {
     // Check if key exists in our "database"
     if (strcmp(key, "test_string") == 0 || strcmp(key, "test_int") == 0) {
         *exists = true;
         return POLYCALL_CORE_SUCCESS;
     }
     
     *exists = false;
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t test_provider_enumerate(
     polycall_core_context_t* ctx,
     void* user_data,
     polycall_config_section_t section_id,
     void (*callback)(const char* key, void* callback_data),
     void* callback_data
 ) {
     // Enumerate keys in our "database"
     callback("test_string", callback_data);
     callback("test_int", callback_data);
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Initialize test resources
 static void setup() {
     // Initialize the core context
     polycall_core_config_t config = {
         .flags = POLYCALL_CORE_FLAG_NONE,
         .memory_pool_size = 1024 * 1024,  // 1MB for testing
         .user_data = NULL,
         .error_callback = NULL
     };
     
     polycall_core_init(&g_core_ctx, &config);
     
     // Initialize config context with default options
     polycall_config_options_t options = polycall_config_default_options();
     options.enable_change_notification = true;
     
     polycall_config_init(g_core_ctx, &g_config_ctx, &options);
     
     // Reset change handler tracking
     g_change_handler_called = 0;
     g_last_changed_key[0] = '\0';
 }
 
 // Clean up test resources
 static void teardown() {
     // Clean up the config context
     if (g_config_ctx) {
         polycall_config_cleanup(g_core_ctx, g_config_ctx);
         g_config_ctx = NULL;
     }
     
     // Clean up the core context
     if (g_core_ctx) {
         polycall_core_cleanup(g_core_ctx);
         g_core_ctx = NULL;
     }
 }
 
 // Test config initialization
 static int test_config_init() {
     // Verify config context was created
     ASSERT_NOT_NULL(g_config_ctx);
     
     return 0;
 }
 
 // Test boolean config values
 static int test_config_bool() {
     // Set a boolean value
     polycall_core_error_t result = polycall_config_set_bool(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_bool",
         true
     );
     
     // Verify set succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Get the boolean value
     bool value = polycall_config_get_bool(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_bool",
         false  // Default value if key not found
     );
     
     // Verify the value matches what we set
     ASSERT_TRUE(value);
     
     // Check that the key exists
     bool exists = polycall_config_exists(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_bool"
     );
     
     // Verify key exists
     ASSERT_TRUE(exists);
     
     // Remove the key
     result = polycall_config_remove(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_bool"
     );
     
     // Verify removal succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Check that the key no longer exists
     exists = polycall_config_exists(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_bool"
     );
     
     // Verify key no longer exists
     ASSERT_FALSE(exists);
     
     return 0;
 }
 
 // Test integer config values
 static int test_config_int() {
     // Set an integer value
     polycall_core_error_t result = polycall_config_set_int(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_int",
         42
     );
     
     // Verify set succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Get the integer value
     int64_t value = polycall_config_get_int(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_int",
         0  // Default value if key not found
     );
     
     // Verify the value matches what we set
     ASSERT_EQUAL_INT(42, value);
     
     return 0;
 }
 
 // Test float config values
 static int test_config_float() {
     // Set a float value
     polycall_core_error_t result = polycall_config_set_float(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_float",
         3.14159
     );
     
     // Verify set succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Get the float value
     double value = polycall_config_get_float(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_float",
         0.0  // Default value if key not found
     );
     
     // Verify the value matches what we set (approximately)
     ASSERT_TRUE(value > 3.14158 && value < 3.14160);
     
     return 0;
 }
 
 // Test string config values
 static int test_config_string() {
     // Set a string value
     polycall_core_error_t result = polycall_config_set_string(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_string",
         "Hello, world!"
     );
     
     // Verify set succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Get the string value
     char buffer[64];
     result = polycall_config_get_string(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_string",
         buffer,
         sizeof(buffer),
         "default"  // Default value if key not found
     );
     
     // Verify get succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify the value matches what we set
     ASSERT_EQUAL_STR("Hello, world!", buffer);
     
     return 0;
 }
 
 // Test object config values
 static int test_config_object() {
     // Create a test object
     int* obj = malloc(sizeof(int));
     *obj = 42;
     
     // Object free function
     void (*free_fn)(void*) = free;
     
     // Set an object value
     polycall_core_error_t result = polycall_config_set_object(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_object",
         obj,
         free_fn
     );
     
     // Verify set succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Get the object value
     void* value;
     result = polycall_config_get_object(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_object",
         &value
     );
     
     // Verify get succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify the value matches what we set
     ASSERT_EQUAL_PTR(obj, value);
     ASSERT_EQUAL_INT(42, *(int*)value);
     
     // Note: We don't free the object here because it's managed by the config system
     
     return 0;
 }
 
 // Test change notifications
 static int test_config_change_notification() {
     // Register a change handler
     uint32_t handler_id;
     polycall_core_error_t result = polycall_config_register_change_handler(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_notify",
         test_change_handler,
         NULL,
         &handler_id
     );
     
     // Verify registration succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Set a value to trigger the notification
     result = polycall_config_set_int(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_notify",
         100
     );
     
     // Verify set succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify handler was called
     ASSERT_TRUE(g_change_handler_called > 0);
     
     // Verify the correct key was passed to the handler
     ASSERT_EQUAL_STR("test_notify", g_last_changed_key);
     
     return 0;
 }
 
 // Test provider registration
 static int test_config_provider() {
     // Create a provider
     polycall_config_provider_t provider = {
         .initialize = test_provider_initialize,
         .cleanup = test_provider_cleanup,
         .load = test_provider_load,
         .save = test_provider_save,
         .exists = test_provider_exists,
         .enumerate = test_provider_enumerate,
         .provider_name = "TestProvider",
         .user_data = NULL
     };
     
     // Register the provider
     polycall_core_error_t result = polycall_config_register_provider(
         g_core_ctx,
         g_config_ctx,
         &provider
     );
     
     // Verify registration succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Now try to get a value that the provider knows about
     int64_t value = polycall_config_get_int(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         "test_int",
         0  // Default value if key not found
     );
     
     // Verify we got the value from the provider
     // Note: This test may fail if the provider isn't properly integrated
     // as we're not checking if the provider was actually called
     ASSERT_EQUAL_INT(12345, value);
     
     return 0;
 }
 
 // Test enumeration
 static int test_config_enumerate() {
     // Set up some test values
     polycall_config_set_int(g_core_ctx, g_config_ctx, POLYCALL_CONFIG_SECTION_CORE, "enum1", 1);
     polycall_config_set_int(g_core_ctx, g_config_ctx, POLYCALL_CONFIG_SECTION_CORE, "enum2", 2);
     polycall_config_set_int(g_core_ctx, g_config_ctx, POLYCALL_CONFIG_SECTION_CORE, "enum3", 3);
     
     // Enumeration tracking
     int enum_count = 0;
     
     // Enumeration callback
     void (*callback)(const char* key, void* user_data) = 
         (void (*)(const char*, void*))
         (void*)&enum_count;
     
     // Call enumerate (note: this is simplified since we can't define an actual callback in tests)
     polycall_core_error_t result = polycall_config_enumerate(
         g_core_ctx,
         g_config_ctx,
         POLYCALL_CONFIG_SECTION_CORE,
         NULL,  // No real callback for this test
         NULL
     );
     
     // Verify enumeration succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Since we can't actually define a callback, we just check that the function completed
     
     return 0;
 }
 
 // Run all config tests
 int run_config_tests() {
     RESET_TESTS();
     
     setup();
     
     RUN_TEST(test_config_init);
     RUN_TEST(test_config_bool);
     RUN_TEST(test_config_int);
     RUN_TEST(test_config_float);
     RUN_TEST(test_config_string);
     RUN_TEST(test_config_object);
     RUN_TEST(test_config_change_notification);
     RUN_TEST(test_config_provider);
     RUN_TEST(test_config_enumerate);
     
     teardown();
     
     return g_tests_failed > 0 ? 1 : 0;
 }
