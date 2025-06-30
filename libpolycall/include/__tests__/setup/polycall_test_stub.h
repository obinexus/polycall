/**
 * @file polycall_test_stub.h
 * @brief Test stub header for polycall component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_POLYCALL_TEST_STUB_H
#define POLYCALL_POLYCALL_TEST_STUB_H

#include "polycall/core/polycall/polycall_polycall.h"

/**
 * @brief Initialize polycall test stubs
 * 
 * @return int 0 on success, non-zero on failure
 */
int polycall_polycall_init_test_stubs(void);

/**
 * @brief Clean up polycall test stubs
 */
void polycall_polycall_cleanup_test_stubs(void);

#endif /* POLYCALL_POLYCALL_TEST_STUB_H */
