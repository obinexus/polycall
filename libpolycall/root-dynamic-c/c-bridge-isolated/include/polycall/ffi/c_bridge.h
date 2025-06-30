/**
 * @file c_bridge.h
 * @brief Isolated C FFI Bridge Header
 * 
 * SINPHASÉ COMPLIANT DESIGN:
 * - No external dependencies (except standard library)
 * - Single responsibility: C function bridging
 * - Bounded complexity: Maximum 5 functions
 * - No circular dependencies
 */

#ifndef POLYCALL_FFI_C_BRIDGE_H
#define POLYCALL_FFI_C_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// Error codes for C bridge (isolated)
typedef enum {
    POLYCALL_FFI_SUCCESS = 0,
    POLYCALL_FFI_ERROR_NOT_INITIALIZED,
    POLYCALL_FFI_ERROR_ALREADY_INITIALIZED,
    POLYCALL_FFI_ERROR_INVALID_PARAMETER,
    POLYCALL_FFI_ERROR_CALL_FAILED
} polycall_ffi_error_t;

// C Bridge API (minimal, bounded)
polycall_ffi_error_t polycall_c_bridge_init(void);
polycall_ffi_error_t polycall_c_bridge_cleanup(void);
polycall_ffi_error_t polycall_c_bridge_call(void* function_ptr, void* args, void* result);
polycall_ffi_error_t polycall_c_bridge_get_stats(size_t* call_count);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_C_BRIDGE_H */
