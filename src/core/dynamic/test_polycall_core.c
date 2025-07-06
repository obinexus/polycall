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
 * @file test_polycall_core.c
 * @brief Main test runner for polycall core module tests
 * @author Nnamdi Okpala (OBINexusComputing)
 */

#include "polycall/core/polycall/polycall_core.h"
#include "unit_test_framework.h"
#include <stdio.h>
#include <string.h>

// External test functions
extern int run_context_tests();
extern int run_error_tests();
extern int run_memory_tests();
extern int run_config_tests();

// Global variables for test
static polycall_core_context_t *g_core_ctx = NULL;

// Initialize test resources
static void setup() {
  // Initialize the core context
  polycall_core_config_t config = {.flags = POLYCALL_CORE_FLAG_NONE,
                                   .memory_pool_size =
                                       1024 * 1024, // 1MB for testing
                                   .user_data = NULL,
                                   .error_callback = NULL};

  polycall_core_init(&g_core_ctx, &config);
}

// Clean up test resources
static void teardown() {
  // Clean up the core context
  if (g_core_ctx) {
    polycall_core_cleanup(g_core_ctx);
    g_core_ctx = NULL;
  }
}

// Test core initialization
static int test_core_init() {
  // Verify core context was created
  ASSERT_NOT_NULL(g_core_ctx);

  return 0;
}

// Test core version
static int test_core_version() {
  // Get version
  const char *version = polycall_core_get_version();

  // Verify version is available
  ASSERT_NOT_NULL(version);
  ASSERT_TRUE(strlen(version) > 0);

  return 0;
}

// Test user data
static int test_core_user_data() {
  // Create some user data
  int test_data = 42;

  // Set user data
  polycall_core_error_t result =
      polycall_core_set_user_data(g_core_ctx, &test_data);

  // Verify set succeeded
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Get user data
  void *user_data = polycall_core_get_user_data(g_core_ctx);

  // Verify user data is correct
  ASSERT_NOT_NULL(user_data);
  ASSERT_EQUAL_INT(42, *(int *)user_data);

  return 0;
}

// Test memory allocation
static int test_core_memory() {
  // Allocate memory
  void *ptr = polycall_core_malloc(g_core_ctx, 1024);

  // Verify allocation succeeded
  ASSERT_NOT_NULL(ptr);

  // Free memory
  polycall_core_free(g_core_ctx, ptr);

  return 0;
}

// Test error handling
static int test_core_error() {
  // Set an error
  polycall_core_error_t error = polycall_core_set_error(
      g_core_ctx, POLYCALL_CORE_ERROR_INVALID_PARAMETERS, "Test error message");

  // Verify error code is returned
  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, error);

  // Get the error
  const char *message;
  error = polycall_core_get_last_error(g_core_ctx, &message);

  // Verify error details
  ASSERT_EQUAL_INT(POLYCALL_CORE_ERROR_INVALID_PARAMETERS, error);
  ASSERT_NOT_NULL(message);
  ASSERT_TRUE(strstr(message, "Test error message") != NULL);

  return 0;
}

// Main test function
int main() {
  int result = 0;

  // Run core-specific tests
  RESET_TESTS();

  setup();

  printf("\n===== Running Core Module Tests =====\n");
  RUN_TEST(test_core_init);
  RUN_TEST(test_core_version);
  RUN_TEST(test_core_user_data);
  RUN_TEST(test_core_memory);
  RUN_TEST(test_core_error);

  teardown();

  result |= (g_tests_failed > 0 ? 1 : 0);

  // Run context module tests
  printf("\n===== Running Context Module Tests =====\n");
  result |= run_context_tests();

  // Run error module tests
  printf("\n===== Running Error Module Tests =====\n");
  result |= run_error_tests();

  // Run memory module tests
  printf("\n===== Running Memory Module Tests =====\n");
  result |= run_memory_tests();

  // Run config module tests
  printf("\n===== Running Config Module Tests =====\n");
  result |= run_config_tests();

  // Print summary and return
  printf("\n===== Test Summary =====\n");
  if (result == 0) {
    printf("All tests passed successfully!\n");
  } else {
    printf("Some tests failed. Check the output for details.\n");
  }

  return result;
}
