/**
 * @file polycall_test_framework.c
 * @brief Implementation of the LibPolyCall test framework
 * @author OBINexusComputing
 */

#include "polycall_test_framework.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/**
 * @brief Global test registry
 */
polycall_test_registry_t g_polycall_test_registry = {0};

/**
 * @brief Global test context
 */
polycall_test_context_t g_polycall_test_context = {0};

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Print formatted message to test output
 *
 * @param color ANSI color code
 * @param format Message format
 * @param ... Format arguments
 */
static void polycall_test_print(const char *color, const char *format, ...) {
  FILE *output = g_polycall_test_registry.output_file
                     ? g_polycall_test_registry.output_file
                     : stdout;

  if (g_polycall_test_registry.color && color) {
    fprintf(output, "%s", color);
  }

  va_list args;
  va_start(args, format);
  vfprintf(output, format, args);
  va_end(args);

  if (g_polycall_test_registry.color && color) {
    fprintf(output, "%s", POLYCALL_COLOR_RESET);
  }
}

/**
 * @brief Print test header
 *
 * @param suite_name Suite name
 * @param test_name Test name
 */
static void polycall_test_print_header(const char *suite_name,
                                       const char *test_name) {
  polycall_test_print(POLYCALL_COLOR_BLUE, "\n===== TEST %s::%s =====\n",
                      suite_name, test_name);
}

/**
 * @brief Print test footer
 *
 * @param suite_name Suite name
 * @param test_name Test name
 * @param status Test status
 * @param message Status message
 */
static void polycall_test_print_footer(const char *suite_name,
                                       const char *test_name,
                                       polycall_test_status_t status,
                                       const char *message) {
  const char *status_str = "";
  const char *color = NULL;

  switch (status) {
  case POLYCALL_TEST_STATUS_PASSED:
    status_str = "PASS";
    color = POLYCALL_COLOR_GREEN;
    break;
  case POLYCALL_TEST_STATUS_FAILED:
    status_str = "FAIL";
    color = POLYCALL_COLOR_RED;
    break;
  case POLYCALL_TEST_STATUS_SKIPPED:
    status_str = "SKIP";
    color = POLYCALL_COLOR_YELLOW;
    break;
  case POLYCALL_TEST_STATUS_ERROR:
    status_str = "ERROR";
    color = POLYCALL_COLOR_RED;
    break;
  }

  polycall_test_print(color, "----- %s: %s::%s", status_str, suite_name,
                      test_name);

  if (message && message[0] != '\0') {
    polycall_test_print(NULL, " (%s)", message);
  }

  polycall_test_print(NULL, " -----\n\n");
}

/**
 * @brief Format an error message
 *
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @param format Message format
 * @param args Format arguments
 */
static void polycall_test_format_message(char *buffer, size_t buffer_size,
                                         const char *format, va_list args) {
  if (!format || format[0] == '\0') {
    buffer[0] = '\0';
    return;
  }

  vsnprintf(buffer, buffer_size, format, args);
  buffer[buffer_size - 1] = '\0';
}

/*******************************************************************************
 * Public API Implementation
 ******************************************************************************/

bool polycall_test_init(bool verbose, bool color, bool exit_on_failure,
                        FILE *output_file, bool output_xml,
                        const char *xml_file) {
  // Initialize registry
  memset(&g_polycall_test_registry, 0, sizeof(g_polycall_test_registry));
  g_polycall_test_registry.verbose = verbose;
  g_polycall_test_registry.color = color;
  g_polycall_test_registry.exit_on_failure = exit_on_failure;
  g_polycall_test_registry.output_file = output_file;
  g_polycall_test_registry.output_xml = output_xml;

  if (xml_file) {
    strncpy(g_polycall_test_registry.xml_file, xml_file,
            POLYCALL_TEST_NAME_MAX_LENGTH - 1);
    g_polycall_test_registry.xml_file[POLYCALL_TEST_NAME_MAX_LENGTH - 1] = '\0';
  }

  // Initialize context
  memset(&g_polycall_test_context, 0, sizeof(g_polycall_test_context));

  return true;
}

