/**
 * @file polycall_memory.h
 * @brief Memory management for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the memory management system for LibPolyCall,
 * providing efficient memory allocation, tracking, and isolation.
 */

#ifndef POLYCALL_MEMORY_H
#define POLYCALL_MEMORY_H

#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory allocation alignment
 */
#define POLYCALL_MEMORY_ALIGNMENT 16

/**
 * @brief Memory block header size
 */
#define POLYCALL_MEMORY_BLOCK_HEADER_SIZE sizeof(memory_block_header_t)

/**
 * @brief Minimum block size
 */
#define POLYCALL_MEMORY_MIN_BLOCK_SIZE 32

/**
 * @brief Memory block magic number for validation
 */
#define POLYCALL_MEMORY_BLOCK_MAGIC 0x50434D45 /* 'PCME' */
#define POLYCALL_POLYCALL_POLYCALL_MEMORY_H_H

/**
 * @brief Memory flags
 */
typedef enum {
  POLYCALL_MEMORY_FLAG_NONE = 0,
  POLYCALL_MEMORY_FLAG_ZERO_INIT = (1 << 0), /**< Zero-initialize memory */
  POLYCALL_MEMORY_FLAG_SECURE =
      (1 << 1), /**< Secure memory (cleared on free) */
  POLYCALL_MEMORY_FLAG_LOCKED =
      (1 << 2), /**< Locked memory (cannot be reallocated) */
  POLYCALL_MEMORY_FLAG_PERSISTENT =
      (1 << 3), /**< Persistent memory (survives resets) */
  POLYCALL_MEMORY_FLAG_SHARED = (1 << 4), /**< Shared memory */
  POLYCALL_MEMORY_FLAG_ISOLATED =
      (1 << 5), /**< Isolated memory (not shared with other components) */
  POLYCALL_MEMORY_FLAG_CUSTOM_ALLOC =
      (1 << 6) /**< Use custom allocation function */
} polycall_memory_flags_t;

/**
 * @brief Memory subsystem error codes
 */
typedef enum {
  POLYCALL_MEMORY_SUCCESS = 0,
  POLYCALL_MEMORY_ERROR_ALLOCATION_FAILED,
  POLYCALL_MEMORY_ERROR_INVALID_ADDRESS,
  POLYCALL_MEMORY_ERROR_OUT_OF_BOUNDS,
  POLYCALL_MEMORY_ERROR_ALIGNMENT,
  POLYCALL_MEMORY_ERROR_DOUBLE_FREE,
  POLYCALL_MEMORY_ERROR_LEAK_DETECTED,
  POLYCALL_MEMORY_ERROR_POOL_EXHAUSTED,
  POLYCALL_MEMORY_ERROR_INVALID_SIZE
} polycall_memory_error_t;
/**
 * @brief Memory permissions
 *
 * This enum defines the possible permissions for memory regions.
 * Permissions can be combined using bitwise OR.
 *
 * - POLYCALL_MEMORY_PERM_NONE: No permissions.
 * - POLYCALL_MEMORY_PERM_READ: Read permission.
 * - POLYCALL_MEMORY_PERM_WRITE: Write permission.
 * - POLYCALL_MEMORY_PERM_EXECUTE: Execute permission.
 */
typedef enum {
  POLYCALL_MEMORY_PERM_NONE = 0,
  POLYCALL_MEMORY_PERM_READ = (1 << 0),   /**< Read permission */
  POLYCALL_MEMORY_PERM_WRITE = (1 << 1),  /**< Write permission */
  POLYCALL_MEMORY_PERM_EXECUTE = (1 << 2) /**< Execute permission */
} polycall_memory_permissions_t;

/**
 * @brief Memory block header
 */
typedef struct memory_block_header {
  uint32_t magic;                   /**< Magic number for validation */
  struct memory_block_header *next; /**< Next block in list */
  struct memory_block_header *prev; /**< Previous block in list */
  size_t size;                      /**< Block size (excluding header) */
  polycall_memory_flags_t flags;    /**< Block flags */
  bool is_free;                     /**< Whether the block is free */
  const char *owner;                /**< Block owner identifier */
} memory_block_header_t;

/**
 * @brief Memory statistics
 */
typedef struct {
  size_t total_allocated;    /**< Total bytes allocated */
  size_t total_freed;        /**< Total bytes freed */
  size_t current_usage;      /**< Current memory usage */
  size_t peak_usage;         /**< Peak memory usage */
  size_t allocation_count;   /**< Number of allocations */
  size_t free_count;         /**< Number of frees */
  size_t failed_allocations; /**< Number of failed allocations */
  size_t pool_capacity;      /**< Total pool capacity */
  size_t pool_available;     /**< Available pool memory */
} polycall_memory_stats_t;

/**
 * @brief Memory region
 */
