/**
#include "polycall/core/telemetry/polycall_telemetry.h"

 * @file polycall_telemetry.c
 * @brief Core Telemetry Infrastructure Implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's Aegis Design Principles
 *
 * Implements a comprehensive, pattern-based telemetry collection and 
 * reporting mechanism aligned with the Program-First design approach.
 */

 #include "polycall/core/telemetry/polycall_telemetry.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <pthread.h>
 #include <time.h>
 
 #define POLYCALL_TELEMETRY_MAGIC 0xA3D1E2F4
 #define MAX_EVENT_CALLBACKS 16
 #define MAX_QUEUE_BUFFER 1024
 
 // Internal telemetry context structure
 struct polycall_telemetry_context {
     uint32_t magic;                        // Magic number for validation
     polycall_telemetry_config_t config;    // Telemetry configuration
     
     // Event queue management
     polycall_telemetry_event_t* event_queue;
     size_t queue_head;
     size_t queue_tail;
     size_t queue_size;
     
     // Callback management
     struct {
         polycall_telemetry_callback_t callback;
         void* user_data;
     } callbacks[MAX_EVENT_CALLBACKS];
     size_t callback_count;
     
     // Synchronization primitives
     pthread_mutex_t queue_mutex;
     pthread_cond_t queue_condition;
     bool is_active;
     
     // Compression and encryption contexts (placeholder)
     void* compression_ctx;
     void* encryption_ctx;
 };
 
 /**
  * @brief Validate telemetry context integrity
  */
 static bool validate_telemetry_context(polycall_telemetry_context_t* ctx) {
     return ctx && ctx->magic == POLYCALL_TELEMETRY_MAGIC;
 }
 
 /**
  * @brief Generate a timestamp using high-resolution monotonic clock
  */
 static uint64_t generate_timestamp(void) {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
 }
 
 /**
  * @brief Initialize telemetry system
  */
 polycall_core_error_t polycall_telemetry_init(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_context_t** telemetry_ctx,
     const polycall_telemetry_config_t* config
 ) {
     if (!core_ctx || !telemetry_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     // Disable telemetry if configured
     if (!config->enable_telemetry) {
         *telemetry_ctx = NULL;
         return POLYCALL_CORE_SUCCESS;
     }
 
     // Allocate telemetry context
     polycall_telemetry_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_telemetry_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_telemetry_context_t));
     
     // Set magic number for validation
     new_ctx->magic = POLYCALL_TELEMETRY_MAGIC;
     
     // Copy configuration
     memcpy(&new_ctx->config, config, sizeof(polycall_telemetry_config_t));
     
     // Copy identifier format from core context
     new_ctx->id_format = core_ctx->default_id_format;
     
     // Allocate event queue
     new_ctx->event_queue = polycall_core_malloc(
         core_ctx, 
         config->max_event_queue_size * sizeof(polycall_telemetry_event_t)
     );
     
     if (!new_ctx->event_queue) {
         polycall_core_free(core_ctx, new_ctx);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize synchronization primitives
     pthread_mutex_init(&new_ctx->queue_mutex, NULL);
     pthread_cond_init(&new_ctx->queue_condition, NULL);
     new_ctx->is_active = true;
     
     *telemetry_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 /**
    * @brief Record a telemetry event with GUID generation
    */
polycall_core_error_t polycall_telemetry_record_event(
    polycall_telemetry_context_t* telemetry_ctx,
    const polycall_telemetry_event_t* event
) {
    if (!validate_telemetry_context(telemetry_ctx) || !event) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Check if event meets minimum severity
    if (event->severity < telemetry_ctx->config.min_severity) {
        return POLYCALL_CORE_SUCCESS;
    }

    pthread_mutex_lock(&telemetry_ctx->queue_mutex);

    // Check if queue is full
    if (telemetry_ctx->queue_size >= telemetry_ctx->config.max_event_queue_size) {
        pthread_mutex_unlock(&telemetry_ctx->queue_mutex);
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }

    // Copy event to queue
    polycall_telemetry_event_t* queue_event = 
        &telemetry_ctx->event_queue[telemetry_ctx->queue_tail];
    
    memcpy(queue_event, event, sizeof(polycall_telemetry_event_t));
    
    // Override timestamp with current time
    queue_event->timestamp = generate_timestamp();
    
    // Generate new GUID or update existing one if parent is specified
    if (polycall_guid_validate(telemetry_ctx->core_ctx, event->parent_guid)) {
        // This is a state transition from a parent event
        queue_event->event_guid = polycall_update_guid_state(
            telemetry_ctx->core_ctx,
            event->parent_guid,
            telemetry_ctx->current_state_id,
            event->event_id
        );
    } else if (!polycall_guid_validate(telemetry_ctx->core_ctx, event->event_guid)) {
        // No valid GUID provided, generate a new one
        queue_event->event_guid = polycall_generate_cryptonomic_guid(
            telemetry_ctx->core_ctx,
            event->source_module,
            telemetry_ctx->current_state_id,
            NULL // User identity would be provided in authenticated contexts
        );
    }

    // Update queue state
    telemetry_ctx->queue_tail = 
        (telemetry_ctx->queue_tail + 1) % telemetry_ctx->config.max_event_queue_size;
    telemetry_ctx->queue_size++;

    // Signal any waiting threads
    pthread_cond_signal(&telemetry_ctx->queue_condition);

    // Invoke registered callbacks
    for (size_t i = 0; i < telemetry_ctx->callback_count; i++) {
        if (telemetry_ctx->callbacks[i].callback) {
            telemetry_ctx->callbacks[i].callback(
                queue_event, 
                telemetry_ctx->callbacks[i].user_data
            );
        }
    }

    pthread_mutex_unlock(&telemetry_ctx->queue_mutex);

    return POLYCALL_CORE_SUCCESS;
}
 
 /**
  * @brief Register a telemetry event callback
  */
 polycall_core_error_t polycall_telemetry_register_callback(
     polycall_telemetry_context_t* telemetry_ctx,
     polycall_telemetry_callback_t callback,
     void* user_data
 ) {
     if (!validate_telemetry_context(telemetry_ctx) || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     pthread_mutex_lock(&telemetry_ctx->queue_mutex);
 
     // Check for callback capacity
     if (telemetry_ctx->callback_count >= MAX_EVENT_CALLBACKS) {
         pthread_mutex_unlock(&telemetry_ctx->queue_mutex);
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
 
     // Register callback
     telemetry_ctx->callbacks[telemetry_ctx->callback_count].callback = callback;
     telemetry_ctx->callbacks[telemetry_ctx->callback_count].user_data = user_data;
     telemetry_ctx->callback_count++;
 
     pthread_mutex_unlock(&telemetry_ctx->queue_mutex);
 
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default telemetry configuration
  */
 polycall_telemetry_config_t polycall_telemetry_create_default_config(void) {
     polycall_telemetry_config_t default_config = {
         .enable_telemetry = true,
         .min_severity = POLYCALL_TELEMETRY_INFO,
         .max_event_queue_size = 1024,
         .enable_encryption = false,
         .enable_compression = false,
         .log_file_path = "/var/log/polycall_telemetry.log",
         .log_rotation_size_mb = 10
     };
 
     return default_config;
 }
 
 /**
  * @brief Cleanup telemetry system
  */
 void polycall_telemetry_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_context_t* telemetry_ctx
 ) {
     if (!validate_telemetry_context(telemetry_ctx)) {
         return;
     }
 
     // Signal thread to stop and wait for completion
     pthread_mutex_lock(&telemetry_ctx->queue_mutex);
     telemetry_ctx->is_active = false;
     pthread_cond_signal(&telemetry_ctx->queue_condition);
     pthread_mutex_unlock(&telemetry_ctx->queue_mutex);
 
     // Destroy synchronization primitives
     pthread_mutex_destroy(&telemetry_ctx->queue_mutex);
     pthread_cond_destroy(&telemetry_ctx->queue_condition);
 
     // Free resources
     if (telemetry_ctx->event_queue) {
         polycall_core_free(core_ctx, telemetry_ctx->event_queue);
     }
 
     // Invalidate magic number
     telemetry_ctx->magic = 0;
 
     // Free context
     polycall_core_free(core_ctx, telemetry_ctx);
 }
    