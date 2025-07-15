/**
 * @file accessibility_test_stub.h
 * @brief Test stub header for accessibility component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_ACCESSIBILITY_TEST_STUB_H
#define POLYCALL_ACCESSIBILITY_TEST_STUB_H

#include "polycall/core/accessibility/polycall_accessibility.h"

/**
 * @brief Initialize accessibility test stubs
 *
 * @return int 0 on success, non-zero on failure
 */
int polycall_accessibility_init_test_stubs(void);

/**
 * @brief Clean up accessibility test stubs
 */
void polycall_accessibility_cleanup_test_stubs(void);

#endif /* POLYCALL_ACCESSIBILITY_TEST_STUB_H */
