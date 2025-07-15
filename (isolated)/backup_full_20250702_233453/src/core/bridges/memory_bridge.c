/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/ffi/memory_bridge.h"
#include "polycall/core/ffi/memory_bridge.h"

#include <assert.h>
#include <string.h>

 * @file memory_bridge.c
 * @brief Memory management bridge implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the memory bridge for LibPolyCall FFI, providing
 * safe memory sharing between different language runtimes with ownership
 * tracking and garbage collection integration.
 */


/**
 * @brief Find a memory region in the ownership registry
 */
static memory_region_descriptor_t* ownership_registry_find(
    ownership_registry_t* registry,
    void* ptr
) {
    if (!registry || !ptr) {
        return NULL;
    }
    
    ownership_registry_lock(registry);
    
    for (size_t i = 0; i < registry->count; i++) {
        if (registry->regions[i]->ptr == ptr) {
            memory_region_descriptor_t* desc = registry->regions[i];
            ownership_registry_unlock(registry);
            return desc;
        }
    }
    
    ownership_registry_unlock(registry);
    return NULL;
}

/**
 * @brief Initialize ownership registry
 */
static polycall_core_error_t ownership_registry_init(
    polycall_core_context_t* ctx,
    ownership_registry_t** registry,
    size_t initial_capacity
) {
    if (!ctx || !registry) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    ownership_registry_t* new_registry = polycall_core_malloc(ctx, sizeof(ownership_registry_t));
    if (!new_registry) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    new_registry->regions = NULL;
    new_registry->count = 0;
    new_registry->capacity = 0;
    new_registry->ctx = ctx;
    
    // Initialize mutex
    if (pthread_mutex_init(&new_registry->mutex, NULL) != 0) {
        polycall_core_free(ctx, new_registry);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to initialize ownership registry mutex");
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Allocate initial region array if capacity > 0
    if (initial_capacity > 0) {
        new_registry->regions = polycall_core_malloc(ctx, initial_capacity * sizeof(memory_region_descriptor_t*));
        if (!new_registry->regions) {
            pthread_mutex_destroy(&new_registry->mutex);
            polycall_core_free(ctx, new_registry);
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        new_registry->capacity = initial_capacity;
    }
    
    *registry = new_registry;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up ownership registry
 */
static void ownership_registry_cleanup(
    polycall_core_context_t* ctx,
    ownership_registry_t* registry
) {
    if (!ctx || !registry) {
        return;
    }
    
    // Free all region descriptors
    for (size_t i = 0; i < registry->count; i++) {
        if (registry->regions[i]) {
            polycall_core_free(ctx, registry->regions[i]);
        }
    }
    
    // Free region array
    if (registry->regions) {
        polycall_core_free(ctx, registry->regions);
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&registry->mutex);
    
    // Free registry
    polycall_core_free(ctx, registry);
}

/**
 * @brief Lock ownership registry
 */
static void ownership_registry_lock(ownership_registry_t* registry) {
    if (registry) {
        pthread_mutex_lock(&registry->mutex);
    }
}

/**
 * @brief Unlock ownership registry
 */
static void ownership_registry_unlock(ownership_registry_t* registry) {
    if (registry) {
        pthread_mutex_unlock(&registry->mutex);
    }
}

/**
 * @brief Add a memory region to the ownership registry
 */
static bool ownership_registry_add(
    ownership_registry_t* registry,
    memory_region_descriptor_t* region_desc
) {
    if (!registry || !region_desc) {
        return false;
    }
    
    ownership_registry_lock(registry);
    
    // Check if we need to expand the registry
    if (registry->count >= registry->capacity) {
        size_t new_capacity = registry->capacity == 0 ? 16 : registry->capacity * 2;
        memory_region_descriptor_t** new_regions = polycall_core_realloc(
            registry->ctx,
            registry->regions,
            new_capacity * sizeof(memory_region_descriptor_t*)
        );
        
        if (!new_regions) {
            ownership_registry_unlock(registry);
            return false;
        }
        
        registry->regions = new_regions;
        registry->capacity = new_capacity;
    }
    
    // Add the region
    registry->regions[registry->count++] = region_desc;
    ownership_registry_unlock(registry);
    return true;
}


/**
 * @brief Remove a memory region from the ownership registry
 */
static bool ownership_registry_remove(
    ownership_registry_t* registry,
    memory_region_descriptor_t* region_desc
) {
    if (!registry || !region_desc) {
        return false;
    }
    
    ownership_registry_lock(registry);
    
    // Find the region
    size_t index = SIZE_MAX;
    for (size_t i = 0; i < registry->count; i++) {
        if (registry->regions[i] == region_desc) {
            index = i;
            break;
        }
    }
    
    if (index == SIZE_MAX) {
        ownership_registry_unlock(registry);
        return false;
    }
    
    // Remove the region by moving the last region to its place
    if (index < registry->count - 1) {
        registry->regions[index] = registry->regions[registry->count - 1];
    }
    
    registry->count--;
    ownership_registry_unlock(registry);
    return true;
}

/**
 * @brief Initialize reference counter
 */
static polycall_core_error_t reference_counter_init(
    polycall_core_context_t* ctx,
    reference_counter_t** counter,
    size_t initial_capacity
) {
    if (!ctx || !counter) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    reference_counter_t* new_counter = polycall_core_malloc(ctx, sizeof(reference_counter_t));
    if (!new_counter) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    new_counter->ptrs = NULL;
    new_counter->counts = NULL;
    new_counter->count = 0;
    new_counter->capacity = 0;
    
    // Initialize mutex
    if (pthread_mutex_init(&new_counter->mutex, NULL) != 0) {
        polycall_core_free(ctx, new_counter);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to initialize reference counter mutex");
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Allocate initial arrays if capacity > 0
    if (initial_capacity > 0) {
        new_counter->ptrs = polycall_core_malloc(ctx, initial_capacity * sizeof(void*));
        if (!new_counter->ptrs) {
            pthread_mutex_destroy(&new_counter->mutex);
            polycall_core_free(ctx, new_counter);
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        new_counter->counts = polycall_core_malloc(ctx, initial_capacity * sizeof(uint32_t));
        if (!new_counter->counts) {
            polycall_core_free(ctx, new_counter->ptrs);
            pthread_mutex_destroy(&new_counter->mutex);
            polycall_core_free(ctx, new_counter);
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        new_counter->capacity = initial_capacity;
    }
    
    *counter = new_counter;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up reference counter
 */
static void reference_counter_cleanup(
    polycall_core_context_t* ctx,
    reference_counter_t* counter
) {
    if (!ctx || !counter) {
        return;
    }
    
    // Free arrays
    if (counter->ptrs) {
        polycall_core_free(ctx, counter->ptrs);
    }
    
    if (counter->counts) {
        polycall_core_free(ctx, counter->counts);
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&counter->mutex);
    
    // Free counter
    polycall_core_free(ctx, counter);
}

/**
 * @brief Increment reference count for a pointer
 */
static bool reference_counter_increment(
    polycall_core_context_t* ctx,
    reference_counter_t* counter,
    void* ptr
) {
    if (!ctx || !counter || !ptr) {
        return false;
    }
    
    pthread_mutex_lock(&counter->mutex);
    
    // Check if pointer already exists
    for (size_t i = 0; i < counter->count; i++) {
        if (counter->ptrs[i] == ptr) {
            counter->counts[i]++;
            pthread_mutex_unlock(&counter->mutex);
            return true;
        }
    }
    
    // Check if we need to expand
    if (counter->count >= counter->capacity) {
        size_t new_capacity = counter->capacity == 0 ? 16 : counter->capacity * 2;
        
        void** new_ptrs = polycall_core_realloc(ctx, counter->ptrs, new_capacity * sizeof(void*));
        if (!new_ptrs) {
            pthread_mutex_unlock(&counter->mutex);
            return false;
        }
        
        uint32_t* new_counts = polycall_core_realloc(ctx, counter->counts, new_capacity * sizeof(uint32_t));
        if (!new_counts) {
            polycall_core_free(ctx, new_ptrs);
            pthread_mutex_unlock(&counter->mutex);
            return false;
        }
        
        counter->ptrs = new_ptrs;
        counter->counts = new_counts;
        counter->capacity = new_capacity;
    }
    
    // Add new pointer
    counter->ptrs[counter->count] = ptr;
    counter->counts[counter->count] = 1;
    counter->count++;
    
    pthread_mutex_unlock(&counter->mutex);
    return true;
}

/**
 * @brief Decrement reference count for a pointer
 */
static uint32_t reference_counter_decrement(
    polycall_core_context_t* ctx,
    reference_counter_t* counter,
    void* ptr
) {
    if (!ctx || !counter || !ptr) {
        return 0;
    }
    
    pthread_mutex_lock(&counter->mutex);
    
    // Find pointer
    for (size_t i = 0; i < counter->count; i++) {
        if (counter->ptrs[i] == ptr) {
            if (counter->counts[i] > 0) {
                counter->counts[i]--;
            }
            
            uint32_t result = counter->counts[i];
            
            // Remove entry if count is 0
            if (result == 0) {
                if (i < counter->count - 1) {
                    // Move last entry to this position
                    counter->ptrs[i] = counter->ptrs[counter->count - 1];
                    counter->counts[i] = counter->counts[counter->count - 1];
                }
                
                counter->count--;
            }
            
            pthread_mutex_unlock(&counter->mutex);
            return result;
        }
    }
    
    pthread_mutex_unlock(&counter->mutex);
    return 0;
}

/**
 * @brief Get reference count for a pointer
 */
static uint32_t reference_counter_get_count(
    reference_counter_t* counter,
    void* ptr
) {
    if (!counter || !ptr) {
        return 0;
    }
    
    pthread_mutex_lock(&counter->mutex);
    
    // Find pointer
    for (size_t i = 0; i < counter->count; i++) {
        if (counter->ptrs[i] == ptr) {
            uint32_t result = counter->counts[i];
            pthread_mutex_unlock(&counter->mutex);
            return result;
        }
    }
    
    pthread_mutex_unlock(&counter->mutex);
    return 0;
}

/**
 * @brief Initialize memory bridge
 */
polycall_core_error_t polycall_memory_bridge_init(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t** memory_mgr,
    const memory_bridge_config_t* config
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate memory manager
    memory_manager_t* mgr = polycall_core_malloc(ctx, sizeof(memory_manager_t));
    if (!mgr) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate memory manager");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize manager fields
    mgr->shared_pool = NULL;
    mgr->ownership = NULL;
    mgr->ref_counts = NULL;
    mgr->gc_callbacks = NULL;
    mgr->gc_callback_count = 0;
    mgr->gc_callback_capacity = 0;
    mgr->snapshot_counter = 0;
    mgr->ctx = ctx;
    
    // Initialize configuration
    mgr->config.shared_pool_size = config->shared_pool_size;
    mgr->config.ownership_capacity = config->ownership_capacity;
    mgr->config.reference_capacity = config->reference_capacity;
    mgr->config.enable_gc_notification = config->enable_gc_notification;
    mgr->config.is_compatible_language = NULL; // Default: no compatibility check
    mgr->config.flags = POLYCALL_MEMORY_CONFIG_THREAD_SAFE; // Default: thread safe
    
    // Initialize mutex
    if (pthread_mutex_init(&mgr->gc_mutex, NULL) != 0) {
        polycall_core_free(ctx, mgr);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to initialize memory manager mutex");
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Create shared memory pool
    if (config->shared_pool_size > 0) {
        polycall_core_error_t pool_result = polycall_memory_create_pool(
            ctx, &mgr->shared_pool, config->shared_pool_size);
        
        if (pool_result != POLYCALL_CORE_SUCCESS) {
            pthread_mutex_destroy(&mgr->gc_mutex);
            polycall_core_free(ctx, mgr);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              pool_result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to create shared memory pool");
            return pool_result;
        }
    }
    
    // Initialize ownership registry
    polycall_core_error_t owner_result = ownership_registry_init(
        ctx, &mgr->ownership, config->ownership_capacity);
    
    if (owner_result != POLYCALL_CORE_SUCCESS) {
        if (mgr->shared_pool) {
            polycall_memory_destroy_pool(ctx, mgr->shared_pool);
        }
        pthread_mutex_destroy(&mgr->gc_mutex);
        polycall_core_free(ctx, mgr);
        return owner_result;
    }
    
    // Initialize reference counter
    polycall_core_error_t ref_result = reference_counter_init(
        ctx, &mgr->ref_counts, config->reference_capacity);
    
    if (ref_result != POLYCALL_CORE_SUCCESS) {
        ownership_registry_cleanup(ctx, mgr->ownership);
        if (mgr->shared_pool) {
            polycall_memory_destroy_pool(ctx, mgr->shared_pool);
        }
        pthread_mutex_destroy(&mgr->gc_mutex);
        polycall_core_free(ctx, mgr);
        return ref_result;
    }
    
    // Initialize GC callback registry if enabled
    if (config->enable_gc_notification) {
        mgr->gc_callback_capacity = 8; // Initial capacity
        mgr->gc_callbacks = polycall_core_malloc(ctx, 
            mgr->gc_callback_capacity * sizeof(gc_callback_entry_t));
        
        if (!mgr->gc_callbacks) {
            reference_counter_cleanup(ctx, mgr->ref_counts);
            ownership_registry_cleanup(ctx, mgr->ownership);
            if (mgr->shared_pool) {
                polycall_memory_destroy_pool(ctx, mgr->shared_pool);
            }
            pthread_mutex_destroy(&mgr->gc_mutex);
            polycall_core_free(ctx, mgr);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate GC callback registry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Initialize with the provided callback if any
        if (config->gc_callback) {
            mgr->gc_callbacks[0].language = NULL; // Global callback
            mgr->gc_callbacks[0].callback = config->gc_callback;
            mgr->gc_callbacks[0].user_data = config->user_data;
            mgr->gc_callback_count = 1;
        }
    }
    
    *memory_mgr = mgr;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up memory bridge
 */
void polycall_memory_bridge_cleanup(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr
) {
    if (!ctx || !memory_mgr) {
        return;
    }
    
    // Clean up GC callbacks
    if (memory_mgr->gc_callbacks) {
        polycall_core_free(ctx, memory_mgr->gc_callbacks);
    }
    
    // Clean up reference counter
    if (memory_mgr->ref_counts) {
        reference_counter_cleanup(ctx, memory_mgr->ref_counts);
    }
    
    // Clean up ownership registry
    if (memory_mgr->ownership) {
        ownership_registry_cleanup(ctx, memory_mgr->ownership);
    }
    
    // Clean up shared memory pool
    if (memory_mgr->shared_pool) {
        polycall_memory_destroy_pool(ctx, memory_mgr->shared_pool);
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&memory_mgr->gc_mutex);
    
    // Free memory manager
    polycall_core_free(ctx, memory_mgr);
}

/**
 * @brief Allocate memory from shared pool
 */
void* polycall_memory_alloc_shared(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    size_t size,
    const char* owner_language,
    polycall_memory_flags_t flags
) {
    if (!ctx || !memory_mgr || !owner_language || size == 0) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid parameters for shared memory allocation");
        return NULL;
    }
    
    // Allocate memory from shared pool if available, otherwise use core malloc
    void* ptr = NULL;
    if (memory_mgr->shared_pool) {
        ptr = polycall_memory_alloc(ctx, memory_mgr->shared_pool, size, POLYCALL_MEMORY_FLAG_NONE);
    } else {
        ptr = polycall_core_malloc(ctx, size);
    }
    
    if (!ptr) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate shared memory of size %zu", size);
        return NULL;
    }
    
    // Create region descriptor
    memory_region_descriptor_t* desc = polycall_core_malloc(ctx, sizeof(memory_region_descriptor_t));
    if (!desc) {
        if (memory_mgr->shared_pool) {
            polycall_memory_free(ctx, memory_mgr->shared_pool, ptr);
        } else {
            polycall_core_free(ctx, ptr);
        }
        
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate memory region descriptor");
        return NULL;
    }
    
    // Set up region information
    desc->ptr = ptr;
    desc->size = size;
    desc->owner = owner_language;
    desc->ref_count = 1;  // Owner has initial reference
    desc->flags = flags | POLYCALL_MEMORY_FLAG_AUTO_FREE;
    desc->permissions = POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE;
    
    // Register with ownership registry
    if (!ownership_registry_add(memory_mgr->ownership, desc)) {
        polycall_core_free(ctx, desc);
        
        if (memory_mgr->shared_pool) {
            polycall_memory_free(ctx, memory_mgr->shared_pool, ptr);
        } else {
            polycall_core_free(ctx, ptr);
        }
        
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to add memory region to registry");
        return NULL;
    }
    
    // Track reference
    reference_counter_increment(ctx, memory_mgr->ref_counts, ptr);
    
    return ptr;
}

/**
 * @brief Free memory from shared pool
 */
polycall_core_error_t polycall_memory_free_shared(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    const char* language
) {
    if (!ctx || !memory_mgr || !ptr || !language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find the memory region
    memory_region_descriptor_t* desc = ownership_registry_find(memory_mgr->ownership, ptr);
    if (!desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Memory region not found for pointer %p", ptr);
        return POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION;
    }
    
    // Verify ownership
    if (strcmp(desc->owner, language) != 0) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot free memory owned by '%s' from '%s'", 
                          desc->owner, language);
        return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
    }
    
    // Decrement reference count
    uint32_t new_count = reference_counter_decrement(ctx, memory_mgr->ref_counts, ptr);
    
    // Only free if reference count is 0
    if (new_count == 0) {
        // Remove from registry
        if (!ownership_registry_remove(memory_mgr->ownership, desc)) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_INVALID_STATE,
                              POLYCALL_ERROR_SEVERITY_WARNING, 
                              "Failed to remove memory region from registry");
        }
        
        // Free the memory
        if (memory_mgr->shared_pool) {
            polycall_memory_free(ctx, memory_mgr->shared_pool, ptr);
        } else {
            polycall_core_free(ctx, ptr);
        }
        
        // Free the descriptor
        polycall_core_free(ctx, desc);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Share memory across language boundaries
 */
polycall_core_error_t polycall_memory_share(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    size_t size,
    const char* source_language,
    const char* target_language,
    polycall_memory_share_flags_t flags,
    memory_region_descriptor_t** region_desc
) {
    // Validate parameters
    if (!ctx || !ffi_ctx || !memory_mgr || !ptr || size == 0 || 
        !source_language || !target_language || !region_desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Invalid parameters for memory sharing");
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Verify the source language owns this memory or it's a new allocation
    memory_region_descriptor_t* existing_desc = ownership_registry_find(memory_mgr->ownership, ptr);
    if (existing_desc) {
        if (strcmp(existing_desc->owner, source_language) != 0) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Cannot share memory owned by '%s' from '%s'", 
                              existing_desc->owner, source_language);
            return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
        }
    }
    
    // Handle copy-on-share if requested
    if (flags & POLYCALL_MEMORY_SHARE_COPY) {
        void* copied_ptr = polycall_memory_alloc_shared(ctx, ffi_ctx, memory_mgr, 
                                                      size, target_language,
                                                      POLYCALL_MEMORY_FLAG_NONE);
        if (!copied_ptr) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for copy-on-share");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy memory contents
        memcpy(copied_ptr, ptr, size);
        
        // Get the descriptor for the new allocation
        *region_desc = ownership_registry_find(memory_mgr->ownership, copied_ptr);
        return POLYCALL_CORE_SUCCESS;
    }
    
    // For transfer ownership
    if (flags & POLYCALL_MEMORY_SHARE_TRANSFER) {
        if (existing_desc) {
            // Transfer ownership
            existing_desc->owner = target_language;
            *region_desc = existing_desc;
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    // For reference sharing (read-only or read-write)
    if (existing_desc) {
        // Update permissions if read-only requested
        if (flags & POLYCALL_MEMORY_SHARE_READ_ONLY) {
            existing_desc->permissions &= ~POLYCALL_MEMORY_PERM_WRITE;
        }
        
        // Increment reference count
        reference_counter_increment(ctx, memory_mgr->ref_counts, ptr);
        
        // For temporary sharing, mark with appropriate flag
        if (flags & POLYCALL_MEMORY_SHARE_TEMPORARY) {
            existing_desc->flags |= POLYCALL_MEMORY_FLAG_AUTO_FREE;
        }
        
        *region_desc = existing_desc;
        return POLYCALL_CORE_SUCCESS;
    }
    
    // If no existing descriptor, create a new one
    memory_region_descriptor_t* new_desc = polycall_core_malloc(ctx, sizeof(memory_region_descriptor_t));
    if (!new_desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate memory region descriptor");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Set up region information
    new_desc->ptr = ptr;
    new_desc->size = size;
    new_desc->owner = source_language;
    new_desc->ref_count = 1;  // Source language has initial reference
    new_desc->flags = flags & ~POLYCALL_MEMORY_SHARE_COPY; // Remove copy flag
    
    // Determine permissions based on flags
    if (flags & POLYCALL_MEMORY_SHARE_READ_ONLY) {
        new_desc->permissions = POLYCALL_MEMORY_PERM_READ;
    } else {
        new_desc->permissions = POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE;
    }
    
    // Register with ownership registry
    if (!ownership_registry_add(memory_mgr->ownership, new_desc)) {
        polycall_core_free(ctx, new_desc);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to add memory region to registry");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Track reference
    reference_counter_increment(ctx, memory_mgr->ref_counts, ptr);
    
    *region_desc = new_desc;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Track a memory reference
 */
polycall_core_error_t polycall_memory_track_reference(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    size_t size,
    const char* language
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !ptr || size == 0 || !language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if memory region already exists
    memory_region_descriptor_t* desc = ownership_registry_find(memory_mgr->ownership, ptr);
    
    if (desc) {
        // Increment reference count
        reference_counter_increment(ctx, memory_mgr->ref_counts, ptr);
    } else {
        // Create new region descriptor
        desc = polycall_core_malloc(ctx, sizeof(memory_region_descriptor_t));
        if (!desc) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory region descriptor");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Set up region information
        desc->ptr = ptr;
        desc->size = size;
        desc->owner = language;
        desc->ref_count = 1;
        desc->flags = POLYCALL_MEMORY_FLAG_NONE;
        desc->permissions = POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE;
        
        // Register with ownership registry
        if (!ownership_registry_add(memory_mgr->ownership, desc)) {
            polycall_core_free(ctx, desc);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to add memory region to registry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Track reference
        reference_counter_increment(ctx, memory_mgr->ref_counts, ptr);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Acquire shared memory
 */
polycall_core_error_t polycall_memory_acquire(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    const char* language,
    polycall_memory_permissions_t permissions
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !ptr || !language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find the memory region
    memory_region_descriptor_t* desc = ownership_registry_find(memory_mgr->ownership, ptr);
    if (!desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Memory region not found for pointer %p", ptr);
        return POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION;
    }
    
    // Check permissions
    if ((permissions & POLYCALL_MEMORY_PERM_WRITE) && 
        !(desc->permissions & POLYCALL_MEMORY_PERM_WRITE)) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Write permission denied for memory region");
        return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
    }
    
    // For strict ownership mode, check language compatibility
    if (memory_mgr->config.flags & POLYCALL_MEMORY_CONFIG_STRICT_OWNERSHIP) {
        if (strcmp(desc->owner, language) != 0) {
            // The requester is not the owner
            if (!memory_mgr->config.is_compatible_language ||
                !memory_mgr->config.is_compatible_language(desc->owner, language)) {
                POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                                  POLYCALL_CORE_ERROR_INCOMPATIBLE_LANGUAGE,
                                  POLYCALL_ERROR_SEVERITY_ERROR, 
                                  "Language '%s' cannot access memory owned by '%s'",
                                  language, desc->owner);
                return POLYCALL_CORE_ERROR_INCOMPATIBLE_LANGUAGE;
            }
        }
    }
    
    // Increment reference count
    reference_counter_increment(ctx, memory_mgr->ref_counts, ptr);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Release shared memory
 */
polycall_core_error_t polycall_memory_release(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    const char* language
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !ptr || !language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find the memory region
    memory_region_descriptor_t* desc = ownership_registry_find(memory_mgr->ownership, ptr);
    if (!desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Memory region not found for pointer %p", ptr);
        return POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION;
    }
    
    // Decrement reference count
    uint32_t new_count = reference_counter_decrement(ctx, memory_mgr->ref_counts, ptr);
    
    // If reference count is 0 and region is marked for auto-free, free the memory
    if (new_count == 0 && (desc->flags & POLYCALL_MEMORY_FLAG_AUTO_FREE)) {
        // Only allow owner to fully release
        if (strcmp(desc->owner, language) != 0) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                              POLYCALL_ERROR_SEVERITY_WARNING, 
                              "Non-owner '%s' cannot fully release memory owned by '%s'",
                              language, desc->owner);
            return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
        }
        
        // Remove from registry
        if (!ownership_registry_remove(memory_mgr->ownership, desc)) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_INVALID_STATE,
                              POLYCALL_ERROR_SEVERITY_WARNING, 
                              "Failed to remove memory region from registry");
        }
        
        // Free the memory
        if (memory_mgr->shared_pool) {
            polycall_memory_free(ctx, memory_mgr->shared_pool, ptr);
        } else {
            polycall_core_free(ctx, ptr);
        }
        
        // Free the descriptor
        polycall_core_free(ctx, desc);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Synchronize memory changes
 */
polycall_core_error_t polycall_memory_synchronize(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    size_t size,
    const char* source_language
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !ptr || size == 0 || !source_language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find the memory region
    memory_region_descriptor_t* desc = ownership_registry_find(memory_mgr->ownership, ptr);
    if (!desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Memory region not found for pointer %p", ptr);
        return POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION;
    }
    
    // Verify write permission
    if (!(desc->permissions & POLYCALL_MEMORY_PERM_WRITE)) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Write permission denied for memory region");
        return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
    }
    
    // For strict ownership mode, check language compatibility
    if (memory_mgr->config.flags & POLYCALL_MEMORY_CONFIG_STRICT_OWNERSHIP) {
        if (strcmp(desc->owner, source_language) != 0) {
            // The requester is not the owner
            if (!memory_mgr->config.is_compatible_language ||
                !memory_mgr->config.is_compatible_language(desc->owner, source_language)) {
                POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                                  POLYCALL_CORE_ERROR_INCOMPATIBLE_LANGUAGE,
                                  POLYCALL_ERROR_SEVERITY_ERROR, 
                                  "Language '%s' cannot modify memory owned by '%s'",
                                  source_language, desc->owner);
                return POLYCALL_CORE_ERROR_INCOMPATIBLE_LANGUAGE;
            }
        }
    }
    
    // Memory barriers or cache flushing would be implemented here if needed
    // This is architecture-dependent, so we'll assume a memory barrier is sufficient
    __sync_synchronize();
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register GC notification callback
 */
polycall_core_error_t polycall_memory_register_gc_callback(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    const char* language,
    gc_notification_callback_t callback,
    void* user_data
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !callback) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if GC notifications are enabled
    if (!memory_mgr->config.enable_gc_notification) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "GC notifications are not enabled");
        return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
    }
    
    pthread_mutex_lock(&memory_mgr->gc_mutex);
    
    // Check if we need to expand the callback array
    if (memory_mgr->gc_callback_count >= memory_mgr->gc_callback_capacity) {
        size_t new_capacity = memory_mgr->gc_callback_capacity * 2;
        gc_callback_entry_t* new_callbacks = polycall_core_realloc(ctx, 
            memory_mgr->gc_callbacks, 
            new_capacity * sizeof(gc_callback_entry_t));
        
        if (!new_callbacks) {
            pthread_mutex_unlock(&memory_mgr->gc_mutex);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to expand GC callback registry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        memory_mgr->gc_callbacks = new_callbacks;
        memory_mgr->gc_callback_capacity = new_capacity;
    }
    
    // Add new callback
    memory_mgr->gc_callbacks[memory_mgr->gc_callback_count].language = language;
    memory_mgr->gc_callbacks[memory_mgr->gc_callback_count].callback = callback;
    memory_mgr->gc_callbacks[memory_mgr->gc_callback_count].user_data = user_data;
    memory_mgr->gc_callback_count++;
    
    pthread_mutex_unlock(&memory_mgr->gc_mutex);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Notify of GC event
 */
polycall_core_error_t polycall_memory_notify_gc(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    const char* language
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if GC notifications are enabled
    if (!memory_mgr->config.enable_gc_notification) {
        return POLYCALL_CORE_SUCCESS; // Not an error, just a no-op
    }
    
    pthread_mutex_lock(&memory_mgr->gc_mutex);
    
    // Mark all regions owned by the language as being in GC
    ownership_registry_lock(memory_mgr->ownership);
    
    for (size_t i = 0; i < memory_mgr->ownership->count; i++) {
        memory_region_descriptor_t* desc = memory_mgr->ownership->regions[i];
        if (strcmp(desc->owner, language) == 0) {
            desc->flags |= POLYCALL_MEMORY_FLAG_IN_GC;
        }
    }
    
    ownership_registry_unlock(memory_mgr->ownership);
    
    // Call relevant callbacks
    for (size_t i = 0; i < memory_mgr->gc_callback_count; i++) {
        if (!memory_mgr->gc_callbacks[i].language || 
            strcmp(memory_mgr->gc_callbacks[i].language, language) == 0) {
            
            // Call callback with NULL ptr to indicate GC start
            memory_mgr->gc_callbacks[i].callback(
                ctx, language, NULL, 0, memory_mgr->gc_callbacks[i].user_data);
        }
    }
    
    pthread_mutex_unlock(&memory_mgr->gc_mutex);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get memory region information
 */
polycall_core_error_t polycall_memory_get_region_info(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    void* ptr,
    memory_region_descriptor_t** region_desc
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !ptr || !region_desc) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find the memory region
    memory_region_descriptor_t* desc = ownership_registry_find(memory_mgr->ownership, ptr);
    if (!desc) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Memory region not found for pointer %p", ptr);
        return POLYCALL_CORE_ERROR_INVALID_MEMORY_REGION;
    }
    
    *region_desc = desc;
    return POLYCALL_CORE_SUCCESS;
}

// Memory snapshot structure
typedef struct {
    uint32_t id;
    memory_region_descriptor_t** regions;
    size_t region_count;
    const char* creator_language;
} memory_snapshot_t;

// Snapshot registry
typedef struct {
    memory_snapshot_t* snapshots;
    size_t count;
    size_t capacity;
} snapshot_registry_t;

// Global snapshot registry (this would ideally be part of memory_manager_t)
static snapshot_registry_t g_snapshot_registry = {NULL, 0, 0};

/**
 * @brief Create a memory snapshot
 */
polycall_core_error_t polycall_memory_create_snapshot(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    const char* language,
    uint32_t* snapshot_id
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !language || !snapshot_id) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Initialize snapshot registry if needed
    if (!g_snapshot_registry.snapshots) {
        g_snapshot_registry.capacity = 16;
        g_snapshot_registry.snapshots = polycall_core_malloc(ctx, 
            g_snapshot_registry.capacity * sizeof(memory_snapshot_t));
        
        if (!g_snapshot_registry.snapshots) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to initialize snapshot registry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Check if we need to expand the snapshot registry
    if (g_snapshot_registry.count >= g_snapshot_registry.capacity) {
        size_t new_capacity = g_snapshot_registry.capacity * 2;
        memory_snapshot_t* new_snapshots = polycall_core_realloc(ctx, 
            g_snapshot_registry.snapshots, 
            new_capacity * sizeof(memory_snapshot_t));
        
        if (!new_snapshots) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to expand snapshot registry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        g_snapshot_registry.snapshots = new_snapshots;
        g_snapshot_registry.capacity = new_capacity;
    }
    
    // Lock ownership registry to ensure consistency
    ownership_registry_lock(memory_mgr->ownership);
    
    // Create new snapshot
    memory_snapshot_t* snapshot = &g_snapshot_registry.snapshots[g_snapshot_registry.count];
    snapshot->id = ++memory_mgr->snapshot_counter;
    snapshot->region_count = memory_mgr->ownership->count;
    snapshot->creator_language = language;
    
    // Copy all region descriptors
    snapshot->regions = polycall_core_malloc(ctx, 
        snapshot->region_count * sizeof(memory_region_descriptor_t*));
    
    if (!snapshot->regions) {
        ownership_registry_unlock(memory_mgr->ownership);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate memory for snapshot regions");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < memory_mgr->ownership->count; i++) {
        memory_region_descriptor_t* src = memory_mgr->ownership->regions[i];
        memory_region_descriptor_t* copy = polycall_core_malloc(ctx, 
            sizeof(memory_region_descriptor_t));
        
        if (!copy) {
            // Clean up already allocated regions
            for (size_t j = 0; j < i; j++) {
                polycall_core_free(ctx, snapshot->regions[j]);
            }
            polycall_core_free(ctx, snapshot->regions);
            ownership_registry_unlock(memory_mgr->ownership);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for snapshot region copy");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy region descriptor
        *copy = *src;
        snapshot->regions[i] = copy;
    }
    
    ownership_registry_unlock(memory_mgr->ownership);
    
    // Increment snapshot count
    g_snapshot_registry.count++;
    
    // Return snapshot ID
    *snapshot_id = snapshot->id;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Restore a memory snapshot
 */
polycall_core_error_t polycall_memory_restore_snapshot(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    memory_manager_t* memory_mgr,
    uint32_t snapshot_id,
    const char* language
) {
    if (!ctx || !ffi_ctx || !memory_mgr || !language) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find the snapshot
    memory_snapshot_t* snapshot = NULL;
    size_t snapshot_index = SIZE_MAX;
    
    for (size_t i = 0; i < g_snapshot_registry.count; i++) {
        if (g_snapshot_registry.snapshots[i].id == snapshot_id) {
            snapshot = &g_snapshot_registry.snapshots[i];
            snapshot_index = i;
            break;
        }
    }
    
    if (!snapshot) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Snapshot with ID %u not found", snapshot_id);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Only the creator can restore a snapshot
    if (strcmp(snapshot->creator_language, language) != 0) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_PERMISSION_DENIED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Language '%s' cannot restore snapshot created by '%s'",
                          language, snapshot->creator_language);
        return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
    }
    
    // Lock ownership registry to ensure consistency
    ownership_registry_lock(memory_mgr->ownership);
    
    // First, free all existing regions to avoid memory leaks
    for (size_t i = 0; i < memory_mgr->ownership->count; i++) {
        memory_region_descriptor_t* desc = memory_mgr->ownership->regions[i];
        
        // Free memory if allocated from shared pool
        if (memory_mgr->shared_pool) {
            polycall_memory_free(ctx, memory_mgr->shared_pool, desc->ptr);
        } else {
            polycall_core_free(ctx, desc->ptr);
        }
        
        // Free descriptor
        polycall_core_free(ctx, desc);
    }
    
    // Clear the registry
    memory_mgr->ownership->count = 0;
    
    // Restore regions from snapshot
    for (size_t i = 0; i < snapshot->region_count; i++) {
        memory_region_descriptor_t* src = snapshot->regions[i];
        memory_region_descriptor_t* copy = polycall_core_malloc(ctx, 
            sizeof(memory_region_descriptor_t));
        
        if (!copy) {
            // This is a critical error - we've already cleared the registry
            ownership_registry_unlock(memory_mgr->ownership);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for restored region");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Allocate memory for the region
        void* ptr = NULL;
        if (memory_mgr->shared_pool) {
            ptr = polycall_memory_alloc(ctx, memory_mgr->shared_pool, src->size, POLYCALL_MEMORY_FLAG_NONE);
        } else {
            ptr = polycall_core_malloc(ctx, src->size);
        }
        
        if (!ptr) {
            polycall_core_free(ctx, copy);
            ownership_registry_unlock(memory_mgr->ownership);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to allocate memory for restored region content");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy region content
        memcpy(ptr, src->ptr, src->size);
        
        // Copy region descriptor
        *copy = *src;
        copy->ptr = ptr;  // Update with new pointer
        
        // Add to registry
        if (!ownership_registry_add(memory_mgr->ownership, copy)) {
            if (memory_mgr->shared_pool) {
                polycall_memory_free(ctx, memory_mgr->shared_pool, ptr);
            } else {
                polycall_core_free(ctx, ptr);
            }
            polycall_core_free(ctx, copy);
            ownership_registry_unlock(memory_mgr->ownership);
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                              POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to add restored region to registry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    ownership_registry_unlock(memory_mgr->ownership);
    
    // Remove the snapshot
    if (snapshot_index < g_snapshot_registry.count - 1) {
        // Move last snapshot to this position
        g_snapshot_registry.snapshots[snapshot_index] = 
            g_snapshot_registry.snapshots[g_snapshot_registry.count - 1];
    }
    g_snapshot_registry.count--;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Create a default memory bridge configuration
 */
memory_bridge_config_t polycall_memory_bridge_create_default_config(void) {
    memory_bridge_config_t config = {
        .shared_pool_size = 1024 * 1024, // 1MB default shared pool
        .ownership_capacity = 1024,      // Initial capacity for 1024 regions
        .reference_capacity = 1024,      // Initial capacity for 1024 references
        .enable_gc_notification = true,  // Enable GC notifications
        .gc_callback = NULL,             // No default callback
        .user_data = NULL                // No default user data
    };
    
    return config;
}
