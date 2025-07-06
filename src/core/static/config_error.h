/**
 * @file config_error.h
 * @brief Error definitions for config module
 */

#ifndef POLYCALL_CONFIG_ERROR_H
#define POLYCALL_CONFIG_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * config error codes
 */
typedef enum config_error {
  CONFIG_ERROR_SUCCESS = 0,
  CONFIG_ERROR_INVALID_PARAMETERS = 1,
  CONFIG_ERROR_OUT_OF_MEMORY = 2,
  CONFIG_ERROR_NOT_INITIALIZED = 3,
  // Add component-specific error codes here
} config_error_t;

/**
 * Get error message for config error code
 *
 * @param error Error code
 * @return const char* Error message
 */
const char *config_get_error_message(config_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_ERROR_H */