void polycall_test_cleanup(void) {
  // Clean up registry
  if (g_polycall_test_registry.output_file &&
      g_polycall_test_registry.output_file != stdout) {
    fclose(g_polycall_test_registry.output_file);
    g_polycall_test_registry.output_file = NULL;
  }

  // Clean up context
  memset(&g_polycall_test_context, 0, sizeof(g_polycall_test_context));
}

int polycall_test_create_suite(
    const char *name, polycall_test_fixture_fn global_setup,
    polycall_test_fixture_cleanup_fn global_cleanup) {
  // Check if we can add more suites
  if (g_polycall_test_registry.suite_count >= POLYCALL_MAX_TEST_SUITES) {
    polycall_test_print(POLYCALL_COLOR_RED,
                        "ERROR: Cannot create suite '%s', maximum number of "
                        "suites reached (%d)\n",
                        name, POLYCALL_MAX_TEST_SUITES);
    return -1;
  }

  // Check if a suite with the same name already exists
  for (int i = 0; i < g_polycall_test_registry.suite_count; i++) {
    if (strcmp(g_polycall_test_registry.suites[i].name, name) == 0) {
      polycall_test_print(POLYCALL_COLOR_YELLOW,
                          "WARNING: Suite '%s' already exists\n", name);
      return i;
    }
  }

  // Create new suite
  polycall_test_suite_t *suite =
      &g_polycall_test_registry.suites[g_polycall_test_registry.suite_count];
  memset(suite, 0, sizeof(polycall_test_suite_t));

  strncpy(suite->name, name, POLYCALL_TEST_NAME_MAX_LENGTH - 1);
  suite->name[POLYCALL_TEST_NAME_MAX_LENGTH - 1] = '\0';

  suite->global_setup = global_setup;
  suite->global_cleanup = global_cleanup;

  if (g_polycall_test_registry.verbose) {
    polycall_test_print(POLYCALL_COLOR_BLUE, "Created test suite: %s\n", name);
  }

  return g_polycall_test_registry.suite_count++;
}

int polycall_test_add_test(int suite_index, const char *name,
                           polycall_test_fn test_fn) {
  // Validate suite index
  if (suite_index < 0 || suite_index >= g_polycall_test_registry.suite_count) {
    polycall_test_print(POLYCALL_COLOR_RED, "ERROR: Invalid suite index: %d\n",
                        suite_index);
    return -1;
  }

  polycall_test_suite_t *suite = &g_polycall_test_registry.suites[suite_index];

  // Check if we can add more tests
  if (suite->test_count >= POLYCALL_MAX_TESTS_PER_SUITE) {
    polycall_test_print(POLYCALL_COLOR_RED,
                        "ERROR: Cannot add test '%s' to suite '%s', maximum "
                        "number of tests reached (%d)\n",
                        name, suite->name, POLYCALL_MAX_TESTS_PER_SUITE);
    return -1;
  }

  // Check if a test with the same name already exists
  for (int i = 0; i < suite->test_count; i++) {
    if (strcmp(suite->tests[i].name, name) == 0) {
      polycall_test_print(POLYCALL_COLOR_YELLOW,
                          "WARNING: Test '%s' already exists in suite '%s'\n",
                          name, suite->name);
      return i;
    }
  }

  // Add new test
  polycall_test_case_t *test = &suite->tests[suite->test_count];
  memset(test, 0, sizeof(polycall_test_case_t));

  strncpy(test->name, name, POLYCALL_TEST_NAME_MAX_LENGTH - 1);
  test->name[POLYCALL_TEST_NAME_MAX_LENGTH - 1] = '\0';

  test->test_fn = test_fn;
  test->status = POLYCALL_TEST_STATUS_PASSED; // Default to passed until run

  if (g_polycall_test_registry.verbose) {
    polycall_test_print(POLYCALL_COLOR_BLUE, "Added test '%s' to suite '%s'\n",
                        name, suite->name);
  }

  return suite->test_count++;
}

