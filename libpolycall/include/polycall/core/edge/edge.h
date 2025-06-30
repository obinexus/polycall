/**
 * @file edge.h
 * @brief Unified Edge Computing API for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file provides the unified API for the Edge Computing module,
 * integrating all subcomponents including compute router, node selection,
 * fallback mechanisms, and security.
 */

 #ifndef POLYCALL_EDGE_EDGE_H_H
 #define POLYCALL_EDGE_EDGE_H_H
 #include <stddef.h>


 #include "compute_router.h"
 #include "fallback.h"
 #include "security.h"
 #include "polycall/core/edge/polycall_edge.h"
 #include "polycall/core/edge/compute_router.h"
 #include "polycall/core/edge/fallback.h"
 #include "polycall/core/edge/node_selector.h"
 #include "polycall/core/edge/security.h"
 #include <stdlib.h>
 #include <string.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Edge computing internal context structure
  * This structure contains all the internal contexts for the edge computing module
  */
struct polycall_edge_context {
    polycall_core_context_t* core_ctx;                 /**< Core context reference */
    polycall_node_selector_context_t* node_selector;   /**< Node selector context */
    polycall_compute_router_context_t* compute_router; /**< Compute router context */
    polycall_fallback_context_t* fallback;             /**< Fallback mechanism context */
    polycall_edge_security_context_t* security;        /**< Security context */
    
    polycall_compute_router_config_t* router_config;   /**< Router configuration */
    polycall_fallback_config_t* fallback_config;       /**< Fallback configuration */
    polycall_edge_security_policy_t security_policy;   /**< Security policy */
    
    /* Statistics tracking */
    uint64_t total_tasks;                             /**< Total tasks routed */
    uint64_t successful_tasks;                        /**< Successfully executed tasks */
    uint64_t failed_tasks;                            /**< Failed tasks */
    uint64_t recovery_attempts;                       /**< Recovery attempts */
    uint64_t successful_recoveries;                   /**< Successful recovery operations */
    
    /* Thread safety */
    void* mutex;                                      /**< Mutex for thread safety */
    
    /* Network and node tracking */
    size_t registered_node_count;                     /**< Number of registered nodes */
    uint32_t last_error;                              /**< Last error code */
    
    uint64_t last_activity_timestamp;                 /**< Last activity timestamp */
    
    /* Callback system */
    void (*on_node_failure)(const char* node_id, void* user_data); /**< Node failure callback */
    void (*on_task_completion)(const char* task_id, void* result, size_t result_size, void* user_data); /**< Task completion callback */
    void* callback_user_data;                         /**< User data for callbacks */
    
    bool initialized;                                 /**< Initialization state */
};

/**
 * @brief Typedef for the edge computing context structure
 */
