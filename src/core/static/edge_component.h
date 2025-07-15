/**
 * @file edge_component.h
 * @brief Comprehensive Edge Component System for LibPolyCall
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This module provides a unified interface for edge computing components,
 * integrating node selection, task routing, fallback mechanisms, security,
 * and runtime management into a cohesive system.
 */

#ifndef POLYCALL_EDGE_EDGE_COMPONENT_H_H
#define POLYCALL_EDGE_EDGE_COMPONENT_H_H

#include "polycall/core/edge/compute_router.h"
#include "polycall/core/edge/edge.h"
#include "polycall/core/edge/edge_runtime.h"
#include "polycall/core/edge/fallback.h"
#include "polycall/core/edge/node_selector.h"
#include "polycall/core/edge/security.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_public.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Edge security configuration structure
 */
typedef struct polycall_edge_security_config {
  bool enable_encryption;         // Enable data encryption
  bool verify_nodes;              // Verify node identity
  bool enable_access_control;     // Enable access control
  uint32_t key_rotation_interval; // Key rotation interval in seconds
} polycall_edge_security_config_t;

/**
 * @brief Edge runtime configuration structure
 */
typedef struct polycall_edge_runtime_config {
  uint32_t worker_threads;    // Number of worker threads
  uint32_t queue_size;        // Task queue size
  bool enable_stats;          // Enable runtime statistics
  uint32_t stats_interval_ms; // Statistics collection interval
} polycall_edge_runtime_config_t;

/**
 * @brief Internal discovery state
 */
typedef struct {
  bool is_active;
  pthread_t discovery_thread;
  uint16_t discovery_port;
  volatile bool should_terminate;
} discovery_state_t;

/**
 * @brief Task processor registry entry
 */
typedef struct {
  polycall_edge_task_processor_t processor;
  void *user_data;
} task_processor_entry_t;

/**
 * @brief Event callback registry entry
 */
typedef struct {
  polycall_edge_component_event_callback_t callback;
  void *user_data;
} event_callback_entry_t;

/**
 * @brief Edge component implementation structure
 */
struct polycall_edge_component {
  // Core references
  polycall_core_context_t *core_ctx;
  polycall_edge_context_t *edge_ctx;
  polycall_edge_runtime_context_t *runtime_ctx;

  // Component identification
  char *component_name;
  char *component_id;
  polycall_edge_component_type_t type;
  polycall_edge_task_policy_t task_policy;

  // Component state
  polycall_edge_component_status_t status;
  polycall_edge_component_metrics_t metrics;

  // Discovery management
  discovery_state_t discovery;

  // Task processing
  task_processor_entry_t task_processor;

  // Configuration
  polycall_edge_component_config_t config;

  // Event callbacks
  event_callback_entry_t *event_callbacks;
  size_t event_callback_count;
  size_t event_callback_capacity;

  // Thread synchronization
  pthread_mutex_t lock;
  pthread_mutex_t metrics_lock;

  // Original user data
  void *user_data;
};

// Forward declarations for types used in this header
typedef struct polycall_compute_router_config polycall_compute_router_config_t;
typedef struct polycall_fallback_config polycall_fallback_config_t;
typedef struct polycall_edge_fallback_config polycall_edge_fallback_config_t;
typedef struct polycall_edge_security_config polycall_edge_security_config_t;
typedef struct polycall_edge_runtime_config polycall_edge_runtime_config_t;
typedef struct polycall_edge_security_config polycall_edge_security_config_t;
typedef struct polycall_edge_runtime_config polycall_edge_runtime_config_t;

/**
 * @brief Edge fallback configuration structure
 */
typedef struct polycall_edge_fallback_config {
  bool enable_fallback;         // Enable fallback mechanism
  uint32_t fallback_timeout_ms; // Timeout before triggering fallback
  uint32_t retry_count;         // Number of retries before fallback
  bool enable_local_fallback;   // Enable local processing fallback
} polycall_edge_fallback_config_t;

