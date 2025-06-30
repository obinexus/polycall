// command_tracking.c - Implementation of command tracking system
#include "polycall/core/protocol/command_tracking.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <string.h>
#include <time.h>

/**
 * @brief Initializes the protocol context with routing and command tracking capabilities
 *
 * This function sets up the core protocol functionality by:
 * 1. Initializing route mapping using the protocol's state machine
 * 2. Setting up command tracking with specified capacity
 * 3. Storing the initialized components in the protocol context
 *
 * @param ctx The core context pointer
 * @param proto_ctx The protocol context to be initialized
 *
 * @return POLYCALL_CORE_SUCCESS on successful initialization
 *         Other error codes if initialization of route mapping or command tracking fails
 *
 * @note The function automatically handles cleanup of allocated resources if any step fails
 * @note Default command tracking capacity is set to 1024 commands
 *
 * @warning Both ctx and proto_ctx must be valid pointers
 * @warning proto_ctx must have valid state_machine and telemetry_ctx members
 */
// 
/**
 * @brief Generate current timestamp in milliseconds
 */
static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/**
 * @brief Initialize command tracking
 */
polycall_core_error_t polycall_command_tracking_init(
    polycall_core_context_t* ctx,
    polycall_command_tracking_t** tracking,
    polycall_telemetry_context_t* telemetry_ctx,
    size_t capacity
) {
    if (!ctx || !tracking) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (capacity == 0) {
        capacity = 1024;  // Default capacity
    }
    
    // Allocate tracking context
    polycall_command_tracking_t* new_tracking = polycall_core_malloc(ctx, sizeof(polycall_command_tracking_t));
    if (!new_tracking) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize context
    memset(new_tracking, 0, sizeof(polycall_command_tracking_t));
    new_tracking->telemetry_ctx = telemetry_ctx;
    new_tracking->capacity = capacity;
    
    // Allocate entries array
    new_tracking->entries = polycall_core_malloc(ctx, capacity * sizeof(polycall_command_tracking_entry_t));
    if (!new_tracking->entries) {
        polycall_core_free(ctx, new_tracking);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize entries
    memset(new_tracking->entries, 0, capacity * sizeof(polycall_command_tracking_entry_t));
    new_tracking->entry_count = 0;
    
    *tracking = new_tracking;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Generate a unique correlation ID
 */
static uint64_t generate_correlation_id(void) {
    // Combine current time with a random component
    uint64_t time_component = get_current_time_ms();
    uint64_t random_component = ((uint64_t)rand() << 32) | rand();
    
    return time_component ^ random_component;
}

/**
 * @brief Begin tracking command execution
 */
uint64_t polycall_command_tracking_begin(
    polycall_core_context_t* ctx,
    polycall_command_tracking_t* tracking,
    uint32_t command_id,
    polycall_protocol_state_t state
) {
    if (!ctx || !tracking) {
        return 0;
    }
    
    // Check if we have space for a new entry
    if (tracking->entry_count >= tracking->capacity) {
        // For simplicity, we'll just wrap around and overwrite the oldest entry
        // In a real implementation, you might want to resize the array or use a circular buffer
        tracking->entry_count = 0;
    }
    
    // Generate correlation ID
    uint64_t correlation_id = generate_correlation_id();
    
    // Add tracking entry
    polycall_command_tracking_entry_t* entry = &tracking->entries[tracking->entry_count++];
    entry->correlation_id = correlation_id;
    entry->command_id = command_id;
    entry->timestamp = get_current_time_ms();
    entry->state = state;
    entry->completed = false;
    entry->result_code = 0;
    
    // Record telemetry if available
    if (tracking->telemetry_ctx) {
        polycall_telemetry_event_t event = {
            .timestamp = 0, // Will be set by telemetry system
            .severity = POLYCALL_TELEMETRY_INFO,
            .category = TELEMETRY_CATEGORY_PROTOCOL,
            .source_module = "command_tracking",
            .event_id = "command_execution_begin",
            .description = "Command execution started",
            .additional_data = &correlation_id,
            .additional_data_size = sizeof(correlation_id)
        };
        
        polycall_telemetry_record_event(tracking->telemetry_ctx, &event);
    }
    
    return correlation_id;
}

/**
 * @brief End tracking command execution
 */
polycall_core_error_t polycall_command_tracking_end(
    polycall_core_context_t* ctx,
    polycall_command_tracking_t* tracking,
    uint64_t correlation_id,
    uint32_t result_code
) {
    if (!ctx || !tracking || correlation_id == 0) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find tracking entry
    polycall_command_tracking_entry_t* entry = NULL;
    for (size_t i = 0; i < tracking->entry_count; i++) {
        if (tracking->entries[i].correlation_id == correlation_id) {
            entry = &tracking->entries[i];
            break;
        }
    }
    
    if (!entry) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Update tracking entry
    entry->completed = true;
    entry->result_code = result_code;
    
    // Record telemetry if available
    if (tracking->telemetry_ctx) {
        polycall_telemetry_event_t event = {
            .timestamp = 0, // Will be set by telemetry system
            .severity = POLYCALL_TELEMETRY_INFO,
            .category = TELEMETRY_CATEGORY_PROTOCOL,
            .source_module = "command_tracking",
            .event_id = "command_execution_end",
            .description = "Command execution completed",
            .additional_data = &correlation_id,
            .additional_data_size = sizeof(correlation_id)
        };
        
        polycall_telemetry_record_event(tracking->telemetry_ctx, &event);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get tracking entry by correlation ID
 */
polycall_command_tracking_entry_t* polycall_command_tracking_get(
    polycall_core_context_t* ctx,
    polycall_command_tracking_t* tracking,
    uint64_t correlation_id
) {
    if (!ctx || !tracking || correlation_id == 0) {
        return NULL;
    }
    
    // Find tracking entry
    for (size_t i = 0; i < tracking->entry_count; i++) {
        if (tracking->entries[i].correlation_id == correlation_id) {
            return &tracking->entries[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Cleanup command tracking
 */
void polycall_command_tracking_cleanup(
    polycall_core_context_t* ctx,
    polycall_command_tracking_t* tracking
) {
    if (!ctx || !tracking) {
        return;
    }
    
    // Free entries array
    if (tracking->entries) {
        polycall_core_free(ctx, tracking->entries);
    }
    
    // Free tracking context
    polycall_core_free(ctx, tracking);
}