int polycall_test_add_fixture(int suite_index, const char *name,
                              polycall_test_fixture_fn setup_fn,
                              polycall_test_fixture_cleanup_fn cleanup_fn) {
  // Validate suite index
  if (suite_index < 0 || suite_index >= g_polycall_test_registry.suite_count) {
    polycall_test_print(POLYCALL_COLOR_RED, "ERROR: Invalid suite index: %d\n",
                        suite_index);
    return -1;
  }

  polycall_test_suite_t *suite = &g_polycall_test_registry.suites[suite_index];

  // Check if we can add more fixtures
  if (suite->fixture_count >= POLYCALL_MAX_FIXTURES_PER_SUITE) {
    polycall_test_print(POLYCALL_COLOR_RED,
                        "ERROR: Cannot add fixture '%s' to suite '%s', maximum "
                        "number of fixtures reached (%d)\n",
                        name, suite->name, POLYCALL_MAX_FIXTURES_PER_SUITE);
    return -1;
  }

  // Check if a fixture with the same name already exists
  for (int i = 0; i < suite->fixture_count; i++) {
    if (strcmp(suite->fixtures[i].name, name) == 0) {
      polycall_test_print(
          POLYCALL_COLOR_YELLOW,
          "WARNING: Fixture '%s' already exists in suite '%s'\n", name,
          suite->name);
      return i;
    }
  }

  // Add new fixture
  polycall_test_fixture_t *fixture = &suite->fixtures[suite->fixture_count];
  memset(fixture, 0, sizeof(polycall_test_fixture_t));

  strncpy(fixture->name, name, POLYCALL_TEST_NAME_MAX_LENGTH - 1);
  fixture->name[POLYCALL_TEST_NAME_MAX_LENGTH - 1] = '\0';

  fixture->setup_fn = setup_fn;
  fixture->cleanup_fn = cleanup_fn;

  if (g_polycall_test_registry.verbose) {
    polycall_test_print(POLYCALL_COLOR_BLUE,
                        "Added fixture '%s' to suite '%s'\n", name,
                        suite->name);
  }

  return suite->fixture_count++;
}

int polycall_test_run_all_suites(void) {
  int failed_tests = 0;

  // Record start time
  g_polycall_test_registry.total_stats.start_time = clock();

  // Print header
  polycall_test_print(POLYCALL_COLOR_BLUE,
                      "\n====== RUNNING ALL TEST SUITES ======\n\n");

  // Run each suite
  for (int i = 0; i < g_polycall_test_registry.suite_count; i++) {
    int suite_failed = polycall_test_run_suite(i);
    failed_tests += suite_failed;

    if (suite_failed > 0 && g_polycall_test_registry.exit_on_failure) {
      polycall_test_print(
          POLYCALL_COLOR_RED,
          "Exiting early due to test failures (--exit-on-failure)\n");
      break;
    }
  }

  // Record end time
  g_polycall_test_registry.total_stats.end_time = clock();

  return failed_tests;
}

