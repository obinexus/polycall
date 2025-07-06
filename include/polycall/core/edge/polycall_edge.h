/**
 * @file polycall_edge.h
 * @brief Comprehensive header for LibPolyCall Edge Computing Module
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the core interfaces and structures for the LibPolyCall
 * edge computing system, providing a robust framework for distributed
 * computational routing and management.
 */

#ifndef POLYCALL_EDGE_POLYCALL_EDGE_H_H
#define POLYCALL_EDGE_POLYCALL_EDGE_H_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Edge node capabilities and performance metrics
 */
typedef struct {
  float compute_power;      // Computational capacity (FLOPS)
  float memory_capacity;    // Available memory (GB)
  float network_bandwidth;  // Network bandwidth (Mbps)
  float current_load;       // Current computational load (0-1)
  uint32_t active_cores;    // Number of active processor cores
  uint32_t available_cores; // Number of available cores
} polycall_edge_node_metrics_t;

/**
 * @brief Edge node selection criteria
 */
typedef enum {
  POLYCALL_NODE_SELECT_PERFORMANCE = 0,  // Optimize for raw performance
  POLYCALL_NODE_SELECT_EFFICIENCY = 1,   // Balance performance and energy
  POLYCALL_NODE_SELECT_AVAILABILITY = 2, // Prioritize node reliability
  POLYCALL_NODE_SELECT_PROXIMITY = 3,    // Minimize network latency
  POLYCALL_NODE_SELECT_SECURITY = 4      // Prioritize security contexts
} polycall_node_selection_strategy_t;

/**
 * @brief Compute task routing configuration
 */
typedef struct {
  polycall_node_selection_strategy_t selection_strategy;
  uint32_t max_routing_attempts; // Maximum routing attempts
  uint32_t task_timeout_ms;      // Task execution timeout
  bool enable_fallback;          // Enable fallback mechanisms
  bool enable_load_balancing;    // Enable dynamic load balancing
  float performance_threshold;   // Performance acceptance threshold
} polycall_compute_router_config_t;

/**
 * @brief Fallback mechanism configuration
 */
typedef struct {
  uint32_t max_fallback_nodes;   // Maximum number of fallback candidates
  uint32_t retry_delay_ms;       // Delay between fallback attempts
  bool enable_partial_execution; // Allow partial task completion
  bool log_fallback_events;      // Log fallback mechanism actions
} polycall_fallback_config_t;

/**
 * @brief Edge security configuration
 */
typedef struct {
  polycall_ffi_security_context_t *ffi_security;
  bool enforce_node_authentication;
  bool enable_end_to_end_encryption;
  bool validate_node_integrity;
  uint32_t security_token_lifetime_ms;
} polycall_edge_security_config_t;

/**
 * @brief Edge computing context
 */
typedef struct polycall_edge_context polycall_edge_context_t;

/**
 * @brief Initialize edge computing context
 *
 * @param core_ctx Core context
 * @param edge_ctx Pointer to receive edge context
 * @param router_config Compute router configuration
 * @param fallback_config Fallback mechanism configuration
 * @param security_config Edge security configuration
 * @return Error code
 */
polycall_core_error_t
polycall_edge_init(polycall_core_context_t *core_ctx,
                   polycall_edge_context_t **edge_ctx,
                   const polycall_compute_router_config_t *router_config,
                   const polycall_fallback_config_t *fallback_config,
                   const polycall_edge_security_config_t *security_config);

/**
 * @brief Register an edge node
 *
 * @param edge_ctx Edge context
 * @param node_metrics Node performance metrics
 * @param node_id Unique node identifier
 * @return Error code
 */
polycall_core_error_t
polycall_edge_register_node(polycall_edge_context_t *edge_ctx,
                            const polycall_edge_node_metrics_t *node_metrics,
                            const char *node_id);

/**
 * @brief Route a computational task to an appropriate edge node
 *
 * @param edge_ctx Edge context
 * @param task_data Task payload
 * @param task_size Task payload size
 * @param selected_node Pointer to receive selected node information
 * @return Error code
 */
polycall_core_error_t
polycall_edge_route_task(polycall_edge_context_t *edge_ctx,
                         const void *task_data, size_t task_size,
                         char *selected_node);

/**
 * @brief Execute a task on a specific edge node
 *
 * @param edge_ctx Edge context
 * @param node_id Target node identifier
 * @param task_data Task payload
 * @param task_size Task payload size
 * @param result_buffer Buffer to receive task result
 * @param result_size Pointer to result buffer size
 * @return Error code
 */
polycall_core_error_t
polycall_edge_execute_task(polycall_edge_context_t *edge_ctx,
                           const char *node_id, const void *task_data,
                           size_t task_size, void *result_buffer,
                           size_t *result_size);

/**
 * @brief Handle node failure and trigger fallback mechanism
 *
 * @param edge_ctx Edge context
 * @param failed_node_id Identifier of the failed node
 * @return Error code
 */
polycall_core_error_t
polycall_edge_handle_node_failure(polycall_edge_context_t *edge_ctx,
                                  const char *failed_node_id);

/**
 * @brief Get current node selection metrics
 *
 * @param edge_ctx Edge context
 * @param node_id Target node identifier
 * @param metrics Pointer to receive node metrics
 * @return Error code
 */
polycall_core_error_t
polycall_edge_get_node_metrics(polycall_edge_context_t *edge_ctx,
                               const char *node_id,
                               polycall_edge_node_metrics_t *metrics);

/**
 * @brief Cleanup edge computing context
 *
 * @param core_ctx Core context
 * @param edge_ctx Edge context to clean up
 */
void polycall_edge_cleanup(polycall_core_context_t *core_ctx,
                           polycall_edge_context_t *edge_ctx);

/**
 * @brief Create default edge computing configurations
 *
 * @param router_config Pointer to receive router configuration
 * @param fallback_config Pointer to receive fallback configuration
 * @param security_config Pointer to receive security configuration
 */
void polycall_edge_create_default_config(
    polycall_compute_router_config_t *router_config,
    polycall_fallback_config_t *fallback_config,
    polycall_edge_security_config_t *security_config);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_EDGE_POLYCALL_EDGE_H_H */