/**
 * @brief Isolation levels for edge components
 */
typedef enum {
  ISOLATION_LEVEL_NONE = 0,      // No isolation
  ISOLATION_LEVEL_PROCESS = 1,   // Process-level isolation
  ISOLATION_LEVEL_CONTAINER = 2, // Container-based isolation
  ISOLATION_LEVEL_VM = 3,        // Virtual machine isolation
  ISOLATION_LEVEL_PHYSICAL = 4,  // Physical hardware isolation
  ISOLATION_LEVEL_CUSTOM = 5     // Custom isolation mechanism
} polycall_isolation_level_t;

/**
 * @brief Edge component types for specialized functionality
 */
typedef enum {
  EDGE_COMPONENT_TYPE_COMPUTE = 0,     // Computational processing component
  EDGE_COMPONENT_TYPE_STORAGE = 1,     // Data storage component
  EDGE_COMPONENT_TYPE_GATEWAY = 2,     // Network gateway component
  EDGE_COMPONENT_TYPE_SENSOR = 3,      // Sensor/input component
  EDGE_COMPONENT_TYPE_ACTUATOR = 4,    // Actuator/output component
  EDGE_COMPONENT_TYPE_COORDINATOR = 5, // Coordination component
  EDGE_COMPONENT_TYPE_CUSTOM = 6       // Custom component type
} polycall_edge_component_type_t;

/**
 * @brief Task handling policy for component
 */
typedef enum {
  EDGE_TASK_POLICY_QUEUE = 0,     // Queue tasks for sequential processing
  EDGE_TASK_POLICY_IMMEDIATE = 1, // Process immediately or reject
  EDGE_TASK_POLICY_PRIORITY = 2,  // Process by priority order
  EDGE_TASK_POLICY_DEADLINE = 3,  // Process by deadline
  EDGE_TASK_POLICY_FAIR_SHARE = 4 // Balanced processing between requesters
} polycall_edge_task_policy_t;

/**
 * @brief Edge component event types
 */
typedef enum {
  EDGE_COMPONENT_EVENT_CREATED = 0,
  EDGE_COMPONENT_EVENT_STARTED = 1,
  EDGE_COMPONENT_EVENT_STOPPED = 2,
  EDGE_COMPONENT_EVENT_TASK_RECEIVED = 3,
  EDGE_COMPONENT_EVENT_TASK_PROCESSED = 4,
  EDGE_COMPONENT_EVENT_TASK_FAILED = 5,
  EDGE_COMPONENT_EVENT_NODE_ADDED = 6,
  EDGE_COMPONENT_EVENT_NODE_REMOVED = 7,
  EDGE_COMPONENT_EVENT_SECURITY_VIOLATION = 8,
  EDGE_COMPONENT_EVENT_RESOURCE_THRESHOLD = 9,
  EDGE_COMPONENT_EVENT_DISCOVERY = 10,
  EDGE_COMPONENT_EVENT_ERROR = 11
} polycall_edge_component_event_t;

/**
 * @brief Edge component configuration
 */
typedef struct {
  // Basic component settings
  const char *component_name;              // Component name
  const char *component_id;                // Unique component identifier
  polycall_edge_component_type_t type;     // Component type
  polycall_edge_task_policy_t task_policy; // Task handling policy
  polycall_isolation_level_t isolation;    // Isolation level

  // Resource limits
  size_t max_memory_mb;     // Maximum memory allocation (MB)
  uint32_t max_tasks;       // Maximum concurrent tasks
  uint32_t max_nodes;       // Maximum managed nodes
  uint32_t task_timeout_ms; // Task execution timeout (ms)

  // Networking settings
  uint16_t discovery_port;    // Node discovery port
  uint16_t command_port;      // Command port
  uint16_t data_port;         // Data transfer port
  bool enable_auto_discovery; // Enable automatic node discovery

  // Security configuration
  polycall_edge_security_config_t security_config;

  // Runtime configuration
  polycall_edge_runtime_config_t runtime_config;

  // Advanced settings
  bool enable_telemetry;                           // Enable telemetry
  bool enable_load_balancing;                      // Enable load balancing
  bool enable_dynamic_scaling;                     // Enable dynamic scaling
  const char *log_path;                            // Path for component logs
  void *user_data;                                 // User-provided context
  polycall_edge_fallback_config_t fallback_config; // Fallback configuration
  polycall_compute_router_config_t *router_config; // Routing configuration
} polycall_edge_component_config_t;

