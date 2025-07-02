/**
 * @file auth_error.h
 * @brief Error definitions for auth module
 */

#ifndef POLYCALL_AUTH_ERROR_H
#define POLYCALL_AUTH_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * auth error codes
 */
typedef enum auth_error {
    AUTH_ERROR_SUCCESS = 0,
    AUTH_ERROR_INVALID_PARAMETERS = 1,
    AUTH_ERROR_OUT_OF_MEMORY = 2,
    AUTH_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} auth_error_t;

/**
 * Get error message for auth error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* auth_get_error_message(auth_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_AUTH_ERROR_H */
