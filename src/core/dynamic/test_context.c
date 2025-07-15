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
 * @file test_context.c
 * @brief Unit tests for the context management functionality in LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 */

#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/polycall_core.h"
#include "unit_test_framework.h"
#include <stdio.h>
#include <string.h>

// Global variables for test
static polycall_core_context_t *g_core_ctx = NULL;
static polycall_context_ref_t *g_test_ctx = NULL;

// Test data structure
typedef struct {
  int value;
  char name[32];
} test_context_data_t;

// Init function for test context
static polycall_core_error_t
test_context_init(polycall_core_context_t *core_ctx, void *ctx_data,
                  void *init_data) {
  test_context_data_t *data = (test_context_data_t *)ctx_data;
  if (!data)
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;

  // Initialize with test values
  data->value = 42;
  strncpy(data->name, "TestContext", sizeof(data->name) - 1);
  data->name[sizeof(data->name) - 1] = '\0';

  return POLYCALL_CORE_SUCCESS;
}

// Cleanup function for test context
static void test_context_cleanup(polycall_core_context_t *core_ctx,
                                 void *ctx_data) {
  // Nothing to clean up in this simple test
}

// Context listener for testing
static void test_context_listener(polycall_context_ref_t *ctx_ref,
                                  void *user_data) {
  int *listener_called = (int *)user_data;
  if (listener_called) {
    (*listener_called)++;
  }
}

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
  // Clean up the test context if it exists
  if (g_test_ctx) {
    polycall_context_cleanup(g_core_ctx, g_test_ctx);
    g_test_ctx = NULL;
  }

  // Clean up the core context
  if (g_core_ctx) {
    polycall_core_cleanup(g_core_ctx);
    g_core_ctx = NULL;
  }
}

// Test context initialization
static int test_context_initialization() {
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER,
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "TestContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_core_error_t result =
      polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Verify initialization succeeded
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
  ASSERT_NOT_NULL(g_test_ctx);

  // Verify context is initialized
  ASSERT_TRUE(polycall_context_is_initialized(g_core_ctx, g_test_ctx));

  return 0;
}

// Test getting context data
static int test_get_context_data() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER,
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "TestContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Get the context data
  test_context_data_t *data =
      (test_context_data_t *)polycall_context_get_data(g_core_ctx, g_test_ctx);

  // Verify data access
  ASSERT_NOT_NULL(data);
  ASSERT_EQUAL_INT(42, data->value);
  ASSERT_EQUAL_STR("TestContext", data->name);

  return 0;
}

// Test finding context by type
static int test_find_context_by_type() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          1, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "TypedContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Find the context by type
  polycall_context_ref_t *found_ctx =
      polycall_context_find_by_type(g_core_ctx, POLYCALL_CONTEXT_TYPE_USER + 1);

  // Verify found context
  ASSERT_NOT_NULL(found_ctx);
  ASSERT_EQUAL_PTR(g_test_ctx, found_ctx);

  return 0;
}

// Test finding context by name
static int test_find_context_by_name() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          2, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "NamedContext", // Unique name
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Find the context by name
  polycall_context_ref_t *found_ctx =
      polycall_context_find_by_name(g_core_ctx, "NamedContext");

  // Verify found context
  ASSERT_NOT_NULL(found_ctx);
  ASSERT_EQUAL_PTR(g_test_ctx, found_ctx);

  return 0;
}

// Test context flags
static int test_context_flags() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          3, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "FlaggedContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Verify initial flags
  polycall_context_flags_t flags =
      polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_INITIALIZED) != 0);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_LOCKED) == 0);

  // Set new flags
  polycall_core_error_t result = polycall_context_set_flags(
      g_core_ctx, g_test_ctx, POLYCALL_CONTEXT_FLAG_SHARED);

  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Verify flags were updated
  flags = polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_SHARED) != 0);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_INITIALIZED) !=
              0); // This flag should be preserved

  return 0;
}

