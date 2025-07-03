/**
 * @file polycall_core.h
 * @brief Core module for LibPolyCall implementing the Program-First approach
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#ifndef POLYCALL_CORE_H
#define POLYCALL_CORE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef POLYCALL_CORE_ERROR_ENUM_DEFINED
#define POLYCALL_CORE_ERROR_ENUM_DEFINED
/**
 * @brief Core API error codes
 */
typedef enum {
    /* Success code */
    POLYCALL_CORE_SUCCESS = 0,
    
    /* General errors */
    POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
    POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
    POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
    POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
    POLYCALL_CORE_ERROR_INVALID_STATE,
    POLYCALL_CORE_ERROR_NOT_INITIALIZED,
    POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
    
/* polycall_core_error_t is defined above with include guard */
    POLYCALL_LOG_ERROR,
    POLYCALL_LOG_FATAL
} polycall_log_level_t;

/**
 * @brief Core API error codes
 */
typedef enum {
    /* Success code */
    POLYCALL_CORE_SUCCESS = 0,
    
    /* General errors */
    POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
    POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
    POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
    POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
    POLYCALL_CORE_ERROR_INVALID_STATE,
    POLYCALL_CORE_ERROR_NOT_INITIALIZED,
    POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
    
    /* Resource errors */
    POLYCALL_CORE_ERROR_NOT_FOUND,
    POLYCALL_CORE_ERROR_ALREADY_EXISTS,
    POLYCALL_CORE_ERROR_RESOURCE_EXISTS,
    POLYCALL_CORE_ERROR_UNAVAILABLE,
    POLYCALL_CORE_ERROR_ACCESS_DENIED,
    POLYCALL_CORE_ERROR_TIMEOUT,
    
    /* Operational errors */
    POLYCALL_CORE_ERROR_NOT_IMPLEMENTED,
    POLYCALL_CORE_ERROR_CANCELED,
    
    /* I/O errors */
    POLYCALL_CORE_ERROR_INTERNAL,
    POLYCALL_CORE_ERROR_NETWORK,
    POLYCALL_CORE_ERROR_IO,
    
    /* End of enumeration */
    POLYCALL_CORE_ERROR_COUNT
} polycall_core_error_t;

/**
 * @brief Configuration flags for the core module
 */
typedef enum {
    POLYCALL_CORE_FLAG_NONE = 0,
    POLYCALL_CORE_FLAG_STRICT_MODE = (1 << 0),
    POLYCALL_CORE_FLAG_DEBUG_MODE = (1 << 1),
    POLYCALL_CORE_FLAG_SECURE_MODE = (1 << 2),
    POLYCALL_CORE_FLAG_TRACE_MODE = (1 << 3)
} polycall_core_flags_t;

/**
 * @brief Initialize the core context
 *
 * This function must be called before any other polycall functions.
 * It initializes the core subsystems and allocates necessary resources.
 *
 * @param ctx Pointer to receive core context
 * @param flags Configuration flags
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_core_init(
    polycall_core_context_t** ctx,
    polycall_core_flags_t flags
);

/**
 * @brief Clean up and release resources associated with the core context
 *
 * @param ctx Core context to clean up
 */
void polycall_core_cleanup(polycall_core_context_t* ctx);

/**
 * @brief Set a core callback function
 *
 * @param ctx Core context
 * @param callback_type Type of callback to set
 * @param callback_fn Callback function pointer
 * @param user_data User data to pass to callback
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_core_set_callback(
    polycall_core_context_t* ctx,
    const char* callback_type,
    void* callback_fn,
    void* user_data
);

/**
 * @brief Allocate memory from the core context
 *
 * @param ctx Core context
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* polycall_core_malloc(polycall_core_context_t* ctx, size_t size);

/**
 * @brief Free memory allocated by polycall_core_malloc
 *
 * @param ctx Core context
 * @param ptr Pointer to memory to free
 */
void polycall_core_free(polycall_core_context_t* ctx, void* ptr);

/**
 * @brief Set an error in the core context
 *
 * @param ctx Core context
 * @param error Error code
 * @param message Error message
 * @return Error code passed in
 */
polycall_core_error_t polycall_core_set_error(
    polycall_core_context_t* ctx,
    polycall_core_error_t error,
    const char* message
);

/**
 * @brief Get the last error from the core context
 *
 * @param ctx Core context
 * @param message Pointer to receive error message (can be NULL)
 * @return Last error code
 */
polycall_core_error_t polycall_core_get_last_error(
    polycall_core_context_t* ctx,
    const char** message
);

/**
 * @brief Get the core context version
 *
 * @return Version string
 */
const char* polycall_core_get_version(void);

/**
 * @brief Get user data from core context
 *
 * @param ctx Core context
 * @return User data pointer
 */
void* polycall_core_get_user_data(polycall_core_context_t* ctx);

/**
 * @brief Set user data in core context
 *
 * @param ctx Core context
 * @param user_data User data pointer
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_core_set_user_data(
    polycall_core_context_t* ctx,
    void* user_data
);

/**
 * @brief Log a message using the core logging system
 *
 * @param ctx Core context
 * @param level Log level
 * @param format Format string
 * @param ... Additional arguments for the format string
 */
void polycall_core_log(
    polycall_core_context_t* ctx,
    polycall_log_level_t level,
    const char* format,
    ...
);

/**
 * @brief Convenience macro for logging
 */
#define POLYCALL_LOG(ctx, level, format, ...) \
    polycall_core_log(ctx, level, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CORE_H */