typedef struct {
  void *base;                          /**< Base address */
  size_t size;                         /**< Region size */
  polycall_memory_permissions_t perms; /**< Access permissions */
  polycall_memory_flags_t flags;       /**< Region flags */
  const char *owner;                   /**< Region owner */
  char shared_with[64]; /**< Component the region is shared with */
} polycall_memory_region_t;

/**
 * @brief Memory pool
 */
typedef struct {
  void *base;                       /**< Base address */
  size_t size;                      /**< Total size */
  size_t used;                      /**< Used size */
  size_t peak_usage;                /**< Peak usage */
  size_t allocation_count;          /**< Number of allocations */
  size_t free_count;                /**< Number of frees */
  size_t failed_allocations;        /**< Number of failed allocations */
  memory_block_header_t *free_list; /**< List of free blocks */
  memory_block_header_t *used_list; /**< List of used blocks */
  void *(*custom_malloc)(size_t size,
                         void *user_data); /**< Custom malloc function */
  void (*custom_free)(void *ptr, void *user_data); /**< Custom free function */
  void *alloc_user_data; /**< User data for allocation functions */
} polycall_memory_pool_t;

/**
 * @brief Create a memory pool
 *
 * @param ctx Core context
 * @param pool Pointer to receive pool
 * @param size Pool size in bytes
 * @return Error code
 */
polycall_core_error_t polycall_memory_create_pool(polycall_core_context_t *ctx,
                                                  polycall_memory_pool_t **pool,
                                                  size_t size);

/**
 * @brief Destroy a memory pool
 *
 * @param ctx Core context
 * @param pool Pool to destroy
 */
void polycall_memory_destroy_pool(polycall_core_context_t *ctx,
                                  polycall_memory_pool_t *pool);

/**
 * @brief Allocate memory from a pool
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @param size Size to allocate
 * @param flags Allocation flags
 * @return Allocated memory, or NULL on failure
 */
void *polycall_memory_alloc(polycall_core_context_t *ctx,
                            polycall_memory_pool_t *pool, size_t size,
                            polycall_memory_flags_t flags);

/**
 * @brief Free memory allocated from a pool
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @param ptr Memory to free
 */
void polycall_memory_free(polycall_core_context_t *ctx,
                          polycall_memory_pool_t *pool, void *ptr);

/**
 * @brief Reallocate memory from a pool
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @param ptr Memory to reallocate
 * @param size New size
 * @return Reallocated memory, or NULL on failure
 */
void *polycall_memory_realloc(polycall_core_context_t *ctx,
                              polycall_memory_pool_t *pool, void *ptr,
                              size_t size);

/**
 * @brief Create a memory region
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @param size Region size
 * @param perms Access permissions
 * @param flags Region flags
 * @param owner Region owner
 * @return Memory region, or NULL on failure
 */
polycall_memory_region_t *
polycall_memory_create_region(polycall_core_context_t *ctx,
                              polycall_memory_pool_t *pool, size_t size,
                              polycall_memory_permissions_t perms,
                              polycall_memory_flags_t flags, const char *owner);

/**
 * @brief Destroy a memory region
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @param region Region to destroy
 */
void polycall_memory_destroy_region(polycall_core_context_t *ctx,
                                    polycall_memory_pool_t *pool,
                                    polycall_memory_region_t *region);

/**
 * @brief Share a memory region with another component
 *
 * @param ctx Core context
 * @param region Region to share
 * @param component Component to share with
 * @return Error code
 */
polycall_core_error_t
polycall_memory_share_region(polycall_core_context_t *ctx,
                             polycall_memory_region_t *region,
                             const char *component);

/**
 * @brief Unshare a memory region
 *
 * @param ctx Core context
 * @param region Region to unshare
 * @return Error code
 */
polycall_core_error_t
polycall_memory_unshare_region(polycall_core_context_t *ctx,
                               polycall_memory_region_t *region);

/**
 * @brief Get memory pool statistics
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @param stats Pointer to receive statistics
 * @return Error code
 */
polycall_core_error_t polycall_memory_get_stats(polycall_core_context_t *ctx,
                                                polycall_memory_pool_t *pool,
                                                polycall_memory_stats_t *stats);

/**
 * @brief Verify memory region permissions
 *
 * @param ctx Core context
 * @param region Memory region
 * @param component Component requesting access
 * @param required_perms Required permissions
 * @return true if the component has the required permissions, false otherwise
 */
bool polycall_memory_verify_permissions(
    polycall_core_context_t *ctx, const polycall_memory_region_t *region,
    const char *component, polycall_memory_permissions_t required_perms);

/**
 * @brief Reset a memory pool
 *
 * @param ctx Core context
 * @param pool Memory pool
 * @return Error code
 */
polycall_core_error_t polycall_memory_reset_pool(polycall_core_context_t *ctx,
                                                 polycall_memory_pool_t *pool);

#ifdef __cplusplus
}
#endif
#endif /* POLYCALL_MEMORY_H */
#endif /* POLYCALL_POLYCALL_POLYCALL_MEMORY_H_H */
