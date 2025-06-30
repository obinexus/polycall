/**
 * @file test_stub_manager.h
 * @brief Test stub manager for LibPolyCall
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_TEST_STUB_MANAGER_H
#define POLYCALL_TEST_STUB_MANAGER_H

#include <stdbool.h>

/**
 * @brief Initialize test stubs for specified components
 * 
 * @param components Array of component names
 * @param count Number of components
 * @return bool true on success, false on failure
 */
bool test_stub_manager_init(const char** components, int count);

/**
 * @brief Clean up all initialized test stubs
 */
void test_stub_manager_cleanup(void);

/**
 * @brief Check if component has been initialized
 * 
 * @param component_name Component name to check
 * @return bool true if initialized, false otherwise
 */
bool test_stub_manager_is_initialized(const char* component_name);

#endif /* POLYCALL_TEST_STUB_MANAGER_H */
