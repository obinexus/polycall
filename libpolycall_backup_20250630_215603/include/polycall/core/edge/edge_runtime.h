/**
 * @file edge_runtime.h
 * @brief Edge Computing Runtime Environment for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the edge computing runtime environment that manages
 * the execution context for distributed computational tasks, providing isolation,
 * resource management, and communication channels between edge nodes.
 */

 #ifndef POLYCALL_EDGE_EDGE_RUNTIME_H_H
 #define POLYCALL_EDGE_EDGE_RUNTIME_H_H
 

 #include <stdbool.h>
 #include <stdint.h>
    #include <stddef.h>
    #include "polycall/core/polycall/core.h"
    #include "polycall/core/edge/edge.h"
    #include "polycall/core/edge/node_selector.h"


 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Maximum number of concurrent tasks per runtime instance
  */
 #define POLYCALL_EDGE_EDGE_RUNTIME_H_H
 
 /**
  * @brief Maximum size of runtime task queue
  */
 #define POLYCALL_EDGE_EDGE_RUNTIME_H_H
 
 /**
  * @brief Edge runtime task execution state
  */
 typedef enum {
     EDGE_TASK_STATE_QUEUED = 0,     // Task is queued for execution
     EDGE_TASK_STATE_RUNNING = 1,    // Task is currently running
     EDGE_TASK_STATE_COMPLETED = 2,  // Task completed successfully
     EDGE_TASK_STATE_FAILED = 3,     // Task execution failed
     EDGE_TASK_STATE_ABORTED = 4     // Task was aborted
 } polycall_edge_task_state_t;
 
 /**
  * @brief Edge runtime execution environment configuration
  */
 typedef struct {
     uint32_t max_concurrent_tasks;      // Maximum concurrent tasks
     uint32_t task_queue_size;           // Task queue size
     bool enable_priority_scheduling;    // Enable task priority scheduling
     bool enable_task_preemption;        // Enable task preemption
     uint32_t task_time_slice_ms;        // Time slice for task execution
     float cpu_utilization_target;       // Target CPU utilization (0.0-1.0)
     float memory_utilization_target;    // Target memory utilization (0.0-1.0)
     void* custom_execution_context;     // Custom execution context
 } polycall_edge_runtime_config_t;
 
 /**
  * @brief Edge runtime task execution metrics
  */
 typedef struct {
     uint64_t queue_time_ms;             // Time spent in queue
     uint64_t execution_time_ms;         // Task execution time
     uint64_t cpu_time_ms;               // CPU time used
     size_t peak_memory_usage;           // Peak memory usage
     uint32_t context_switches;          // Number of context switches
     float cpu_utilization;              // CPU utilization during execution
     float memory_utilization;           // Memory utilization during execution
 } polycall_edge_task_metrics_t;
 
 /**
  * @brief Edge runtime execution callback for task results
  */
 typedef void (*polycall_edge_runtime_task_callback_t)(
     const void* result_data,
     size_t result_size,
     polycall_edge_task_state_t task_state,
     polycall_edge_task_metrics_t* metrics,
     void* user_data
 );
 
 /**
  * @brief Edge runtime task error handling options
  */
 typedef enum {
     EDGE_RUNTIME_ON_ERROR_ABORT = 0,    // Abort task on error
     EDGE_RUNTIME_ON_ERROR_RETRY = 1,    // Retry task on error
     EDGE_RUNTIME_ON_ERROR_CONTINUE = 2  // Continue with partial results
 } polycall_edge_runtime_error_policy_t;
 
 /**
  * @brief Edge runtime task descriptor
  */
 typedef struct {
     void* task_data;                           // Task data pointer
     size_t task_size;                          // Task data size
     polycall_edge_task_state_t state;          // Current task state
     polycall_edge_runtime_task_callback_t callback; // Completion callback
     void* user_data;                           // User data for callback
     polycall_edge_runtime_error_policy_t error_policy; // Error handling policy
     uint8_t priority;                          // Task priority (0-255)
     uint32_t max_retries;                      // Maximum retry attempts
     uint32_t retry_count;                      // Current retry count
     polycall_edge_task_metrics_t metrics;      // Task execution metrics
     uint64_t task_id;                          // Unique task identifier
     uint64_t creation_timestamp;               // Creation timestamp
     uint64_t start_timestamp;                  // Execution start timestamp
     uint64_t completion_timestamp;             // Completion timestamp
 } polycall_edge_runtime_task_t;
 
 /**
  * @brief Edge runtime context structure (opaque)
  */
 typedef struct polycall_edge_runtime_context polycall_edge_runtime_context_t;
 
 /**
  * @brief Initialize the edge runtime environment
  *
  * @param core_ctx Core context
  * @param runtime_ctx Pointer to receive runtime context
  * @param node_id Local node identifier
  * @param config Runtime configuration
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_init(
     polycall_core_context_t* core_ctx,
     polycall_edge_runtime_context_t** runtime_ctx,
     const char* node_id,
     const polycall_edge_runtime_config_t* config
 );
 
 /**
  * @brief Submit a task to the edge runtime for execution
  *
  * @param runtime_ctx Runtime context
  * @param task_data Task data
  * @param task_size Task data size
  * @param priority Task priority (0-255)
  * @param callback Completion callback
  * @param user_data User data for callback
  * @param task_id Pointer to receive task ID
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_submit_task(
     polycall_edge_runtime_context_t* runtime_ctx,
     const void* task_data,
     size_t task_size,
     uint8_t priority,
     polycall_edge_runtime_task_callback_t callback,
     void* user_data,
     uint64_t* task_id
 );
 
 /**
  * @brief Check the status of a submitted task
  *
  * @param runtime_ctx Runtime context
  * @param task_id Task identifier
  * @param task_state Pointer to receive task state
  * @param metrics Pointer to receive task metrics (can be NULL)
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_check_task(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t task_id,
     polycall_edge_task_state_t* task_state,
     polycall_edge_task_metrics_t* metrics
 );
 
 /**
  * @brief Cancel a submitted task
  *
  * @param runtime_ctx Runtime context
  * @param task_id Task identifier
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_cancel_task(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t task_id
 );
 
 /**
  * @brief Update edge runtime metrics and node status
  *
  * @param runtime_ctx Runtime context
  * @param selector_ctx Node selector context
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_update_metrics(
     polycall_edge_runtime_context_t* runtime_ctx,
     polycall_node_selector_context_t* selector_ctx
 );
 
 /**
  * @brief Get current runtime statistics
  *
  * @param runtime_ctx Runtime context
  * @param total_tasks Pointer to receive total tasks count
  * @param completed_tasks Pointer to receive completed tasks count
  * @param failed_tasks Pointer to receive failed tasks count
  * @param avg_execution_time_ms Pointer to receive average execution time
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_get_stats(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t* total_tasks,
     uint64_t* completed_tasks,
     uint64_t* failed_tasks,
     uint64_t* avg_execution_time_ms
 );
 
 /**
  * @brief Register custom task type handler
  *
  * @param runtime_ctx Runtime context
  * @param task_type Task type identifier
  * @param handler Task handler function
  * @param user_data User data for handler
  * @return Error code
  */
 polycall_core_error_t polycall_edge_runtime_register_handler(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint32_t task_type,
     void (*handler)(void* task_data, size_t task_size, void* result_buffer, size_t* result_size, void* user_data),
     void* user_data
 );
 
 /**
  * @brief Create default edge runtime configuration
  *
  * @return Default runtime configuration
  */
 polycall_edge_runtime_config_t polycall_edge_runtime_default_config(void);
 
 /**
  * @brief Clean up edge runtime context
  *
  * @param core_ctx Core context
  * @param runtime_ctx Runtime context to clean up
  */
 void polycall_edge_runtime_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_edge_runtime_context_t* runtime_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_EDGE_EDGE_RUNTIME_H_H */