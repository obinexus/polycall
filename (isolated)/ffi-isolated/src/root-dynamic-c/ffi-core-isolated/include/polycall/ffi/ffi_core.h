#ifndef POLYCALL_FFI_CORE_H
#define POLYCALL_FFI_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
    POLYCALL_FFI_SUCCESS = 0,
    POLYCALL_FFI_ERROR_NOT_INITIALIZED,
    POLYCALL_FFI_ERROR_ALREADY_INITIALIZED,
    POLYCALL_FFI_ERROR_INVALID_PARAMETER,
    POLYCALL_FFI_ERROR_BRIDGE_LIMIT_EXCEEDED
} polycall_ffi_error_t;

// FFI Core API (minimal, star topology coordinator)
polycall_ffi_error_t polycall_ffi_core_init(void);
polycall_ffi_error_t polycall_ffi_core_register_bridge(const char* bridge_name);
polycall_ffi_error_t polycall_ffi_core_get_bridge_count(size_t* count);
polycall_ffi_error_t polycall_ffi_core_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_CORE_H */
