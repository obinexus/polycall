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
 * @file polycall_test_utils.c
 * @brief Enhanced test utilities for IoC-driven libpolycall testing
 * @author OBINexus LibPolyCall Testing Framework
 */

#include "polycall_test_utils.h"
#include <polycall/core/config/config.h>
#include <polycall/core/polycall/polycall.h>
#include <polycall/core/telemetry/polycall_telemetry.h>
#include <string.h>
#include <sys/stat.h>

// Global test configuration context
static polycall_test_context_t *g_test_context = NULL;

/**
 * @brief Initialize IoC-aware test environment
 */
polycall_core_error_t
polycall_test_init_context(polycall_test_context_t **ctx) {
  if (!ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Allocate test context
  *ctx = calloc(1, sizeof(polycall_test_context_t));
  if (!*ctx) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  polycall_test_context_t *test_ctx = *ctx;

  // Create temporary test directory
  snprintf(test_ctx->temp_dir, sizeof(test_ctx->temp_dir),
           "/tmp/polycall_test_%d", getpid());
  if (mkdir(test_ctx->temp_dir, 0755) != 0) {
    free(test_ctx);
    return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
  }

  // Initialize core context from test configuration
  polycall_core_error_t result =
      polycall_core_context_create(&test_ctx->core_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_test_cleanup_context(test_ctx);
    return result;
  }

  // Load test configuration
  const char *test_config = getenv("POLYCALL_TEST_CONFIG_FILE");
  if (!test_config) {
    test_config = "tests/fixtures/config.Polycallfile.test";
  }

  polycall_config_context_t *config_ctx = NULL;
  result = polycall_config_init(test_ctx->core_ctx, &config_ctx, NULL);
  if (result == POLYCALL_CORE_SUCCESS) {
    result =
        polycall_config_load_file(test_ctx->core_ctx, config_ctx, test_config);
    test_ctx->config_ctx = config_ctx;
  }

  // Initialize telemetry for test monitoring
  result = polycall_telemetry_init(test_ctx->core_ctx, &test_ctx->telemetry_ctx,
                                   NULL);
  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_test_cleanup_context(test_ctx);
    return result;
  }

  // Record initial memory state
  test_ctx->initial_memory =
      polycall_core_get_allocated_memory(test_ctx->core_ctx);
  test_ctx->start_time = clock();

  g_test_context = test_ctx;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup IoC test environment
 */
polycall_core_error_t
polycall_test_cleanup_context(polycall_test_context_t *ctx) {
  if (!ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Check for memory leaks
  if (ctx->core_ctx) {
    size_t final_memory = polycall_core_get_allocated_memory(ctx->core_ctx);
    if (final_memory > ctx->initial_memory) {
      fprintf(stderr, "MEMORY LEAK DETECTED: %zu bytes leaked\n",
              final_memory - ctx->initial_memory);
    }
  }

  // Cleanup telemetry
  if (ctx->telemetry_ctx && ctx->core_ctx) {
    polycall_telemetry_cleanup(ctx->core_ctx, ctx->telemetry_ctx);
  }

  // Cleanup configuration
  if (ctx->config_ctx && ctx->core_ctx) {
    polycall_config_cleanup(ctx->core_ctx, ctx->config_ctx);
  }

  // Cleanup core context
  if (ctx->core_ctx) {
    polycall_core_context_destroy(ctx->core_ctx);
  }

  // Remove temporary directory
  if (strlen(ctx->temp_dir) > 0) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", ctx->temp_dir);
    system(cmd);
  }

  free(ctx);
  g_test_context = NULL;

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Setup module-specific test fixture with IoC integration
 */
polycall_core_error_t
polycall_test_setup_module_fixture(const char *module_name,
                                   polycall_test_module_fixture_t **fixture) {
  if (!module_name || !fixture || !g_test_context) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  *fixture = calloc(1, sizeof(polycall_test_module_fixture_t));
  if (!*fixture) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  polycall_test_module_fixture_t *mod_fixture = *fixture;

  // Copy module name
  strncpy(mod_fixture->module_name, module_name,
          sizeof(mod_fixture->module_name) - 1);

  // Reference global test context
  mod_fixture->test_ctx = g_test_context;

  // Initialize module-specific context based on module name
  polycall_core_error_t result = POLYCALL_CORE_SUCCESS;

  if (strcmp(module_name, "protocol") == 0) {
    result = polycall_protocol_init(
        g_test_context->core_ctx, &mod_fixture->module_ctx.protocol_ctx, NULL);
  } else if (strcmp(module_name, "network") == 0) {
    result = polycall_network_init(g_test_context->core_ctx,
                                   &mod_fixture->module_ctx.network_ctx, NULL);
  } else if (strcmp(module_name, "telemetry") == 0) {
    mod_fixture->module_ctx.telemetry_ctx = g_test_context->telemetry_ctx;
  } else if (strcmp(module_name, "config") == 0) {
    mod_fixture->module_ctx.config_ctx = g_test_context->config_ctx;
  } else {
    // Generic module initialization
    result = POLYCALL_CORE_SUCCESS;
  }

  if (result != POLYCALL_CORE_SUCCESS) {
    free(mod_fixture);
    *fixture = NULL;
  }

  return result;
}

/**
 * @brief Cleanup module-specific test fixture
 */
polycall_core_error_t
polycall_test_cleanup_module_fixture(polycall_test_module_fixture_t *fixture) {
  if (!fixture) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Cleanup module-specific context
  if (strcmp(fixture->module_name, "protocol") == 0 &&
      fixture->module_ctx.protocol_ctx) {
    polycall_protocol_cleanup(fixture->test_ctx->core_ctx,
                              fixture->module_ctx.protocol_ctx);
  } else if (strcmp(fixture->module_name, "network") == 0 &&
             fixture->module_ctx.network_ctx) {
    polycall_network_cleanup(fixture->test_ctx->core_ctx,
                             fixture->module_ctx.network_ctx);
  }

  free(fixture);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Validate telemetry data integrity
 */
polycall_core_error_t
polycall_test_validate_telemetry(polycall_test_context_t *ctx,
                                 const char *operation_name,
                                 size_t expected_count) {
  if (!ctx || !operation_name || !ctx->telemetry_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  polycall_telemetry_stats_t stats;
  polycall_core_error_t result =
      polycall_telemetry_get_stats(ctx->core_ctx, ctx->telemetry_ctx, &stats);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Validate operation count
  if (stats.operation_count < expected_count) {
    fprintf(stderr,
            "Telemetry validation failed: expected %zu operations, found %zu\n",
            expected_count, stats.operation_count);
    return POLYCALL_CORE_ERROR_INVALID_STATE;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Generate test data with specified characteristics
 */
void polycall_test_generate_data(void *buffer, size_t size,
                                 polycall_test_data_type_t type) {
  if (!buffer || size == 0) {
    return;
  }

  unsigned char *bytes = (unsigned char *)buffer;

  switch (type) {
  case POLYCALL_TEST_DATA_RANDOM:
    for (size_t i = 0; i < size; i++) {
      bytes[i] = (unsigned char)(rand() % 256);
    }
    break;

  case POLYCALL_TEST_DATA_SEQUENTIAL:
    for (size_t i = 0; i < size; i++) {
      bytes[i] = (unsigned char)(i % 256);
    }
    break;

  case POLYCALL_TEST_DATA_ZEROS:
    memset(buffer, 0, size);
    break;

  case POLYCALL_TEST_DATA_ONES:
    memset(buffer, 0xFF, size);
    break;

  default:
    memset(buffer, 0xAA, size); // Alternating pattern
    break;
  }
}

/**
 * @brief Performance measurement utilities
 */
double polycall_test_measure_performance(polycall_test_context_t *ctx) {
  if (!ctx) {
    return 0.0;
  }

  clock_t end_time = clock();
  return ((double)(end_time - ctx->start_time)) / CLOCKS_PER_SEC;
}

/**
 * @brief Resource usage validation
 */
polycall_core_error_t
polycall_test_validate_resources(polycall_test_context_t *ctx,
                                 size_t max_memory_bytes,
                                 double max_cpu_seconds) {
  if (!ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Check memory usage
  size_t current_memory = polycall_core_get_allocated_memory(ctx->core_ctx);
  if (current_memory > max_memory_bytes) {
    fprintf(stderr, "Memory limit exceeded: %zu bytes > %zu bytes\n",
            current_memory, max_memory_bytes);
    return POLYCALL_CORE_ERROR_RESOURCE_EXHAUSTED;
  }

  // Check CPU time
  double cpu_time = polycall_test_measure_performance(ctx);
  if (cpu_time > max_cpu_seconds) {
    fprintf(stderr, "CPU time limit exceeded: %.2f seconds > %.2f seconds\n",
            cpu_time, max_cpu_seconds);
    return POLYCALL_CORE_ERROR_RESOURCE_EXHAUSTED;
  }

  return POLYCALL_CORE_SUCCESS;
}
