/**
 * @file edge_test_stub.h
 * @brief Test stub header for edge component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_EDGE_TEST_STUB_H
#define POLYCALL_EDGE_TEST_STUB_H

#include "polycall/core/edge/polycall_edge.h"

/**
 * @brief Initialize edge test stubs
 * 
 * @return int 0 on success, non-zero on failure
 */
int polycall_edge_init_test_stubs(void);

/**
 * @brief Clean up edge test stubs
 */
void polycall_edge_cleanup_test_stubs(void);

#endif /* POLYCALL_EDGE_TEST_STUB_H */