/**
 * @brief Edge component metrics for monitoring
 */
typedef struct {
  // Task processing statistics
  uint64_t total_tasks_received;
  uint64_t total_tasks_processed;
  uint64_t total_tasks_failed;
  uint64_t total_tasks_forwarded;

  // Timing metrics
  uint64_t avg_processing_time_ms;
  uint64_t max_processing_time_ms;
  uint64_t min_processing_time_ms;

  // Resource usage
  float cpu_utilization;     // 0.0 - 1.0
  float memory_utilization;  // 0.0 - 1.0
  float network_utilization; // 0.0 - 1.0

  // Node metrics
  uint32_t total_nodes;
  uint32_t active_nodes;
  uint32_t degraded_nodes;
  uint32_t failed_nodes;

  // Security metrics
  uint32_t security_violations;
  uint32_t authentication_failures;

  // System health
  float system_health; // 0.0 - 1.0
  uint64_t uptime_seconds;
} polycall_edge_component_metrics_t;

/**
 * @brief Edge component status
 */
typedef enum {
  EDGE_COMPONENT_STATUS_UNINITIALIZED = 0,
  EDGE_COMPONENT_STATUS_INITIALIZED = 1,
  EDGE_COMPONENT_STATUS_STARTING = 2,
  EDGE_COMPONENT_STATUS_RUNNING = 3,
  EDGE_COMPONENT_STATUS_PAUSED = 4,
  EDGE_COMPONENT_STATUS_STOPPING = 5,
  EDGE_COMPONENT_STATUS_STOPPED = 6,
  EDGE_COMPONENT_STATUS_ERROR = 7
} polycall_edge_component_status_t;

/**
 * @brief Edge component opaque structure
 */
typedef struct polycall_edge_component polycall_edge_component_t;

/**
 * @brief Edge component event callback
 */
typedef void (*polycall_edge_component_event_callback_t)(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    polycall_edge_component_event_t event, const void *event_data,
    size_t event_data_size, void *user_data);

/**
 * @brief Edge component task processor callback
 */
typedef polycall_core_error_t (*polycall_edge_task_processor_t)(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    const void *task_data, size_t task_size, void *result_buffer,
    size_t *result_size, void *user_data);

/**
 * @brief Create an edge computing component
 *
 * @param core_ctx Core context
 * @param component Pointer to receive component
 * @param config Component configuration
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_create(polycall_core_context_t *core_ctx,
                               polycall_edge_component_t **component,
                               const polycall_edge_component_config_t *config);

/**
 * @brief Start the edge component and associated systems
 *
 * @param core_ctx Core context
 * @param component Edge component to start
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_start(polycall_core_context_t *core_ctx,
                              polycall_edge_component_t *component);

/**
 * @brief Stop the edge component and associated systems
 *
 * @param core_ctx Core context
 * @param component Edge component to stop
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_stop(polycall_core_context_t *core_ctx,
                             polycall_edge_component_t *component);

/**
 * @brief Get current status of the edge component
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param status Pointer to receive status
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_get_status(polycall_core_context_t *core_ctx,
                                   polycall_edge_component_t *component,
                                   polycall_edge_component_status_t *status);

/**
 * @brief Register a task processor for the component
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param processor Task processor callback
 * @param user_data User data for callback
 * @return Error code
 */
