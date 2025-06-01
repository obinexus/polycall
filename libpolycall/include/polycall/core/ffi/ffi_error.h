/**
 * @file ffi_error.h
 * @brief Error definitions for ffi module
 */

#ifndef POLYCALL_FFI_ERROR_H
#define POLYCALL_FFI_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ffi error codes
 */
typedef enum ffi_error {
    FFI_ERROR_SUCCESS = 0,
    FFI_ERROR_INVALID_PARAMETERS = 1,
    FFI_ERROR_OUT_OF_MEMORY = 2,
    FFI_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} ffi_error_t;

/**
 * Get error message for ffi error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* ffi_get_error_message(ffi_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_ERROR_H */