int polycall_test_run_suite(int suite_index) {
  // Validate suite index
  if (suite_index < 0 || suite_index >= g_polycall_test_registry.suite_count) {
    polycall_test_print(POLYCALL_COLOR_RED, "ERROR: Invalid suite index: %d\n",
                        suite_index);
    return -1;
  }

  polycall_test_suite_t *suite = &g_polycall_test_registry.suites[suite_index];

  // Initialize suite statistics
  memset(&suite->stats, 0, sizeof(polycall_test_stats_t));
  suite->stats.start_time = clock();

  // Print suite header
  polycall_test_print(POLYCALL_COLOR_BLUE,
                      "\n====== RUNNING TEST SUITE: %s ======\n\n",
                      suite->name);

  // Set up global context if needed
  if (suite->global_setup) {
    if (g_polycall_test_registry.verbose) {
      polycall_test_print(POLYCALL_COLOR_BLUE,
                          "Setting up global context for suite '%s'\n",
                          suite->name);
    }

    suite->global_context = suite->global_setup();
  }

  int failed_tests = 0;

  // Run each test in the suite
  for (int i = 0; i < suite->test_count; i++) {
    polycall_test_status_t status = polycall_test_run_test(suite_index, i);

    // Update suite statistics
    suite->stats.tests_run++;

    switch (status) {
    case POLYCALL_TEST_STATUS_PASSED:
      suite->stats.tests_passed++;
      break;
    case POLYCALL_TEST_STATUS_FAILED:
      suite->stats.tests_failed++;
      failed_tests++;
      break;
    case POLYCALL_TEST_STATUS_SKIPPED:
      suite->stats.tests_skipped++;
      break;
    case POLYCALL_TEST_STATUS_ERROR:
      suite->stats.tests_errored++;
      failed_tests++;
      break;
    }

    // Exit early if configured
    if ((status == POLYCALL_TEST_STATUS_FAILED ||
         status == POLYCALL_TEST_STATUS_ERROR) &&
        g_polycall_test_registry.exit_on_failure) {
      polycall_test_print(
          POLYCALL_COLOR_RED,
          "Exiting early due to test failures (--exit-on-failure)\n");
      break;
    }
  }

  // Clean up global context if needed
  if (suite->global_cleanup && suite->global_context) {
    if (g_polycall_test_registry.verbose) {
      polycall_test_print(POLYCALL_COLOR_BLUE,
                          "Cleaning up global context for suite '%s'\n",
                          suite->name);
    }

    suite->global_cleanup(suite->global_context);
    suite->global_context = NULL;
  }

  // Record end time
  suite->stats.end_time = clock();

  // Print suite results
  double time_taken =
      (double)(suite->stats.end_time - suite->stats.start_time) /
      CLOCKS_PER_SEC;
  polycall_test_print(POLYCALL_COLOR_BLUE,
                      "\n------ SUITE RESULTS: %s ------\n", suite->name);
  polycall_test_print(POLYCALL_COLOR_BLUE, "Tests run:    %d\n",
                      suite->stats.tests_run);
  polycall_test_print(POLYCALL_COLOR_GREEN, "Tests passed: %d\n",
                      suite->stats.tests_passed);
  polycall_test_print(POLYCALL_COLOR_RED, "Tests failed: %d\n",
                      suite->stats.tests_failed);
  polycall_test_print(POLYCALL_COLOR_YELLOW, "Tests skipped: %d\n",
                      suite->stats.tests_skipped);
  polycall_test_print(POLYCALL_COLOR_RED, "Tests errored: %d\n",
                      suite->stats.tests_errored);
  polycall_test_print(POLYCALL_COLOR_BLUE, "Time taken:   %.3f seconds\n\n",
                      time_taken);

  // Update total statistics
  g_polycall_test_registry.total_stats.tests_run += suite->stats.tests_run;
  g_polycall_test_registry.total_stats.tests_passed +=
      suite->stats.tests_passed;
  g_polycall_test_registry.total_stats.tests_failed +=
      suite->stats.tests_failed;
  g_polycall_test_registry.total_stats.tests_skipped +=
      suite->stats.tests_skipped;
  g_polycall_test_registry.total_stats.tests_errored +=
      suite->stats.tests_errored;

  return failed_tests;
}

