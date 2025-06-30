/**
 * @file compute_router.h
 * @brief Intelligent Compute Routing Interface for LibPolyCall Edge Computing
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides advanced computational task routing mechanisms for distributed computing
 */

 #ifndef POLYCALL_EDGE_COMPUTE_ROUTER_H_H
 #define POLYCALL_EDGE_COMPUTE_ROUTER_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_event.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/edge/edge_config.h"
 #include "polycall/core/edge/node_selector.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /** Maximum number of routing attempts */
 #define POLYCALL_EDGE_COMPUTE_ROUTER_H_H
 
 /** Task routing context (forward declaration) */
 typedef struct polycall_compute_router_context polycall_compute_router_context_t;
 
 /** 
  * Task routing event callback - using the enum type from polycall_event.h
  * This needs to match the definition in edge_config.h
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
  * @brief Initialize compute router
  * 
  * @param core_ctx Core LibPolyCall context
  * @param router_ctx Pointer to receive compute router context
  * @param node_selector Node selection context
  * @param router_config Router configuration
  * @param event_callback Optional event tracking callback
  * @param user_data User-provided context for callback
  * @return Error code
  */
 polycall_core_error_t polycall_compute_router_init(
     polycall_core_context_t* core_ctx,
     polycall_compute_router_context_t** router_ctx,
     polycall_node_selector_context_t* node_selector,
     const polycall_compute_router_config_t* router_config,
     polycall_routing_event_callback_t event_callback,
     void* user_data
 );
 
 /**
  * @brief Route computational task to optimal node
  * 
  * @param router_ctx Compute router context
  * @param task_data Task payload
  * @param task_size Task payload size
  * @param task_requirements Computational requirements
  * @param result_buffer Buffer to receive task result
  * @param result_size Pointer to result buffer size
  * @return Error code
  */
 polycall_core_error_t polycall_compute_router_route_task(
     polycall_compute_router_context_t* router_ctx,
     const void* task_data,
     size_t task_size,
     const polycall_edge_node_metrics_t* task_requirements,
     void* result_buffer,
     size_t* result_size
 );
 
 /**
  * @brief Handle node failure during task routing
  * 
  * @param router_ctx Compute router context
  * @param failed_node_id Node identifier that failed
  * @return Error code
  */
 polycall_core_error_t polycall_compute_router_handle_node_failure(
     polycall_compute_router_context_t* router_ctx,
     const char* failed_node_id
 );
 
 /**
  * @brief Get current routing statistics
  * 
  * @param router_ctx Compute router context
  * @param total_tasks Total tasks routed
  * @param successful_tasks Successful task routings
  * @param failed_tasks Failed task routings
  * @return Error code
  */
 polycall_core_error_t polycall_compute_router_get_stats(
     polycall_compute_router_context_t* router_ctx,
     uint64_t* total_tasks,
     uint64_t* successful_tasks,
     uint64_t* failed_tasks
 );
 
 /**
  * @brief Cleanup compute router context
  * 
  * @param core_ctx Core LibPolyCall context
  * @param router_ctx Compute router context to clean up
  */
 void polycall_compute_router_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_compute_router_context_t* router_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_EDGE_COMPUTE_ROUTER_H_H */