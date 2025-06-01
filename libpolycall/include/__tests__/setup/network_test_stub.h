/**
 * @file network_test_stub.h
 * @brief Test stub header for network component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_NETWORK_TEST_STUB_H
#define POLYCALL_NETWORK_TEST_STUB_H

#include "polycall/core/network/polycall_network.h"

/**
 * @brief Initialize network test stubs
 * 
 * @return int 0 on success, non-zero on failure
 */
int polycall_network_init_test_stubs(void);

/**
 * @brief Clean up network test stubs
 */
void polycall_network_cleanup_test_stubs(void);

#endif /* POLYCALL_NETWORK_TEST_STUB_H */
