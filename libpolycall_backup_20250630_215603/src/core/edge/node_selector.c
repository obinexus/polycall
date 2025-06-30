/**
#include "polycall/core/edge/node_selector.h"
#include "polycall/core/edge/node_selector.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"


 * @file node_selector.c
 * @brief Intelligent Node Selection Implementation for LibPolyCall Edge Computing
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides advanced node selection mechanisms for distributed computational routing
 */

 /**
  * @brief Calculate node performance score
  * 
  * Computes a comprehensive performance score based on multiple metrics
  * 
  * @param metrics Node metrics
  * @return Performance score (0.0 - 1.0)
  */
 static float calculate_performance_score(const polycall_edge_node_metrics_t* metrics) {
     float compute_score = fminf(metrics->compute_power / 1000.0f, 1.0f);
     float memory_score = fminf(metrics->memory_capacity / 128.0f, 1.0f);
     float network_score = fminf(metrics->network_bandwidth / 1000.0f, 1.0f);
     float load_score = 1.0f - metrics->current_load;
     float core_score = fminf((float)metrics->available_cores / 64.0f, 1.0f);
 
     // Weighted scoring system
     return (
         (compute_score * 0.3f) +
         (memory_score * 0.2f) +
         (network_score * 0.2f) +
         (load_score * 0.15f) +
         (core_score * 0.15f)
     );
 }
 
 /**
  * @brief Initialize node selector context
  */
 polycall_core_error_t polycall_node_selector_init(
     polycall_core_context_t* core_ctx,
     polycall_node_selector_context_t** selector_ctx,
     polycall_node_selection_strategy_t selection_strategy
 ) {
     if (!core_ctx || !selector_ctx) {
         return POLYCALL_ERROR_INVALID_ARGUMENT;
     }
 
     // Allocate selector context
     polycall_node_selector_context_t* new_selector = 
         polycall_core_malloc(core_ctx, sizeof(polycall_node_selector_context_t));
     
     if (!new_selector) {
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
 
     // Initialize context
     memset(new_selector, 0, sizeof(polycall_node_selector_context_t));
     new_selector->strategy = selection_strategy;
     
     // Initialize mutex
     if (pthread_mutex_init(&new_selector->lock, NULL) != 0) {
         polycall_core_free(core_ctx, new_selector);
         return POLYCALL_ERROR_INTERNAL;
     }
 
     *selector_ctx = new_selector;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register a new node in the selector
  */
 polycall_core_error_t polycall_node_selector_register(
     polycall_node_selector_context_t* selector_ctx,
     const polycall_edge_node_metrics_t* node_metrics,
     const char* node_id
 ) {
     if (!selector_ctx || !node_metrics || !node_id) {
         return POLYCALL_ERROR_INVALID_ARGUMENT;
     }
 
     pthread_mutex_lock(&selector_ctx->lock);
 
     // Check if node limit is reached
     if (selector_ctx->node_count >= POLYCALL_MAX_TRACKED_NODES) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_ERROR_UNAVAILABLE;
     }
 
     // Check for duplicate node
     for (size_t i = 0; i < selector_ctx->node_count; i++) {
         if (strcmp(selector_ctx->nodes[i].node_id, node_id) == 0) {
             pthread_mutex_unlock(&selector_ctx->lock);
             return POLYCALL_ERROR_ALREADY_INITIALIZED;
         }
     }
 
     // Add new node
     polycall_node_entry_t* new_node = &selector_ctx->nodes[selector_ctx->node_count++];
     
     // Initialize node entry
     strncpy(new_node->node_id, node_id, sizeof(new_node->node_id) - 1);
     memcpy(&new_node->metrics, node_metrics, sizeof(polycall_edge_node_metrics_t));
     
     // Set initial node status
     new_node->status = NODE_STATUS_HEALTHY;
     new_node->last_successful_task_time = (uint64_t)time(NULL);
     new_node->total_task_count = 0;
     new_node->failed_task_count = 0;
     new_node->cumulative_performance_score = calculate_performance_score(node_metrics);
     new_node->is_authenticated = false;
 
     pthread_mutex_unlock(&selector_ctx->lock);
 
     return POLYCALL_CORE_ERROR_NONE;
 }
 
 /**
  * @brief Select optimal node for task execution
  */
 polycall_core_error_t polycall_node_selector_select(
     polycall_node_selector_context_t* selector_ctx,
     const polycall_edge_node_metrics_t* task_requirements,
     char* selected_node
 ) {
     if (!selector_ctx || !task_requirements || !selected_node) {
         return POLYCALL_ERROR_INVALID_ARGUMENT;
     }
 
     pthread_mutex_lock(&selector_ctx->lock);
 
     // No nodes available
     if (selector_ctx->node_count == 0) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_ERROR_NOT_FOUND;
     }
 
     float best_score = -1.0f;
     int64_t best_node_index = -1;
 
     // Selection based on strategy
     for (size_t i = 0; i < selector_ctx->node_count; i++) {
         polycall_node_entry_t* current_node = &selector_ctx->nodes[i];
 
         // Skip offline or critical nodes
         if (current_node->status >= NODE_STATUS_CRITICAL) {
             continue;
         }
 
         float node_score = -1.0f;
 
         switch (selector_ctx->strategy) {
             case POLYCALL_NODE_SELECTION_STRATEGY_PERFORMANCE:
                 // Prioritize raw performance
                 node_score = current_node->cumulative_performance_score * 
                              (1.0f - current_node->metrics.current_load);
                 break;
 
             case POLYCALL_NODE_SELECTION_STRATEGY_ENERGY_EFFICIENT:
                 // Balance performance and energy efficiency
                 node_score = current_node->cumulative_performance_score * 
                              (1.0f / (current_node->metrics.current_load + 0.1f));
                 break;
 
             case POLYCALL_NODE_SELECTION_STRATEGY_LOAD_BALANCING:
                 // Prioritize node reliability
                 node_score = current_node->cumulative_performance_score * 
                              (1.0f - (float)current_node->failed_task_count / 
                               (current_node->total_task_count + 1));
                 break;
 
             case POLYCALL_NODE_SELECTION_STRATEGY_PROXIMITY:
                 // Minimize network latency (placeholder logic)
                 node_score = current_node->cumulative_performance_score * 
                              (1.0f / (current_node->metrics.network_bandwidth + 1.0f));
                 break;
 
 default:
     // Default to raw performance for other strategies including security
     node_score = current_node->cumulative_performance_score * 
                  (current_node->is_authenticated ? 1.0f : 0.5f);
     break;
         }
 
         // Check if node meets task requirements
         bool meets_requirements = 
             current_node->metrics.compute_power >= task_requirements->compute_power &&
             current_node->metrics.memory_capacity >= task_requirements->memory_capacity &&
             current_node->metrics.available_cores >= task_requirements->available_cores;
 
         // Apply additional scoring if requirements are met
         if (meets_requirements && node_score > best_score) {
             best_score = node_score;
             best_node_index = i;
         }
     }
 
     // No suitable node found
     if (best_node_index == -1) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_ERROR_NOT_FOUND;
     }
 
     // Copy selected node ID
     strncpy(selected_node, 
             selector_ctx->nodes[best_node_index].node_id, 
             64);
 
     pthread_mutex_unlock(&selector_ctx->lock);
 
     return POLYCALL_CORE_ERROR_NONE;
 }
 
 /**
  * @brief Update node metrics and performance tracking
  */
 polycall_core_error_t polycall_node_selector_update_metrics(
     polycall_node_selector_context_t* selector_ctx,
     const char* node_id,
     const polycall_edge_node_metrics_t* new_metrics
 ) {
     if (!selector_ctx || !node_id || !new_metrics) {
         return POLYCALL_ERROR_INVALID_ARGUMENT;
     }
 
     pthread_mutex_lock(&selector_ctx->lock);
 
     // Find the node
     polycall_node_entry_t* target_node = NULL;
     for (size_t i = 0; i < selector_ctx->node_count; i++) {
         if (strcmp(selector_ctx->nodes[i].node_id, node_id) == 0) {
             target_node = &selector_ctx->nodes[i];
             break;
         }
     }
 
     if (!target_node) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
 
     // Update metrics
     memcpy(&target_node->metrics, new_metrics, sizeof(polycall_edge_node_metrics_t));
 
     // Recalculate performance score
     target_node->cumulative_performance_score = 
         (target_node->cumulative_performance_score * 0.7f) + 
         (calculate_performance_score(new_metrics) * 0.3f);
 
     // Update node status based on load and performance
     if (new_metrics->current_load > 0.9f) {
         target_node->status = NODE_STATUS_CRITICAL;
     } else if (new_metrics->current_load > 0.7f) {
         target_node->status = NODE_STATUS_DEGRADED;
     } else {
         target_node->status = NODE_STATUS_HEALTHY;
     }
 
     pthread_mutex_unlock(&selector_ctx->lock);
 
     return POLYCALL_CORE_ERROR_NONE;
 }
 
 /**
  * @brief Record task execution result for performance tracking
  */
 polycall_core_error_t polycall_node_selector_record_task(
     polycall_node_selector_context_t* selector_ctx,
     const char* node_id,
     bool task_success,
     uint32_t execution_time
 ) {
     if (!selector_ctx || !node_id) {
         return POLYCALL_ERROR_INVALID_ARGUMENT;
     }
 
     pthread_mutex_lock(&selector_ctx->lock);
 
     // Find the node
     polycall_node_entry_t* target_node = NULL;
     for (size_t i = 0; i < selector_ctx->node_count; i++) {
         if (strcmp(selector_ctx->nodes[i].node_id, node_id) == 0) {
             target_node = &selector_ctx->nodes[i];
             break;
         }
     }
 
     if (!target_node) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_ERROR_NOT_FOUND;
     }
 
     // Update task tracking
     target_node->total_task_count++;
     
     if (!task_success) {
         target_node->failed_task_count++;
         
         // Adjust node status for repeated failures
         if (target_node->failed_task_count > (target_node->total_task_count / 2)) {
             target_node->status = NODE_STATUS_CRITICAL;
         }
     } else {
         // Update last successful task time
         target_node->last_successful_task_time = (uint64_t)time(NULL);
     }
 
     // Adjust performance score based on task execution
     float success_ratio = 1.0f - ((float)target_node->failed_task_count / target_node->total_task_count);
     target_node->cumulative_performance_score *= success_ratio;
 
     pthread_mutex_unlock(&selector_ctx->lock);
 
     return POLYCALL_CORE_ERROR_NONE;
 }
 
 /**
  * @brief Get current metrics for a specific node
  */
 polycall_core_error_t polycall_node_selector_get_node_metrics(
     polycall_node_selector_context_t* selector_ctx,
     const char* node_id,
     polycall_edge_node_metrics_t* node_metrics
 ) {
     if (!selector_ctx || !node_id || !node_metrics) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     pthread_mutex_lock(&selector_ctx->lock);
 
     // Find the node
     polycall_node_entry_t* target_node = NULL;
     for (size_t i = 0; i < selector_ctx->node_count; i++) {
         if (strcmp(selector_ctx->nodes[i].node_id, node_id) == 0) {
             target_node = &selector_ctx->nodes[i];
             break;
         }
     }
 
     if (!target_node) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_ERROR_NOT_FOUND;
     }

     // Copy node metrics
     memcpy(node_metrics, &target_node->metrics, sizeof(polycall_edge_node_metrics_t));
 
     pthread_mutex_unlock(&selector_ctx->lock);
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Remove a node from tracking
  */
 polycall_core_error_t polycall_node_selector_remove_node(
     polycall_node_selector_context_t* selector_ctx,
     const char* node_id
 ) {
     if (!selector_ctx || !node_id) {
         return POLYCALL_ERROR_INVALID_ARGUMENT;
     }
 
     pthread_mutex_lock(&selector_ctx->lock);
 
     // Find the node
     ssize_t node_index = -1;
     for (size_t i = 0; i < selector_ctx->node_count; i++) {
         if (strcmp(selector_ctx->nodes[i].node_id, node_id) == 0) {
             node_index = i;
             break;
         }
     }
 
     if (node_index == -1) {
         pthread_mutex_unlock(&selector_ctx->lock);
         return POLYCALL_ERROR_NOT_FOUND;
     }
 
     // Remove node by shifting array
     for (size_t i = node_index; i < selector_ctx->node_count - 1; i++) {
         memcpy(&selector_ctx->nodes[i], &selector_ctx->nodes[i + 1], 
                sizeof(polycall_node_entry_t));
     }
 
     selector_ctx->node_count--;
 
     pthread_mutex_unlock(&selector_ctx->lock);
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup node selector context
  */
 void polycall_node_selector_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_node_selector_context_t* selector_ctx
 ) {
     if (!core_ctx || !selector_ctx) {
         return;
     }
 
     // Destroy mutex
     pthread_mutex_destroy(&selector_ctx->lock);
 
     // Free context
     polycall_core_free(core_ctx, selector_ctx);
 }
