/**
 * @file telemetry_test_stub.h
 * @brief Test stub header for telemetry component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_TELEMETRY_TEST_STUB_H
#define POLYCALL_TELEMETRY_TEST_STUB_H

#include "polycall/core/telemetry/polycall_telemetry.h"

/**
 * @brief Initialize telemetry test stubs
 *
 * @return int 0 on success, non-zero on failure
 */
int polycall_telemetry_init_test_stubs(void);

/**
 * @brief Clean up telemetry test stubs
 */
void polycall_telemetry_cleanup_test_stubs(void);

#endif /* POLYCALL_TELEMETRY_TEST_STUB_H */
