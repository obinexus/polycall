/**
 * @file test_network_config.c
 * @brief Unit tests for the network configuration module of LibPolyCall
 * @author Unit tests for LibPolyCall
 *
 * This file contains unit tests for the configuration management interface in LibPolyCall.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>
 
 #include "polycall/core/network/network_config.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_memory.h"
 
 // Configuration validation callback
 static bool test_validation_callback_called = false;
 static bool test_validation_callback(
     polycall_core_context_t* ctx,
     polycall_network_config_t* config,
     void* user_data
 ) {
     test_validation_callback_called = true;
     return true; // Configuration is valid
 }
 
 // Configuration enumeration callback
 static int test_enum_count = 0;
 static bool test_enum_callback(
     const char* section,
     const char* key,
     void* user_data
 ) {
     test_enum_count++;
     return true; // Continue enumeration
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
 
 void test_config_create_destroy(void) {
     printf("Testing config_create and config_destroy functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Test with NULL parameters
     polycall_network_config_t* config = NULL;
     polycall_core_error_t result = polycall_network_config_create(NULL, &config, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_create(core_ctx, NULL, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters but no config file
     result = polycall_network_config_create(core_ctx, &config, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(config != NULL);
     
     // Verify the config was initialized properly
     assert(config->core_ctx == core_ctx);
     assert(config->initialized == true);
     assert(config->modified == false);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     
     // Test with config file path
     result = polycall_network_config_create(core_ctx, &config, "test_config.ini");
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(config != NULL);
     assert(strcmp(config->config_file, "test_config.ini") == 0);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     
     // Test destroy with NULL parameters
     polycall_network_config_destroy(NULL, config);  // Should not crash
     polycall_network_config_destroy(core_ctx, NULL); // Should not crash
     
     free(core_ctx);
     
     printf("config_create and config_destroy tests passed!\n");
 }
 
 void test_config_set_validator(void) {
     printf("Testing config_set_validator function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test with NULL parameters
     polycall_core_error_t result = polycall_network_config_set_validator(NULL, config, test_validation_callback, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_validator(core_ctx, NULL, test_validation_callback, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test with valid parameters
     int user_data = 12345;
     result = polycall_network_config_set_validator(core_ctx, config, test_validation_callback, &user_data);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Verify validator was set
     assert(config->validate_callback == test_validation_callback);
     assert(config->validate_user_data == &user_data);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_set_validator tests passed!\n");
 }
 
 void test_config_set_get_int(void) {
     printf("Testing config_set_int and config_get_int functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test set_int with NULL parameters
     polycall_core_error_t result = polycall_network_config_set_int(NULL, config, "test", "int_value", 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_int(core_ctx, NULL, "test", "int_value", 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_int(core_ctx, config, NULL, "int_value", 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_int(core_ctx, config, "test", NULL, 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test set_int with valid parameters
     result = polycall_network_config_set_int(core_ctx, config, "test", "int_value", 12345);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Verify config was modified
     assert(config->modified == true);
     
     // Test get_int with NULL parameters
     int value = 0;
     result = polycall_network_config_get_int(NULL, config, "test", "int_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_int(core_ctx, NULL, "test", "int_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_int(core_ctx, config, NULL, "int_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_int(core_ctx, config, "test", NULL, &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_int(core_ctx, config, "test", "int_value", NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test get_int with valid parameters
     value = 0;
     result = polycall_network_config_get_int(core_ctx, config, "test", "int_value", &value);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(value == 12345);
     
     // Test get_int with non-existent key
     result = polycall_network_config_get_int(core_ctx, config, "test", "non_existent", &value);
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_set_int and config_get_int tests passed!\n");
 }
 
 void test_config_set_get_uint(void) {
     printf("Testing config_set_uint and config_get_uint functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test set_uint with NULL parameters
     polycall_core_error_t result = polycall_network_config_set_uint(NULL, config, "test", "uint_value", 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_uint(core_ctx, NULL, "test", "uint_value", 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_uint(core_ctx, config, NULL, "uint_value", 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_uint(core_ctx, config, "test", NULL, 12345);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test set_uint with valid parameters
     result = polycall_network_config_set_uint(core_ctx, config, "test", "uint_value", 12345);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Test get_uint with NULL parameters
     unsigned int value = 0;
     result = polycall_network_config_get_uint(NULL, config, "test", "uint_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_uint(core_ctx, NULL, "test", "uint_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_uint(core_ctx, config, NULL, "uint_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_uint(core_ctx, config, "test", NULL, &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_uint(core_ctx, config, "test", "uint_value", NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test get_uint with valid parameters
     value = 0;
     result = polycall_network_config_get_uint(core_ctx, config, "test", "uint_value", &value);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(value == 12345);
     
     // Test get_uint with non-existent key
     result = polycall_network_config_get_uint(core_ctx, config, "test", "non_existent", &value);
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_set_uint and config_get_uint tests passed!\n");
 }
 
 void test_config_set_get_bool(void) {
     printf("Testing config_set_bool and config_get_bool functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test set_bool with NULL parameters
     polycall_core_error_t result = polycall_network_config_set_bool(NULL, config, "test", "bool_value", true);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_bool(core_ctx, NULL, "test", "bool_value", true);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_bool(core_ctx, config, NULL, "bool_value", true);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_bool(core_ctx, config, "test", NULL, true);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test set_bool with valid parameters
     result = polycall_network_config_set_bool(core_ctx, config, "test", "bool_value", true);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Test get_bool with NULL parameters
     bool value = false;
     result = polycall_network_config_get_bool(NULL, config, "test", "bool_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_bool(core_ctx, NULL, "test", "bool_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_bool(core_ctx, config, NULL, "bool_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_bool(core_ctx, config, "test", NULL, &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_bool(core_ctx, config, "test", "bool_value", NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test get_bool with valid parameters
     value = false;
     result = polycall_network_config_get_bool(core_ctx, config, "test", "bool_value", &value);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(value == true);
     
     // Test get_bool with non-existent key
     result = polycall_network_config_get_bool(core_ctx, config, "test", "non_existent", &value);
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_set_bool and config_get_bool tests passed!\n");
 }
 
 void test_config_set_get_string(void) {
     printf("Testing config_set_string and config_get_string functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test set_string with NULL parameters
     polycall_core_error_t result = polycall_network_config_set_string(NULL, config, "test", "string_value", "test string");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_string(core_ctx, NULL, "test", "string_value", "test string");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_string(core_ctx, config, NULL, "string_value", "test string");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_string(core_ctx, config, "test", NULL, "test string");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_string(core_ctx, config, "test", "string_value", NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test set_string with valid parameters
     result = polycall_network_config_set_string(core_ctx, config, "test", "string_value", "test string");
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Test get_string with NULL parameters
     char value[64];
     result = polycall_network_config_get_string(NULL, config, "test", "string_value", value, sizeof(value));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_string(core_ctx, NULL, "test", "string_value", value, sizeof(value));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_string(core_ctx, config, NULL, "string_value", value, sizeof(value));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_string(core_ctx, config, "test", NULL, value, sizeof(value));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_string(core_ctx, config, "test", "string_value", NULL, sizeof(value));
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_string(core_ctx, config, "test", "string_value", value, 0);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test get_string with valid parameters
     memset(value, 0, sizeof(value));
     result = polycall_network_config_get_string(core_ctx, config, "test", "string_value", value, sizeof(value));
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(strcmp(value, "test string") == 0);
     
     // Test get_string with buffer too small
     char small_buffer[4];
     result = polycall_network_config_get_string(core_ctx, config, "test", "string_value", small_buffer, sizeof(small_buffer));
     assert(result == POLYCALL_CORE_ERROR_BUFFER_UNDERFLOW);
     
     // Test get_string with non-existent key
     result = polycall_network_config_get_string(core_ctx, config, "test", "non_existent", value, sizeof(value));
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_set_string and config_get_string tests passed!\n");
 }
 
 void test_config_set_get_float(void) {
     printf("Testing config_set_float and config_get_float functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test set_float with NULL parameters
     polycall_core_error_t result = polycall_network_config_set_float(NULL, config, "test", "float_value", 123.45f);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_float(core_ctx, NULL, "test", "float_value", 123.45f);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_float(core_ctx, config, NULL, "float_value", 123.45f);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_set_float(core_ctx, config, "test", NULL, 123.45f);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test set_float with valid parameters
     result = polycall_network_config_set_float(core_ctx, config, "test", "float_value", 123.45f);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Test get_float with NULL parameters
     float value = 0.0f;
     result = polycall_network_config_get_float(NULL, config, "test", "float_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_float(core_ctx, NULL, "test", "float_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_float(core_ctx, config, NULL, "float_value", &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_float(core_ctx, config, "test", NULL, &value);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_get_float(core_ctx, config, "test", "float_value", NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test get_float with valid parameters
     value = 0.0f;
     result = polycall_network_config_get_float(core_ctx, config, "test", "float_value", &value);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(value == 123.45f);
     
     // Test get_float with non-existent key
     result = polycall_network_config_get_float(core_ctx, config, "test", "non_existent", &value);
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_set_float and config_get_float tests passed!\n");
 }
 
 void test_config_load_save(void) {
     // Note: This test doesn't actually load/save from disk since that would require
     // file operations that are problematic in unit tests. Instead, we just test
     // the parameter validation.
     
     printf("Testing config_load and config_save functions...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Test load with NULL parameters
     polycall_core_error_t result = polycall_network_config_load(NULL, config, "test_config.ini");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_load(core_ctx, NULL, "test_config.ini");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_load(core_ctx, config, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test save with NULL parameters
     result = polycall_network_config_save(NULL, config, "test_config.ini");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_save(core_ctx, NULL, "test_config.ini");
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test save with no filename (should use the one from config)
     strcpy(config->config_file, "test_config.ini");
     result = polycall_network_config_save(core_ctx, config, NULL);
     
     // Since we're not actually writing to disk, just check that it accepted the parameters
     assert(result != POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_load and config_save tests passed!\n");
 }
 
 void test_config_reset(void) {
     printf("Testing config_reset function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Add some values
     polycall_network_config_set_int(core_ctx, config, "test", "int_value", 12345);
     polycall_network_config_set_string(core_ctx, config, "test", "string_value", "test string");
     
     // Test reset with NULL parameters
     polycall_core_error_t result = polycall_network_config_reset(NULL, config);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_reset(core_ctx, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test reset with valid parameters
     result = polycall_network_config_reset(core_ctx, config);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Verify previous values are gone
     int int_value = 0;
     result = polycall_network_config_get_int(core_ctx, config, "test", "int_value", &int_value);
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     char string_value[64];
     result = polycall_network_config_get_string(core_ctx, config, "test", "string_value", string_value, sizeof(string_value));
     assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);
     
     // Verify config is modified
     assert(config->modified == true);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_reset tests passed!\n");
 }
 
 void test_config_enumerate(void) {
     printf("Testing config_enumerate function...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Add some values in different sections
     polycall_network_config_set_int(core_ctx, config, "section1", "int_value1", 12345);
     polycall_network_config_set_int(core_ctx, config, "section1", "int_value2", 67890);
     polycall_network_config_set_string(core_ctx, config, "section2", "string_value1", "test string 1");
     polycall_network_config_set_string(core_ctx, config, "section2", "string_value2", "test string 2");
     
     // Test enumerate with NULL parameters
     test_enum_count = 0;
     polycall_core_error_t result = polycall_network_config_enumerate(NULL, config, "section1", test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_enumerate(core_ctx, NULL, "section1", test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_enumerate(core_ctx, config, NULL, test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     result = polycall_network_config_enumerate(core_ctx, config, "section1", NULL, NULL);
     assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);
     
     // Test enumerate for section1
     test_enum_count = 0;
     result = polycall_network_config_enumerate(core_ctx, config, "section1", test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(test_enum_count == 2); // Two values in section1
     
     // Test enumerate for section2
     test_enum_count = 0;
     result = polycall_network_config_enumerate(core_ctx, config, "section2", test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(test_enum_count == 2); // Two values in section2
     
     // Test enumerate for all sections
     test_enum_count = 0;
     result = polycall_network_config_enumerate(core_ctx, config, "", test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(test_enum_count == 4); // All four values
     
     // Test enumerate for non-existent section
     test_enum_count = 0;
     result = polycall_network_config_enumerate(core_ctx, config, "non_existent", test_enum_callback, NULL);
     assert(result == POLYCALL_CORE_SUCCESS);
     assert(test_enum_count == 0); // No values in non-existent section
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config_enumerate tests passed!\n");
 }
 
 void test_config_validation(void) {
     printf("Testing config validation callback...\n");
     
     // Initialize core context
     polycall_core_context_t* core_ctx = create_test_core_context();
     
     // Create config
     polycall_network_config_t* config = NULL;
     polycall_network_config_create(core_ctx, &config, NULL);
     
     // Set validation callback
     test_validation_callback_called = false;
     polycall_network_config_set_validator(core_ctx, config, test_validation_callback, NULL);
     
     // Set a value to trigger validation
     polycall_core_error_t result = polycall_network_config_set_int(core_ctx, config, "test", "int_value", 12345);
     assert(result == POLYCALL_CORE_SUCCESS);
     
     // Verify validation callback was called
     assert(test_validation_callback_called == true);
     
     // Clean up
     polycall_network_config_destroy(core_ctx, config);
     free(core_ctx);
     
     printf("config validation callback tests passed!\n");
 }
 
 int main(void) {
     int main(void) {
     printf("Running network configuration module unit tests...\n");
     
     // Run all test functions
     test_config_create_destroy();
     test_config_set_validator();
     test_config_set_get_int();
     test_config_set_get_uint();
     test_config_set_get_bool();
     test_config_set_get_string();
     test_config_set_get_float();
     test_config_load_save();
     test_config_reset();
     test_config_enumerate();
     test_config_validation();
     
     printf("All network configuration module tests passed!\n");
     return 0;
 }