polycall_test_status_t polycall_test_run_test(int suite_index, int test_index) {
  // Validate suite index
  if (suite_index < 0 || suite_index >= g_polycall_test_registry.suite_count) {
    polycall_test_print(POLYCALL_COLOR_RED, "ERROR: Invalid suite index: %d\n",
                        suite_index);
    return POLYCALL_TEST_STATUS_ERROR;
  }

  polycall_test_suite_t *suite = &g_polycall_test_registry.suites[suite_index];

  // Validate test index
  if (test_index < 0 || test_index >= suite->test_count) {
    polycall_test_print(POLYCALL_COLOR_RED, "ERROR: Invalid test index: %d\n",
                        test_index);
    return POLYCALL_TEST_STATUS_ERROR;
  }

  polycall_test_case_t *test = &suite->tests[test_index];

  // Set up test context
  strncpy(g_polycall_test_context.current_suite_name, suite->name,
          POLYCALL_TEST_NAME_MAX_LENGTH - 1);
  g_polycall_test_context
      .current_suite_name[POLYCALL_TEST_NAME_MAX_LENGTH - 1] = '\0';

  strncpy(g_polycall_test_context.current_test_name, test->name,
          POLYCALL_TEST_NAME_MAX_LENGTH - 1);
  g_polycall_test_context.current_test_name[POLYCALL_TEST_NAME_MAX_LENGTH - 1] =
      '\0';

  g_polycall_test_context.has_error = false;
  g_polycall_test_context.error_message[0] = '\0';
  g_polycall_test_context.error_line = 0;
  g_polycall_test_context.error_file[0] = '\0';
  g_polycall_test_context.fixture_context = NULL;

  // Print test header
  polycall_test_print_header(suite->name, test->name);

  // Reset test status
  test->status = POLYCALL_TEST_STATUS_PASSED;
  test->message[0] = '\0';
  test->start_time = clock();

  // Run test
  if (test->test_fn) {
    test->test_fn(suite->global_context);

    // Check for errors
    if (g_polycall_test_context.has_error) {
      test->status = POLYCALL_TEST_STATUS_FAILED;
      strncpy(test->message, g_polycall_test_context.error_message,
              POLYCALL_TEST_MESSAGE_MAX_LENGTH - 1);
      test->message[POLYCALL_TEST_MESSAGE_MAX_LENGTH - 1] = '\0';
    }
  } else {
    test->status = POLYCALL_TEST_STATUS_ERROR;
    strncpy(test->message, "Test function is NULL",
            POLYCALL_TEST_MESSAGE_MAX_LENGTH - 1);
    test->message[POLYCALL_TEST_MESSAGE_MAX_LENGTH - 1] = '\0';
  }

  // Record end time
  test->end_time = clock();

  // Print test footer
  polycall_test_print_footer(suite->name, test->name, test->status,
                             test->message);

  return test->status;
}

void polycall_test_print_summary(void) {
  // Calculate total run time
  double time_taken =
      (double)(g_polycall_test_registry.total_stats.end_time -
               g_polycall_test_registry.total_stats.start_time) /
      CLOCKS_PER_SEC;

  // Print summary header
  polycall_test_print(POLYCALL_COLOR_BLUE, "\n====== TEST SUMMARY ======\n");

  // Print statistics
  polycall_test_print(POLYCALL_COLOR_BLUE, "Tests run:    %d\n",
                      g_polycall_test_registry.total_stats.tests_run);
  polycall_test_print(POLYCALL_COLOR_GREEN, "Tests passed: %d\n",
                      g_polycall_test_registry.total_stats.tests_passed);
  polycall_test_print(POLYCALL_COLOR_RED, "Tests failed: %d\n",
                      g_polycall_test_registry.total_stats.tests_failed);
  polycall_test_print(POLYCALL_COLOR_YELLOW, "Tests skipped: %d\n",
                      g_polycall_test_registry.total_stats.tests_skipped);
  polycall_test_print(POLYCALL_COLOR_RED, "Tests errored: %d\n",
                      g_polycall_test_registry.total_stats.tests_errored);
  polycall_test_print(POLYCALL_COLOR_BLUE, "Time taken:   %.3f seconds\n",
                      time_taken);

  // Print overall result
  int total_failures = g_polycall_test_registry.total_stats.tests_failed +
                       g_polycall_test_registry.total_stats.tests_errored;

  if (total_failures == 0) {
    polycall_test_print(POLYCALL_COLOR_GREEN, "\nALL TESTS PASSED!\n\n");
  } else {
    polycall_test_print(POLYCALL_COLOR_RED, "\n%d TEST(S) FAILED!\n\n",
                        total_failures);
  }
}

