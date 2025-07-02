/**
#include "polycall/core/edge/edge_runtime.h"

 * @file edge_runtime.c
 * @brief Edge Computing Runtime Environment Implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements the edge computing runtime that manages execution of distributed
 * computational tasks, providing task scheduling, resource management, and
 * execution coordination.
 */

 #include "polycall/core/edge/edge_runtime.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <pthread.h>
 #include <time.h>
 
 /**
  * @brief Task handler registry entry
  */
 typedef struct {
     uint32_t task_type;
     void (*handler)(void* task_data, size_t task_size, void* result_buffer, size_t* result_size, void* user_data);
     void* user_data;
 } task_handler_entry_t;
 
 /**
  * @brief Edge runtime task queue
  */
 typedef struct {
     polycall_edge_runtime_task_t* tasks;
     size_t capacity;
     size_t count;
     size_t head;
     size_t tail;
     pthread_mutex_t lock;
     pthread_cond_t not_empty;
     pthread_cond_t not_full;
 } task_queue_t;
 
 /**
  * @brief Edge runtime context structure
  */
 struct polycall_edge_runtime_context {
     polycall_core_context_t* core_ctx;
     char node_id[64];
     polycall_edge_runtime_config_t config;
     
     // Task queue
     task_queue_t task_queue;
     
     // Task registry
     polycall_edge_runtime_task_t* active_tasks[POLYCALL_EDGE_MAX_CONCURRENT_TASKS];
     uint32_t active_task_count;
     
     // Task handlers
     task_handler_entry_t* task_handlers;
     size_t handler_count;
     size_t handler_capacity;
     
     // Runtime statistics
     struct {
         uint64_t total_tasks;
         uint64_t completed_tasks;
         uint64_t failed_tasks;
         uint64_t total_execution_time_ms;
         float avg_execution_time_ms;
     } stats;
     
     // Worker thread management
     pthread_t* worker_threads;
     size_t worker_count;
     bool shutdown_requested;
     pthread_mutex_t worker_lock;
     
     // Task ID generation
     uint64_t next_task_id;
     pthread_mutex_t id_lock;
     
     // Current node metrics
     polycall_edge_node_metrics_t node_metrics;
 };
 
 /**
  * @brief Task worker thread function
  */
 static void* task_worker_thread(void* arg) {
     polycall_edge_runtime_context_t* runtime_ctx = (polycall_edge_runtime_context_t*)arg;
     
     while (!runtime_ctx->shutdown_requested) {
         // Dequeue a task
         polycall_edge_runtime_task_t* task = NULL;
         
         pthread_mutex_lock(&runtime_ctx->task_queue.lock);
         
         while (runtime_ctx->task_queue.count == 0 && !runtime_ctx->shutdown_requested) {
             pthread_cond_wait(&runtime_ctx->task_queue.not_empty, &runtime_ctx->task_queue.lock);
         }
         
         if (runtime_ctx->shutdown_requested) {
             pthread_mutex_unlock(&runtime_ctx->task_queue.lock);
             break;
         }
         
         // Get the task from the queue
         size_t index = runtime_ctx->task_queue.head;
         task = &runtime_ctx->task_queue.tasks[index];
         runtime_ctx->task_queue.head = (runtime_ctx->task_queue.head + 1) % runtime_ctx->task_queue.capacity;
         runtime_ctx->task_queue.count--;
         
         pthread_cond_signal(&runtime_ctx->task_queue.not_full);
         pthread_mutex_unlock(&runtime_ctx->task_queue.lock);
         
         // Execute the task
         if (task) {
             // Record start time
             task->start_timestamp = (uint64_t)time(NULL) * 1000;
             task->state = EDGE_TASK_STATE_RUNNING;
             
             // Allocate result buffer
             size_t result_size = task->task_size;  // Default to same size as input
             void* result_buffer = polycall_core_malloc(runtime_ctx->core_ctx, result_size);
             
             if (!result_buffer) {
                 task->state = EDGE_TASK_STATE_FAILED;
                 
                 // Invoke callback with failure
                 if (task->callback) {
                     task->callback(NULL, 0, EDGE_TASK_STATE_FAILED, &task->metrics, task->user_data);
                 }
                 
                 continue;
             }
             
             // Simple task execution (placeholder)
             // In a real implementation, this would use registered task handlers
             memcpy(result_buffer, task->task_data, task->task_size);
             
             // Update task metrics
             task->completion_timestamp = (uint64_t)time(NULL) * 1000;
             task->metrics.execution_time_ms = task->completion_timestamp - task->start_timestamp;
             task->metrics.queue_time_ms = task->start_timestamp - task->creation_timestamp;
             task->metrics.cpu_utilization = 0.5f;  // Placeholder
             task->metrics.memory_utilization = 0.3f;  // Placeholder
             
             // Update task state
             task->state = EDGE_TASK_STATE_COMPLETED;
             
             // Update statistics
             pthread_mutex_lock(&runtime_ctx->worker_lock);
             runtime_ctx->stats.completed_tasks++;
             runtime_ctx->stats.total_execution_time_ms += task->metrics.execution_time_ms;
             runtime_ctx->stats.avg_execution_time_ms = 
                 (float)runtime_ctx->stats.total_execution_time_ms / runtime_ctx->stats.completed_tasks;
             pthread_mutex_unlock(&runtime_ctx->worker_lock);
             
             // Invoke callback
             if (task->callback) {
                 task->callback(result_buffer, result_size, EDGE_TASK_STATE_COMPLETED, 
                                &task->metrics, task->user_data);
             }
             
             // Cleanup
             polycall_core_free(runtime_ctx->core_ctx, result_buffer);
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Initialize task queue
  */
 static polycall_core_error_t init_task_queue(
     polycall_core_context_t* core_ctx,
     task_queue_t* queue,
     size_t capacity
 ) {
     if (!core_ctx || !queue || capacity == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate task array
     queue->tasks = polycall_core_malloc(core_ctx, sizeof(polycall_edge_runtime_task_t) * capacity);
     if (!queue->tasks) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize queue properties
     queue->capacity = capacity;
     queue->count = 0;
     queue->head = 0;
     queue->tail = 0;
     
     // Initialize synchronization primitives
     pthread_mutex_init(&queue->lock, NULL);
     pthread_cond_init(&queue->not_empty, NULL);
     pthread_cond_init(&queue->not_full, NULL);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up task queue
  */
 static void cleanup_task_queue(
     polycall_core_context_t* core_ctx,
     task_queue_t* queue
 ) {
     if (!core_ctx || !queue) {
         return;
     }
     
     // Free all task data
     if (queue->tasks) {
         for (size_t i = 0; i < queue->capacity; i++) {
             if (queue->tasks[i].task_data) {
                 polycall_core_free(core_ctx, queue->tasks[i].task_data);
                 queue->tasks[i].task_data = NULL;
             }
         }
         
         polycall_core_free(core_ctx, queue->tasks);
         queue->tasks = NULL;
     }
     
     // Destroy synchronization primitives
     pthread_mutex_destroy(&queue->lock);
     pthread_cond_destroy(&queue->not_empty);
     pthread_cond_destroy(&queue->not_full);
 }
 
 /**
  * @brief Initialize the edge runtime environment
  */
 polycall_core_error_t polycall_edge_runtime_init(
     polycall_core_context_t* core_ctx,
     polycall_edge_runtime_context_t** runtime_ctx,
     const char* node_id,
     const polycall_edge_runtime_config_t* config
 ) {
     if (!core_ctx || !runtime_ctx || !node_id || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate runtime context
     polycall_edge_runtime_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_edge_runtime_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_edge_runtime_context_t));
     new_ctx->core_ctx = core_ctx;
     
     // Copy node ID
     strncpy(new_ctx->node_id, node_id, sizeof(new_ctx->node_id) - 1);
     
     // Copy configuration
     memcpy(&new_ctx->config, config, sizeof(polycall_edge_runtime_config_t));
     
     // Initialize task queue
     uint32_t queue_size = config->task_queue_size;
     if (queue_size == 0 || queue_size > POLYCALL_EDGE_MAX_TASK_QUEUE_SIZE) {
         queue_size = POLYCALL_EDGE_MAX_TASK_QUEUE_SIZE;
     }
     
     polycall_core_error_t result = init_task_queue(core_ctx, &new_ctx->task_queue, queue_size);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(core_ctx, new_ctx);
         return result;
     }
     
     // Initialize task handlers
     new_ctx->handler_capacity = 16;  // Initial capacity
     new_ctx->task_handlers = polycall_core_malloc(core_ctx, 
                                                sizeof(task_handler_entry_t) * new_ctx->handler_capacity);
     
     if (!new_ctx->task_handlers) {
         cleanup_task_queue(core_ctx, &new_ctx->task_queue);
         polycall_core_free(core_ctx, new_ctx);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize mutex
     pthread_mutex_init(&new_ctx->worker_lock, NULL);
     pthread_mutex_init(&new_ctx->id_lock, NULL);
     
     // Initialize worker threads
     uint32_t worker_count = config->max_concurrent_tasks;
     if (worker_count == 0 || worker_count > POLYCALL_EDGE_MAX_CONCURRENT_TASKS) {
         worker_count = POLYCALL_EDGE_MAX_CONCURRENT_TASKS;
     }
     
     new_ctx->worker_count = worker_count;
     new_ctx->worker_threads = polycall_core_malloc(core_ctx, sizeof(pthread_t) * worker_count);
     
     if (!new_ctx->worker_threads) {
         cleanup_task_queue(core_ctx, &new_ctx->task_queue);
         polycall_core_free(core_ctx, new_ctx->task_handlers);
         pthread_mutex_destroy(&new_ctx->worker_lock);
         pthread_mutex_destroy(&new_ctx->id_lock);
         polycall_core_free(core_ctx, new_ctx);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Create worker threads
     for (size_t i = 0; i < worker_count; i++) {
         if (pthread_create(&new_ctx->worker_threads[i], NULL, task_worker_thread, new_ctx) != 0) {
             // Failed to create thread
             new_ctx->worker_count = i;  // Only count successfully created threads
             
             // Shutdown worker threads
             new_ctx->shutdown_requested = true;
             pthread_cond_broadcast(&new_ctx->task_queue.not_empty);
             
             for (size_t j = 0; j < i; j++) {
                 pthread_join(new_ctx->worker_threads[j], NULL);
             }
             
             // Clean up
             cleanup_task_queue(core_ctx, &new_ctx->task_queue);
             polycall_core_free(core_ctx, new_ctx->task_handlers);
             polycall_core_free(core_ctx, new_ctx->worker_threads);
             pthread_mutex_destroy(&new_ctx->worker_lock);
             pthread_mutex_destroy(&new_ctx->id_lock);
             polycall_core_free(core_ctx, new_ctx);
             
             return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
         }
     }
     
     *runtime_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Generate a unique task ID
  */
 static uint64_t generate_task_id(polycall_edge_runtime_context_t* runtime_ctx) {
     pthread_mutex_lock(&runtime_ctx->id_lock);
     uint64_t task_id = ++runtime_ctx->next_task_id;
     pthread_mutex_unlock(&runtime_ctx->id_lock);
     return task_id;
 }
 
 /**
  * @brief Submit a task to the edge runtime for execution
  */
 polycall_core_error_t polycall_edge_runtime_submit_task(
     polycall_edge_runtime_context_t* runtime_ctx,
     const void* task_data,
     size_t task_size,
     uint8_t priority,
     polycall_edge_runtime_task_callback_t callback,
     void* user_data,
     uint64_t* task_id
 ) {
     if (!runtime_ctx || !task_data || task_size == 0 || !task_id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock the task queue
     pthread_mutex_lock(&runtime_ctx->task_queue.lock);
     
     // Check if queue is full
     while (runtime_ctx->task_queue.count == runtime_ctx->task_queue.capacity) {
         pthread_cond_wait(&runtime_ctx->task_queue.not_full, &runtime_ctx->task_queue.lock);
     }
     
     // Generate task ID
     *task_id = generate_task_id(runtime_ctx);
     
     // Get task entry
     size_t index = runtime_ctx->task_queue.tail;
     polycall_edge_runtime_task_t* task = &runtime_ctx->task_queue.tasks[index];
     
     // Allocate task data
     task->task_data = polycall_core_malloc(runtime_ctx->core_ctx, task_size);
     if (!task->task_data) {
         pthread_mutex_unlock(&runtime_ctx->task_queue.lock);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize task
     memcpy(task->task_data, task_data, task_size);
     task->task_size = task_size;
     task->state = EDGE_TASK_STATE_QUEUED;
     task->callback = callback;
     task->user_data = user_data;
     task->error_policy = EDGE_RUNTIME_ON_ERROR_ABORT;  // Default policy
     task->priority = priority;
     task->max_retries = 0;
     task->retry_count = 0;
     memset(&task->metrics, 0, sizeof(polycall_edge_task_metrics_t));
     task->task_id = *task_id;
     task->creation_timestamp = (uint64_t)time(NULL) * 1000;
     task->start_timestamp = 0;
     task->completion_timestamp = 0;
     
     // Update queue
     runtime_ctx->task_queue.tail = (runtime_ctx->task_queue.tail + 1) % runtime_ctx->task_queue.capacity;
     runtime_ctx->task_queue.count++;
     
     // Update statistics
     pthread_mutex_lock(&runtime_ctx->worker_lock);
     runtime_ctx->stats.total_tasks++;
     pthread_mutex_unlock(&runtime_ctx->worker_lock);
     
     // Signal worker threads
     pthread_cond_signal(&runtime_ctx->task_queue.not_empty);
     pthread_mutex_unlock(&runtime_ctx->task_queue.lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Find task by ID
  */
 static polycall_edge_runtime_task_t* find_task_by_id(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t task_id
 ) {
     pthread_mutex_lock(&runtime_ctx->task_queue.lock);
     
     // Search the queue
     size_t index = runtime_ctx->task_queue.head;
     for (size_t i = 0; i < runtime_ctx->task_queue.count; i++) {
         if (runtime_ctx->task_queue.tasks[index].task_id == task_id) {
             pthread_mutex_unlock(&runtime_ctx->task_queue.lock);
             return &runtime_ctx->task_queue.tasks[index];
         }
         
         index = (index + 1) % runtime_ctx->task_queue.capacity;
     }
     
     pthread_mutex_unlock(&runtime_ctx->task_queue.lock);
     
     // Check active tasks
     pthread_mutex_lock(&runtime_ctx->worker_lock);
     for (uint32_t i = 0; i < runtime_ctx->active_task_count; i++) {
         if (runtime_ctx->active_tasks[i]->task_id == task_id) {
             pthread_mutex_unlock(&runtime_ctx->worker_lock);
             return runtime_ctx->active_tasks[i];
         }
     }
     pthread_mutex_unlock(&runtime_ctx->worker_lock);
     
     return NULL;
 }
 
 /**
  * @brief Check the status of a submitted task
  */
 polycall_core_error_t polycall_edge_runtime_check_task(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t task_id,
     polycall_edge_task_state_t* task_state,
     polycall_edge_task_metrics_t* metrics
 ) {
     if (!runtime_ctx || !task_state) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the task
     polycall_edge_runtime_task_t* task = find_task_by_id(runtime_ctx, task_id);
     if (!task) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Return task state
     *task_state = task->state;
     
     // Return metrics if requested
     if (metrics) {
         memcpy(metrics, &task->metrics, sizeof(polycall_edge_task_metrics_t));
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cancel a submitted task
  */
 polycall_core_error_t polycall_edge_runtime_cancel_task(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t task_id
 ) {
     if (!runtime_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the task
     polycall_edge_runtime_task_t* task = find_task_by_id(runtime_ctx, task_id);
     if (!task) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Only queued tasks can be cancelled
     if (task->state != EDGE_TASK_STATE_QUEUED) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Mark task as aborted
     task->state = EDGE_TASK_STATE_ABORTED;
     
     // Update statistics
     pthread_mutex_lock(&runtime_ctx->worker_lock);
     runtime_ctx->stats.failed_tasks++;
     pthread_mutex_unlock(&runtime_ctx->worker_lock);
     
     // Invoke callback if available
     if (task->callback) {
         task->callback(NULL, 0, EDGE_TASK_STATE_ABORTED, &task->metrics, task->user_data);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Update edge runtime metrics and node status
  */
 polycall_core_error_t polycall_edge_runtime_update_metrics(
     polycall_edge_runtime_context_t* runtime_ctx,
     polycall_node_selector_context_t* selector_ctx
 ) {
     if (!runtime_ctx || !selector_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Calculate updated metrics based on current system state
     pthread_mutex_lock(&runtime_ctx->worker_lock);
     
     // Update CPU and memory utilization
     float cpu_utilization = (float)runtime_ctx->active_task_count / runtime_ctx->config.max_concurrent_tasks;
     if (cpu_utilization > 1.0f) {
         cpu_utilization = 1.0f;
     }
     
     // Calculate memory utilization (placeholder)
     float memory_utilization = 0.5f;  // Simple placeholder
     
     // Update node metrics
     runtime_ctx->node_metrics.current_load = cpu_utilization;
     runtime_ctx->node_metrics.active_cores = runtime_ctx->active_task_count;
     
     pthread_mutex_unlock(&runtime_ctx->worker_lock);
     
     // Update node selector with current metrics
     polycall_core_error_t result = polycall_node_selector_update_metrics(
         selector_ctx,
         runtime_ctx->node_id,
         &runtime_ctx->node_metrics
     );
     
     return result;
 }
 
 /**
  * @brief Get current runtime statistics
  */
 polycall_core_error_t polycall_edge_runtime_get_stats(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint64_t* total_tasks,
     uint64_t* completed_tasks,
     uint64_t* failed_tasks,
     uint64_t* avg_execution_time_ms
 ) {
     if (!runtime_ctx || !total_tasks || !completed_tasks || !failed_tasks || !avg_execution_time_ms) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&runtime_ctx->worker_lock);
     
     *total_tasks = runtime_ctx->stats.total_tasks;
     *completed_tasks = runtime_ctx->stats.completed_tasks;
     *failed_tasks = runtime_ctx->stats.failed_tasks;
     *avg_execution_time_ms = (uint64_t)runtime_ctx->stats.avg_execution_time_ms;
     
     pthread_mutex_unlock(&runtime_ctx->worker_lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register custom task type handler
  */
 polycall_core_error_t polycall_edge_runtime_register_handler(
     polycall_edge_runtime_context_t* runtime_ctx,
     uint32_t task_type,
     void (*handler)(void* task_data, size_t task_size, void* result_buffer, size_t* result_size, void* user_data),
     void* user_data
 ) {
     if (!runtime_ctx || !handler) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&runtime_ctx->worker_lock);
     
     // Check if handler already exists
     for (size_t i = 0; i < runtime_ctx->handler_count; i++) {
         if (runtime_ctx->task_handlers[i].task_type == task_type) {
             // Update existing handler
             runtime_ctx->task_handlers[i].handler = handler;
             runtime_ctx->task_handlers[i].user_data = user_data;
             
             pthread_mutex_unlock(&runtime_ctx->worker_lock);
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Ensure capacity
     if (runtime_ctx->handler_count >= runtime_ctx->handler_capacity) {
         size_t new_capacity = runtime_ctx->handler_capacity * 2;
         task_handler_entry_t* new_handlers = polycall_core_malloc(
             runtime_ctx->core_ctx,
             sizeof(task_handler_entry_t) * new_capacity
         );
         
         if (!new_handlers) {
             pthread_mutex_unlock(&runtime_ctx->worker_lock);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing handlers
         memcpy(new_handlers, runtime_ctx->task_handlers, 
                sizeof(task_handler_entry_t) * runtime_ctx->handler_count);
         
         // Update handlers array
         polycall_core_free(runtime_ctx->core_ctx, runtime_ctx->task_handlers);
         runtime_ctx->task_handlers = new_handlers;
         runtime_ctx->handler_capacity = new_capacity;
     }
     
     // Add new handler
     runtime_ctx->task_handlers[runtime_ctx->handler_count].task_type = task_type;
     runtime_ctx->task_handlers[runtime_ctx->handler_count].handler = handler;
     runtime_ctx->task_handlers[runtime_ctx->handler_count].user_data = user_data;
     runtime_ctx->handler_count++;
     
     pthread_mutex_unlock(&runtime_ctx->worker_lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default edge runtime configuration
  */
 polycall_edge_runtime_config_t polycall_edge_runtime_default_config(void) {
     polycall_edge_runtime_config_t config;
     
     config.max_concurrent_tasks = 4;
     config.task_queue_size = 64;
     config.enable_priority_scheduling = true;
     config.enable_task_preemption = false;
     config.task_time_slice_ms = 100;
     config.cpu_utilization_target = 0.8f;
     config.memory_utilization_target = 0.7f;
     config.custom_execution_context = NULL;
     
     return config;
 }
 
 /**
  * @brief Clean up edge runtime context
  */
 void polycall_edge_runtime_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_edge_runtime_context_t* runtime_ctx
 ) {
     if (!core_ctx || !runtime_ctx) {
         return;
     }
     
     // Signal worker threads to shut down
     runtime_ctx->shutdown_requested = true;
     pthread_cond_broadcast(&runtime_ctx->task_queue.not_empty);
     
     // Wait for worker threads to terminate
     for (size_t i = 0; i < runtime_ctx->worker_count; i++) {
         pthread_join(runtime_ctx->worker_threads[i], NULL);
     }
     
     // Free worker threads
     polycall_core_free(core_ctx, runtime_ctx->worker_threads);
     
     // Clean up task handlers
     if (runtime_ctx->task_handlers) {
         polycall_core_free(core_ctx, runtime_ctx->task_handlers);
     }
     
     // Clean up task queue
     cleanup_task_queue(core_ctx, &runtime_ctx->task_queue);
     
     // Destroy mutexes
     pthread_mutex_destroy(&runtime_ctx->worker_lock);
     pthread_mutex_destroy(&runtime_ctx->id_lock);
     
     // Free runtime context
     polycall_core_free(core_ctx, runtime_ctx);
 }