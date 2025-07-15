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
 * @file test_ignore_integration.c
 * @brief Integration test for the ignore pattern systems
 * @author Based on Nnamdi Okpala's architecture design
 */

#include "polycall/core/polycall/core/polycall/config/ignore/polycall_ignore.h"
#include "polycall/core/polycall/core/polycall/config/polycallfile/ignore/polycallfile_ignore.h"
#include "polycall/core/polycall/core/polycall/config/polycallrc/ignore/polycallrc_ignore.h"
#include "polycall/core/polycall/core/polycall/polycall_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

/**
 * @brief Initialize a core context for testing
 */
static polycall_core_context_t *init_test_context(void) {
  polycall_core_context_t *ctx = malloc(sizeof(polycall_core_context_t));
  if (!ctx) {
    return NULL;
  }

  // Minimal initialization for testing
  memset(ctx, 0, sizeof(polycall_core_context_t));
  return ctx;
}

/**
 * @brief Clean up test context
 */
static void cleanup_test_context(polycall_core_context_t *ctx) { free(ctx); }

/**
 * @brief Test the core ignore system
 */
static int test_core_ignore(polycall_core_context_t *ctx) {
  printf("Testing core ignore system...\n");

  polycall_ignore_context_t *ignore_ctx = NULL;
  polycall_core_error_t result =
      polycall_ignore_context_init(ctx, &ignore_ctx, false);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to initialize core ignore context\n");
    return TEST_FAILURE;
  }

  // Add patterns
  polycall_ignore_add_pattern(ignore_ctx, "**/*.pem");
  polycall_ignore_add_pattern(ignore_ctx, "**/.git/");
  polycall_ignore_add_pattern(ignore_ctx, "**/secrets.json");
  polycall_ignore_add_pattern(ignore_ctx, "temp/*.log");
  polycall_ignore_add_pattern(ignore_ctx,
                              "!temp/important.log"); // Negation pattern

  // Test patterns
  if (!polycall_ignore_should_ignore(ignore_ctx, "certs/server.pem")) {
    printf("Error: should ignore 'certs/server.pem'\n");
    polycall_ignore_context_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycall_ignore_should_ignore(ignore_ctx, ".git/HEAD")) {
    printf("Error: should ignore '.git/HEAD'\n");
    polycall_ignore_context_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycall_ignore_should_ignore(ignore_ctx, "config/secrets.json")) {
    printf("Error: should ignore 'config/secrets.json'\n");
    polycall_ignore_context_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycall_ignore_should_ignore(ignore_ctx, "temp/debug.log")) {
    printf("Error: should ignore 'temp/debug.log'\n");
    polycall_ignore_context_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (polycall_ignore_should_ignore(ignore_ctx, "temp/important.log")) {
    printf(
        "Error: should NOT ignore 'temp/important.log' (negation pattern)\n");
    polycall_ignore_context_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (polycall_ignore_should_ignore(ignore_ctx, "server.c")) {
    printf("Error: should NOT ignore 'server.c'\n");
    polycall_ignore_context_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  printf("Core ignore test passed\n");
  polycall_ignore_context_cleanup(ctx, ignore_ctx);
  return TEST_SUCCESS;
}

/**
 * @brief Test the Polycallfile ignore system
 */
static int test_polycallfile_ignore(polycall_core_context_t *ctx) {
  printf("Testing Polycallfile ignore system...\n");

  polycallfile_ignore_context_t *ignore_ctx = NULL;
  polycall_core_error_t result =
      polycallfile_ignore_init(ctx, &ignore_ctx, false);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to initialize Polycallfile ignore context\n");
    return TEST_FAILURE;
  }

  // Add default patterns
  result = polycallfile_ignore_add_defaults(ignore_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to add default patterns\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Add custom patterns
  polycallfile_ignore_add_pattern(ignore_ctx, "custom_dir/*.tmp");
  polycallfile_ignore_add_pattern(ignore_ctx, "config.Polycallfile.custom");

  // Test against security sensitive files (should be in defaults)
  if (!polycallfile_ignore_should_ignore(ignore_ctx, "credentials.json")) {
    printf("Error: should ignore 'credentials.json'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycallfile_ignore_should_ignore(ignore_ctx, "certs/key.pem")) {
    printf("Error: should ignore 'certs/key.pem'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Test against dev artifacts (should be in defaults)
  if (!polycallfile_ignore_should_ignore(ignore_ctx, ".git/config")) {
    printf("Error: should ignore '.git/config'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycallfile_ignore_should_ignore(ignore_ctx,
                                         "node_modules/package.json")) {
    printf("Error: should ignore 'node_modules/package.json'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Test custom patterns
  if (!polycallfile_ignore_should_ignore(ignore_ctx, "custom_dir/file.tmp")) {
    printf("Error: should ignore 'custom_dir/file.tmp'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycallfile_ignore_should_ignore(ignore_ctx,
                                         "config.Polycallfile.custom")) {
    printf("Error: should ignore 'config.Polycallfile.custom'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Test valid files
  if (polycallfile_ignore_should_ignore(ignore_ctx, "config.Polycallfile")) {
    printf("Error: should NOT ignore 'config.Polycallfile'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (polycallfile_ignore_should_ignore(ignore_ctx, "src/main.c")) {
    printf("Error: should NOT ignore 'src/main.c'\n");
    polycallfile_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  printf("Polycallfile ignore test passed\n");
  polycallfile_ignore_cleanup(ctx, ignore_ctx);
  return TEST_SUCCESS;
}

/**
 * @brief Test the PolycallRC ignore system
 */
static int test_polycallrc_ignore(polycall_core_context_t *ctx) {
  printf("Testing PolycallRC ignore system...\n");

  polycallrc_ignore_context_t *ignore_ctx = NULL;
  polycall_core_error_t result =
      polycallrc_ignore_init(ctx, &ignore_ctx, false);

  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to initialize PolycallRC ignore context\n");
    return TEST_FAILURE;
  }

  // Add default patterns
  result = polycallrc_ignore_add_defaults(ignore_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    printf("Failed to add default patterns\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Add custom patterns
  polycallrc_ignore_add_pattern(ignore_ctx, "binding_temp/*.cache");
  polycallrc_ignore_add_pattern(ignore_ctx, ".polycallrc.debug");

  // Test against security sensitive files (should be in defaults)
  if (!polycallrc_ignore_should_ignore(ignore_ctx, "credentials.json")) {
    printf("Error: should ignore 'credentials.json'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycallrc_ignore_should_ignore(ignore_ctx, "certs/key.pem")) {
    printf("Error: should ignore 'certs/key.pem'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Test against RC-specific files
  if (!polycallrc_ignore_should_ignore(ignore_ctx, ".polycallrc.bak")) {
    printf("Error: should ignore '.polycallrc.bak'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycallrc_ignore_should_ignore(ignore_ctx, ".binding_cache/session")) {
    printf("Error: should ignore '.binding_cache/session'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Test custom patterns
  if (!polycallrc_ignore_should_ignore(ignore_ctx, "binding_temp/data.cache")) {
    printf("Error: should ignore 'binding_temp/data.cache'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (!polycallrc_ignore_should_ignore(ignore_ctx, ".polycallrc.debug")) {
    printf("Error: should ignore '.polycallrc.debug'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  // Test valid files
  if (polycallrc_ignore_should_ignore(ignore_ctx, ".polycallrc")) {
    printf("Error: should NOT ignore '.polycallrc'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  if (polycallrc_ignore_should_ignore(ignore_ctx, "src/binding.c")) {
    printf("Error: should NOT ignore 'src/binding.c'\n");
    polycallrc_ignore_cleanup(ctx, ignore_ctx);
    return TEST_FAILURE;
  }

  printf("PolycallRC ignore test passed\n");
  polycallrc_ignore_cleanup(ctx, ignore_ctx);
  return TEST_SUCCESS;
}

/**
 * @brief Main test function
 */
int main(void) {
  printf("=== LibPolyCall Ignore System Integration Test ===\n\n");

  polycall_core_context_t *ctx = init_test_context();
  if (!ctx) {
    printf("Failed to initialize test context\n");
    return EXIT_FAILURE;
  }

  int result = TEST_SUCCESS;

  // Test core ignore system
  if (test_core_ignore(ctx) != TEST_SUCCESS) {
    result = TEST_FAILURE;
  }

  printf("\n");

  // Test Polycallfile ignore system
  if (test_polycallfile_ignore(ctx) != TEST_SUCCESS) {
    result = TEST_FAILURE;
  }

  printf("\n");

  // Test PolycallRC ignore system
  if (test_polycallrc_ignore(ctx) != TEST_SUCCESS) {
    result = TEST_FAILURE;
  }

  cleanup_test_context(ctx);

  printf("\n=== Integration Test %s ===\n",
         result == TEST_SUCCESS ? "PASSED" : "FAILED");

  return result == TEST_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
