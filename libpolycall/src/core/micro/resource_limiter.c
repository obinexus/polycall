/**
#include "polycall/core/micro/resource_limiter.h"

 * @file resource_limiter.c
 * @brief Resource limitation implementation for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the resource limitation mechanisms defined in
 * polycall_micro_resource.h, providing memory and CPU usage quotas for
 * micro components.
 */

 #include "polycall/core/micro/polycall_micro_resource.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <pthread.h>
 #include <time.h>
 
 // Memory block header for tracking allocations
 typedef struct memory_block_header {
     size_t size;
     struct memory_block_header* next;
     struct memory_block_header* prev;
     uint32_t magic;
 } memory_block_header_t;
 
 #define MEMORY_BLOCK_MAGIC 0xDEADBEEF
 #define MAX_THRESHOLD_CALLBACKS 8
 
 // Threshold callback information
 typedef struct {
     polycall_resource_type_t resource_type;
     uint8_t threshold;
     resource_threshold_callback_t callback;
     void* user_data;
 } threshold_callback_info_t;
 
 // Resource limiter structure
 struct resource_limiter {
     // Resource quotas
     size_t memory_quota;
     uint32_t cpu_quota;
     uint32_t io_quota;
     
     // Current usage
     size_t memory_usage;
     uint32_t cpu_usage;
     uint32_t io_usage;
     
     // Peak usage
     size_t peak_memory_usage;
     uint32_t peak_cpu_usage;
     uint32_t peak_io_usage;
     
     // Usage statistics
     uint32_t limit_violations;
     uint32_t memory_allocations;
     uint32_t memory_frees;
     
     // Flags
     bool enforce_limits;
     bool track_usage;
     
     // Allocated blocks list for memory tracking
     memory_block_header_t* block_list;
     
     // Threshold callbacks
     threshold_callback_info_t threshold_callbacks[MAX_THRESHOLD_CALLBACKS];
     size_t threshold_callback_count;
     
     // CPU usage tracking
     clock_t cpu_start_time;
     bool cpu_tracking_active;
     
     // Thread safety
     pthread_mutex_t lock;
 };
 
 // Resource quota structure
 struct resource_quota {
     size_t memory_quota;
     uint32_t cpu_quota;
     uint32_t io_quota;
 };
 
 /**
  * @brief Check if a threshold has been crossed
  */
 static void check_threshold(
     polycall_core_context_t* ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     resource_limiter_t* limiter,
     polycall_resource_type_t resource_type,
     size_t current_usage,
     size_t quota
 ) {
     if (!limiter || quota == 0) {
         return;
     }
     
     // Calculate usage percentage
     uint8_t usage_percent = (uint8_t)((current_usage * 100) / quota);
     
     // Check thresholds
     for (size_t i = 0; i < limiter->threshold_callback_count; i++) {
         threshold_callback_info_t* info = &limiter->threshold_callbacks[i];
         
         if (info->resource_type == resource_type && 
             usage_percent >= info->threshold && 
             info->callback) {
             
             // Call the threshold callback
             info->callback(ctx, micro_ctx, component, resource_type, 
                           current_usage, quota, info->user_data);
         }
     }
 }
 
 /**
  * @brief Initialize resource limiter
  */
 polycall_core_error_t resource_limiter_init(
     polycall_core_context_t* ctx,
     resource_limiter_t** limiter,
     const resource_limiter_config_t* config
 ) {
     if (!ctx || !limiter || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate limiter structure
     resource_limiter_t* new_limiter = polycall_core_malloc(ctx, sizeof(resource_limiter_t));
     if (!new_limiter) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate resource limiter");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize limiter
     memset(new_limiter, 0, sizeof(resource_limiter_t));
     
     // Set quotas from config
     new_limiter->memory_quota = config->memory_quota;
     new_limiter->cpu_quota = config->cpu_quota;
     new_limiter->io_quota = config->io_quota;
     
     // Set flags
     new_limiter->enforce_limits = config->enforce_limits;
     new_limiter->track_usage = config->track_usage;
     
         // Initialize mutex
         if (pthread_mutex_init(&new_limiter->lock, NULL) != 0) {
             polycall_core_free(ctx, new_limiter);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                               POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to initialize resource limiter mutex");
             return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
         }
         
         *limiter = new_limiter;
         return POLYCALL_CORE_SUCCESS;
    }

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