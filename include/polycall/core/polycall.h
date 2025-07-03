/**
 * @file polycall.h
 * @brief Main header file for LibPolyCall
 * @author OBINexus Engineering - Aegis Project Phase 2
 *
 * This is the primary header file for LibPolyCall, providing the public API
 * for cross-language polymorphic function calls. It includes all necessary
 * type definitions and function declarations for using LibPolyCall.
 */

#ifndef POLYCALL_POLYCALL_H
#define POLYCALL_POLYCALL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Version information */
#define POLYCALL_VERSION_MAJOR 2
#define POLYCALL_VERSION_MINOR 0
#define POLYCALL_VERSION_PATCH 0
#define POLYCALL_VERSION_STRING "2.0.0"

/* Core includes - order matters */
#include "polycall/core/polycall/polycall_types.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/polycall_core.h"

/* FFI subsystem */
#include "polycall/core/ffi/ffi_types.h"
#include "polycall/core/ffi/ffi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LibPolyCall
 *
 * This function must be called before any other LibPolyCall functions.
 * It initializes the runtime system and prepares all subsystems.
 *
 * @param flags Initialization flags
 * @return Error code
 */
polycall_core_error_t polycall_init(uint32_t flags);

/**
 * @brief Shutdown LibPolyCall
 *
 * Cleans up all resources and shuts down the runtime system.
 * After calling this function, polycall_init() must be called again
 * before using any LibPolyCall functions.
 */
void polycall_shutdown(void);

/**
 * @brief Get LibPolyCall version string
 *
 * @return Version string in format "major.minor.patch"
 */
const char* polycall_get_version(void);

/**
 * @brief Check if LibPolyCall is initialized
 *
 * @return true if initialized, false otherwise
 */
bool polycall_is_initialized(void);

/**
 * @brief Create a new execution context
 *
 * Creates a new execution context for polymorphic function calls.
 * The context manages language bridges, type conversions, and security.
 *
 * @param ctx Pointer to receive the created context
 * @param config Configuration options (can be NULL for defaults)
 * @return Error code
 */
polycall_core_error_t polycall_create_context(
    polycall_core_context_t** ctx,
    const polycall_config_t* config
);

/**
 * @brief Destroy an execution context
 *
 * @param ctx Context to destroy
 */
void polycall_destroy_context(polycall_core_context_t* ctx);

/**
 * @brief Register a language binding
 *
 * Registers a language bridge with the specified context, enabling
 * cross-language function calls to/from that language.
 *
 * @param ctx Execution context
 * @param language Language identifier (e.g., "python", "nodejs", "go")
 * @param bridge Language bridge implementation
 * @return Error code
 */
polycall_core_error_t polycall_register_language(
    polycall_core_context_t* ctx,
    const char* language,
    const language_bridge_t* bridge
);

/**
 * @brief Export a function for cross-language calls
 *
 * Makes a function available for calling from other languages.
 *
 * @param ctx Execution context
 * @param name Function name (globally unique)
 * @param func Function pointer
 * @param signature Function signature description
 * @param language Source language
 * @return Error code
 */
polycall_core_error_t polycall_export_function(
    polycall_core_context_t* ctx,
    const char* name,
    void* func,
    const polycall_signature_t* signature,
    const char* language
);

/**
 * @brief Import a function from another language
 *
 * Creates a callable wrapper for a function implemented in another language.
 *
 * @param ctx Execution context
 * @param name Function name
 * @param signature Expected function signature
 * @param source_language Language where function is implemented
 * @param wrapper Pointer to receive the wrapper function
 * @return Error code
 */
polycall_core_error_t polycall_import_function(
    polycall_core_context_t* ctx,
    const char* name,
    const polycall_signature_t* signature,
    const char* source_language,
    void** wrapper
);

/**
 * @brief Call a polymorphic function
 *
 * Invokes a function by name, handling all necessary type conversions
 * and language bridge operations.
 *
 * @param ctx Execution context
 * @param name Function name
 * @param args Array of arguments
 * @param arg_count Number of arguments
 * @param result Pointer to receive the result
 * @return Error code
 */
polycall_core_error_t polycall_call(
    polycall_core_context_t* ctx,
    const char* name,
    const polycall_value_t* args,
    size_t arg_count,
    polycall_value_t* result
);

/**
 * @brief Call a polymorphic function asynchronously
 *
 * @param ctx Execution context
 * @param name Function name
 * @param args Array of arguments
 * @param arg_count Number of arguments
 * @param callback Callback function for result
 * @param user_data User data for callback
 * @return Error code
 */
polycall_core_error_t polycall_call_async(
    polycall_core_context_t* ctx,
    const char* name,
    const polycall_value_t* args,
    size_t arg_count,
    polycall_async_callback_t callback,
    void* user_data
);

/**
 * @brief Set error handler
 *
 * Sets a custom error handler for the context. If not set,
 * errors are handled according to the default policy.
 *
 * @param ctx Execution context
 * @param handler Error handler function
 * @param user_data User data for handler
 * @return Error code
 */
polycall_core_error_t polycall_set_error_handler(
    polycall_core_context_t* ctx,
    polycall_error_handler_t handler,
    void* user_data
);

/**
 * @brief Enable performance profiling
 *
 * @param ctx Execution context
 * @param enable true to enable, false to disable
 * @return Error code
 */
polycall_core_error_t polycall_enable_profiling(
    polycall_core_context_t* ctx,
    bool enable
);

/**
 * @brief Get performance statistics
 *
 * @param ctx Execution context
 * @param stats Pointer to receive statistics
 * @return Error code
 */
polycall_core_error_t polycall_get_stats(
    polycall_core_context_t* ctx,
    polycall_stats_t* stats
);

/**
 * @brief Configure security policy
 *
 * @param ctx Execution context
 * @param policy Security policy configuration
 * @return Error code
 */
polycall_core_error_t polycall_set_security_policy(
    polycall_core_context_t* ctx,
    const polycall_security_policy_t* policy
);

/**
 * @brief Get last error message
 *
 * @param ctx Execution context
 * @return Error message string or NULL
 */
const char* polycall_get_error_message(polycall_core_context_t* ctx);

/**
 * @brief Clear error state
 *
 * @param ctx Execution context
 */
void polycall_clear_error(polycall_core_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_H */