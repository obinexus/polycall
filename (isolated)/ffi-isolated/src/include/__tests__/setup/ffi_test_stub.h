/**
 * @file ffi_test_stub.h
 * @brief Test stub header for ffi component
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_FFI_TEST_STUB_H
#define POLYCALL_FFI_TEST_STUB_H

#include "polycall/core/ffi/polycall_ffi.h"

/**
 * @brief Initialize ffi test stubs
 * 
 * @return int 0 on success, non-zero on failure
 */
int polycall_ffi_init_test_stubs(void);

/**
 * @brief Clean up ffi test stubs
 */
void polycall_ffi_cleanup_test_stubs(void);

#endif /* POLYCALL_FFI_TEST_STUB_H */
