/**
 * @file edge_config.h
 * @brief Configuration Management for LibPolyCall Edge Computing System
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This module provides configuration parsing, validation, storage, and application
 * for the LibPolyCall edge computing system, supporting both global and component-specific
 * configurations.
 */

 #ifndef POLYCALL_EDGE_EDGE_CONFIG_H_H
 #define POLYCALL_EDGE_EDGE_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
    #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/edge/edge_component.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <ctype.h>
 #include "polycall/core/polycall/polycall_config.h"

 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif

/* Forward declaration of config node type to prevent undefined identifier errors */
typedef struct polycall_config_node polycall_config_node_t;
typedef struct polycall_config_value polycall_config_value_t;
typedef struct polycall_config_parser polycall_config_parser_t;
typedef struct polycall_edge_component_config polycall_edge_component_config_t;
typedef struct polycall_edge_component polycall_edge_component_t;

/**
 * @brief Routing event types for edge computation
 * @note This definition resolves duplication between polycall_event.h and compute_router.h
 */
typedef enum polycall_routing_event {
    POLYCALL_ROUTING_EVENT_INIT = 0,           /* Routing initialization */
    POLYCALL_ROUTING_EVENT_NODE_SELECTED = 1,  /* Node selected for routing */
    POLYCALL_ROUTING_EVENT_TASK_SENT = 2,      /* Task sent to node */
    POLYCALL_ROUTING_EVENT_TASK_RECEIVED = 3,  /* Task received by node */
    POLYCALL_ROUTING_EVENT_TASK_COMPLETED = 4, /* Task processing completed */
    POLYCALL_ROUTING_EVENT_ERROR = 5           /* Error during routing */
} polycall_routing_event_t;

/* Forward declaration of compute router context to resolve circular dependencies */
typedef struct polycall_compute_router_context polycall_compute_router_context_t;

/**
 * @brief Task routing event callback function type
 * @param router_ctx Compute router context
 * @param event_type Type of routing event
 * @param node_id Identifier of the node involved
 * @param task_data Task data
 * @param task_size Size of task data
 * @param user_data User-provided data
 */
typedef void (*polycall_routing_event_callback_t)(
    polycall_compute_router_context_t* router_ctx,
    polycall_routing_event_t event_type,
    const char* node_id,
    const void* task_data,
    size_t task_size,
    void* user_data
);
 /**
  * @brief Edge configuration source types
  */
 typedef enum {
     EDGE_CONFIG_SOURCE_DEFAULT = 0,      // Default configuration
     EDGE_CONFIG_SOURCE_GLOBAL = 1,       // Global configuration file
     EDGE_CONFIG_SOURCE_COMPONENT = 2,    // Component-specific configuration
     EDGE_CONFIG_SOURCE_RUNTIME = 3,      // Runtime configuration update
     EDGE_CONFIG_SOURCE_ENVIRONMENT = 4   // Environment variables
 } polycall_edge_config_source_t;
 
 /**
  * @brief Edge configuration validation level
  */
 typedef enum {
     EDGE_CONFIG_VALIDATE_NONE = 0,       // No validation
     EDGE_CONFIG_VALIDATE_TYPES = 1,      // Validate types only
     EDGE_CONFIG_VALIDATE_CONSTRAINTS = 2,// Validate types and value constraints
     EDGE_CONFIG_VALIDATE_SECURITY = 3,   // Full validation including security policies
     EDGE_CONFIG_VALIDATE_STRICT = 4      // Strictest validation with dependency checks
 } polycall_edge_config_validation_t;

 /**
  * @brief Configuration value types
  */
 typedef enum {
     POLYCALL_CONFIG_VALUE_TYPE_STRING = 0,  // String value
     POLYCALL_CONFIG_VALUE_TYPE_INT = 1,     // Integer value
     POLYCALL_CONFIG_VALUE_TYPE_FLOAT = 2,   // Float value
     POLYCALL_CONFIG_VALUE_TYPE_BOOL = 3,    // Boolean value
     POLYCALL_CONFIG_VALUE_TYPE_OBJECT = 4,  // Object value
     POLYCALL_CONFIG_VALUE_TYPE_ARRAY = 5    // Array value
 } polycall_config_value_type_t;
 
 /**
  * @brief Edge configuration load status
  */
 typedef struct {
     bool success;                        // Overall success status
     uint32_t total_entries;              // Total configuration entries processed
     uint32_t invalid_entries;            // Number of invalid entries detected
     uint32_t overridden_entries;         // Number of entries overridden
     uint32_t security_violations;        // Number of security policy violations
     const char* failed_section;          // First section that failed validation (or NULL)
     const char* error_message;           // Error message (if any)
 } polycall_edge_config_load_status_t;