bool polycall_test_generate_xml_report(const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    polycall_test_print(POLYCALL_COLOR_RED,
                        "ERROR: Could not open XML report file: %s\n",
                        filename);
    return false;
  }

  // Calculate total run time
  double time_taken =
      (double)(g_polycall_test_registry.total_stats.end_time -
               g_polycall_test_registry.total_stats.start_time) /
      CLOCKS_PER_SEC;

  // Write XML header
  fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(file,
          "<testsuites tests=\"%d\" failures=\"%d\" errors=\"%d\" "
          "skipped=\"%d\" time=\"%.3f\">\n",
          g_polycall_test_registry.total_stats.tests_run,
          g_polycall_test_registry.total_stats.tests_failed,
          g_polycall_test_registry.total_stats.tests_errored,
          g_polycall_test_registry.total_stats.tests_skipped, time_taken);

  // Write each test suite
  for (int i = 0; i < g_polycall_test_registry.suite_count; i++) {
    polycall_test_suite_t *suite = &g_polycall_test_registry.suites[i];

    // Calculate suite run time
    double suite_time =
        (double)(suite->stats.end_time - suite->stats.start_time) /
        CLOCKS_PER_SEC;

    // Write suite header
    fprintf(file,
            "  <testsuite name=\"%s\" tests=\"%d\" failures=\"%d\" "
            "errors=\"%d\" skipped=\"%d\" time=\"%.3f\">\n",
            suite->name, suite->stats.tests_run, suite->stats.tests_failed,
            suite->stats.tests_errored, suite->stats.tests_skipped, suite_time);

    // Write each test case
    for (int j = 0; j < suite->test_count; j++) {
      polycall_test_case_t *test = &suite->tests[j];

      // Calculate test run time
      double test_time =
          (double)(test->end_time - test->start_time) / CLOCKS_PER_SEC;

      // Write test case
      fprintf(file, "    <testcase name=\"%s\" classname=\"%s\" time=\"%.3f\"",
              test->name, suite->name, test_time);

      // Write test result
      switch (test->status) {
      case POLYCALL_TEST_STATUS_PASSED:
        fprintf(file, "/>\n");
        break;
      case POLYCALL_TEST_STATUS_FAILED:
        fprintf(file, ">\n");
        fprintf(file, "      <failure message=\"%s\"/>\n", test->message);
        fprintf(file, "    </testcase>\n");
        break;
      case POLYCALL_TEST_STATUS_SKIPPED:
        fprintf(file, ">\n");
        fprintf(file, "      <skipped message=\"%s\"/>\n", test->message);
        fprintf(file, "    </testcase>\n");
        break;
      case POLYCALL_TEST_STATUS_ERROR:
        fprintf(file, ">\n");
        fprintf(file, "      <error message=\"%s\"/>\n", test->message);
        fprintf(file, "    </testcase>\n");
        break;
      }
    }

    // Write suite footer
    fprintf(file, "  </testsuite>\n");
  }

  // Write XML footer
  fprintf(file, "</testsuites>\n");

  fclose(file);

  polycall_test_print(POLYCALL_COLOR_BLUE, "XML report written to %s\n",
                      filename);

  return true;
}

