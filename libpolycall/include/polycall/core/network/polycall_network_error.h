/**
 * @file polycall_network_error.h
 * @brief Error handling for network module
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#ifndef POLYCALL_NETWORK_ERROR_H
#define POLYCALL_NETWORK_ERROR_H

#include "polycall/core/polycall/polycall_hierarchical_error.h"
#include "polycall/core/polycall/polycall_error.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network module error codes
 */
typedef enum {
    POLYCALL_NETWORK_SUCCESS = 0,
    POLYCALL_NETWORK_ERROR_INITIALIZATION_FAILED,
    POLYCALL_NETWORK_ERROR_INVALID_PARAMETERS,
    POLYCALL_NETWORK_ERROR_INVALID_STATE,
    POLYCALL_NETWORK_ERROR_NOT_INITIALIZED,
    POLYCALL_NETWORK_ERROR_ALREADY_INITIALIZED,
    POLYCALL_NETWORK_ERROR_UNSUPPORTED_OPERATION,
    POLYCALL_NETWORK_ERROR_RESOURCE_ALLOCATION,
    POLYCALL_NETWORK_ERROR_TIMEOUT,
    POLYCALL_NETWORK_ERROR_PERMISSION_DENIED,
    /* Component-specific error codes here */
    POLYCALL_NETWORK_ERROR_CUSTOM_START = 1000
} polycall_network_error_t;

/**
 * @brief Initialize network error subsystem
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_network_error_init(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx
);

/**
 * @brief Clean up network error subsystem
 *
 * @param core_ctx Core context
 */
void polycall_network_error_cleanup(
    polycall_core_context_t* core_ctx
);

/**
 * @brief Get last network error
 *
 * @param core_ctx Core context
 * @param error_record Pointer to receive error record
 * @return true if error was retrieved, false otherwise
 */
bool polycall_network_error_get_last(
    polycall_core_context_t* core_ctx,
    polycall_error_record_t* error_record
);

/**
 * @brief Set network error
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
void polycall_network_error_set(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx,
    polycall_network_error_t code,
    polycall_error_severity_t severity,
    const char* file,
    int line,
    const char* message,
    ...
);

/**
 * @brief Clear network errors
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_network_error_clear(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx
);

/**
 * @brief Register network error handler
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @param handler Error handler function
 * @param user_data User data for handler
 * @return Error code
 */
polycall_core_error_t polycall_network_error_register_handler(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx,
    polycall_hierarchical_error_handler_fn handler,
    void* user_data
);

/**
 * @brief Convert network error code to string
 *
 * @param error Error code
 * @return Error string
 */
const char* polycall_network_error_to_string(
    polycall_network_error_t error
);

/**
 * @brief Macro for setting network error with file and line info
 */
#define POLYCALL_NETWORK_ERROR_SET(ctx, hier_ctx, code, severity, message, ...) \
    polycall_network_error_set(ctx, hier_ctx, code, severity, __FILE__, __LINE__, message, ##__VA_ARGS__)

/**
 * @brief Macro for checking network error condition
 */
#define POLYCALL_NETWORK_CHECK_ERROR(ctx, hier_ctx, expr, code, severity, message, ...) \
    do { \
        if (!(expr)) { \
            POLYCALL_NETWORK_ERROR_SET(ctx, hier_ctx, code, severity, message, ##__VA_ARGS__); \
            return code; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_ERROR_H */
