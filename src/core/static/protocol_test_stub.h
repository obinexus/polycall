/**
 * @file protocol_test_stub.h
 * @brief Test stub header for protocol component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_PROTOCOL_TEST_STUB_H
#define POLYCALL_PROTOCOL_TEST_STUB_H

#include "polycall/core/protocol/polycall_protocol.h"

/**
 * @brief Initialize protocol test stubs
 *
 * @return int 0 on success, non-zero on failure
 */
int polycall_protocol_init_test_stubs(void);

/**
 * @brief Clean up protocol test stubs
 */
void polycall_protocol_cleanup_test_stubs(void);

#endif /* POLYCALL_PROTOCOL_TEST_STUB_H */
