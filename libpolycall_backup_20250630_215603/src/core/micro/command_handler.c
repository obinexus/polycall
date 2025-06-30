/**
#include "polycall/core/micro/command_handler.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/micro/polycall_micro_command.h"
#include "polycall/core/micro/polycall_micro_component.h"
#include "polycall/core/micro/polycall_micro_context.h"
#include "polycall/core/micro/polycall_micro_resource.h"
#include "polycall/core/micro/polycall_micro_security.h"
 * @file command_handler.c
 * @brief Command handler implementation for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the command handling system for the LibPolyCall micro 
 * command subsystem, providing registration, execution, and lifecycle management
 * for commands within isolated components.
 */


// Maximum command name length
#define MAX_COMMAND_NAME_LENGTH 64

// Maximum number of pending async commands
#define MAX_ASYNC_COMMANDS 128

// Memory block header for resource tracking
typedef struct memory_block_header {
    size_t size;                       // Size of the allocated block
    uint32_t magic;                    // Magic number for validation
    struct memory_block_header* next;  // Next block in the list
    struct memory_block_header* prev;  // Previous block in the list
} memory_block_header_t;

// Magic number for memory block validation
#define MEMORY_BLOCK_MAGIC 0xCAFEBABE

// Maximum number of threshold callbacks
#define MAX_THRESHOLD_CALLBACKS 16

// Threshold callback information
typedef struct {
    polycall_resource_type_t resource_type;  // Resource type
    uint8_t threshold;                      // Threshold percentage
    resource_threshold_callback_t callback;  // Callback function
    void* user_data;                        // User data for callback
} threshold_callback_info_t;

// Resource limiter structure
struct resource_limiter {
    size_t memory_quota;                    // Memory quota in bytes
    size_t memory_usage;                    // Current memory usage in bytes
    size_t peak_memory_usage;               // Peak memory usage in bytes
    uint32_t cpu_quota;                     // CPU quota in milliseconds
    uint32_t cpu_usage;                     // Current CPU usage in milliseconds
    uint32_t peak_cpu_usage;                // Peak CPU usage in milliseconds
    uint32_t io_quota;                      // I/O quota in operations
    uint32_t io_usage;                      // Current I/O usage in operations
    uint32_t peak_io_usage;                 // Peak I/O usage in operations
    bool enforce_limits;                    // Whether to enforce limits
    bool track_usage;                       // Whether to track usage
    memory_block_header_t* block_list;      // List of allocated memory blocks
    uint32_t limit_violations;              // Count of limit violations
    uint32_t memory_allocations;            // Count of memory allocations
    uint32_t memory_frees;                  // Count of memory frees
    threshold_callback_info_t threshold_callbacks[MAX_THRESHOLD_CALLBACKS]; // Threshold callbacks
    size_t threshold_callback_count;        // Number of threshold callbacks
    pthread_mutex_t lock;                   // Mutex for thread safety
};

// Micro command structure
struct polycall_micro_command {
    char name[MAX_COMMAND_NAME_LENGTH];    // Command name
    polycall_command_handler_t handler;    // Command handler function
    polycall_command_flags_t flags;        // Command flags
    void* user_data;                       // User data for handler
    polycall_micro_component_t* component; // Owner component
    command_security_attributes_t* security_attrs; // Security attributes
};

// Micro component structure
struct polycall_micro_component {
    char name[64];                          // Component name
    polycall_isolation_level_t isolation;   // Isolation level
    polycall_component_state_t state;       // Current state
    resource_limiter_t* resource_limiter;   // Resource limiter
    component_security_context_t* security_ctx; // Security context
    polycall_micro_command_t** commands;    // Array of commands
    size_t command_count;                   // Number of commands
    size_t command_capacity;                // Capacity of command array
    void* user_data;                        // User data
    component_event_callback_t* callbacks;  // Array of state callbacks
    size_t callback_count;                  // Number of callbacks
    size_t callback_capacity;               // Capacity of callback array
    pthread_mutex_t lock;                   // Mutex for thread safety
};

// Async command data structure
typedef struct async_command_data {
    polycall_core_context_t* ctx;           // Core context
    polycall_micro_context_t* micro_ctx;    // Micro context
    polycall_micro_component_t* component;  // Target component
    char command_name[MAX_COMMAND_NAME_LENGTH]; // Command name
    void* params;                           // Command parameters
    void* result;                           // Command result buffer
    void (*callback)(                       // Completion callback
        polycall_core_context_t* ctx,
        polycall_micro_context_t* micro_ctx,
        polycall_micro_component_t* component,
        const char* command_name,
        void* result,
        polycall_core_error_t error,
        void* user_data
    );
    void* user_data;                        // User data for callback
} async_command_data_t;

