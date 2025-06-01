
#include "polycall/core/edge/compute_router.h"
#include "polycall/core/edge/compute_router.h"

/**
* @file compute_router.c
* @brief Intelligent Compute Routing Implementation for LibPolyCall Edge Computing
* @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
*
* Provides advanced computational task routing mechanisms for distributed computing
*/





/**
* @brief Internal function to dispatch task to a selected node
*/
static polycall_core_error_t dispatch_task_to_node(
polycall_compute_router_context_t* router_ctx,
const char* node_id,
const void* task_data,
size_t task_size,
void* result_buffer,
size_t* result_size
) {
// In a real implementation, this would use network communication
// For now, simulate task dispatching and execution

// Event notification for task dispatched
if (router_ctx->event_callback) {
    router_ctx->event_callback(
        router_ctx, 
        ROUTING_EVENT_TASK_DISPATCHED, 
        node_id, 
        task_data, 
        task_size, 
        router_ctx->event_user_data
    );
}

// Simulate task execution (placeholder)
if (result_buffer && result_size) {
    // Copy a small portion of task data as a mock result
    size_t copy_size = *result_size < task_size ? *result_size : task_size;
    memcpy(result_buffer, task_data, copy_size);
    *result_size = copy_size;
}

return POLYCALL_CORE_SUCCESS;
}

/**
* @brief Initialize compute router context
*/
polycall_core_error_t polycall_compute_router_init(
polycall_core_context_t* core_ctx,
polycall_compute_router_context_t** router_ctx,
polycall_node_selector_context_t* node_selector,
const polycall_compute_router_config_t* router_config,
polycall_routing_event_callback_t event_callback,
void* user_data
) {
if (!core_ctx || !router_ctx || !node_selector || !router_config) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Allocate router context
polycall_compute_router_context_t* new_router = 
    polycall_core_malloc(core_ctx, sizeof(polycall_compute_router_context_t));

if (!new_router) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
}

// Initialize context
memset(new_router, 0, sizeof(polycall_compute_router_context_t));

// Set core context and node selector
new_router->core_ctx = core_ctx;
new_router->node_selector = node_selector;

// Copy router configuration
memcpy(&new_router->config, router_config, sizeof(polycall_compute_router_config_t));

// Initialize mutex for stats tracking
if (pthread_mutex_init(&new_router->stats_lock, NULL) != 0) {
    polycall_core_free(core_ctx, new_router);
    return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
}

// Set event callback
new_router->event_callback = event_callback;
new_router->event_user_data = user_data;

*router_ctx = new_router;
return POLYCALL_CORE_SUCCESS;
}

