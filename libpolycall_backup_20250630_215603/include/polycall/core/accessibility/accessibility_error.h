/**
 * @file accessibility_error.h
 * @brief Error definitions for accessibility module
 */

#ifndef POLYCALL_ACCESSIBILITY_ERROR_H
#define POLYCALL_ACCESSIBILITY_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * accessibility error codes
 */
typedef enum accessibility_error {
    ACCESSIBILITY_ERROR_SUCCESS = 0,
    ACCESSIBILITY_ERROR_INVALID_PARAMETERS = 1,
    ACCESSIBILITY_ERROR_OUT_OF_MEMORY = 2,
    ACCESSIBILITY_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} accessibility_error_t;

/**
 * Get error message for accessibility error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* accessibility_get_error_message(accessibility_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_ACCESSIBILITY_ERROR_H */
