/**
 * @file protocol_error.h
 * @brief Error definitions for protocol module
 */

#ifndef POLYCALL_PROTOCOL_ERROR_H
#define POLYCALL_PROTOCOL_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * protocol error codes
 */
typedef enum protocol_error {
  PROTOCOL_ERROR_SUCCESS = 0,
  PROTOCOL_ERROR_INVALID_PARAMETERS = 1,
  PROTOCOL_ERROR_OUT_OF_MEMORY = 2,
  PROTOCOL_ERROR_NOT_INITIALIZED = 3,
  // Add component-specific error codes here
} protocol_error_t;

/**
 * Get error message for protocol error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char *protocol_get_error_message(protocol_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_PROTOCOL_ERROR_H */
