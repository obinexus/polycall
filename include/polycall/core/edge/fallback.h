/**
 * @file fallback.h
 * @brief Fallback Mechanism Interface for LibPolyCall Edge Computing
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * Defines the comprehensive fallback strategy for distributed computational
 * tasks, providing resilience and continuity in edge computing environments.
 */

#ifndef POLYCALL_EDGE_FALLBACK_H_H
#define POLYCALL_EDGE_FALLBACK_H_H

#include "polycall/core/edge/polycall_edge.h"
#include "polycall/core/polycall/polycall_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of fallback strategies */
#define POLYCALL_EDGE_FALLBACK_H_H
typedef struct polycall_fallback_config {
  uint32_t max_fallback_attempts; // Maximum fallback attempts
  bool use_local_processing;      // Use local processing as fallback
} polycall_fallback_config_t;

/** Fallback event types for comprehensive tracking */
typedef enum {
  FALLBACK_EVENT_INITIAL_FAILURE = 0,
  FALLBACK_EVENT_NODE_UNAVAILABLE = 1,
  FALLBACK_EVENT_PARTIAL_EXECUTION = 2,
  FALLBACK_EVENT_FULL_RECOVERY = 3,
  FALLBACK_EVENT_CRITICAL_FAILURE = 4,
  FALLBACK_EVENT_TASK_REDISTRIBUTION = 5
} polycall_fallback_event_t;

/** Fallback strategy types */
typedef enum {
  FALLBACK_STRATEGY_REDUNDANT_NODES = 0,
  FALLBACK_STRATEGY_TASK_DECOMPOSITION = 1,
  FALLBACK_STRATEGY_REPLICA_EXECUTION = 2,
  FALLBACK_STRATEGY_ALTERNATIVE_ROUTE = 3,
  FALLBACK_STRATEGY_RETRY_WITH_BACKOFF = 4,
  FALLBACK_STRATEGY_PARTIAL_EXECUTION = 5,
  FALLBACK_STRATEGY_CHECKPOINT_RESUME = 6,
  FALLBACK_STRATEGY_ADAPTIVE_REROUTE = 7
} polycall_fallback_strategy_t;

/** Fallback event callback for monitoring and logging */
typedef void (*polycall_fallback_event_callback_t)(
    polycall_core_context_t *core_ctx, polycall_fallback_event_t event_type,
    const char *node_id, const void *task_data, size_t task_size,
    polycall_fallback_strategy_t strategy_used, void *user_data);

/** Task checkpoint structure for resumable computations */
typedef struct {
  void *checkpoint_data;
  size_t checkpoint_size;
  uint64_t checkpoint_timestamp;
  size_t executed_portion;
  bool is_final_checkpoint;
} polycall_task_checkpoint_t;

/** Fallback context structure */
typedef struct polycall_fallback_context polycall_fallback_context_t;

/**
 * @brief Initialize fallback mechanism context
 *
 * @param core_ctx Core LibPolyCall context
 * @param fallback_ctx Pointer to receive fallback context
 * @param config Fallback configuration
 * @param event_callback Optional event tracking callback
 * @param user_data User-provided context for callback
 * @return Error code
 */
polycall_core_error_t
polycall_fallback_init(polycall_core_context_t *core_ctx,
                       polycall_fallback_context_t **fallback_ctx,
                       const polycall_fallback_config_t *config,
                       polycall_fallback_event_callback_t event_callback,
                       void *user_data);

/**
 * @brief Handle task execution failure with fallback strategies
 *
 * @param fallback_ctx Fallback context
 * @param failed_node_id Node that failed task execution
 * @param task_data Original task payload
 * @param task_size Task payload size
 * @param checkpoint Optional previous checkpoint
 * @param result_buffer Buffer to receive task result
 * @param result_size Pointer to result buffer size
 * @return Error code
 */
polycall_core_error_t
polycall_fallback_handle_failure(polycall_fallback_context_t *fallback_ctx,
                                 const char *failed_node_id,
                                 const void *task_data, size_t task_size,
                                 const polycall_task_checkpoint_t *checkpoint,
                                 void *result_buffer, size_t *result_size);

/**
 * @brief Create a task checkpoint for resumable computation
 *
 * @param fallback_ctx Fallback context
 * @param task_data Current task state
 * @param task_size Task state size
 * @param executed_portion Percentage of task completed
 * @param checkpoint Pointer to receive checkpoint
 * @return Error code
 */
polycall_core_error_t
polycall_fallback_create_checkpoint(polycall_fallback_context_t *fallback_ctx,
                                    const void *task_data, size_t task_size,
                                    size_t executed_portion,
                                    polycall_task_checkpoint_t *checkpoint);

/**
 * @brief Resume task from a previous checkpoint
 *
 * @param fallback_ctx Fallback context
 * @param checkpoint Previous task checkpoint
 * @param result_buffer Buffer to receive task result
 * @param result_size Pointer to result buffer size
 * @return Error code
 */
polycall_core_error_t polycall_fallback_resume_from_checkpoint(
    polycall_fallback_context_t *fallback_ctx,
    const polycall_task_checkpoint_t *checkpoint, void *result_buffer,
    size_t *result_size);

/**
 * @brief Get fallback mechanism statistics
 *
 * @param fallback_ctx Fallback context
 * @param total_failures Total task failures
 * @param successful_recoveries Successful fallback recoveries
 * @param critical_failures Unrecoverable failures
 * @return Error code
 */
polycall_core_error_t polycall_fallback_get_stats(
    polycall_fallback_context_t *fallback_ctx, uint64_t *total_failures,
    uint64_t *successful_recoveries, uint64_t *critical_failures);

/**
 * @brief Cleanup fallback mechanism context
 *
 * @param core_ctx Core LibPolyCall context
 * @param fallback_ctx Fallback context to clean up
 */
void polycall_fallback_cleanup(polycall_core_context_t *core_ctx,
                               polycall_fallback_context_t *fallback_ctx);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_EDGE_FALLBACK_H_H */