/**
 * @file polycall_micro_error.h
 * @brief Error handling for micro module
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#ifndef POLYCALL_MICRO_ERROR_H
#define POLYCALL_MICRO_ERROR_H

#include "polycall/core/polycall/polycall_hierarchical_error.h"
#include "polycall/core/polycall/polycall_error.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Micro module error codes
 */
typedef enum {
    POLYCALL_MICRO_SUCCESS = 0,
    POLYCALL_MICRO_ERROR_INITIALIZATION_FAILED,
    POLYCALL_MICRO_ERROR_INVALID_PARAMETERS,
    POLYCALL_MICRO_ERROR_INVALID_STATE,
    POLYCALL_MICRO_ERROR_NOT_INITIALIZED,
    POLYCALL_MICRO_ERROR_ALREADY_INITIALIZED,
    POLYCALL_MICRO_ERROR_UNSUPPORTED_OPERATION,
    POLYCALL_MICRO_ERROR_RESOURCE_ALLOCATION,
    POLYCALL_MICRO_ERROR_TIMEOUT,
    POLYCALL_MICRO_ERROR_PERMISSION_DENIED,
    /* Component-specific error codes here */
    POLYCALL_MICRO_ERROR_CUSTOM_START = 1000
} polycall_micro_error_t;

/**
 * @brief Initialize micro error subsystem
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_micro_error_init(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx
);

/**
 * @brief Clean up micro error subsystem
 *
 * @param core_ctx Core context
 */
void polycall_micro_error_cleanup(
    polycall_core_context_t* core_ctx
);

/**
 * @brief Get last micro error
 *
 * @param core_ctx Core context
 * @param error_record Pointer to receive error record
 * @return true if error was retrieved, false otherwise
 */
bool polycall_micro_error_get_last(
    polycall_core_context_t* core_ctx,
    polycall_error_record_t* error_record
);

/**
 * @brief Set micro error
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
void polycall_micro_error_set(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx,
    polycall_micro_error_t code,
    polycall_error_severity_t severity,
    const char* file,
    int line,
    const char* message,
    ...
);

/**
 * @brief Clear micro errors
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_micro_error_clear(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx
);

/**
 * @brief Register micro error handler
 *
 * @param core_ctx Core context
 * @param hier_error_ctx Hierarchical error context
 * @param handler Error handler function
 * @param user_data User data for handler
 * @return Error code
 */
polycall_core_error_t polycall_micro_error_register_handler(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx,
    polycall_hierarchical_error_handler_fn handler,
    void* user_data
);

/**
 * @brief Convert micro error code to string
 *
 * @param error Error code
 * @return Error string
 */
const char* polycall_micro_error_to_string(
    polycall_micro_error_t error
);

/**
 * @brief Macro for setting micro error with file and line info
 */
#define POLYCALL_MICRO_ERROR_SET(ctx, hier_ctx, code, severity, message, ...) \
    polycall_micro_error_set(ctx, hier_ctx, code, severity, __FILE__, __LINE__, message, ##__VA_ARGS__)

/**
 * @brief Macro for checking micro error condition
 */
#define POLYCALL_MICRO_CHECK_ERROR(ctx, hier_ctx, expr, code, severity, message, ...) \
    do { \
        if (!(expr)) { \
            POLYCALL_MICRO_ERROR_SET(ctx, hier_ctx, code, severity, message, ##__VA_ARGS__); \
            return code; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_MICRO_ERROR_H */
