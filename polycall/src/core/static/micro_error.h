/**
 * @file micro_error.h
 * @brief Error definitions for micro module
 */

#ifndef POLYCALL_MICRO_ERROR_H
#define POLYCALL_MICRO_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * micro error codes
 */
typedef enum micro_error {
    MICRO_ERROR_SUCCESS = 0,
    MICRO_ERROR_INVALID_PARAMETERS = 1,
    MICRO_ERROR_OUT_OF_MEMORY = 2,
    MICRO_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} micro_error_t;

/**
 * Get error message for micro error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* micro_get_error_message(micro_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_MICRO_ERROR_H */
