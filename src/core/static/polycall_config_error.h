/**
 * @file polycall_config_error.h
 * @brief Error handling for config module
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#ifndef POLYCALL_CONFIG_ERROR_H
#define POLYCALL_CONFIG_ERROR_H

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_hierarchical_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Config module error codes
 */
typedef enum {
  POLYCALL_CONFIG_SUCCESS = 0,
  POLYCALL_CONFIG_ERROR_INITIALIZATION_FAILED,
  POLYCALL_CONFIG_ERROR_INVALID_PARAMETERS,
  POLYCALL_CONFIG_ERROR_INVALID_STATE,
  POLYCALL_CONFIG_ERROR_NOT_INITIALIZED,
  POLYCALL_CONFIG_ERROR_ALREADY_INITIALIZED,
  POLYCALL_CONFIG_ERROR_UNSUPPORTED_OPERATION,
  POLYCALL_CONFIG_ERROR_RESOURCE_ALLOCATION,
  POLYCALL_CONFIG_ERROR_TIMEOUT,
  POLYCALL_CONFIG_ERROR_PERMISSION_DENIED,
  /* Component-specific error codes here */
  POLYCALL_CONFIG_ERROR_CUSTOM_START = 1000
} polycall_config_error_t;

/**
 * @brief Initialize config error subsystem
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_config_error_init(
    polycall_core_context_t *core_ctx,
    polycall_hierarchical_error_context_t *hier_error_ctx);

/**
 * @brief Clean up config error subsystem
 *
 * @param core_ctx Core context
 */
void polycall_config_error_cleanup(polycall_core_context_t *core_ctx);

/**
 * @brief Get last config error
 *
 * @param core_ctx Core context
 * @param error_record Pointer to receive error record
 * @return true if error was retrieved, false otherwise
 */
bool polycall_config_error_get_last(polycall_core_context_t *core_ctx,
                                    polycall_error_record_t *error_record);

/**
 * @brief Set config error
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @param code Error code
 * @param severity Error severity
 * @param file Source file
 * @param line Source line
 * @param message Error message format
 * @param ... Format arguments
 */
void polycall_config_error_set(
    polycall_core_context_t *core_ctx,
    polycall_hierarchical_error_context_t *hier_error_ctx,
    polycall_config_error_t code, polycall_error_severity_t severity,
    const char *file, int line, const char *message, ...);

/**
 * @brief Clear config errors
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_config_error_clear(
    polycall_core_context_t *core_ctx,
    polycall_hierarchical_error_context_t *hier_error_ctx);

/**
 * @brief Register config error handler
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @param handler Error handler function
 * @param user_data User data for handler
 * @return Error code
 */
polycall_core_error_t polycall_config_error_register_handler(
    polycall_core_context_t *core_ctx,
    polycall_hierarchical_error_context_t *hier_error_ctx,
    polycall_hierarchical_error_handler_fn handler, void *user_data);

/**
 * @brief Convert config error code to string
 *
 * @param error Error code
 * @return Error string
 */
const char *polycall_config_error_to_string(polycall_config_error_t error);

/**
 * @brief Macro for setting config error with file and line info
 */
#define POLYCALL_CONFIG_ERROR_SET(ctx, hier_ctx, code, severity, message, ...) \
  polycall_config_error_set(ctx, hier_ctx, code, severity, __FILE__, __LINE__, \
                            message, ##__VA_ARGS__)

/**
 * @brief Macro for checking config error condition
 */
#define POLYCALL_CONFIG_CHECK_ERROR(ctx, hier_ctx, expr, code, severity,       \
                                    message, ...)                              \
  do {                                                                         \
    if (!(expr)) {                                                             \
      POLYCALL_CONFIG_ERROR_SET(ctx, hier_ctx, code, severity, message,        \
                                ##__VA_ARGS__);                                \
      return code;                                                             \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_ERROR_H */