void polycall_test_set_error(const char *file, int line, const char *format,
                             ...) {
  g_polycall_test_context.has_error = true;

  // Set error location
  strncpy(g_polycall_test_context.error_file, file,
          POLYCALL_TEST_NAME_MAX_LENGTH - 1);
  g_polycall_test_context.error_file[POLYCALL_TEST_NAME_MAX_LENGTH - 1] = '\0';
  g_polycall_test_context.error_line = line;

  // Format error message
  va_list args;
  va_start(args, format);
  polycall_test_format_message(g_polycall_test_context.error_message,
                               POLYCALL_TEST_MESSAGE_MAX_LENGTH, format, args);
  va_end(args);

  // Print error message
  polycall_test_print(POLYCALL_COLOR_RED, "ERROR (%s:%d): %s\n", file, line,
                      g_polycall_test_context.error_message);
}

void polycall_test_skip(const char *file, int line, const char *format, ...) {
  polycall_test_suite_t *suite = NULL;
  polycall_test_case_t *test = NULL;

  // Find the current test
  for (int i = 0; i < g_polycall_test_registry.suite_count; i++) {
    if (strcmp(g_polycall_test_registry.suites[i].name,
               g_polycall_test_context.current_suite_name) == 0) {
      suite = &g_polycall_test_registry.suites[i];
      break;
    }
  }

  if (suite) {
    for (int i = 0; i < suite->test_count; i++) {
      if (strcmp(suite->tests[i].name,
                 g_polycall_test_context.current_test_name) == 0) {
        test = &suite->tests[i];
        break;
      }
    }
  }

  if (test) {
    // Format skip message
    va_list args;
    va_start(args, format);
    polycall_test_format_message(
        test->message, POLYCALL_TEST_MESSAGE_MAX_LENGTH, format, args);
    va_end(args);

    // Update test status
    test->status = POLYCALL_TEST_STATUS_SKIPPED;
    test->end_time = clock();

    // Print skip message
    polycall_test_print(POLYCALL_COLOR_YELLOW, "SKIPPED (%s:%d): %s\n", file,
                        line, test->message);
  }
}

bool polycall_test_assert_true(const char *file, int line, bool condition,
                               const char *format, ...) {
  if (condition) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line, "Assertion failed: %s", message);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: expected TRUE, got FALSE");
  }

  return false;
}

bool polycall_test_assert_false(const char *file, int line, bool condition,
                                const char *format, ...) {
  if (!condition) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line, "Assertion failed: %s", message);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: expected FALSE, got TRUE");
  }

  return false;
}

bool polycall_test_assert_null(const char *file, int line, const void *pointer,
                               const char *format, ...) {
  if (pointer == NULL) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line, "Assertion failed: %s", message);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: expected NULL, got %p", pointer);
  }

  return false;
}

bool polycall_test_assert_not_null(const char *file, int line,
                                   const void *pointer, const char *format,
                                   ...) {
  if (pointer != NULL) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line, "Assertion failed: %s", message);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: expected non-NULL, got NULL");
  }

  return false;
}

bool polycall_test_assert_int_equal(const char *file, int line,
                                    long long expected, long long actual,
                                    const char *format, ...) {
  if (expected == actual) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line,
                            "Assertion failed: %s (expected %lld, got %lld)",
                            message, expected, actual);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: expected %lld, got %lld",
                            expected, actual);
  }

  return false;
}

bool polycall_test_assert_int_not_equal(const char *file, int line,
                                        long long expected, long long actual,
                                        const char *format, ...) {
  if (expected != actual) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line,
                            "Assertion failed: %s (expected not %lld)", message,
                            expected);
  } else {
    polycall_test_set_error(file, line, "Assertion failed: expected not %lld",
                            expected);
  }

  return false;
}

