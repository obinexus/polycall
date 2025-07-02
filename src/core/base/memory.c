/**
#include "polycall/core/polycall/memory.h"

 * @file memory.c
 * @brief Memory management implementation for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file implements the memory management functions defined in
 * polycall_memory.h, providing efficient memory allocation, tracking,
 * and isolation in alignment with the Program-First approach.
 */
 #include "polycall/core/polycall/polycall_memory.h"


 
 // Alignment helpers
 static size_t align_size(size_t size) {
     return (size + (MEMORY_ALIGNMENT - 1)) & ~(MEMORY_ALIGNMENT - 1);
 }
 
static void* __attribute__((unused)) align_ptr(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    addr = (addr + (MEMORY_ALIGNMENT - 1)) & ~(MEMORY_ALIGNMENT - 1);
    return (void*)addr;
}
 
 // Block validation
 static bool validate_block(memory_block_header_t* block) {
     return block && block->magic == MEMORY_BLOCK_MAGIC;
 }
 
 // Memory block operations
 static memory_block_header_t* create_block(void* addr, size_t size, bool is_free) {
     memory_block_header_t* block = (memory_block_header_t*)addr;
     block->size = size - MEMORY_HEADER_SIZE;
     block->next = NULL;
     block->prev = NULL;
     block->flags = POLYCALL_MEMORY_FLAG_NONE;
     block->is_free = is_free;
     block->magic = MEMORY_BLOCK_MAGIC;
     block->owner = NULL;
     return block;
 }
 
 static void* block_to_data(memory_block_header_t* block) {
     return (void*)((char*)block + MEMORY_HEADER_SIZE);
 }
 
 static memory_block_header_t* data_to_block(void* data) {
     if (!data) return NULL;
     return (memory_block_header_t*)((char*)data - MEMORY_HEADER_SIZE);
 }
 
 // List operations
 static void add_to_list(memory_block_header_t** list, memory_block_header_t* block) {
     block->next = *list;
     block->prev = NULL;
     if (*list) {
         (*list)->prev = block;
     }
     *list = block;
 }
 
 static void remove_from_list(memory_block_header_t** list, memory_block_header_t* block) {
     if (block->prev) {
         block->prev->next = block->next;
     } else {
         *list = block->next;
     }
     
     if (block->next) {
         block->next->prev = block->prev;
     }
     
     block->next = NULL;
     block->prev = NULL;
 }
 
 // Memory pool operations
 static void init_memory_pool(
     polycall_memory_pool_t* pool,
     void* base,
     size_t size
 ) {
     pool->base = base;
     pool->size = size;
     pool->used = 0;
     pool->peak_usage = 0;
     pool->allocation_count = 0;
     pool->free_count = 0;
     pool->failed_allocations = 0;
     
     // Create initial free block
     memory_block_header_t* initial_block = create_block(base, size, true);
     pool->free_list = initial_block;
     pool->used_list = NULL;
 }
 
 // Implementation of memory pool creation
 polycall_core_error_t polycall_memory_create_pool(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t** pool,
     size_t size
 ) {
     if (!ctx || !pool || size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure minimum size
     if (size < MEMORY_HEADER_SIZE + MEMORY_MIN_BLOCK_SIZE) {
         size = MEMORY_HEADER_SIZE + MEMORY_MIN_BLOCK_SIZE;
     }
     
     // Align size
     size = align_size(size);
     
     // Allocate pool structure
     polycall_memory_pool_t* new_pool = polycall_core_malloc(ctx, sizeof(polycall_memory_pool_t));
     if (!new_pool) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Allocate pool memory
     void* pool_memory = polycall_core_malloc(ctx, size);
     if (!pool_memory) {
         polycall_core_free(ctx, new_pool);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize pool
     init_memory_pool(new_pool, pool_memory, size);
     new_pool->custom_malloc = NULL;
     new_pool->custom_free = NULL;
     new_pool->alloc_user_data = NULL;
     
     *pool = new_pool;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Implementation of memory pool destruction
 void polycall_memory_destroy_pool(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool
 ) {
     if (!ctx || !pool) return;
     
     // Check for memory leaks
     if (pool->used > 0) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_WARNING, "Memory pool destroyed with %zu bytes still allocated",
                           pool->used);
     }
     
     // Free pool memory
     if (pool->base) {
         polycall_core_free(ctx, pool->base);
     }
     
     // Free pool structure
     polycall_core_free(ctx, pool);
 }
 
 // Find a free block of sufficient size
 static memory_block_header_t* find_free_block(polycall_memory_pool_t* pool, size_t size) {
     memory_block_header_t* current = pool->free_list;
     while (current) {
         if (current->size >= size) {
             return current;
         }
         current = current->next;
     }
     return NULL;
 }
 
 // Split a block if it's large enough
 static void split_block(
     polycall_memory_pool_t* pool,
     memory_block_header_t* block,
     size_t size
 ) {
     // Only split if the remaining size is enough for a new block
     size_t remaining_size = block->size - size;
     if (remaining_size > MEMORY_HEADER_SIZE + MEMORY_MIN_BLOCK_SIZE) {
         // Create new block from the extra space
         void* new_block_addr = (char*)block_to_data(block) + size;
         memory_block_header_t* new_block = create_block(new_block_addr, remaining_size + MEMORY_HEADER_SIZE, true);
         
         // Update original block size
         block->size = size;
         
         // Add new block to free list
         add_to_list(&pool->free_list, new_block);
     }
 }
 
 // Implementation of memory allocation
 void* polycall_memory_alloc(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool,
     size_t size,
     polycall_memory_flags_t flags
 ) {
     if (!ctx || !pool || size == 0) {
         if (pool) {
             pool->failed_allocations++;
         }
         return NULL;
     }
     
     // Use custom allocator if provided
     if (pool->custom_malloc) {
         void* ptr = pool->custom_malloc(size, pool->alloc_user_data);
         if (!ptr) {
             pool->failed_allocations++;
             return NULL;
         }
         
         pool->used += size;
         if (pool->used > pool->peak_usage) {
             pool->peak_usage = pool->used;
         }
         pool->allocation_count++;
         
         return ptr;
     }
     
     // Align size
     size = align_size(size);
     
     // Find a free block
     memory_block_header_t* block = find_free_block(pool, size);
     if (!block) {
         pool->failed_allocations++;
         return NULL;
     }
     
     // Remove from free list
     remove_from_list(&pool->free_list, block);
     
     // Split block if necessary
     split_block(pool, block, size);
     
     // Update block state
     block->is_free = false;
     block->flags = flags;
     
     // Add to used list
     add_to_list(&pool->used_list, block);
     
     // Update pool stats
     pool->used += block->size + MEMORY_HEADER_SIZE;
     if (pool->used > pool->peak_usage) {
         pool->peak_usage = pool->used;
     }
     pool->allocation_count++;
     
     // Zero-initialize if requested
     void* data = block_to_data(block);
     if (flags & POLYCALL_MEMORY_FLAG_ZERO_INIT) {
         memset(data, 0, block->size);
     }
     
     return data;
 }
 
 // Implementation of memory free
 void polycall_memory_free(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool,
     void* ptr
 ) {
     if (!ctx || !pool || !ptr) {
         return;
     }
     
     // Use custom free if provided
     if (pool->custom_free) {
         pool->custom_free(ptr, pool->alloc_user_data);
         return;
     }
     
     // Get block from pointer
     memory_block_header_t* block = data_to_block(ptr);
     
     // Validate block
     if (!validate_block(block)) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, "Invalid memory block");
         return;
     }
     
     // Check if already freed
     if (block->is_free) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, "Double free detected");
         return;
     }
     
     // Clear sensitive data if needed
     if (block->flags & POLYCALL_MEMORY_FLAG_SECURE) {
         memset(ptr, 0, block->size);
     }
     
     // Update block state
     block->is_free = true;
     
     // Remove from used list
     remove_from_list(&pool->used_list, block);
     
     // Add to free list
     add_to_list(&pool->free_list, block);
     
     // Update pool stats
     pool->used -= block->size + MEMORY_HEADER_SIZE;
     pool->free_count++;
 }
 
 // Implementation of memory reallocation
 void* polycall_memory_realloc(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool,
     void* ptr,
     size_t size
 ) {
     if (!ctx || !pool) {
         return NULL;
     }
     
     // Handle NULL pointer (behave like malloc)
     if (!ptr) {
         return polycall_memory_alloc(ctx, pool, size, POLYCALL_MEMORY_FLAG_NONE);
     }
     
     // Handle zero size (behave like free)
     if (size == 0) {
         polycall_memory_free(ctx, pool, ptr);
         return NULL;
     }
     
     // Get block from pointer
     memory_block_header_t* block = data_to_block(ptr);
     
     // Validate block
     if (!validate_block(block)) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, "Invalid memory block");
         return NULL;
     }
     
     // Check if the block is locked
     if (block->flags & POLYCALL_MEMORY_FLAG_LOCKED) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, "Cannot reallocate locked memory block");
         return NULL;
     }
     
     // Align size
     size = align_size(size);
     
     // If new size fits in current block, just adjust the size
     if (size <= block->size) {
         // Split block if possible
         split_block(pool, block, size);
         return ptr;
     }
     
     // Need to allocate a new block
     void* new_ptr = polycall_memory_alloc(ctx, pool, size, block->flags);
     if (!new_ptr) {
         return NULL;
     }
     
     // Copy data to new block
     memcpy(new_ptr, ptr, block->size);
     
     // Free old block
     polycall_memory_free(ctx, pool, ptr);
     
     return new_ptr;
 }
 
 // Implementation of memory region creation
 polycall_memory_region_t* polycall_memory_create_region(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool,
     size_t size,
     polycall_memory_permissions_t perms,
     polycall_memory_flags_t flags,
     const char* owner
 ) {
     if (!ctx || !pool || size == 0 || !owner) {
         return NULL;
     }
     
     // Allocate memory for the region
     void* region_mem = polycall_memory_alloc(ctx, pool, sizeof(polycall_memory_region_t), POLYCALL_MEMORY_FLAG_ZERO_INIT);
     if (!region_mem) {
         return NULL;
     }
     
     // Allocate memory for the region's base
     void* base = polycall_memory_alloc(ctx, pool, size, flags);
     if (!base) {
         polycall_memory_free(ctx, pool, region_mem);
         return NULL;
     }
     
     // Initialize region structure
     polycall_memory_region_t* region = (polycall_memory_region_t*)region_mem;
     region->base = base;
     region->size = size;
     region->perms = perms;
     region->flags = flags;
     region->owner = owner;
     region->shared_with[0] = '\0';  // Not shared initially
     
     return region;
 }
 
 // Implementation of memory region destruction
 void polycall_memory_destroy_region(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool,
     polycall_memory_region_t* region
 ) {
     if (!ctx || !pool || !region) {
         return;
     }
     
     // Free the region's base memory
     if (region->base) {
         polycall_memory_free(ctx, pool, region->base);
     }
     
     // Free the region structure
     polycall_memory_free(ctx, pool, region);
 }
 
 // Implementation of sharing a memory region
 polycall_core_error_t polycall_memory_share_region(
     polycall_core_context_t* ctx,
     polycall_memory_region_t* region,
     const char* component
 ) {
     if (!ctx || !region || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if the region is already shared
     if (region->shared_with[0] != '\0') {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check if the region can be shared
     if (region->flags & POLYCALL_MEMORY_FLAG_ISOLATED) {
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     // Share the region
     strncpy(region->shared_with, component, sizeof(region->shared_with) - 1);
     region->shared_with[sizeof(region->shared_with) - 1] = '\0';
     
     // Set the shared flag
     region->flags |= POLYCALL_MEMORY_FLAG_SHARED;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Implementation of unsharing a memory region
 polycall_core_error_t polycall_memory_unshare_region(
     polycall_core_context_t* ctx,
     polycall_memory_region_t* region
 ) {
     if (!ctx || !region) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if the region is shared
     if (region->shared_with[0] == '\0') {
         return POLYCALL_CORE_SUCCESS;  // Already not shared
     }
     
     // Unshare the region
     region->shared_with[0] = '\0';
     
     // Clear the shared flag
     region->flags &= ~POLYCALL_MEMORY_FLAG_SHARED;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Implementation of memory pool statistics retrieval
 polycall_core_error_t polycall_memory_get_stats(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool,
     polycall_memory_stats_t* stats
 ) {
     if (!ctx || !pool || !stats) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Fill statistics
     stats->total_allocated = pool->allocation_count;
     stats->total_freed = pool->free_count;
     stats->current_usage = pool->used;
     stats->peak_usage = pool->peak_usage;
     stats->allocation_count = pool->allocation_count;
     stats->free_count = pool->free_count;
     stats->failed_allocations = pool->failed_allocations;
     stats->pool_capacity = pool->size;
     stats->pool_available = pool->size - pool->used;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Implementation of permission verification
 bool polycall_memory_verify_permissions(
     polycall_core_context_t* ctx,
     const polycall_memory_region_t* region,
     const char* component,
     polycall_memory_permissions_t required_perms
 ) {
     if (!ctx || !region || !component) {
         return false;
     }
     
     // Owner has all permissions
     if (strcmp(region->owner, component) == 0) {
         return true;
     }
     
     // Component must be the one the region is shared with
     if (region->shared_with[0] == '\0' || strcmp(region->shared_with, component) != 0) {
         return false;
     }
     
     // Check if required permissions are a subset of region permissions
     return (region->perms & required_perms) == required_perms;
 }
 
 // Implementation of memory pool reset
 polycall_core_error_t polycall_memory_reset_pool(
     polycall_core_context_t* ctx,
     polycall_memory_pool_t* pool
 ) {
     if (!ctx || !pool) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check for persistent memory blocks
     memory_block_header_t* current = pool->used_list;
     while (current) {
         if (current->flags & POLYCALL_MEMORY_FLAG_PERSISTENT) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, POLYCALL_CORE_ERROR_INVALID_STATE,
                               POLYCALL_ERROR_SEVERITY_WARNING, 
                               "Cannot reset pool with persistent memory blocks");
             return POLYCALL_CORE_ERROR_INVALID_STATE;
         }
         current = current->next;
     }
     
     // Reset pool by reinitializing
     void* base = pool->base;
     size_t size = pool->size;
     
     pool->used_list = NULL;
     pool->free_list = NULL;
     pool->used = 0;
     pool->allocation_count = 0;
     pool->free_count = 0;
     
     // Create initial free block
     memory_block_header_t* initial_block = create_block(base, size, true);
     pool->free_list = initial_block;
     
     return POLYCALL_CORE_SUCCESS;
 }