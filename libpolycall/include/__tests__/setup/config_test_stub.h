/**
 * @file config_test_stub.h
 * @brief Test stub header for config component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_CONFIG_TEST_STUB_H
#define POLYCALL_CONFIG_TEST_STUB_H

#include "polycall/core/config/polycall_config.h"

/**
 * @brief Initialize config test stubs
 * 
 * @return int 0 on success, non-zero on failure
 */
int polycall_config_init_test_stubs(void);

/**
 * @brief Clean up config test stubs
 */
void polycall_config_cleanup_test_stubs(void);

#endif /* POLYCALL_CONFIG_TEST_STUB_H */
