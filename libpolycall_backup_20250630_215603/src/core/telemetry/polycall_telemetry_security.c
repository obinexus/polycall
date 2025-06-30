/**
#include "polycall/core/telemetry/polycall_telemetry_security.h"

 * @file polycall_telemetry_security.c
 * @brief Advanced Security Telemetry Implementation for LibPolyCall
 * @author Nnamdi Okpala (OBINexus Computing)
 *
 * Implements comprehensive security event tracking and advanced 
 * threat detection mechanisms for distributed computing environments.
 */

 #include "polycall/core/telemetry/polycall_telemetry_security.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <pthread.h>
 
 #define POLYCALL_SECURITY_TELEMETRY_MAGIC 0xB4C5D6E7
 
 // Internal security telemetry context structure
 struct polycall_security_telemetry_context {
     uint32_t magic;                                    // Validation magic number
     polycall_security_telemetry_config_t config;       // Configuration settings
     
     // Event storage
     polycall_security_event_t* event_queue;            // Circular event queue
     size_t queue_head;                                 // Queue head index
     size_t queue_tail;                                 // Queue tail index
     size_t queue_size;                                 // Current queue size
     size_t max_queue_size;                             // Maximum queue size
     
     // Pattern matching
     polycall_security_pattern_t patterns[POLYCALL_MAX_SECURITY_PATTERNS];
     size_t pattern_count;
     
     // Callback management
     struct {
         polycall_security_event_callback_t callback;
         void* user_data;
     } callbacks[POLYCALL_MAX_SECURITY_CALLBACKS];
     size_t callback_count;
     
     // Synchronization primitives
     pthread_mutex_t queue_mutex;
     pthread_cond_t queue_condition;
     bool is_active;
 };
 
 /**
  * @brief Validation helper for security telemetry context
  */
 static bool validate_security_context(
     polycall_security_telemetry_context_t* ctx
 ) {
     return ctx && ctx->magic == POLYCALL_SECURITY_TELEMETRY_MAGIC;
 }
 
 /**
  * @brief Generate high-resolution timestamp
  */
 static uint64_t generate_timestamp(void) {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
 }
 
 /**
  * @brief Initialize security telemetry system
  */
 polycall_core_error_t polycall_security_telemetry_init(
     polycall_core_context_t* core_ctx,
     polycall_security_telemetry_context_t** telemetry_ctx,
     const polycall_security_telemetry_config_t* config
 ) {
     if (!core_ctx || !telemetry_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     // Disable telemetry if not configured
     if (!config->enable_security_tracking) {
         *telemetry_ctx = NULL;
         return POLYCALL_CORE_SUCCESS;
     }
 
     // Allocate security telemetry context
     polycall_security_telemetry_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_security_telemetry_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_security_telemetry_context_t));
     
     // Set magic number
     new_ctx->magic = POLYCALL_SECURITY_TELEMETRY_MAGIC;
     
     // Copy configuration
     memcpy(&new_ctx->config, config, sizeof(polycall_security_telemetry_config_t));
     
     // Set default queue size (can be customized later)
     new_ctx->max_queue_size = 1024;
     
     // Allocate event queue
     new_ctx->event_queue = polycall_core_malloc(
         core_ctx, 
         new_ctx->max_queue_size * sizeof(polycall_security_event_t)
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
  * @brief Record a security event
  */
 polycall_core_error_t polycall_security_telemetry_record_event(
     polycall_security_telemetry_context_t* telemetry_ctx,
     const polycall_security_event_t* event
 ) {
     if (!validate_security_context(telemetry_ctx) || !event) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     pthread_mutex_lock(&telemetry_ctx->queue_mutex);
 
     // Check if queue is full
     if (telemetry_ctx->queue_size >= telemetry_ctx->max_queue_size) {
         // Optional: Remove oldest event if queue is full
         telemetry_ctx->queue_head = 
             (telemetry_ctx->queue_head + 1) % telemetry_ctx->max_queue_size;
         telemetry_ctx->queue_size--;
     }
 
     // Copy event to queue
     polycall_security_event_t* queue_event = 
         &telemetry_ctx->event_queue[telemetry_ctx->queue_tail];
     
     memcpy(queue_event, event, sizeof(polycall_security_event_t));
     
     // Override timestamp with current time for consistency
     queue_event->timestamp = generate_timestamp();
 
     // Update queue state
     telemetry_ctx->queue_tail = 
         (telemetry_ctx->queue_tail + 1) % telemetry_ctx->max_queue_size;
     telemetry_ctx->queue_size++;
 
     // Check pattern matching
     if (telemetry_ctx->config.enable_pattern_matching) {
         for (size_t i = 0; i < telemetry_ctx->pattern_count; i++) {
             polycall_security_pattern_t* pattern = 
                 &telemetry_ctx->patterns[i];
             if (pattern->match_callback) {
                 pattern->match_callback(queue_event, pattern->user_data);
             }
         }
     }

        // Invoke registered callbacks
        for (size_t i = 0; i < telemetry_ctx->callback_count; i++) {
            if (telemetry_ctx->callbacks[i].callback) {
                telemetry_ctx->callbacks[i].callback(
                    queue_event, 
                    telemetry_ctx->callbacks[i].user_data
                );
            }
        }

        pthread_cond_signal(&telemetry_ctx->queue_condition);
        pthread_mutex_unlock(&telemetry_ctx->queue_mutex);
        return POLYCALL_CORE_SUCCESS;
    }

/**
 * 
 * @brief Register a security event callback
 */
polycall_core_error_t polycall_security_telemetry_register_callback(
    polycall_security_telemetry_context_t* telemetry_ctx,
    polycall_security_event_callback_t callback,
    void* user_data
) {
    if (!validate_security_context(telemetry_ctx) || !callback) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    pthread_mutex_lock(&telemetry_ctx->queue_mutex);

    // Check callback capacity
    if (telemetry_ctx->callback_count >= POLYCALL_MAX_SECURITY_CALLBACKS) {
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
 * @brief Generate security incident report
 */
polycall_core_error_t polycall_security_telemetry_generate_report(
    polycall_security_telemetry_context_t* telemetry_ctx,
    uint64_t start_time,
    uint64_t end_time,
    void* report_buffer,
    size_t buffer_size,
    size_t* required_size
) {
    if (!validate_security_context(telemetry_ctx) || !required_size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Placeholder for report generation logic
    // In a full implementation, this would:
    // 1. Filter events based on time range
    // 2. Aggregate statistics based on report type
    // 3. Generate comprehensive report

    size_t total_report_size = sizeof(polycall_security_report_header_t);
    *required_size = total_report_size;

    if (buffer_size < total_report_size) {
        return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
    }

    polycall_security_report_header_t* report_header = 
        (polycall_security_report_header_t*)report_buffer;

    report_header->report_type = POLYCALL_SECURITY_REPORT_TYPE_INCIDENT;
    report_header->start_timestamp = start_time;
    report_header->end_timestamp = end_time;
    report_header->generation_timestamp = generate_timestamp();

    return POLYCALL_CORE_SUCCESS;
}
/**
 * @brief Reset security context tracking
 */
polycall_core_error_t polycall_security_telemetry_reset(
    polycall_security_telemetry_context_t* telemetry_ctx
) {
    if (!validate_security_context(telemetry_ctx)) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    pthread_mutex_lock(&telemetry_ctx->queue_mutex);

    // Reset consecutive failure counts
    for (size_t i = 0; i < telemetry_ctx->pattern_count; i++) {
        telemetry_ctx->patterns[i].consecutive_failures = 0;
    }

    pthread_mutex_unlock(&telemetry_ctx->queue_mutex);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup security telemetry resources
 */
void polycall_security_telemetry_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_security_telemetry_context_t* telemetry_ctx
) {
    if (!core_ctx || !validate_security_context(telemetry_ctx)) {
        return;
    }

    // Deactivate context
    telemetry_ctx->is_active = false;
    
    // Signal any waiting threads
    pthread_cond_signal(&telemetry_ctx->queue_condition);
    
    // Destroy synchronization primitives
    pthread_mutex_destroy(&telemetry_ctx->queue_mutex);
    pthread_cond_destroy(&telemetry_ctx->queue_condition);
    
    // Clear sensitive data from event queue
    if (telemetry_ctx->event_queue) {
        memset(telemetry_ctx->event_queue, 0, 
               telemetry_ctx->max_queue_size * sizeof(polycall_security_event_t));
        polycall_core_free(core_ctx, telemetry_ctx->event_queue);
    }

    // Zero out callbacks and patterns for security
    memset(telemetry_ctx->callbacks, 0, 
           sizeof(telemetry_ctx->callbacks));
    memset(telemetry_ctx->patterns, 0,
           sizeof(telemetry_ctx->patterns));
    
    // Clear magic number to invalidate context
    telemetry_ctx->magic = 0;
    
    // Free the context itself
    polycall_core_free(core_ctx, telemetry_ctx);
}
    