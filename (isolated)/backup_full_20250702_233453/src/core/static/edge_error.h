/**
 * @file edge_error.h
 * @brief Error definitions for edge module
 */

#ifndef POLYCALL_EDGE_ERROR_H
#define POLYCALL_EDGE_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * edge error codes
 */
typedef enum edge_error {
    EDGE_ERROR_SUCCESS = 0,
    EDGE_ERROR_INVALID_PARAMETERS = 1,
    EDGE_ERROR_OUT_OF_MEMORY = 2,
    EDGE_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} edge_error_t;

/**
 * Get error message for edge error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* edge_get_error_message(edge_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_EDGE_ERROR_H */