/**
 * @brief Compute router selection strategies
 */
typedef enum {
    POLYCALL_ROUTER_STRATEGY_ROUND_ROBIN = 0,   // Round-robin selection
    POLYCALL_ROUTER_STRATEGY_LOAD_BASED = 1,    // Load-based selection
    POLYCALL_ROUTER_STRATEGY_LATENCY = 2,       // Latency-based selection
    POLYCALL_ROUTER_STRATEGY_PRIORITY = 3,      // Priority-based selection
    POLYCALL_ROUTER_STRATEGY_CUSTOM = 4         // Custom selection strategy
} polycall_router_selection_strategy_t;

/**
 * @brief Compute router configuration
 */
typedef struct {
    polycall_router_selection_strategy_t selection_strategy;  // Router selection strategy
    uint32_t max_routing_attempts;                           // Maximum routing attempts
    bool enable_load_balancing;                              // Enable load balancing
    uint32_t route_timeout_ms;                               // Routing timeout in milliseconds
    double load_threshold;                                   // Load threshold for routing decisions
    bool dynamic_routing;                                    // Enable dynamic routing
    char* preferred_nodes;            // Comma-separated list of preferred nodes
    uint32_t task_timeout_ms;                   // Timeout for tasks in milliseconds
} polycall_compute_router_config_t;

/**
 * @brief Fallback strategies for compute routing
 */
typedef enum {
    POLYCALL_FALLBACK_NONE = 0,                // No fallback (fail on error)
    POLYCALL_FALLBACK_LOCAL_COMPUTE = 1,       // Use local compute as fallback
    POLYCALL_FALLBACK_ALTERNATE_NODE = 2,      // Try alternate node
    POLYCALL_FALLBACK_DEGRADED_RESPONSE = 3,   // Return degraded response
    POLYCALL_FALLBACK_CACHED_RESULT = 4        // Use cached result
} polycall_fallback_strategy_t;

 /**
  * @brief Configuration node context
  */
 typedef struct {
    polycall_config_node_t* node;        // Configuration node
    polycall_config_node_t* defaults;    // Default values node
    polycall_config_node_t* schema;      // Schema node
} config_node_context_t;

/**
 * @brief Configuration value with metadata
 */
typedef struct {
    polycall_config_value_t* value;
    polycall_edge_config_source_t source;
    uint64_t timestamp;
    bool is_modified;
} config_value_entry_t;

/**
 * @brief Edge configuration manager structure
 */
struct polycall_edge_config_manager {
    polycall_core_context_t* core_ctx;           // Core context reference
    polycall_config_parser_t* parser;            // Configuration parser
    
    // Options
    polycall_edge_config_manager_options_t options;
    
    // Configuration nodes
    polycall_config_node_t* root;                // Root configuration node
    polycall_config_node_t* defaults_root;       // Default values root
    polycall_config_node_t* global_root;         // Global config root
    polycall_config_node_t* component_root;      // Component config root
    polycall_config_node_t* schema_root;         // Schema root
    
    // Component configurations
    polycall_config_node_t* components;          // Components node
    
    // Configuration status
    polycall_edge_config_load_status_t last_load_status;
    
    // Change tracking
    bool has_changes;
    
    // Value cache
    config_value_entry_t* value_cache;
    size_t value_cache_count;
    size_t value_cache_capacity;
    
    // Temporary buffer for path resolution
    char path_buffer[512];
};
/**
 * @brief Fallback configuration for compute routing
 */
