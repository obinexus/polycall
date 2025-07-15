/**
 * @file test_utils.h
 * @brief Common testing utilities for LibPolyCall
 * @author OBINexus LibPolyCall Testing Framework
 */

#ifndef LIBPOLYCALL_TEST_UTILS_H
#define LIBPOLYCALL_TEST_UTILS_H

#include <assert.h>
#include <polycall/core/polycall/polycall.h>
#include <polycall/core/polycall/polycall_error.h>
#include <polycall/core/telemetry/polycall_telemetry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Test assertion macros
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", message, __FILE__,    \
              __LINE__);                                                       \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_NOT_NULL(pointer, message)                                 \
  TEST_ASSERT((pointer) != NULL, message)

#define TEST_ASSERT_NULL(pointer, message)                                     \
  TEST_ASSERT((pointer) == NULL, message)

#define TEST_ASSERT_SUCCESS(result, message)                                   \
  TEST_ASSERT((result) == POLYCALL_CORE_SUCCESS, message)

// Test fixture structure
typedef struct {
  polycall_core_context_t *core_ctx;
  polycall_telemetry_context_t *telemetry_ctx;
  size_t initial_memory;
  clock_t start_time;
} test_fixture_t;

// Common test utilities
polycall_core_error_t test_setup_core_context(polycall_core_context_t **ctx);
polycall_core_error_t test_cleanup_core_context(polycall_core_context_t *ctx);
polycall_core_error_t
test_setup_telemetry(polycall_core_context_t *core_ctx,
                     polycall_telemetry_context_t **telemetry_ctx);
polycall_core_error_t
test_cleanup_telemetry(polycall_core_context_t *core_ctx,
                       polycall_telemetry_context_t *telemetry_ctx);

// Memory tracking utilities
size_t test_get_memory_usage(polycall_core_context_t *ctx);
void test_check_memory_leaks(polycall_core_context_t *ctx,
                             size_t initial_memory);

// Performance measurement utilities
void test_start_performance_measurement(test_fixture_t *fixture);
double test_end_performance_measurement(test_fixture_t *fixture);

// Test data generation utilities
char *test_generate_random_string(size_t length);
void test_generate_random_data(void *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // LIBPOLYCALL_TEST_UTILS_H
