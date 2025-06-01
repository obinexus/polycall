/**
#include "polycall/core/edge/edge_component.h"

 * @file edge_component.c
 * @brief Implementation of the Edge Component System for LibPolyCall
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This module provides the implementation for the unified edge computing component,
 * integrating node selection, task routing, fallback mechanisms, security,
 * and runtime management into a cohesive system.
 */

 #include "polycall/core/edge/edge_component.h"

 /**
  * @brief Node discovery thread function
  */
 static void* discovery_thread_func(void* arg) {
     polycall_edge_component_t* component = (polycall_edge_component_t*)arg;
     discovery_state_t* discovery = &component->discovery;
     
     // Notify that discovery has started
     pthread_mutex_lock(&component->lock);
     discovery->is_active = true;
     pthread_mutex_unlock(&component->lock);
     
     // Fire discovery started event
     if (component->event_callback_count > 0) {
         for (size_t i = 0; i < component->event_callback_count; i++) {
             component->event_callbacks[i].callback(
                 component->core_ctx,
                 component,
                 EDGE_COMPONENT_EVENT_DISCOVERY,
                 NULL,
                 0,
                 component->event_callbacks[i].user_data
             );
         }
     }
     
     // Main discovery loop
     while (!discovery->should_terminate) {
         // Simulate discovery (in a real implementation, this would use actual network discovery)
         // This is a placeholder for the actual network discovery mechanism
         
         // Sleep for a short duration to avoid busy-waiting
         struct timespec ts;
         ts.tv_sec = 0;
         ts.tv_nsec = 500000000; // 500ms
         nanosleep(&ts, NULL);
     }
     
     // Clean up and mark discovery as inactive
     pthread_mutex_lock(&component->lock);
     discovery->is_active = false;
     pthread_mutex_unlock(&component->lock);
     
     return NULL;
 }
 
 /**
  * @brief Route a task to an appropriate node and execute it
  */
 static polycall_core_error_t route_and_execute_task(
     polycall_edge_component_t* component,
     const void* task_data,
     size_t task_size,
     void* result_buffer,
     size_t* result_size
 ) {
     if (!component || !task_data || task_size == 0 || !result_buffer || !result_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate buffer for selected node ID
     char selected_node[64];
     
     // Use the edge context to route the task
     polycall_core_error_t result = polycall_edge_route_task(
         component->edge_ctx,
         task_data,
         task_size,
         selected_node
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Execute the task on the selected node
     result = polycall_edge_execute_task(
         component->edge_ctx,
         selected_node,
         task_data,
         task_size,
         result_buffer,
         result_size
     );
     
     // Update metrics based on task execution result
     pthread_mutex_lock(&component->metrics_lock);
     component->metrics.total_tasks_processed++;
     if (result != POLYCALL_CORE_SUCCESS) {
         component->metrics.total_tasks_failed++;
     }
     pthread_mutex_unlock(&component->metrics_lock);
     
     return result;
 }
 
 /**
  * @brief Initialize event callback registry
  */
 static polycall_core_error_t init_event_callbacks(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initial capacity for event callbacks
     component->event_callback_capacity = 4;
     component->event_callback_count = 0;
     
     // Allocate callback array
     component->event_callbacks = polycall_core_malloc(
         core_ctx,
         sizeof(event_callback_entry_t) * component->event_callback_capacity
     );
     
     if (!component->event_callbacks) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup event callback registry
  */
 static void cleanup_event_callbacks(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return;
     }
     
     if (component->event_callbacks) {
         polycall_core_free(core_ctx, component->event_callbacks);
         component->event_callbacks = NULL;
         component->event_callback_count = 0;
         component->event_callback_capacity = 0;
     }
 }
 
 /**
  * @brief Fire component event to all registered callbacks
  */
 static void fire_component_event(
     polycall_edge_component_t* component,
     polycall_edge_component_event_t event,
     const void* event_data,
     size_t event_data_size
 ) {
     if (!component) {
         return;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Iterate through all registered callbacks
     for (size_t i = 0; i < component->event_callback_count; i++) {
         if (component->event_callbacks[i].callback) {
             component->event_callbacks[i].callback(
                 component->core_ctx,
                 component,
                 event,
                 event_data,
                 event_data_size,
                 component->event_callbacks[i].user_data
             );
         }
     }
     
     pthread_mutex_unlock(&component->lock);
 }
 
 /**
  * @brief Handle fallback event callback
  */
 static void fallback_event_callback(
     polycall_core_context_t* core_ctx,
     polycall_fallback_event_t event_type,
     const char* node_id,
     const void* task_data,
     size_t task_size,
     polycall_fallback_strategy_t strategy_used,
     void* user_data
 ) {
     polycall_edge_component_t* component = (polycall_edge_component_t*)user_data;
     
     // Map fallback event to component event
     polycall_edge_component_event_t component_event;
     switch (event_type) {
         case FALLBACK_EVENT_INITIAL_FAILURE:
             component_event = EDGE_COMPONENT_EVENT_TASK_FAILED;
             break;
         case FALLBACK_EVENT_FULL_RECOVERY:
             component_event = EDGE_COMPONENT_EVENT_TASK_PROCESSED;
             break;
         case FALLBACK_EVENT_CRITICAL_FAILURE:
             component_event = EDGE_COMPONENT_EVENT_TASK_FAILED; // Changed from undefined ERROR to TASK_FAILED
             break;
         default:
             return; // Skip unmapped events
     }
     
     // Create event data structure
     struct {
         const char* node_id;
         polycall_fallback_strategy_t strategy;
     } event_data = {
         .node_id = node_id,
         .strategy = strategy_used
     };
     
     // Fire the component event
     fire_component_event(component, component_event, &event_data, sizeof(event_data));
 }
 
 /**
  * @brief Create an edge computing component
  */
 polycall_core_error_t polycall_edge_component_create(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t** component,
     const polycall_edge_component_config_t* config
 ) {
     if (!core_ctx || !component || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate component structure
     polycall_edge_component_t* new_component = 
         polycall_core_malloc(core_ctx, sizeof(polycall_edge_component_t));
     if (!new_component) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize component structure
     memset(new_component, 0, sizeof(polycall_edge_component_t));
     new_component->core_ctx = core_ctx;
     new_component->status = EDGE_COMPONENT_STATUS_UNINITIALIZED;
     new_component->type = config->type;
     new_component->task_policy = config->task_policy;
     new_component->user_data = config->user_data;
     
     // Initialize mutexes
     if (pthread_mutex_init(&new_component->lock, NULL) != 0) {
         polycall_core_free(core_ctx, new_component);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     if (pthread_mutex_init(&new_component->metrics_lock, NULL) != 0) {
         pthread_mutex_destroy(&new_component->lock);
         polycall_core_free(core_ctx, new_component);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Copy component name and ID
     if (config->component_name) {
         new_component->component_name = 
             polycall_core_malloc(core_ctx, strlen(config->component_name) + 1);
         if (!new_component->component_name) {
             pthread_mutex_destroy(&new_component->lock);
             pthread_mutex_destroy(&new_component->metrics_lock);
             polycall_core_free(core_ctx, new_component);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(new_component->component_name, config->component_name);
     }
     
     if (config->component_id) {
         new_component->component_id = 
             polycall_core_malloc(core_ctx, strlen(config->component_id) + 1);
         if (!new_component->component_id) {
             if (new_component->component_name) {
                 polycall_core_free(core_ctx, new_component->component_name);
             }
             pthread_mutex_destroy(&new_component->lock);
             pthread_mutex_destroy(&new_component->metrics_lock);
             polycall_core_free(core_ctx, new_component);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         strcpy(new_component->component_id, config->component_id);
     } else {
         // Generate a default component ID if not provided
         new_component->component_id = 
             polycall_core_malloc(core_ctx, 64);
         if (!new_component->component_id) {
             if (new_component->component_name) {
                 polycall_core_free(core_ctx, new_component->component_name);
             }
             pthread_mutex_destroy(&new_component->lock);
             pthread_mutex_destroy(&new_component->metrics_lock);
             polycall_core_free(core_ctx, new_component);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Simple random ID generation (would be more sophisticated in production)
         snprintf(new_component->component_id, 64, "edge_%08x", (uint32_t)time(NULL));
     }
     
     // Copy configuration
     memcpy(&new_component->config, config, sizeof(polycall_edge_component_config_t));
     
     // Initialize event callbacks
     polycall_core_error_t result = init_event_callbacks(core_ctx, new_component);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(core_ctx, new_component->component_id);
         if (new_component->component_name) {
             polycall_core_free(core_ctx, new_component->component_name);
         }
         pthread_mutex_destroy(&new_component->lock);
         pthread_mutex_destroy(&new_component->metrics_lock);
         polycall_core_free(core_ctx, new_component);
         return result;
     }
     
     // Initialize the edge computing system
     result = polycall_edge_init(
         core_ctx,
         &new_component->edge_ctx,
         &config->router_config,
         &config->fallback_config,
         &config->security_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_event_callbacks(core_ctx, new_component);
         polycall_core_free(core_ctx, new_component->component_id);
         if (new_component->component_name) {
             polycall_core_free(core_ctx, new_component->component_name);
         }
         pthread_mutex_destroy(&new_component->lock);
         pthread_mutex_destroy(&new_component->metrics_lock);
         polycall_core_free(core_ctx, new_component);
         return result;
     }
     
     // Initialize runtime environment
     result = polycall_edge_runtime_init(
         core_ctx,
         &new_component->runtime_ctx,
         new_component->component_id,
         &config->runtime_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_edge_cleanup(core_ctx, new_component->edge_ctx);
         cleanup_event_callbacks(core_ctx, new_component);
         polycall_core_free(core_ctx, new_component->component_id);
         if (new_component->component_name) {
             polycall_core_free(core_ctx, new_component->component_name);
         }
         pthread_mutex_destroy(&new_component->lock);
         pthread_mutex_destroy(&new_component->metrics_lock);
         polycall_core_free(core_ctx, new_component);
         return result;
     }
     
     // Initialize metrics
     memset(&new_component->metrics, 0, sizeof(polycall_edge_component_metrics_t));
     new_component->metrics.system_health = 1.0f; // Start with perfect health
     
     // Component is now initialized
     new_component->status = EDGE_COMPONENT_STATUS_INITIALIZED;
     
     // Fire creation event
     fire_component_event(new_component, EDGE_COMPONENT_EVENT_CREATED, NULL, 0);
     
     *component = new_component;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Start the edge component and associated systems
  */
 polycall_core_error_t polycall_edge_component_start(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check current status
     if (component->status == EDGE_COMPONENT_STATUS_RUNNING) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_SUCCESS; // Already running
     }
     
     if (component->status != EDGE_COMPONENT_STATUS_INITIALIZED && 
         component->status != EDGE_COMPONENT_STATUS_STOPPED) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update status to starting
     component->status = EDGE_COMPONENT_STATUS_STARTING;
     pthread_mutex_unlock(&component->lock);
     
     // Start node discovery if enabled
     if (component->config.enable_auto_discovery) {
         discovery_state_t* discovery = &component->discovery;
         discovery->should_terminate = false;
         discovery->discovery_port = component->config.discovery_port;
         
         // Create discovery thread
         if (pthread_create(&discovery->discovery_thread, NULL, discovery_thread_func, component) != 0) {
             // Failed to create discovery thread
             pthread_mutex_lock(&component->lock);
             component->status = EDGE_COMPONENT_STATUS_ERROR;
             pthread_mutex_unlock(&component->lock);
             
             // Fire error event
             const char error_message[] = "Failed to create discovery thread";
             fire_component_event(component, EDGE_COMPONENT_EVENT_ERROR, error_message, sizeof(error_message));
             
             return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
         }
     }
     
     // Update component status
     pthread_mutex_lock(&component->lock);
     component->status = EDGE_COMPONENT_STATUS_RUNNING;
     pthread_mutex_unlock(&component->lock);
     
     // Fire started event
     fire_component_event(component, EDGE_COMPONENT_EVENT_STARTED, NULL, 0);
     
     // Reset uptime in metrics
     pthread_mutex_lock(&component->metrics_lock);
     component->metrics.uptime_seconds = 0;
     pthread_mutex_unlock(&component->metrics_lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Stop the edge component and associated systems
  */
 polycall_core_error_t polycall_edge_component_stop(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check current status
     if (component->status == EDGE_COMPONENT_STATUS_STOPPED) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_SUCCESS; // Already stopped
     }
     
     if (component->status != EDGE_COMPONENT_STATUS_RUNNING && 
         component->status != EDGE_COMPONENT_STATUS_PAUSED) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update status to stopping
     component->status = EDGE_COMPONENT_STATUS_STOPPING;
     pthread_mutex_unlock(&component->lock);
     
     // Stop discovery thread if active
     if (component->discovery.is_active) {
         component->discovery.should_terminate = true;
         pthread_join(component->discovery.discovery_thread, NULL);
         component->discovery.is_active = false;
     }
     
     // Update component status
     pthread_mutex_lock(&component->lock);
     component->status = EDGE_COMPONENT_STATUS_STOPPED;
     pthread_mutex_unlock(&component->lock);
     
     // Fire stopped event
     fire_component_event(component, EDGE_COMPONENT_EVENT_STOPPED, NULL, 0);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get current status of the edge component
  */
 polycall_core_error_t polycall_edge_component_get_status(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     polycall_edge_component_status_t* status
 ) {
     if (!core_ctx || !component || !status) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     *status = component->status;
     pthread_mutex_unlock(&component->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register a task processor for the component
  */
 polycall_core_error_t polycall_edge_component_register_processor(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     polycall_edge_task_processor_t processor,
     void* user_data
 ) {
     if (!core_ctx || !component || !processor) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Set task processor
     component->task_processor.processor = processor;
     component->task_processor.user_data = user_data;
     
     pthread_mutex_unlock(&component->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register event callback for component events
  */
 polycall_core_error_t polycall_edge_component_register_event_callback(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     polycall_edge_component_event_callback_t callback,
     void* user_data
 ) {
     if (!core_ctx || !component || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check if we need to expand the callback array
     if (component->event_callback_count >= component->event_callback_capacity) {
         size_t new_capacity = component->event_callback_capacity * 2;
         event_callback_entry_t* new_callbacks = polycall_core_malloc(
             core_ctx,
             sizeof(event_callback_entry_t) * new_capacity
         );
         
         if (!new_callbacks) {
             pthread_mutex_unlock(&component->lock);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing callbacks
         memcpy(new_callbacks, component->event_callbacks, 
                sizeof(event_callback_entry_t) * component->event_callback_count);
         
         // Free old array and update pointers
         polycall_core_free(core_ctx, component->event_callbacks);
         component->event_callbacks = new_callbacks;
         component->event_callback_capacity = new_capacity;
     }
     
     // Add new callback
     component->event_callbacks[component->event_callback_count].callback = callback;
     component->event_callbacks[component->event_callback_count].user_data = user_data;
     component->event_callback_count++;
     
     pthread_mutex_unlock(&component->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Process a task through the edge component
  */
 polycall_core_error_t polycall_edge_component_process_task(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     const void* task_data,
     size_t task_size,
     void* result_buffer,
     size_t* result_size
 ) {
     if (!core_ctx || !component || !task_data || !result_buffer || !result_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check component status
     if (component->status != EDGE_COMPONENT_STATUS_RUNNING) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update task metrics
     pthread_mutex_lock(&component->metrics_lock);
     component->metrics.total_tasks_received++;
     pthread_mutex_unlock(&component->metrics_lock);
     
     // Check if we have a registered task processor
     polycall_edge_task_processor_t processor = component->task_processor.processor;
     void* processor_user_data = component->task_processor.user_data;
     
     pthread_mutex_unlock(&component->lock);
     
     // Fire task received event
     fire_component_event(component, EDGE_COMPONENT_EVENT_TASK_RECEIVED, task_data, task_size);
     
     // Processing start time
     uint64_t start_time = (uint64_t)time(NULL) * 1000;
     
     // Process the task
     polycall_core_error_t result;
     if (processor) {
         // Use registered processor
         result = processor(
             core_ctx,
             component,
             task_data,
             task_size,
             result_buffer,
             result_size,
             processor_user_data
         );
     } else {
         // Use default routing and execution
         result = route_and_execute_task(
             component,
             task_data,
             task_size,
             result_buffer,
             result_size
         );
     }
     
     // Processing end time
     uint64_t end_time = (uint64_t)time(NULL) * 1000;
     uint64_t processing_time = end_time - start_time;
     
     // Update timing metrics
     pthread_mutex_lock(&component->metrics_lock);
     
     if (component->metrics.total_tasks_processed == 0) {
         // First task
         component->metrics.avg_processing_time_ms = processing_time;
         component->metrics.min_processing_time_ms = processing_time;
         component->metrics.max_processing_time_ms = processing_time;
     } else {
         // Update average
         uint64_t total_time = component->metrics.avg_processing_time_ms * 
                              component->metrics.total_tasks_processed;
         total_time += processing_time;
         component->metrics.avg_processing_time_ms = 
             total_time / (component->metrics.total_tasks_processed + 1);
         
         // Update min/max
         if (processing_time < component->metrics.min_processing_time_ms) {
             component->metrics.min_processing_time_ms = processing_time;
         }
         if (processing_time > component->metrics.max_processing_time_ms) {
             component->metrics.max_processing_time_ms = processing_time;
         }
     }
     
     // Update success/failure counts
     if (result == POLYCALL_CORE_SUCCESS) {
         component->metrics.total_tasks_processed++;
         // Fire task processed event
         pthread_mutex_unlock(&component->metrics_lock);
         fire_component_event(component, EDGE_COMPONENT_EVENT_TASK_PROCESSED, result_buffer, *result_size);
     } else {
         component->metrics.total_tasks_failed++;
         // Fire task failed event
         pthread_mutex_unlock(&component->metrics_lock);
         fire_component_event(component, EDGE_COMPONENT_EVENT_TASK_FAILED, &result, sizeof(result));
     }
     
     return result;
 }
 
 /**
  * @brief Process task asynchronously
  */
 polycall_core_error_t polycall_edge_component_process_task_async(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     const void* task_data,
     size_t task_size,
     polycall_edge_runtime_task_callback_t callback,
     void* user_data,
     uint64_t* task_id
 ) {
     if (!core_ctx || !component || !task_data || !callback || !task_id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check component status
     if (component->status != EDGE_COMPONENT_STATUS_RUNNING) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update task metrics
     pthread_mutex_lock(&component->metrics_lock);
     component->metrics.total_tasks_received++;
     pthread_mutex_unlock(&component->metrics_lock);
     
     pthread_mutex_unlock(&component->lock);
     
     // Fire task received event
     fire_component_event(component, EDGE_COMPONENT_EVENT_TASK_RECEIVED, task_data, task_size);
     
     // Submit task to runtime for async execution
     return polycall_edge_runtime_submit_task(
         component->runtime_ctx,
         task_data,
         task_size,
         0, // Default priority
         callback,
         user_data,
         task_id
     );
 }
 
 /**
  * @brief Add a node to the component's node registry
  */
 polycall_core_error_t polycall_edge_component_add_node(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     const polycall_edge_node_metrics_t* node_metrics,
     const char* node_id
 ) {
     if (!core_ctx || !component || !node_metrics || !node_id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Register node with edge system
     polycall_core_error_t result = polycall_edge_register_node(
         component->edge_ctx,
         node_metrics,
         node_id
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         // Update metrics
         pthread_mutex_lock(&component->metrics_lock);
         component->metrics.total_nodes++;
         component->metrics.active_nodes++;
         pthread_mutex_unlock(&component->metrics_lock);
         
         // Fire node added event
         fire_component_event(component, EDGE_COMPONENT_EVENT_NODE_ADDED, node_id, strlen(node_id) + 1);
     }
     
     return result;
 }
 
 /**
  * @brief Remove a node from the component's registry
  */
 polycall_core_error_t polycall_edge_component_remove_node(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     const char* node_id
 ) {
     if (!core_ctx || !component || !node_id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Handle node failure in edge system
     polycall_core_error_t result = polycall_edge_handle_node_failure(
         component->edge_ctx,
         node_id
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         // Update metrics
         pthread_mutex_lock(&component->metrics_lock);
         component->metrics.active_nodes--;
         component->metrics.failed_nodes++;
         pthread_mutex_unlock(&component->metrics_lock);
         
         // Fire node removed event
         fire_component_event(component, EDGE_COMPONENT_EVENT_NODE_REMOVED, node_id, strlen(node_id) + 1);
     }
     
     return result;
 }
 
 /**
  * @brief Start node auto-discovery
  */
 polycall_core_error_t polycall_edge_component_start_discovery(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check if discovery is already active
     if (component->discovery.is_active) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Check component status
     if (component->status != EDGE_COMPONENT_STATUS_RUNNING) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Initialize discovery parameters
     component->discovery.should_terminate = false;
     component->discovery.discovery_port = component->config.discovery_port;
     
     // Create discovery thread
     if (pthread_create(&component->discovery.discovery_thread, NULL, 
                       discovery_thread_func, component) != 0) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     pthread_mutex_unlock(&component->lock);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Stop node auto-discovery
  */
 polycall_core_error_t polycall_edge_component_stop_discovery(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check if discovery is active
     if (!component->discovery.is_active) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Signal thread to terminate
     component->discovery.should_terminate = true;
     pthread_mutex_unlock(&component->lock);
     
     // Wait for thread to exit
     pthread_join(component->discovery.discovery_thread, NULL);
     
     // Update discovery state
     pthread_mutex_lock(&component->lock);
     component->discovery.is_active = false;
     pthread_mutex_unlock(&component->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get component metrics
  */
 polycall_core_error_t polycall_edge_component_get_metrics(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     polycall_edge_component_metrics_t* metrics
 ) {
     if (!core_ctx || !component || !metrics) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->metrics_lock);
     memcpy(metrics, &component->metrics, sizeof(polycall_edge_component_metrics_t));
     pthread_mutex_unlock(&component->metrics_lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get list of all nodes in the component's registry
  */
 polycall_core_error_t polycall_edge_component_get_nodes(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     char** node_ids,
     size_t* node_count
 ) {
     // Implementation would typically retrieve nodes from the underlying edge context
     // This is a placeholder for the actual implementation
     
     if (!core_ctx || !component || !node_ids || !node_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // This would be implemented by querying the node selector from the edge context
     // For now, return empty list
     *node_count = 0;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get node metrics for a specific node
  */
 polycall_core_error_t polycall_edge_component_get_node_metrics(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     const char* node_id,
     polycall_edge_node_metrics_t* metrics
 ) {
     if (!core_ctx || !component || !node_id || !metrics) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Forward to edge system
     return polycall_edge_get_node_metrics(
         component->edge_ctx,
         node_id,
         metrics
     );
 }
 
 /**
  * @brief Update component configuration
  */
 polycall_core_error_t polycall_edge_component_update_config(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     const polycall_edge_component_config_t* config
 ) {
     if (!core_ctx || !component || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     
     // Check if component is in a state where configuration can be updated
     if (component->status != EDGE_COMPONENT_STATUS_INITIALIZED && 
         component->status != EDGE_COMPONENT_STATUS_STOPPED) {
         pthread_mutex_unlock(&component->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update configuration
     memcpy(&component->config, config, sizeof(polycall_edge_component_config_t));
     
     pthread_mutex_unlock(&component->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get component configuration
  */
 polycall_core_error_t polycall_edge_component_get_config(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component,
     polycall_edge_component_config_t* config
 ) {
     if (!core_ctx || !component || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&component->lock);
     memcpy(config, &component->config, sizeof(polycall_edge_component_config_t));
     pthread_mutex_unlock(&component->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default component configuration
  */
 polycall_edge_component_config_t polycall_edge_component_default_config(void) {
     polycall_edge_component_config_t config;
     memset(&config, 0, sizeof(polycall_edge_component_config_t));
     
     // Set basic component defaults
     config.component_name = NULL;
     config.component_id = NULL;
     config.type = EDGE_COMPONENT_TYPE_COMPUTE;
     config.task_policy = EDGE_TASK_POLICY_QUEUE;
     config.isolation = POLYCALL_ISOLATION_MEMORY;
     
     // Resource defaults
     config.max_memory_mb = 512;
     config.max_tasks = 100;
     config.max_nodes = 16;
     config.task_timeout_ms = 5000;
     
     // Networking defaults
     config.discovery_port = 7700;
     config.command_port = 7701;
     config.data_port = 7702;
     config.enable_auto_discovery = true;
     
     // Create default configurations for sub-components
     polycall_edge_create_default_config(
         &config.router_config,
         &config.fallback_config,
         &config.security_config
     );
     
     // Set runtime defaults
     config.runtime_config = polycall_edge_runtime_default_config();
     
     // Advanced settings
     config.enable_telemetry = true;
     config.enable_load_balancing = true;
     config.enable_dynamic_scaling = false;
     config.log_path = NULL;
     config.user_data = NULL;
     
     return config;
 }
 
 /**
  * @brief Destroy edge component and release resources
  */
 void polycall_edge_component_destroy(
     polycall_core_context_t* core_ctx,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !component) {
         return;
     }
     
     // Stop component if running
     if (component->status == EDGE_COMPONENT_STATUS_RUNNING || 
         component->status == EDGE_COMPONENT_STATUS_PAUSED) {
         polycall_edge_component_stop(core_ctx, component);
     }
     
     // Cleanup edge runtime
     if (component->runtime_ctx) {
         polycall_edge_runtime_cleanup(core_ctx, component->runtime_ctx);
     }
     
     // Cleanup edge context
     if (component->edge_ctx) {
         polycall_edge_cleanup(core_ctx, component->edge_ctx);
     }
     
     // Cleanup event callbacks
     cleanup_event_callbacks(core_ctx, component);
     
     // Free component name and ID
     if (component->component_name) {
         polycall_core_free(core_ctx, component->component_name);
     }
     
     if (component->component_id) {
         polycall_core_free(core_ctx, component->component_id);
     }
     
     // Destroy mutexes
     pthread_mutex_destroy(&component->lock);
     pthread_mutex_destroy(&component->metrics_lock);
     
     // Free component structure
     polycall_core_free(core_ctx, component);
 }