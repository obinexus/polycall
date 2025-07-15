/**
 * @file auth_test_stub.h
 * @brief Test stub header for auth component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_AUTH_TEST_STUB_H
#define POLYCALL_AUTH_TEST_STUB_H

#include "polycall/core/auth/polycall_auth.h"

/**
 * @brief Initialize auth test stubs
 *
 * @return int 0 on success, non-zero on failure
 */
int polycall_auth_init_test_stubs(void);

/**
 * @brief Clean up auth test stubs
 */
void polycall_auth_cleanup_test_stubs(void);

#endif /* POLYCALL_AUTH_TEST_STUB_H */