bool polycall_test_assert_string_equal(const char *file, int line,
                                       const char *expected, const char *actual,
                                       const char *format, ...) {
  // Handle NULL pointers
  if (expected == NULL && actual == NULL) {
    return true;
  }

  if (expected == NULL || actual == NULL) {
    // Format error message
    char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

    va_list args;
    va_start(args, format);
    polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                                 format, args);
    va_end(args);

    // Set error
    if (message[0] != '\0') {
      polycall_test_set_error(
          file, line, "Assertion failed: %s (expected %s, got %s)", message,
          expected ? expected : "NULL", actual ? actual : "NULL");
    } else {
      polycall_test_set_error(
          file, line, "Assertion failed: expected %s, got %s",
          expected ? expected : "NULL", actual ? actual : "NULL");
    }

    return false;
  }

  if (strcmp(expected, actual) == 0) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(
        file, line, "Assertion failed: %s (expected \"%s\", got \"%s\")",
        message, expected, actual);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: expected \"%s\", got \"%s\"",
                            expected, actual);
  }

  return false;
}

bool polycall_test_assert_string_not_equal(const char *file, int line,
                                           const char *expected,
                                           const char *actual,
                                           const char *format, ...) {
  // Handle NULL pointers
  if (expected == NULL && actual == NULL) {
    // Format error message
    char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

    va_list args;
    va_start(args, format);
    polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                                 format, args);
    va_end(args);

    // Set error
    if (message[0] != '\0') {
      polycall_test_set_error(
          file, line, "Assertion failed: %s (both strings are NULL)", message);
    } else {
      polycall_test_set_error(file, line,
                              "Assertion failed: both strings are NULL");
    }

    return false;
  }

  if (expected == NULL || actual == NULL) {
    return true;
  }

  if (strcmp(expected, actual) != 0) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line,
                            "Assertion failed: %s (expected not \"%s\")",
                            message, expected);
  } else {
    polycall_test_set_error(file, line, "Assertion failed: expected not \"%s\"",
                            expected);
  }

  return false;
}

bool polycall_test_assert_memory_equal(const char *file, int line,
                                       const void *expected, const void *actual,
                                       size_t size, const char *format, ...) {
  // Handle NULL pointers
  if (expected == NULL && actual == NULL) {
    return true;
  }

  if (expected == NULL || actual == NULL ||
      memcmp(expected, actual, size) != 0) {
    // Format error message
    char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

    va_list args;
    va_start(args, format);
    polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                                 format, args);
    va_end(args);

    // Set error
    if (message[0] != '\0') {
      polycall_test_set_error(file, line,
                              "Assertion failed: %s (memory comparison failed)",
                              message);
    } else {
      polycall_test_set_error(file, line,
                              "Assertion failed: memory comparison failed");
    }

    return false;
  }

  return true;
}

bool polycall_test_assert_memory_not_equal(const char *file, int line,
                                           const void *expected,
                                           const void *actual, size_t size,
                                           const char *format, ...) {
  // Handle NULL pointers
  if (expected == NULL && actual == NULL) {
    // Format error message
    char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

    va_list args;
    va_start(args, format);
    polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                                 format, args);
    va_end(args);

    // Set error
    if (message[0] != '\0') {
      polycall_test_set_error(
          file, line, "Assertion failed: %s (both pointers are NULL)", message);
    } else {
      polycall_test_set_error(file, line,
                              "Assertion failed: both pointers are NULL");
    }

    return false;
  }

  if (expected == NULL || actual == NULL) {
    return true;
  }

  if (memcmp(expected, actual, size) != 0) {
    return true;
  }

  // Format error message
  char message[POLYCALL_TEST_MESSAGE_MAX_LENGTH];

  va_list args;
  va_start(args, format);
  polycall_test_format_message(message, POLYCALL_TEST_MESSAGE_MAX_LENGTH,
                               format, args);
  va_end(args);

  // Set error
  if (message[0] != '\0') {
    polycall_test_set_error(file, line,
                            "Assertion failed: %s (memory comparison matched)",
                            message);
  } else {
    polycall_test_set_error(file, line,
                            "Assertion failed: memory comparison matched");
  }

  return false;
}