// Async command queue
typedef struct {
    async_command_data_t commands[MAX_ASYNC_COMMANDS]; // Command queue
    size_t head;                            // Queue head
    size_t tail;                            // Queue tail
    size_t count;                           // Number of commands in queue
    pthread_mutex_t lock;                   // Mutex for thread safety
    pthread_cond_t cond;                    // Condition variable for signaling
} async_command_queue_t;

// Micro context structure
struct polycall_micro_context {
    polycall_micro_config_t config;         // Configuration
    component_registry_t* component_registry; // Component registry
    security_policy_t* security_policy;     // Security policy
    async_command_queue_t async_queue;      // Async command queue
    pthread_t async_thread;                 // Async command thread
    bool async_thread_active;               // Whether async thread is active
    pthread_mutex_t lock;                   // General mutex
    pthread_mutex_t async_lock;             // Async queue mutex
    pthread_cond_t async_cond;              // Async queue condition variable
};

// Forward declarations of internal functions
static polycall_core_error_t find_command(
    polycall_core_context_t* ctx,
    polycall_micro_component_t* component,
    const char* command_name,
    polycall_micro_command_t** command
);

static polycall_core_error_t verify_command_execution(
    polycall_core_context_t* ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    polycall_micro_command_t* command
);

static void notify_component_state_change(
    polycall_core_context_t* ctx,
    polycall_micro_component_t* component,
    polycall_component_state_t old_state,
    polycall_component_state_t new_state
);

static void* async_command_thread(void* arg);

static polycall_core_error_t queue_async_command(
    polycall_micro_context_t* micro_ctx,
    async_command_data_t* cmd_data
);

static void check_threshold(
    polycall_core_context_t* ctx,
    polycall_micro_context_t* micro_ctx, 
    polycall_micro_component_t* component,
    resource_limiter_t* limiter,
    polycall_resource_type_t resource_type,
    size_t current_usage,
    size_t quota
);

/**
 * @brief Resource limiter configuration structure
 */
typedef struct {
    size_t memory_quota;
    uint32_t cpu_quota;
    uint32_t io_quota;
    bool enforce_limits;
    bool track_usage;
} resource_limiter_config_t;

/**
 * @brief Clean up resource limiter
 */
void resource_limiter_cleanup(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter
) {
    if (!ctx || !limiter) {
        return;
    }
    
    // Free all memory blocks if tracking is enabled
    if (limiter->track_usage && limiter->block_list) {
        memory_block_header_t* block = limiter->block_list;
        
        while (block) {
            memory_block_header_t* next = block->next;
            polycall_core_free(ctx, block);
            block = next;
        }
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&limiter->lock);
    
    // Free limiter structure
    polycall_core_free(ctx, limiter);
}

/**
 * @brief Set resource quota
 */