// Test context locking
static int test_context_locking() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          4, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "LockableContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Lock the context
  polycall_core_error_t result = polycall_context_lock(g_core_ctx, g_test_ctx);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Verify context is locked
  polycall_context_flags_t flags =
      polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_LOCKED) != 0);

  // Try to change flags (should fail when locked)
  result = polycall_context_set_flags(g_core_ctx, g_test_ctx,
                                      POLYCALL_CONTEXT_FLAG_SHARED);
  ASSERT_TRUE(result != POLYCALL_CORE_SUCCESS);

  // Unlock the context
  result = polycall_context_unlock(g_core_ctx, g_test_ctx);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Verify context is unlocked
  flags = polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_LOCKED) == 0);

  // Now changing flags should work
  result = polycall_context_set_flags(g_core_ctx, g_test_ctx,
                                      POLYCALL_CONTEXT_FLAG_SHARED);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  return 0;
}

// Test context sharing
static int test_context_sharing() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          5, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "SharableContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Share the context
  polycall_core_error_t result =
      polycall_context_share(g_core_ctx, g_test_ctx, "TestComponent");
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Verify context is shared
  polycall_context_flags_t flags =
      polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_SHARED) != 0);

  // Unshare the context
  result = polycall_context_unshare(g_core_ctx, g_test_ctx);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Verify context is no longer shared
  flags = polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_SHARED) == 0);

  return 0;
}

// Test context isolation
static int test_context_isolation() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          6, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "IsolatableContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Isolate the context
  polycall_core_error_t result =
      polycall_context_isolate(g_core_ctx, g_test_ctx);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Verify context is isolated
  polycall_context_flags_t flags =
      polycall_context_get_flags(g_core_ctx, g_test_ctx);
  ASSERT_TRUE((flags & POLYCALL_CONTEXT_FLAG_ISOLATED) != 0);

  // Try to share the context (should fail when isolated)
  result = polycall_context_share(g_core_ctx, g_test_ctx, "TestComponent");
  ASSERT_TRUE(result != POLYCALL_CORE_SUCCESS);

  return 0;
}

// Test context listeners
static int test_context_listeners() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          7, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "ListenableContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Setup listener data
  int listener_called = 0;

  // Register a listener
  polycall_core_error_t result = polycall_context_register_listener(
      g_core_ctx, g_test_ctx, test_context_listener, &listener_called);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Trigger an event that should notify listeners
  polycall_context_lock(g_core_ctx, g_test_ctx);

  // Verify listener was called
  ASSERT_TRUE(listener_called > 0);

  // Reset counter for next test
  listener_called = 0;

  // Unregister the listener
  result = polycall_context_unregister_listener(
      g_core_ctx, g_test_ctx, test_context_listener, &listener_called);
  ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);

  // Trigger another event
  polycall_context_unlock(g_core_ctx, g_test_ctx);

  // Verify listener was not called this time
  ASSERT_EQUAL_INT(0, listener_called);

  return 0;
}

// Test context cleanup
static int test_context_cleanup() {
  // First initialize the context
  polycall_context_init_t init = {.type = POLYCALL_CONTEXT_TYPE_USER +
                                          8, // Use a unique type
                                  .data_size = sizeof(test_context_data_t),
                                  .flags = POLYCALL_CONTEXT_FLAG_NONE,
                                  .name = "CleanupContext",
                                  .init_fn = test_context_init,
                                  .cleanup_fn = test_context_cleanup,
                                  .init_data = NULL};

  polycall_context_init(g_core_ctx, &g_test_ctx, &init);

  // Clean up the context
  polycall_context_cleanup(g_core_ctx, g_test_ctx);

  // Verify context is no longer findable
  polycall_context_ref_t *found_ctx =
      polycall_context_find_by_name(g_core_ctx, "CleanupContext");
  ASSERT_NULL(found_ctx);

  // Reset test_ctx to avoid double cleanup in teardown
  g_test_ctx = NULL;

  return 0;
}

// Run all context tests
int run_context_tests() {
  RESET_TESTS();

  setup();

  RUN_TEST(test_context_initialization);
  RUN_TEST(test_get_context_data);
  RUN_TEST(test_find_context_by_type);
  RUN_TEST(test_find_context_by_name);
  RUN_TEST(test_context_flags);
  RUN_TEST(test_context_locking);
  RUN_TEST(test_context_sharing);
  RUN_TEST(test_context_isolation);
  RUN_TEST(test_context_listeners);
  RUN_TEST(test_context_cleanup);

  teardown();

  return g_tests_failed > 0 ? 1 : 0;
}
