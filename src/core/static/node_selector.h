/**
#include <math.h>
#include <pthread.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stddef.h>
#include <stddef.h>
#include <stdint.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

 * @file node_selector.h
 * @brief Intelligent Node Selection Interface for LibPolyCall Edge Computing
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides advanced node selection mechanisms for distributed computational routing
 */

#ifndef POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Node status constants
 */
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H

/**
 * Forward declaration of types from core and edge modules
 */
typedef struct polycall_core_context polycall_core_context_t;

/**
 * Node selection strategies
 */
typedef enum {
	POLYCALL_NODE_SELECTION_STRATEGY_PERFORMANCE = 0,
	POLYCALL_NODE_SELECTION_STRATEGY_LOAD_BALANCING,
	POLYCALL_NODE_SELECTION_STRATEGY_ENERGY_EFFICIENT,
	POLYCALL_NODE_SELECTION_STRATEGY_PROXIMITY
} polycall_node_selection_strategy_t;

/**
 * Edge node metrics structure
 */
typedef struct {
	float compute_power;       // Normalized compute capacity
	float memory_capacity;     // Memory capacity in GB
	float network_bandwidth;   // Network bandwidth in Mbps
	float current_load;        // Current load (0.0 - 1.0)
	uint8_t available_cores;   // Number of available CPU cores
	float battery_level;       // Battery level for mobile nodes (0.0 - 1.0)
	float latency;            // Latency in milliseconds
	bool is_mobile_device;    // Whether the node is a mobile device
	uint64_t uptime;          // Node uptime in seconds
} polycall_edge_node_metrics_t;

/**
 * Node entry structure
 */
typedef struct {
	char node_id[64];         // Unique node identifier
	polycall_edge_node_metrics_t metrics; // Current node metrics
	uint8_t status;           // Current node status
	uint64_t last_successful_task_time;  // Timestamp of last successful task
	uint64_t total_task_count;           // Total tasks assigned to this node
	uint64_t failed_task_count;          // Number of failed tasks
	float cumulative_performance_score;   // Overall performance score
	bool is_authenticated;               // Whether node is authenticated
} polycall_node_entry_t;

// Internal node selector context structure
struct polycall_node_selector_context {
	polycall_node_entry_t nodes[POLYCALL_MAX_TRACKED_NODES];
	size_t node_count;
	polycall_node_selection_strategy_t strategy;
	pthread_mutex_t lock;
};

/**
 * Edge node metrics structure
 */
typedef struct {
	float compute_power;       // Normalized compute capacity
    float memory_capacity;     // Memory capacity in GB
    float network_bandwidth;   // Network bandwidth in Mbps
    float current_load;        // Current load (0.0 - 1.0)
    uint8_t available_cores;   // Number of available CPU cores

/**
 * Node status constants
 */
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H

/**
 * Node status constants
 */
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
#define POLYCALL_EDGE_NODE_SELECTOR_H_H
	float battery_level;       // Battery level for mobile nodes (0.0 - 1.0)
	float latency;             // Latency in milliseconds
	bool is_mobile_device;     // Whether the node is a mobile device
	uint64_t uptime;           // Node uptime in seconds
} polycall_edge_node_metrics_t;

/**
 * Node entry structure
 */
typedef struct {
	char node_id[64];          // Unique node identifier
	polycall_edge_node_metrics_t metrics; // Current node metrics
	uint8_t status;            // Current node status
	uint64_t last_successful_task_time;   // Timestamp of last successful task
	uint64_t total_task_count;            // Total tasks assigned to this node
	uint64_t failed_task_count;           // Number of failed tasks
	float cumulative_performance_score;   // Overall performance score
	bool is_authenticated;                // Whether node is authenticated
} polycall_node_entry_t;

// Forward declarations
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_node_selector_context polycall_node_selector_context_t;

/**
 * @brief Initialize node selector context
 * 
 * @param core_ctx Core context
 * @param selector_ctx Pointer to store initialized selector context
 * @param selection_strategy Strategy for node selection
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_init(
	polycall_core_context_t* core_ctx,
	polycall_node_selector_context_t** selector_ctx,
	polycall_node_selection_strategy_t selection_strategy
);

/**
 * @brief Register a new node in the selector
 * 
 * @param selector_ctx Node selector context
 * @param node_metrics Initial metrics for the node
 * @param node_id Unique identifier for the node
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_register(
	polycall_node_selector_context_t* selector_ctx,
	const polycall_edge_node_metrics_t* node_metrics,
	const char* node_id
);

/**
 * @brief Select optimal node for task execution
 * 
 * @param selector_ctx Node selector context
 * @param task_requirements Metrics required for the task
 * @param selected_node Output buffer to store selected node ID
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_select(
	polycall_node_selector_context_t* selector_ctx,
	const polycall_edge_node_metrics_t* task_requirements,
	char* selected_node
);

/**
 * @brief Update node metrics and performance tracking
 * 
 * @param selector_ctx Node selector context
 * @param node_id Target node identifier
 * @param new_metrics Updated metrics for the node
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_update_metrics(
	polycall_node_selector_context_t* selector_ctx,
	const char* node_id,
	const polycall_edge_node_metrics_t* new_metrics
);

/**
 * @brief Record task execution result for performance tracking
 * 
 * @param selector_ctx Node selector context
 * @param node_id Node that executed the task
 * @param task_success Whether the task completed successfully
 * @param execution_time Time taken to execute the task (ms)
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_record_task(
	polycall_node_selector_context_t* selector_ctx,
	const char* node_id,
	bool task_success,
	uint32_t execution_time
);

/**
 * @brief Get metrics for a specific node
 * 
 * @param selector_ctx Node selector context
 * @param node_id Target node identifier
 * @param node_metrics Output buffer for node metrics
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_get_node_metrics(
	polycall_node_selector_context_t* selector_ctx,
	const char* node_id,
	polycall_edge_node_metrics_t* node_metrics
);

/**
 * @brief Remove node from the selector
 * 
 * @param selector_ctx Node selector context
 * @param node_id Node to remove
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_node_selector_remove_node(
	polycall_node_selector_context_t* selector_ctx,
	const char* node_id
);

/**
 * @brief Clean up selector resources
 * 
 * @param core_ctx Core context
 * @param selector_ctx Node selector context to clean up
 */
void polycall_node_selector_cleanup(
	polycall_core_context_t* core_ctx,
	polycall_node_selector_context_t* selector_ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_EDGE_NODE_SELECTOR_H_H */