polycall_core_error_t resource_limiter_set_quota(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    polycall_resource_type_t resource_type,
    size_t quota
) {
    if (!ctx || !limiter) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (resource_type >= POLYCALL_RESOURCE_COUNT) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid resource type: %d", resource_type);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    // Set quota based on resource type
    switch (resource_type) {
        case POLYCALL_RESOURCE_MEMORY:
            limiter->memory_quota = quota;
            break;
            
        case POLYCALL_RESOURCE_CPU:
            limiter->cpu_quota = (uint32_t)quota;
            break;
            
        case POLYCALL_RESOURCE_IO:
            limiter->io_quota = (uint32_t)quota;
            break;
            
        default:
            pthread_mutex_unlock(&limiter->lock);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_unlock(&limiter->lock);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get resource quota
 */
polycall_core_error_t resource_limiter_get_quota(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    polycall_resource_type_t resource_type,
    size_t* quota
) {
    if (!ctx || !limiter || !quota) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (resource_type >= POLYCALL_RESOURCE_COUNT) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid resource type: %d", resource_type);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    // Get quota based on resource type
    switch (resource_type) {
        case POLYCALL_RESOURCE_MEMORY:
            *quota = limiter->memory_quota;
            break;
            
        case POLYCALL_RESOURCE_CPU:
            *quota = limiter->cpu_quota;
            break;
            
        case POLYCALL_RESOURCE_IO:
            *quota = limiter->io_quota;
            break;
            
        default:
            pthread_mutex_unlock(&limiter->lock);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_unlock(&limiter->lock);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Allocate resource
 */
polycall_core_error_t resource_limiter_allocate(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    polycall_resource_type_t resource_type,
    size_t amount
) {
    if (!ctx || !limiter) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (resource_type >= POLYCALL_RESOURCE_COUNT) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid resource type: %d", resource_type);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
    
    // Allocate resource based on type
    switch (resource_type) {
        case POLYCALL_RESOURCE_MEMORY:
            if (limiter->enforce_limits && 
                limiter->memory_quota > 0 && 
                limiter->memory_usage + amount > limiter->memory_quota) {
                
                // Quota exceeded
                limiter->limit_violations++;
                result = POLYCALL_CORE_ERROR_QUOTA_EXCEEDED;
            } else {
                // Update usage
                limiter->memory_usage += amount;
                if (limiter->memory_usage > limiter->peak_memory_usage) {
                    limiter->peak_memory_usage = limiter->memory_usage;
                }
                
                limiter->memory_allocations++;
                
                // Check thresholds
                if (limiter->memory_quota > 0) {
                    check_threshold(ctx, NULL, NULL, limiter, 
                                   POLYCALL_RESOURCE_MEMORY, 
                                   limiter->memory_usage, 
                                   limiter->memory_quota);
                }
            }
            break;
            
        case POLYCALL_RESOURCE_CPU:
            if (limiter->enforce_limits && 
                limiter->cpu_quota > 0 && 
                limiter->cpu_usage + (uint32_t)amount > limiter->cpu_quota) {
                
                // Quota exceeded
                limiter->limit_violations++;
                result = POLYCALL_CORE_ERROR_QUOTA_EXCEEDED;
            } else {
                // Update usage
                limiter->cpu_usage += (uint32_t)amount;
                if (limiter->cpu_usage > limiter->peak_cpu_usage) {
                    limiter->peak_cpu_usage = limiter->cpu_usage;
                }
                
                // Check thresholds
                if (limiter->cpu_quota > 0) {
                    check_threshold(ctx, NULL, NULL, limiter, 
                                   POLYCALL_RESOURCE_CPU, 
                                   limiter->cpu_usage, 
                                   limiter->cpu_quota);
                }
            }
            break;
            
        case POLYCALL_RESOURCE_IO:
            if (limiter->enforce_limits && 
                limiter->io_quota > 0 && 
                limiter->io_usage + (uint32_t)amount > limiter->io_quota) {
                
                // Quota exceeded
                limiter->limit_violations++;
                result = POLYCALL_CORE_ERROR_QUOTA_EXCEEDED;
            } else {
                // Update usage
                limiter->io_usage += (uint32_t)amount;
                if (limiter->io_usage > limiter->peak_io_usage) {
                    limiter->peak_io_usage = limiter->io_usage;
                }
                
                // Check thresholds
                if (limiter->io_quota > 0) {
                    check_threshold(ctx, NULL, NULL, limiter, 
                                   POLYCALL_RESOURCE_IO, 
                                   limiter->io_usage, 
                                   limiter->io_quota);
                }
            }
            break;
            
        default:
            pthread_mutex_unlock(&limiter->lock);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_unlock(&limiter->lock);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Resource quota exceeded for type %d", resource_type);
    }
    
    return result;
}

/**
 * @brief Release resource
 */
polycall_core_error_t resource_limiter_release(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    polycall_resource_type_t resource_type,
    size_t amount
) {
    if (!ctx || !limiter) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (resource_type >= POLYCALL_RESOURCE_COUNT) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid resource type: %d", resource_type);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    // Release resource based on type
    switch (resource_type) {
        case POLYCALL_RESOURCE_MEMORY:
            if (amount > limiter->memory_usage) {
                limiter->memory_usage = 0;
            } else {
                limiter->memory_usage -= amount;
            }
            
            limiter->memory_frees++;
            break;
            
        case POLYCALL_RESOURCE_CPU:
            if ((uint32_t)amount > limiter->cpu_usage) {
                limiter->cpu_usage = 0;
            } else {
                limiter->cpu_usage -= (uint32_t)amount;
            }
            break;
            
        case POLYCALL_RESOURCE_IO:
            if ((uint32_t)amount > limiter->io_usage) {
                limiter->io_usage = 0;
            } else {
                limiter->io_usage -= (uint32_t)amount;
            }
            break;
            
        default:
            pthread_mutex_unlock(&limiter->lock);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_unlock(&limiter->lock);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get current resource usage
 */
polycall_core_error_t resource_limiter_get_usage(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    polycall_resource_usage_t* usage
) {
    if (!ctx || !limiter || !usage) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    // Copy usage statistics
    usage->memory_usage = limiter->memory_usage;
    usage->peak_memory_usage = limiter->peak_memory_usage;
    usage->cpu_usage = limiter->cpu_usage;
    usage->peak_cpu_usage = limiter->peak_cpu_usage;
    usage->io_usage = limiter->io_usage;
    usage->peak_io_usage = limiter->peak_io_usage;
    usage->limit_violations = limiter->limit_violations;
    usage->memory_allocations = limiter->memory_allocations;
    usage->memory_frees = limiter->memory_frees;
    
    pthread_mutex_unlock(&limiter->lock);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Reset resource usage counters
 */
polycall_core_error_t resource_limiter_reset_usage(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter
) {
    if (!ctx || !limiter) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    // Reset current usage
    limiter->memory_usage = 0;
    limiter->cpu_usage = 0;
    limiter->io_usage = 0;
    
    // Reset peak usage
    limiter->peak_memory_usage = 0;
    limiter->peak_cpu_usage = 0;
    limiter->peak_io_usage = 0;
    
    // Reset statistics
    limiter->limit_violations = 0;
    limiter->memory_allocations = 0;
    limiter->memory_frees = 0;
    
    pthread_mutex_unlock(&limiter->lock);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register resource threshold callback
 */
polycall_core_error_t resource_limiter_register_threshold(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    polycall_resource_type_t resource_type,
    uint8_t threshold,
    resource_threshold_callback_t callback,
    void* user_data
) {
    if (!ctx || !limiter || !callback) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (resource_type >= POLYCALL_RESOURCE_COUNT) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid resource type: %d", resource_type);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Threshold must be between 1 and 100
    if (threshold < 1 || threshold > 100) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid threshold: %d (must be 1-100)", threshold);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&limiter->lock);
    
    // Check if we've reached the maximum number of callbacks
    if (limiter->threshold_callback_count >= MAX_THRESHOLD_CALLBACKS) {
        pthread_mutex_unlock(&limiter->lock);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Maximum number of threshold callbacks reached");
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Add callback
    threshold_callback_info_t* info = &limiter->threshold_callbacks[limiter->threshold_callback_count];
    info->resource_type = resource_type;
    info->threshold = threshold;
    info->callback = callback;
    info->user_data = user_data;
    
    limiter->threshold_callback_count++;
    
    pthread_mutex_unlock(&limiter->lock);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Memory allocation wrapper for resource-limited components
 */
void* resource_limiter_malloc(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    size_t size
) {
    if (!ctx || !limiter || size == 0) {
        return NULL;
    }
    
    // Calculate total size including header
    size_t total_size = size + sizeof(memory_block_header_t);
    
    // Check if allocation would exceed quota
    polycall_core_error_t result = resource_limiter_allocate(
        ctx, limiter, POLYCALL_RESOURCE_MEMORY, total_size);
        
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Memory allocation would exceed quota");
        return NULL;
    }
    
    // Allocate memory
    memory_block_header_t* block = polycall_core_malloc(ctx, total_size);
    if (!block) {
        // Release the allocation from the quota
        resource_limiter_release(ctx, limiter, POLYCALL_RESOURCE_MEMORY, total_size);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate memory");
        return NULL;
    }
    
    // Initialize block header
    block->size = size;
    block->magic = MEMORY_BLOCK_MAGIC;
    
    // Add to block list if tracking is enabled
    if (limiter->track_usage) {
        pthread_mutex_lock(&limiter->lock);
        
        block->next = limiter->block_list;
        block->prev = NULL;
        
        if (limiter->block_list) {
            limiter->block_list->prev = block;
        }
        
        limiter->block_list = block;
        
        pthread_mutex_unlock(&limiter->lock);
    }
    
    // Return pointer to user data (after header)
    return (void*)((char*)block + sizeof(memory_block_header_t));
}

/**
 * @brief Memory free wrapper for resource-limited components
 */
void resource_limiter_free(
    polycall_core_context_t* ctx,
    resource_limiter_t* limiter,
    void* ptr
) {
    if (!ctx || !limiter || !ptr) {
        return;
    }
    
    // Get block header
    memory_block_header_t* block = (memory_block_header_t*)(
        (char*)ptr - sizeof(memory_block_header_t));
        
    // Validate block header
    if (block->magic != MEMORY_BLOCK_MAGIC) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid memory block header");
        return;
    }
    
    // Get block size
    size_t total_size = block->size + sizeof(memory_block_header_t);
    
    // Remove from block list if tracking is enabled
    if (limiter->track_usage) {
        pthread_mutex_lock(&limiter->lock);
        
        if (block->prev) {
            block->prev->next = block->next;
        } else {
            limiter->block_list = block->next;
        }
        
        if (block->next) {
            block->next->prev = block->prev;
        }
        
        pthread_mutex_unlock(&limiter->lock);
    }
    
    // Release memory from quota
    resource_limiter_release(ctx, limiter, POLYCALL_RESOURCE_MEMORY, total_size);
    
    // Free block
    polycall_core_free(ctx, block);
}

/**
 * @brief Create default resource limiter configuration
 */
resource_limiter_config_t resource_limiter_create_default_config(void) {
    resource_limiter_config_t config;
    
    config.memory_quota = 10 * 1024 * 1024; // 10MB default memory quota
    config.cpu_quota = 1000;               // 1000ms default CPU quota
    config.io_quota = 1000;                // 1000 operations default I/O quota
    config.enforce_limits = true;          // Enforce limits by default
    config.track_usage = true;             // Track usage by default
    
    return config;
}

/**
 * @brief Cleanup micro command subsystem
 */
void polycall_micro_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx
) {
    if (!core_ctx || !micro_ctx) {
        return;
    }
    
    // Stop async command thread
    if (micro_ctx->async_thread_active) {
        pthread_mutex_lock(&micro_ctx->async_lock);
        micro_ctx->async_thread_active = false;
        pthread_cond_signal(&micro_ctx->async_cond);
        pthread_mutex_unlock(&micro_ctx->async_lock);
        
        // Wait for thread to exit
        pthread_join(micro_ctx->async_thread, NULL);
    }
    
    // Clean up security policy
    if (micro_ctx->security_policy) {
        security_policy_cleanup(core_ctx, micro_ctx->security_policy);
    }
    
    // Clean up component registry
    if (micro_ctx->component_registry) {
        component_registry_cleanup(core_ctx, micro_ctx->component_registry);
    }
    
    // Destroy mutexes and condition variable
    pthread_mutex_destroy(&micro_ctx->lock);
    pthread_mutex_destroy(&micro_ctx->async_lock);
    pthread_cond_destroy(&micro_ctx->async_cond);
    
    // Free context
    polycall_core_free(core_ctx, micro_ctx);
}

/**
 * @brief Create a component
 */
polycall_core_error_t polycall_micro_create_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t** component,
    const char* name,
    polycall_isolation_level_t isolation_level
) {
    if (!core_ctx || !micro_ctx || !component || !name) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create the component
    polycall_core_error_t result = polycall_micro_component_create(
        core_ctx, component, name, isolation_level);
        
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to create component '%s'", name);
        return result;
    }
    
    // Set up resource limits if requested
    if (micro_ctx->config.enable_resource_limits) {
        resource_limiter_config_t limiter_config = resource_limiter_create_default_config();
        limiter_config.memory_quota = micro_ctx->config.default_memory_quota;
        limiter_config.cpu_quota = micro_ctx->config.default_cpu_quota;
        limiter_config.io_quota = micro_ctx->config.default_io_quota;
        
        resource_limiter_t* limiter = NULL;
        result = resource_limiter_init(core_ctx, &limiter, &limiter_config);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_micro_component_destroy(core_ctx, *component);
            *component = NULL;
            
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to initialize resource limiter for component '%s'", name);
            return result;
        }
        
        // Attach limiter to component
        // Note: In a real implementation, we would have a proper way to do this
        (*component)->resource_limiter = limiter;
    }
    
    // Set up security context if requested
    if (micro_ctx->config.enable_security && micro_ctx->security_policy) {
        component_security_context_t* security_ctx = NULL;
        result = polycall_micro_component_init_security(core_ctx, *component, &security_ctx);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            if ((*component)->resource_limiter) {
                resource_limiter_cleanup(core_ctx, (*component)->resource_limiter);
            }
            
            polycall_micro_component_destroy(core_ctx, *component);
            *component = NULL;
            
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to initialize security context for component '%s'", name);
            return result;
        }
        
        // Attach security context to component
        (*component)->security_ctx = security_ctx;
    }
    
    // Register component with registry
    result = component_registry_register(core_ctx, micro_ctx->component_registry, *component);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        if ((*component)->security_ctx) {
            // Clean up security context (would need implementation)
        }
        
        if ((*component)->resource_limiter) {
            resource_limiter_cleanup(core_ctx, (*component)->resource_limiter);
        }
        
        polycall_micro_component_destroy(core_ctx, *component);
        *component = NULL;
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to register component '%s'", name);
        return result;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Destroy a component
 */
polycall_core_error_t polycall_micro_destroy_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component
) {
    if (!core_ctx || !micro_ctx || !component) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Unregister component from registry
    polycall_core_error_t result = component_registry_unregister(
        core_ctx, micro_ctx->component_registry, component);
        
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to unregister component '%s'", component->name);
        return result;
    }
    
    // Clean up resources
    polycall_micro_component_destroy(core_ctx, component);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Find a component by name
 */
polycall_core_error_t polycall_micro_find_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    const char* name,
    polycall_micro_component_t** component
) {
    if (!core_ctx || !micro_ctx || !name || !component) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    return component_registry_find(core_ctx, micro_ctx->component_registry, name, component);
}

/**
 * @brief Register a command with a component
 */
polycall_core_error_t polycall_micro_register_command(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    polycall_micro_command_t** command,
    const char* name,
    polycall_command_handler_t handler,
    polycall_command_flags_t flags,
    void* user_data
) {
    if (!core_ctx || !micro_ctx || !component || !command || !name || !handler) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check name length
    if (strlen(name) >= MAX_COMMAND_NAME_LENGTH) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Command name too long: '%s'", name);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate command structure
    polycall_micro_command_t* new_command = polycall_core_malloc(core_ctx, sizeof(polycall_micro_command_t));
    if (!new_command) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate command structure");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize command
    memset(new_command, 0, sizeof(polycall_micro_command_t));
    strncpy(new_command->name, name, MAX_COMMAND_NAME_LENGTH - 1);
    new_command->name[MAX_COMMAND_NAME_LENGTH - 1] = '\0';
    new_command->handler = handler;
    new_command->flags = flags;
    new_command->user_data = user_data;
    new_command->component = component;
    
    // Create security attributes if needed
    if (micro_ctx->config.enable_security) {
        polycall_permission_t required_permissions = POLYCALL_PERMISSION_EXECUTE;
        
        // Add READ permission if command is readonly
        if (flags & POLYCALL_COMMAND_FLAG_READONLY) {
            required_permissions |= POLYCALL_PERMISSION_READ;
        } else {
            // Add WRITE permission if command can modify state
            required_permissions |= (POLYCALL_PERMISSION_READ | POLYCALL_PERMISSION_WRITE);
        }
        
        // Add ADMIN permission if command is privileged
        if (flags & POLYCALL_COMMAND_FLAG_PRIVILEGED) {
            required_permissions |= POLYCALL_PERMISSION_ADMIN;
        }
        
        // Create attributes
        command_security_attributes_t* attrs = NULL;
        polycall_core_error_t result = security_create_command_attributes(
            core_ctx, &attrs, required_permissions);
            
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_core_free(core_ctx, new_command);
            
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to create security attributes for command '%s'", name);
            return result;
        }
        
        new_command->security_attrs = attrs;
    }
    
    // Register command with component
    pthread_mutex_lock(&component->lock);
    
    // Check if command already exists
    for (size_t i = 0; i < component->command_count; i++) {
        if (strcmp(component->commands[i]->name, name) == 0) {
            pthread_mutex_unlock(&component->lock);
            
            if (new_command->security_attrs) {
                polycall_core_free(core_ctx, new_command->security_attrs);
            }
            
            polycall_core_free(core_ctx, new_command);
            
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              POLYCALL_CORE_ERROR_ALREADY_REGISTERED,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Command '%s' already registered with component '%s'", 
                              name, component->name);
            return POLYCALL_CORE_ERROR_ALREADY_REGISTERED;
        }
    }
    
    // Expand command array if needed
    if (component->command_count >= component->command_capacity) {
        size_t new_capacity = component->command_capacity * 2;
        polycall_micro_command_t** new_commands = polycall_core_realloc(
            core_ctx, component->commands, new_capacity * sizeof(polycall_micro_command_t*));
            
        if (!new_commands) {
            pthread_mutex_unlock(&component->lock);
            
            if (new_command->security_attrs) {
                polycall_core_free(core_ctx, new_command->security_attrs);
            }
            
            polycall_core_free(core_ctx, new_command);
            
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to expand command array");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        component->commands = new_commands;
        component->command_capacity = new_capacity;
    }
    
    // Add command to component
    component->commands[component->command_count++] = new_command;
    
    pthread_mutex_unlock(&component->lock);
    
    // Return command
    *command = new_command;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Execute a command on a component
 */
polycall_core_error_t polycall_micro_execute_command(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    const char* command_name,
    void* params,
    void* result
) {
    if (!core_ctx || !micro_ctx || !component || !command_name) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find command
    polycall_micro_command_t* command = NULL;
    polycall_core_error_t find_result = find_command(
        core_ctx, component, command_name, &command);
        
    if (find_result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          find_result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Command '%s' not found in component '%s'", 
                          command_name, component->name);
        return find_result;
    }
    
    // Check component state
    polycall_component_state_t state;
    polycall_core_error_t state_result = polycall_micro_get_component_state(
        core_ctx, micro_ctx, component, &state);
        
    if (state_result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          state_result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to get component state");
        return state_result;
    }
    
    if (state != POLYCALL_COMPONENT_STATE_RUNNING) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Component '%s' is not in running state", component->name);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Verify command execution if security is enabled
    if (micro_ctx->config.enable_security) {
        polycall_core_error_t verify_result = verify_command_execution(
            core_ctx, micro_ctx, component, command);
            
        if (verify_result != POLYCALL_CORE_SUCCESS) {
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              verify_result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Command execution not allowed");
            return verify_result;
        }
    }
    
    // Execute command
    return command->handler(core_ctx, micro_ctx, component, params, result, command->user_data);
}

/**
 * @brief Execute a command asynchronously
 */
polycall_core_error_t polycall_micro_execute_command_async(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    const char* command_name,
    void* params,
    void (*callback)(
        polycall_core_context_t* ctx,
        polycall_micro_context_t* micro_ctx,
        polycall_micro_component_t* component,
        const char* command_name,
        void* result,
        polycall_core_error_t error,
        void* user_data
    ),
    void* user_data
) {
    if (!core_ctx || !micro_ctx || !component || !command_name || !callback) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find command
    polycall_micro_command_t* command = NULL;
    polycall_core_error_t find_result = find_command(
        core_ctx, component, command_name, &command);
        
    if (find_result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          find_result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Command '%s' not found in component '%s'", 
                          command_name, component->name);
        return find_result;
    }
    
    // Check component state
    polycall_component_state_t state;
    polycall_core_error_t state_result = polycall_micro_get_component_state(
        core_ctx, micro_ctx, component, &state);
        
    if (state_result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          state_result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to get component state");
        return state_result;
    }
    
    if (state != POLYCALL_COMPONENT_STATE_RUNNING) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Component '%s' is not in running state", component->name);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Verify command execution if security is enabled
    if (micro_ctx->config.enable_security) {
        polycall_core_error_t verify_result = verify_command_execution(
            core_ctx, micro_ctx, component, command);
            
        if (verify_result != POLYCALL_CORE_SUCCESS) {
            POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              verify_result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Command execution not allowed");
            return verify_result;
        }
    }
    
    // Prepare async command data
    async_command_data_t cmd_data;
    memset(&cmd_data, 0, sizeof(async_command_data_t));
    
    cmd_data.ctx = core_ctx;
    cmd_data.micro_ctx = micro_ctx;
    cmd_data.component = component;
    strncpy(cmd_data.command_name, command_name, MAX_COMMAND_NAME_LENGTH - 1);
    cmd_data.command_name[MAX_COMMAND_NAME_LENGTH - 1] = '\0';
    cmd_data.params = params;
    cmd_data.result = polycall_core_malloc(core_ctx, 1024); // Allocate result buffer (size depends on command)
    cmd_data.callback = callback;
    cmd_data.user_data = user_data;
    
    if (!cmd_data.result) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate result buffer");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Queue command for async execution
    queue_async_command(micro_ctx, &cmd_data);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Set component resource limits
 */
polycall_core_error_t polycall_micro_set_resource_limits(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    size_t memory_quota,
    uint32_t cpu_quota,
    uint32_t io_quota
) {
    if (!core_ctx || !micro_ctx || !component) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if resource limits are enabled
    if (!micro_ctx->config.enable_resource_limits) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Resource limits are not enabled");
        return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
    }
    
    // Check if component has a resource limiter
    if (!component->resource_limiter) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_NOT_INITIALIZED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Component does not have a resource limiter");
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    

/**
 * @brief Set component user data
 */
polycall_core_error_t polycall_micro_set_component_data(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    void* user_data
) {
    if (!core_ctx || !micro_ctx || !component) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&component->lock);
    component->user_data = user_data;
    pthread_mutex_unlock(&component->lock);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Initialize command handler system
 */
polycall_core_error_t command_handler_init(
    polycall_core_context_t* ctx,
    polycall_micro_context_t* micro_ctx
) {
    if (!ctx || !micro_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Initialize async command queue
    memset(&micro_ctx->async_queue, 0, sizeof(async_command_queue_t));
    
    pthread_mutex_init(&micro_ctx->async_queue.lock, NULL);
    pthread_cond_init(&micro_ctx->async_queue.cond, NULL);
    
    // Create async command thread if requested
    if (micro_ctx->config.enable_async_commands) {
        micro_ctx->async_thread_active = true;
        
        if (pthread_create(&micro_ctx->async_thread, NULL, async_command_thread, micro_ctx) != 0) {
            pthread_mutex_destroy(&micro_ctx->async_queue.lock);
            pthread_cond_destroy(&micro_ctx->async_queue.cond);
            
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                              POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to create async command thread");
            return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
        }
/**
 * @brief Clean up command handler system
 */
void command_handler_cleanup(
    polycall_core_context_t* ctx,
    polycall_micro_context_t* micro_ctx
) {
    if (!ctx || !micro_ctx) {
        return;
    }
    
    // Stop async command thread
    if (micro_ctx->async_thread_active) {
        pthread_mutex_lock(&micro_ctx->async_queue.lock);
        micro_ctx->async_thread_active = false;
        pthread_cond_signal(&micro_ctx->async_queue.cond);
        pthread_mutex_unlock(&micro_ctx->async_queue.lock);
        
        pthread_join(micro_ctx->async_thread, NULL);
    }
    
    // Clean up any pending async commands
    pthread_mutex_lock(&micro_ctx->async_queue.lock);
    
    for (size_t i = 0; i < micro_ctx->async_queue.count; i++) {
        size_t idx = (micro_ctx->async_queue.head + i) % MAX_ASYNC_COMMANDS;
        async_command_data_t* cmd = &micro_ctx->async_queue.commands[idx];
        
        // Free result buffer
        if (cmd->result) {
            polycall_core_free(ctx, cmd->result);
        }
    }
    
    pthread_mutex_unlock(&micro_ctx->async_queue.lock);
    
    // Destroy mutex and condition variable
    pthread_mutex_destroy(&micro_ctx->async_queue.lock);
    pthread_cond_destroy(&micro_ctx->async_queue.cond);
}

    // Set CPU quota
    result = resource_limiter_set_quota(
        core_ctx, component->resource_limiter, POLYCALL_RESOURCE_CPU, cpu_quota);
        
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to set CPU quota");
        return result;
    }
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to set CPU quota");
        return result;
    }
    
    // Set I/O quota
    result = resource_limiter_set_quota(
        core_ctx, component->resource_limiter, POLYCALL_RESOURCE_IO, io_quota);
        
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          result,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to set I/O quota");
        return result;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Start a component
 */
polycall_core_error_t polycall_micro_start_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component
) {
    if (!core_ctx || !micro_ctx || !component) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&component->lock);
    
    // Check current state
    if (component->state == POLYCALL_COMPONENT_STATE_RUNNING) {
        pthread_mutex_unlock(&component->lock);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_WARNING, 
                          "Component '%s' is already running", component->name);
        return POLYCALL_CORE_SUCCESS; // Not an error
    }
    
    if (component->state == POLYCALL_COMPONENT_STATE_ERROR) {
        pthread_mutex_unlock(&component->lock);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Component '%s' is in error state", component->name);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Transition state: UNINITIALIZED -> STARTING -> RUNNING
    polycall_component_state_t old_state = component->state;
    
    // First transition to STARTING
    component->state = POLYCALL_COMPONENT_STATE_STARTING;
    
    // Notify state change
    pthread_mutex_unlock(&component->lock);
    notify_component_state_change(core_ctx, component, old_state, POLYCALL_COMPONENT_STATE_STARTING);
    
    // Perform initialization tasks here
    // ...
    
    // Transition to RUNNING
    pthread_mutex_lock(&component->lock);
    old_state = component->state;
    component->state = POLYCALL_COMPONENT_STATE_RUNNING;
    pthread_mutex_unlock(&component->lock);
    
    // Notify state change
    notify_component_state_change(core_ctx, component, old_state, POLYCALL_COMPONENT_STATE_RUNNING);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Stop a component
 */
polycall_core_error_t polycall_micro_stop_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component
) {
    if (!core_ctx || !micro_ctx || !component) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&component->lock);
    
    // Check current state
    if (component->state == POLYCALL_COMPONENT_STATE_STOPPED) {
        pthread_mutex_unlock(&component->lock);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_WARNING, 
                          "Component '%s' is already stopped", component->name);
        return POLYCALL_CORE_SUCCESS; // Not an error
    }
    
    if (component->state == POLYCALL_COMPONENT_STATE_UNINITIALIZED) {
        pthread_mutex_unlock(&component->lock);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Component '%s' is not initialized", component->name);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Transition state: RUNNING -> STOPPING -> STOPPED
    polycall_component_state_t old_state = component->state;
    
    // First transition to STOPPING
    component->state = POLYCALL_COMPONENT_STATE_STOPPING;
    
    // Notify state change
    pthread_mutex_unlock(&component->lock);
    notify_component_state_change(core_ctx, component, old_state, POLYCALL_COMPONENT_STATE_STOPPING);
    
    // Perform cleanup tasks here
    // ...
    
    // Transition to STOPPED
    pthread_mutex_lock(&component->lock);
    old_state = component->state;
    component->state = POLYCALL_COMPONENT_STATE_STOPPED;
    pthread_mutex_unlock(&component->lock);
    
    // Notify state change
    notify_component_state_change(core_ctx, component, old_state, POLYCALL_COMPONENT_STATE_STOPPED);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get component state
 */
polycall_core_error_t polycall_micro_get_component_state(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component,
    polycall_component_state_t* state
) {
    if (!core_ctx || !micro_ctx || !component || !state) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&component->lock);
    *state = component->state;
    pthread_mutex_unlock(&component->lock);
    
    return POLYCALL_CORE_SUCCESS;
}