typedef struct {
    polycall_fallback_strategy_t strategy;      // Fallback strategy
    bool enable_auto_recovery;                  // Enable automatic recovery
    uint32_t retry_interval_ms;                 // Retry interval in milliseconds
    uint32_t max_fallback_attempts;             // Maximum fallback attempts
    bool persist_fallback_status;               // Persist fallback status
    double quality_threshold;                   // Quality threshold for fallback
} polycall_fallback_config_t;
 
 /**
  * @brief Edge configuration manager options
  */
 typedef struct {
     const char* global_config_path;      // Path to global configuration file
     const char* component_config_path;   // Path to component-specific configuration
     const char* schema_path;             // Path to configuration schema
     bool allow_missing_global;           // Allow missing global configuration
     bool apply_environment_vars;         // Apply environment variable overrides
     polycall_edge_config_validation_t validation_level; // Validation strictness
     bool trace_config_changes;           // Enable change tracing for debugging
     bool merge_with_defaults;            // Merge loaded config with defaults
     const char* config_namespace;        // Configuration namespace (e.g., "edge")
 } polycall_edge_config_manager_options_t;
 
 /**
  * @brief Edge configuration manager (opaque)
  */
 typedef struct polycall_edge_config_manager polycall_edge_config_manager_t;
 
 /**
  * @brief Initialize edge configuration manager
  * 
  * @param core_ctx Core context
  * @param config_manager Pointer to receive config manager
  * @param options Configuration options
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_init(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t** config_manager,
     const polycall_edge_config_manager_options_t* options
 );
 
 /**
  * @brief Load configurations from specified sources
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param load_status Pointer to receive load status
  * @param error_buffer Buffer to receive detailed error message
  * @param error_buffer_size Size of error buffer
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_load(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     polycall_edge_config_load_status_t* load_status,
     char* error_buffer,
     size_t error_buffer_size
 );
 
 /**
  * @brief Apply configuration to edge component
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param component Edge component to configure
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_apply(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     polycall_edge_component_t* component
 );
 
 /**
  * @brief Get component configuration from configuration manager
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param component_name Component name
  * @param config Pointer to receive component configuration
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_get_component_config(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* component_name,
     polycall_edge_component_config_t* config
 );
 
 /**
  * @brief Get string value from configuration
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param path Configuration path (e.g., "router.selection_strategy")
  * @param default_value Default value if path not found
  * @param result Pointer to receive result (must be freed by caller)
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_get_string(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     const char* default_value,
     char** result
 );
 
 /**
  * @brief Get integer value from configuration
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param path Configuration path
  * @param default_value Default value if path not found
  * @param result Pointer to receive result
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_get_int(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     int64_t default_value,
     int64_t* result
 );
 
 /**
  * @brief Get float value from configuration
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param path Configuration path
  * @param default_value Default value if path not found
  * @param result Pointer to receive result
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_get_float(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     double default_value,
     double* result
 );
 
 /**
  * @brief Get boolean value from configuration
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param path Configuration path
  * @param default_value Default value if path not found
  * @param result Pointer to receive result
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_get_bool(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     bool default_value,
     bool* result
 );
 
 /**
  * @brief Set configuration value (runtime update)
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param path Configuration path
  * @param value_type Value type
  * @param value Pointer to value
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_set_value(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     polycall_config_value_type_t value_type,
     const void* value
 );
 
 /**
  * @brief Save current configuration to file
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @param file_path Path to save configuration
  * @param include_defaults Whether to include default values
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_save(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* file_path,
     bool include_defaults
 );
 
 /**
  * @brief Reset configuration to defaults
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager
  * @return Error code
  */
 polycall_core_error_t polycall_edge_config_manager_reset(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager
 );
 
 /**
  * @brief Create default configuration manager options
  * 
  * @return Default options
  */
 polycall_edge_config_manager_options_t polycall_edge_config_manager_default_options(void);
 
 /**
  * @brief Clean up edge configuration manager
  * 
  * @param core_ctx Core context
  * @param config_manager Configuration manager to clean up
  */
 void polycall_edge_config_manager_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_EDGE_EDGE_CONFIG_H_H */