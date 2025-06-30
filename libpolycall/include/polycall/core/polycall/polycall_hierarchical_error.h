/**
 * @file polycall_hierarchical_error.h
 * @brief Hierarchical Error Handling for LibPolyCall
 * @author Implementation for OBINexusComputing
 *
 * Provides advanced error handling with inheritance, component-specific
 * error reporting, and error propagation for complex protocol interactions.
 */

#ifndef POLYCALL_HIERARCHICAL_ERROR_H
#define POLYCALL_HIERARCHICAL_ERROR_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length of a component name
 */
#define POLYCALL_MAX_COMPONENT_NAME_LENGTH 64

/**
 * @brief Maximum error hierarchy depth
 */
#define POLYCALL_MAX_ERROR_HIERARCHY_DEPTH 8

/**
 * @brief Maximum number of component-specific error handlers
 */
#define POLYCALL_MAX_ERROR_HANDLERS 32

/**
 * @brief Error propagation mode
 */
typedef enum {
    /** Do not propagate errors */
    POLYCALL_ERROR_PROPAGATE_NONE = 0x00,
    /** Propagate errors to parent components */
    POLYCALL_ERROR_PROPAGATE_UPWARD = 0x01,
    /** Propagate errors to child components */
    POLYCALL_ERROR_PROPAGATE_DOWNWARD = 0x02,
    /** Propagate errors in both directions */
    POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL = 0x03
} polycall_error_propagation_mode_t;

/**
 * @brief Hierarchical error context (opaque)
 */
typedef struct polycall_hierarchical_error_context polycall_hierarchical_error_context_t;

/**
 * @brief Error handler function signature
 */
typedef void (*polycall_hierarchical_error_handler_fn)(
    polycall_core_context_t* ctx,
    const char* component_name,
    polycall_error_source_t source,
    int32_t code,
    polycall_error_severity_t severity,
    const char* message,
    void* user_data
);

/**
 * @brief Component-specific error handler configuration
 */
typedef struct {
    char component_name[POLYCALL_MAX_COMPONENT_NAME_LENGTH];  /**< Component name */
    polycall_error_source_t source;                          /**< Error source */
    polycall_hierarchical_error_handler_fn handler;           /**< Error handler */
    void* user_data;                                         /**< User data */
    polycall_error_propagation_mode_t propagation_mode;       /**< Error propagation mode */
    char parent_component[POLYCALL_MAX_COMPONENT_NAME_LENGTH];/**< Parent component name */
} polycall_hierarchical_error_handler_config_t;

/**
 * @brief Initialize hierarchical error system
 *
 * @param core_ctx Core context
 * @param error_ctx Pointer to receive hierarchical error context
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_init(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t** error_ctx
);

/**
 * @brief Clean up hierarchical error system
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 */
void polycall_hierarchical_error_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx
);

/**
 * @brief Register a component-specific error handler
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param config Handler configuration
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_register_handler(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const polycall_hierarchical_error_handler_config_t* config
);

/**
 * @brief Unregister a component-specific error handler
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_unregister_handler(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name
);

/**
 * @brief Set an error with propagation
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @param source Error source
 * @param code Error code
 * @param severity Error severity
 * @param file Source file
 * @param line Source line
 * @param message Error message format
 * @param ... Format arguments
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_set(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name,
    polycall_error_source_t source,
    int32_t code,
    polycall_error_severity_t severity,
    const char* file,
    int line,
    const char* message,
    ...
);

/**
 * @brief Get the parent component
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @param parent_buffer Buffer to receive parent name
 * @param buffer_size Buffer size
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_get_parent(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name,
    char* parent_buffer,
    size_t buffer_size
);

/**
 * @brief Get child components
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @param children Array to receive child component names
 * @param max_children Maximum number of children to retrieve
 * @param child_count Pointer to receive number of children
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_get_children(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name,
    char children[][POLYCALL_MAX_COMPONENT_NAME_LENGTH],
    uint32_t max_children,
    uint32_t* child_count
);

/**
 * @brief Set error propagation mode
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @param mode Propagation mode
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_set_propagation(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name,
    polycall_error_propagation_mode_t mode
);

/**
 * @brief Check if component has error handling
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @return true if component has a handler, false otherwise
 */
bool polycall_hierarchical_error_has_handler(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name
);

/**
 * @brief Get last error for a component
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @param record Pointer to receive error record
 * @return true if error record was retrieved, false otherwise
 */
bool polycall_hierarchical_error_get_last(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name,
    polycall_error_record_t* record
);

/**
 * @brief Clear last error for a component
 *
 * @param core_ctx Core context
 * @param error_ctx Hierarchical error context
 * @param component_name Component name
 * @return Error code
 */
polycall_core_error_t polycall_hierarchical_error_clear(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* error_ctx,
    const char* component_name
);

/**
 * @brief Convenience macro for setting a hierarchical error with file and line info
 */
#define POLYCALL_HIERARCHICAL_ERROR_SET(ctx, error_ctx, component, source, code, severity, message, ...) \
    polycall_hierarchical_error_set(ctx, error_ctx, component, source, code, severity, __FILE__, __LINE__, message, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_HIERARCHICAL_ERROR_H */