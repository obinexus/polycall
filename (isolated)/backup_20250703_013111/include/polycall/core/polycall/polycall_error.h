/**
 * @file polycall_error.h
 * @brief Error handling module for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#ifndef POLYCALL_ERROR_H
#define POLYCALL_ERROR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations to avoid circular dependencies */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_error_record polycall_error_record_t;

/* Include core types after forward declarations */
#include "polycall/core/polycall/polycall_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error source module identifiers
 */
typedef enum {
    POLYCALL_ERROR_SOURCE_CORE = 0,
    POLYCALL_ERROR_SOURCE_MEMORY,
    POLYCALL_ERROR_SOURCE_CONTEXT,
    POLYCALL_ERROR_SOURCE_PROTOCOL,
    POLYCALL_ERROR_SOURCE_NETWORK,
    POLYCALL_ERROR_SOURCE_PARSER,
    POLYCALL_ERROR_SOURCE_MICRO,
    POLYCALL_ERROR_SOURCE_EDGE,
    POLYCALL_ERROR_SOURCE_CONFIG,
    POLYCALL_ERROR_SOURCE_AUTH = 5, /* Specific value as defined in polycall_auth_context.h */
    POLYCALL_ERROR_SOURCE_USER = 0x1000   /**< Start of user-defined sources */
} polycall_error_source_t;

/**
 * @brief Maximum error message length
 */
#define POLYCALL_ERROR_MAX_MESSAGE_LENGTH 256

/**
 * @brief Error severity levels
 */
typedef enum {
    POLYCALL_ERROR_SEVERITY_INFO = 0,
    POLYCALL_ERROR_SEVERITY_WARNING,
    POLYCALL_ERROR_SEVERITY_ERROR,
    POLYCALL_ERROR_SEVERITY_FATAL
} polycall_error_severity_t;

/**
 * @brief Error record structure
 */
struct polycall_error_record {
    polycall_error_source_t source;                         /**< Error source module */
    int32_t code;                                           /**< Error code */
    polycall_error_severity_t severity;                     /**< Error severity */
    char message[POLYCALL_ERROR_MAX_MESSAGE_LENGTH];        /**< Error message */
    const char* file;                                       /**< Source file name */
    int line;                                               /**< Source line number */
    uint64_t timestamp;                                     /**< Error timestamp */
};

/**
 * @brief Error callback function type
 */
typedef void (*polycall_error_callback_fn)(
    polycall_core_context_t* ctx,
    polycall_error_record_t* record,
    void* user_data
);

/**
 * @brief Public API error codes
 */
typedef enum {
    POLYCALL_OK = 0,
    POLYCALL_ERROR_INVALID_PARAMETERS,
    POLYCALL_ERROR_INITIALIZATION,
    POLYCALL_ERROR_OUT_OF_MEMORY,
    POLYCALL_ERROR_UNSUPPORTED,
    POLYCALL_ERROR_INVALID_STATE,
    POLYCALL_ERROR_NOT_FOUND,
    POLYCALL_ERROR_TIMEOUT,
    POLYCALL_ERROR_ACCESS_DENIED,
    POLYCALL_ERROR_NOT_IMPLEMENTED,
    POLYCALL_ERROR_INVALID_FORMAT,
    POLYCALL_ERROR_BUFFER_OVERFLOW,
    POLYCALL_ERROR_BUFFER_UNDERFLOW,
    POLYCALL_ERROR_IO,
    POLYCALL_ERROR_PROTOCOL,
    POLYCALL_ERROR_SECURITY,
    POLYCALL_ERROR_INTERNAL,
} polycall_status_t;

/**
 * @brief Initialize the error subsystem
 *
 * @param ctx Core context
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_error_init(polycall_core_context_t* ctx);

/**
 * @brief Clean up the error subsystem
 *
 * @param ctx Core context
 */
void polycall_error_cleanup(polycall_core_context_t* ctx);

/**
 * @brief Register an error callback function
 *
 * @param ctx Core context
 * @param callback Error callback function
 * @param user_data User data to pass to callback
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_error_register_callback(
    polycall_core_context_t* ctx,
    polycall_error_callback_fn callback,
    void* user_data
);

/**
 * @brief Set an error with full details
 *
 * @param ctx Core context
 * @param source Error source module
 * @param code Error code
 * @param severity Error severity level
 * @param file Source file name
 * @param line Source line number
 * @param message Error message format string
 * @param ... Arguments for format string
 * @return Error code passed in
 */
polycall_core_error_t polycall_error_set_full(
    polycall_core_context_t* ctx,
    polycall_error_source_t source,
    int32_t code,
    polycall_error_severity_t severity,
    const char* file,
    int line,
    const char* message,
    ...
);

/**
 * @brief Set an error with basic details
 *
 * @param ctx Core context
 * @param source Error source module
 * @param code Error code
 * @param message Error message format string
 * @param ... Arguments for format string
 * @return Error code passed in
 */
int32_t polycall_error_set(
    polycall_core_context_t* ctx,
    polycall_error_source_t source,
    int32_t code,
    const char* message,
    ...
);

/**
 * @brief Get the last error record
 *
 * @param ctx Core context
 * @param record Pointer to receive error record
 * @return true if an error record was retrieved, false otherwise
 */
bool polycall_error_get_last(
    polycall_core_context_t* ctx,
    polycall_error_record_t* record
);

/**
 * @brief Clear the last error
 *
 * @param ctx Core context
 */
void polycall_error_clear(polycall_core_context_t* ctx);

/**
 * @brief Check if an error has occurred
 *
 * @param ctx Core context
 * @return true if an error has occurred, false otherwise
 */
bool polycall_error_has_occurred(polycall_core_context_t* ctx);

/**
 * @brief Get the last error message
 *
 * @param ctx Core context
 * @return Error message string, or NULL if no error has occurred
 */
const char* polycall_error_get_message(polycall_core_context_t* ctx);

/**
 * @brief Get the last error code
 *
 * @param ctx Core context
 * @param source Pointer to receive error source (can be NULL)
 * @return Error code, or 0 if no error has occurred
 */
int32_t polycall_error_get_code(
    polycall_core_context_t* ctx,
    polycall_error_source_t* source
);

/**
 * @brief Create a formatted error message
 *
 * @param buffer Buffer to receive formatted message
 * @param size Buffer size
 * @param format Format string
 * @param ... Arguments for format string
 * @return Number of characters written to buffer (excluding null terminator)
 */
size_t polycall_error_format_message(
    char* buffer,
    size_t size,
    const char* format,
    ...
);

/**
 * @brief Convenience macro for setting an error with file and line info
 */
#define POLYCALL_ERROR_SET(ctx, source, code, severity, message, ...) \
    polycall_error_set_full(ctx, source, code, severity, __FILE__, __LINE__, message, ##__VA_ARGS__)

/**
 * @brief Convenience macro for checking an error and returning on failure
 */
#define POLYCALL_ERROR_CHECK_RETURN(ctx, source, expr, code, message, ...) \
    do { \
        if (!(expr)) { \
            POLYCALL_ERROR_SET(ctx, source, code, POLYCALL_ERROR_SEVERITY_ERROR, message, ##__VA_ARGS__); \
            return code; \
        } \
    } while (0)

/**
 * @brief Convenience macro for checking an error and going to a label on failure
 */
#define POLYCALL_ERROR_CHECK_GOTO(ctx, source, expr, code, label, message, ...) \
    do { \
        if (!(expr)) { \
            POLYCALL_ERROR_SET(ctx, source, code, POLYCALL_ERROR_SEVERITY_ERROR, message, ##__VA_ARGS__); \
            goto label; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_ERROR_H */