polycall_core_error_t polycall_edge_component_register_processor(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    polycall_edge_task_processor_t processor, void *user_data);

/**
 * @brief Register event callback for component events
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param callback Event callback function
 * @param user_data User data for callback
 * @return Error code
 */
polycall_core_error_t polycall_edge_component_register_event_callback(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    polycall_edge_component_event_callback_t callback, void *user_data);

/**
 * @brief Process a task through the edge component
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param task_data Task data
 * @param task_size Task data size
 * @param result_buffer Buffer to receive result
 * @param result_size Pointer to result buffer size
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_process_task(polycall_core_context_t *core_ctx,
                                     polycall_edge_component_t *component,
                                     const void *task_data, size_t task_size,
                                     void *result_buffer, size_t *result_size);

/**
 * @brief Process task asynchronously
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param task_data Task data
 * @param task_size Task data size
 * @param callback Completion callback
 * @param user_data User data for callback
 * @param task_id Pointer to receive task ID
 * @return Error code
 */
polycall_core_error_t polycall_edge_component_process_task_async(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    const void *task_data, size_t task_size,
    polycall_edge_runtime_task_callback_t callback, void *user_data,
    uint64_t *task_id);

/**
 * @brief Add a node to the component's node registry
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param node_metrics Node metrics
 * @param node_id Node identifier
 * @return Error code
 */
polycall_core_error_t polycall_edge_component_add_node(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    const polycall_edge_node_metrics_t *node_metrics, const char *node_id);

/**
 * @brief Remove a node from the component's registry
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param node_id Node identifier
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_remove_node(polycall_core_context_t *core_ctx,
                                    polycall_edge_component_t *component,
                                    const char *node_id);

/**
 * @brief Start node auto-discovery
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_start_discovery(polycall_core_context_t *core_ctx,
                                        polycall_edge_component_t *component);

/**
 * @brief Stop node auto-discovery
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_stop_discovery(polycall_core_context_t *core_ctx,
                                       polycall_edge_component_t *component);

/**
 * @brief Get component metrics
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param metrics Pointer to receive metrics
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_get_metrics(polycall_core_context_t *core_ctx,
                                    polycall_edge_component_t *component,
                                    polycall_edge_component_metrics_t *metrics);

/**
 * @brief Get list of all nodes in the component's registry
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param node_ids Array to receive node IDs (must be at least max_nodes)
 * @param node_count Pointer to node count (in/out)
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_get_nodes(polycall_core_context_t *core_ctx,
                                  polycall_edge_component_t *component,
                                  char **node_ids, size_t *node_count);

/**
 * @brief Get node metrics for a specific node
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param node_id Node identifier
 * @param metrics Pointer to receive node metrics
 * @return Error code
 */
polycall_core_error_t polycall_edge_component_get_node_metrics(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    const char *node_id, polycall_edge_node_metrics_t *metrics);

/**
 * @brief Update component configuration
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param config New configuration
 * @return Error code
 */
polycall_core_error_t polycall_edge_component_update_config(
    polycall_core_context_t *core_ctx, polycall_edge_component_t *component,
    const polycall_edge_component_config_t *config);

/**
 * @brief Get component configuration
 *
 * @param core_ctx Core context
 * @param component Edge component
 * @param config Pointer to receive configuration
 * @return Error code
 */
polycall_core_error_t
polycall_edge_component_get_config(polycall_core_context_t *core_ctx,
                                   polycall_edge_component_t *component,
                                   polycall_edge_component_config_t *config);

/**
 * @brief Create default component configuration
 *
 * @return Default configuration
 */
polycall_edge_component_config_t polycall_edge_component_default_config(void);

/**
 * @brief Destroy edge component and release resources
 *
 * @param core_ctx Core context
 * @param component Edge component to destroy
 */
void polycall_edge_component_destroy(polycall_core_context_t *core_ctx,
                                     polycall_edge_component_t *component);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_EDGE_EDGE_COMPONENT_H_H */