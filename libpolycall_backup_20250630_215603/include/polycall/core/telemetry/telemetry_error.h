/**
 * @file telemetry_error.h
 * @brief Error definitions for telemetry module
 */

#ifndef POLYCALL_TELEMETRY_ERROR_H
#define POLYCALL_TELEMETRY_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * telemetry error codes
 */
typedef enum telemetry_error {
    TELEMETRY_ERROR_SUCCESS = 0,
    TELEMETRY_ERROR_INVALID_PARAMETERS = 1,
    TELEMETRY_ERROR_OUT_OF_MEMORY = 2,
    TELEMETRY_ERROR_NOT_INITIALIZED = 3,
    // Add component-specific error codes here
} telemetry_error_t;

/**
 * Get error message for telemetry error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char* telemetry_get_error_message(telemetry_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_TELEMETRY_ERROR_H */
