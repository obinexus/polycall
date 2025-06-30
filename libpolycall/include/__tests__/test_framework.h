/**
 * @file test_framework.h
 * @brief Main test framework header for LibPolyCall
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_TEST_FRAMEWORK_H
#define POLYCALL_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

/*******************************************************************************
 * Test Framework Configuration
 ******************************************************************************/

/**
 * @brief Maximum length of test and suite names
 */
#define POLYCALL_TEST_NAME_MAX_LENGTH 128

/**
 * @brief Maximum number of test cases per suite
 */
#define POLYCALL_MAX_TESTS_PER_SUITE 256

/**
 * @brief Maximum number of test suites
 */
#define POLYCALL_MAX_TEST_SUITES 64

/**
 * @brief Maximum number of fixtures per suite
 */
#define POLYCALL_MAX_FIXTURES_PER_SUITE 32

/**
 * @brief Maximum length of test messages
 */
#define POLYCALL_TEST_MESSAGE_MAX_LENGTH 1024

/*******************************************************************************
 * AAA Pattern Macros
 ******************************************************************************/

/**
 * @brief Arrange test phase - setup the test environment
 */
#define POLYCALL_ARRANGE_PHASE(description)     do {         polycall_test_log_info("ARRANGE: %s", description);     } while (0)

/**
 * @brief Act test phase - perform the action being tested
 */
#define POLYCALL_ACT_PHASE(description)     do {         polycall_test_log_info("ACT: %s", description);     } while (0)

/**
 * @brief Assert test phase - verify the expected outcome
 */
#define POLYCALL_ASSERT_PHASE(description)     do {         polycall_test_log_info("ASSERT: %s", description);     } while (0)

/*******************************************************************************
 * Component Test Interface
 ******************************************************************************/

/**
 * @brief Component-specific stub initialization function type
 */
typedef int (*polycall_component_init_stub_fn)(void);

/**
 * @brief Component-specific stub cleanup function type
 */
typedef void (*polycall_component_cleanup_stub_fn)(void);

/**
 * @brief Initialize all test stubs for a specific test
 * 
 * @param components Array of component names to initialize
 * @param count Number of components
 * @return int 0 on success, non-zero on failure
 */
int polycall_test_init_component_stubs(const char** components, int count);

/**
 * @brief Clean up all test stubs
 */
void polycall_test_cleanup_component_stubs(void);

/**
 * @brief Log an informational message
 * 
 * @param format Format string
 * @param ... Format arguments
 */
void polycall_test_log_info(const char* format, ...);

/*******************************************************************************
 * Include detailed framework definitions
 ******************************************************************************/

// Include the rest of the test framework implementation
#include "polycall_test_framework.h"

#endif /* POLYCALL_TEST_FRAMEWORK_H */