/**
* @brief Route computational task to optimal node
*/
polycall_core_error_t polycall_compute_router_route_task(
polycall_compute_router_context_t* router_ctx,
const void* task_data,
size_t task_size,
const polycall_edge_node_metrics_t* task_requirements,
void* result_buffer,
size_t* result_size
) {
if (!router_ctx || !task_data || !task_requirements || !result_buffer || !result_size) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Update total tasks statistic
pthread_mutex_lock(&router_ctx->stats_lock);
router_ctx->total_tasks++;
pthread_mutex_unlock(&router_ctx->stats_lock);

// Event notification for task initiation
if (router_ctx->event_callback) {
    router_ctx->event_callback(
        router_ctx, 
        ROUTING_EVENT_TASK_INITIATED, 
        NULL, 
        task_data, 
        task_size, 
        router_ctx->event_user_data
    );
}

// Routing attempts loop
for (uint32_t attempt = 0; attempt < router_ctx->config.max_routing_attempts; attempt++) {
    char selected_node[64];
    
    // Select optimal node
    polycall_core_error_t select_result = polycall_node_selector_select(
        router_ctx->node_selector,
        task_requirements,
        selected_node
    );
    
    if (select_result != POLYCALL_CORE_SUCCESS) {
        // No suitable node found
        if (attempt == router_ctx->config.max_routing_attempts - 1) {
            // Final attempt failed
            pthread_mutex_lock(&router_ctx->stats_lock);
            router_ctx->failed_tasks++;
            pthread_mutex_unlock(&router_ctx->stats_lock);
            
            // Event notification for routing failure
            if (router_ctx->event_callback) {
                router_ctx->event_callback(
                    router_ctx, 
                    ROUTING_EVENT_ROUTING_FAILED, 
                    NULL, 
                    task_data, 
                    task_size, 
                    router_ctx->event_user_data
                );
            }
            
            return POLYCALL_CORE_ERROR_NOT_FOUND;
        }
        
        // Wait before next attempt if fallback is enabled
        if (router_ctx->config.enable_fallback) {
            // Simulate short delay between routing attempts
            usleep(router_ctx->config.task_timeout_ms * 1000);
        }
        
        continue;
    }
    
    // Event notification for node selection
    if (router_ctx->event_callback) {
        router_ctx->event_callback(
            router_ctx, 
            ROUTING_EVENT_NODE_SELECTED, 
            selected_node, 
            task_data, 
            task_size, 
            router_ctx->event_user_data
        );
    }
    
    // Dispatch task to selected node
    polycall_core_error_t dispatch_result = dispatch_task_to_node(
        router_ctx,
        selected_node,
        task_data,
        task_size,
        result_buffer,
        result_size
    );
    
    if (dispatch_result == POLYCALL_CORE_SUCCESS) {
        // Task successfully routed and executed
        pthread_mutex_lock(&router_ctx->stats_lock);
        router_ctx->successful_tasks++;
        pthread_mutex_unlock(&router_ctx->stats_lock);
        
        // Event notification for task completion
        if (router_ctx->event_callback) {
            router_ctx->event_callback(
                router_ctx, 
                ROUTING_EVENT_TASK_COMPLETED, 
                selected_node, 
                task_data, 
                task_size, 
                router_ctx->event_user_data
            );
        }
        
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Node failed, mark for potential removal
    polycall_node_selector_record_task(
        router_ctx->node_selector, 
        selected_node, 
        false, 
        router_ctx->config.task_timeout_ms
    );
    
    // If fallback is disabled, exit after first failure
    if (!router_ctx->config.enable_fallback) {
        break;
    }
}

// If all routing attempts fail
pthread_mutex_lock(&router_ctx->stats_lock);
router_ctx->failed_tasks++;
pthread_mutex_unlock(&router_ctx->stats_lock);

// Event notification for routing failure
if (router_ctx->event_callback) {
    router_ctx->event_callback(
        router_ctx, 
        ROUTING_EVENT_ROUTING_FAILED, 
        NULL, 
        task_data, 
        task_size, 
        router_ctx->event_user_data
    );
}

return POLYCALL_CORE_ERROR_NOT_FOUND;
}

/**
* @brief Handle node failure during task routing
*/
polycall_core_error_t polycall_compute_router_handle_node_failure(
polycall_compute_router_context_t* router_ctx,
const char* failed_node_id
) {
if (!router_ctx || !failed_node_id) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Remove failed node from node selector
polycall_core_error_t remove_result = polycall_node_selector_remove_node(
    router_ctx->node_selector, 
    failed_node_id
);
if (remove_result != POLYCALL_CORE_SUCCESS) {
    return remove_result;
}
// Event notification for node failure
if (router_ctx->event_callback) {
    router_ctx->event_callback(
        router_ctx, 
        ROUTING_EVENT_NODE_FAILURE, 
        failed_node_id, 
        NULL, 
        0, 
        router_ctx->event_user_data
    );
}
// Update failed tasks statistic
pthread_mutex_lock(&router_ctx->stats_lock);
router_ctx->failed_tasks++;
pthread_mutex_unlock(&router_ctx->stats_lock);
// Notify node selector to re-evaluate node status
polycall_node_selector_re_evaluate_node_status(
    router_ctx->node_selector, 
    failed_node_id
);
return POLYCALL_CORE_SUCCESS;
    // Remove failed node from node selector
    polycall_core_error_t remove_result = polycall_node_selector_remove_node(
        router_ctx->node_selector, 
        failed_node_id
    );

    return remove_result;
}

/**
* @brief Get current routing statistics
*/
polycall_core_error_t polycall_compute_router_get_stats(
polycall_compute_router_context_t* router_ctx,
uint64_t* total_tasks,
uint64_t* successful_tasks,
uint64_t* failed_tasks
) {
if (!router_ctx || !total_tasks || !successful_tasks || !failed_tasks) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

pthread_mutex_lock(&router_ctx->stats_lock);

*total_tasks = router_ctx->total_tasks;
*successful_tasks = router_ctx->successful_tasks;
*failed_tasks = router_ctx->failed_tasks;

pthread_mutex_unlock(&router_ctx->stats_lock);

return POLYCALL_CORE_SUCCESS;
}

/**
* @brief Cleanup compute router context
*/
void polycall_compute_router_cleanup(
polycall_core_context_t* core_ctx,
polycall_compute_router_context_t* router_ctx
) {
if (!core_ctx || !router_ctx) {
    return;
}

// Destroy stats mutex
pthread_mutex_destroy(&router_ctx->stats_lock);

// Free router context
polycall_core_free(core_ctx, router_ctx);
}

/**
* @brief Default event callback for demonstration purposes
* 
* This function is provided as an example and can be used as a template
* for implementing custom event handlers.
*/

static void __attribute__((unused)) default_routing_event_callback(
polycall_compute_router_context_t* router_ctx,
polycall_routing_event_t event_type,
const char* node_id,
const void* task_data,
size_t task_size,
void* user_data
) {
    // Prevent unused parameter warnings
    (void)router_ctx;
    (void)task_data;
    (void)user_data;
    
    // Log or track routing events
    switch (event_type) {
        case ROUTING_EVENT_TASK_INITIATED:
            printf("Task initiated: size %zu\n", task_size);
            break;
        
        case ROUTING_EVENT_NODE_SELECTED:
            printf("Node selected: %s\n", node_id);
            break;
        
        case ROUTING_EVENT_TASK_DISPATCHED:
            printf("Task dispatched to node: %s\n", node_id);
            break;
        
        case ROUTING_EVENT_TASK_COMPLETED:
            printf("Task completed on node: %s\n", node_id);
            break;
        
        case ROUTING_EVENT_ROUTING_FAILED:
            printf("Task routing failed\n");
            break;
        
        default:
            printf("Unknown routing event: %d\n", event_type);
            break;
    }
}