typedef struct polycall_edge_context polycall_edge_context_t;
 
 /**
  * @brief Initialize the edge computing module with configurations
  * 
  * @param core_ctx Core context
  * @param edge_ctx Pointer to receive edge context
  * @param router_config Compute router configuration
  * @param fallback_config Fallback mechanism configuration
  * @param security_config Edge security configuration
  * @return Error code
  */
 polycall_core_error_t polycall_edge_init(
     polycall_core_context_t* core_ctx,
     polycall_edge_context_t** edge_ctx,
     const polycall_compute_router_config_t* router_config,
     const polycall_fallback_config_t* fallback_config,
     const polycall_edge_security_config_t* security_config
 );
 
 /**
  * @brief Register an edge node
  * 
  * @param edge_ctx Edge context
  * @param node_metrics Node performance metrics
  * @param node_id Unique node identifier
  * @return Error code
  */
 polycall_core_error_t polycall_edge_register_node(
     polycall_edge_context_t* edge_ctx,
     const polycall_edge_node_metrics_t* node_metrics,
     const char* node_id
 );
 
 /**
  * @brief Route a computational task to an appropriate edge node
  * 
  * @param edge_ctx Edge context
  * @param task_data Task payload
  * @param task_size Task payload size
  * @param selected_node Pointer to receive selected node information (buffer must be at least 64 bytes)
  * @return Error code
  */
 polycall_core_error_t polycall_edge_route_task(
     polycall_edge_context_t* edge_ctx,
     const void* task_data,
     size_t task_size,
     char* selected_node
 );
 
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
 polycall_core_error_t polycall_edge_execute_task(
     polycall_edge_context_t* edge_ctx,
     const char* node_id,
     const void* task_data,
     size_t task_size,
     void* result_buffer,
     size_t* result_size
 );
 
 /**
  * @brief Handle node failure and trigger fallback mechanism
  * 
  * @param edge_ctx Edge context
  * @param failed_node_id Identifier of the failed node
  * @return Error code
  */
 polycall_core_error_t polycall_edge_handle_node_failure(
     polycall_edge_context_t* edge_ctx,
     const char* failed_node_id
 );
 
 /**
  * @brief Get current node selection metrics
  * 
  * @param edge_ctx Edge context
  * @param node_id Target node identifier
  * @param metrics Pointer to receive node metrics
  * @return Error code
  */
 polycall_core_error_t polycall_edge_get_node_metrics(
     polycall_edge_context_t* edge_ctx,
     const char* node_id,
     polycall_edge_node_metrics_t* metrics
 );
 
 /**
  * @brief Authenticate an edge node with the security system
  * 
  * @param edge_ctx Edge context
  * @param node_id Node identifier
  * @param auth_token Authentication token
  * @param token_size Authentication token size
  * @return Error code
  */
 polycall_core_error_t polycall_edge_authenticate_node(
     polycall_edge_context_t* edge_ctx,
     const char* node_id,
     const void* auth_token,
     size_t token_size
 );
 
 /**
  * @brief Assess the security threat level of a node
  * 
  * @param edge_ctx Edge context
  * @param node_id Node identifier
  * @param threat_level Pointer to receive threat level
  * @return Error code
  */
 polycall_core_error_t polycall_edge_assess_node_threat(
     polycall_edge_context_t* edge_ctx,
     const char* node_id,
     polycall_edge_threat_level_t* threat_level
 );
 
 /**
  * @brief Create a checkpoint for a task for resumable computation
  * 
  * @param edge_ctx Edge context
  * @param task_data Current task state
  * @param task_size Task state size
  * @param executed_portion Percentage of task completed
  * @param checkpoint Pointer to receive checkpoint
  * @return Error code
  */
 polycall_core_error_t polycall_edge_create_task_checkpoint(
     polycall_edge_context_t* edge_ctx,
     const void* task_data,
     size_t task_size,
     size_t executed_portion,
     polycall_task_checkpoint_t* checkpoint
 );
 
 /**
  * @brief Resume a task from a previous checkpoint
  * 
  * @param edge_ctx Edge context
  * @param checkpoint Previous task checkpoint
  * @param result_buffer Buffer to receive task result
  * @param result_size Pointer to result buffer size
  * @return Error code
  */
 polycall_core_error_t polycall_edge_resume_task(
     polycall_edge_context_t* edge_ctx,
     const polycall_task_checkpoint_t* checkpoint,
     void* result_buffer,
     size_t* result_size
 );
 
 /**
  * @brief Get statistics from the edge computing module
  * 
  * @param edge_ctx Edge context
  * @param total_tasks Total tasks routed
  * @param successful_tasks Successfully executed tasks
  * @param failed_tasks Failed tasks
  * @param recovery_attempts Recovery attempts
  * @param successful_recoveries Successful recovery operations
  * @return Error code
  */
 polycall_core_error_t polycall_edge_get_statistics(
     polycall_edge_context_t* edge_ctx,
     uint64_t* total_tasks,
     uint64_t* successful_tasks,
     uint64_t* failed_tasks,
     uint64_t* recovery_attempts,
     uint64_t* successful_recoveries
 );
 
 /**
  * @brief Cleanup edge computing context and release resources
  * 
  * @param core_ctx Core context
  * @param edge_ctx Edge context to clean up
  */
 void polycall_edge_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_edge_context_t* edge_ctx
 );
 
 /**
  * @brief Create default edge computing configurations
  * 
  * @param router_config Pointer to receive router configuration
  * @param fallback_config Pointer to receive fallback configuration
  * @param security_config Pointer to receive security configuration
  */
 void polycall_edge_create_default_config(
     polycall_compute_router_config_t* router_config,
     polycall_fallback_config_t* fallback_config,
     polycall_edge_security_config_t* security_config
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_EDGE_EDGE_H_H */