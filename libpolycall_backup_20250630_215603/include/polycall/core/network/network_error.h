/**
 * @file network_error.h
 * @brief Error definitions for network module
 */

#ifndef POLYCALL_NETWORK_ERROR_H
#define POLYCALL_NETWORK_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * network error codes
 */
typedef enum network_error {
    NETWORK_ERROR_SUCCESS = 0,
    NETWORK_ERROR_INVALID_PARAMETERS = 1,
    NETWORK_ERROR_OUT_OF_MEMORY = 2,
    NETWORK_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} network_error_t;

/**
 * Get error message for network error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* network_get_error_message(network_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_ERROR_H */
