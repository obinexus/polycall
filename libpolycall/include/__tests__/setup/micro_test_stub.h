/**
 * @file micro_test_stub.h
 * @brief Test stub header for micro component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_MICRO_TEST_STUB_H
#define POLYCALL_MICRO_TEST_STUB_H

#include "polycall/core/micro/polycall_micro.h"

/**
 * @brief Initialize micro test stubs
 * 
 * @return int 0 on success, non-zero on failure
 */
int polycall_micro_init_test_stubs(void);

/**
 * @brief Clean up micro test stubs
 */
void polycall_micro_cleanup_test_stubs(void);

#endif /* POLYCALL_MICRO_TEST_STUB_H */
