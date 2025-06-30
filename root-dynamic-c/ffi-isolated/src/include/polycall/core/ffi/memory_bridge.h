/**
 * @file memory_bridge.h
 * @brief Memory management bridge for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the memory bridge for LibPolyCall FFI, which enables
 * safe memory sharing between different language runtimes with ownership
 * tracking and garbage collection integration.
 */

 #ifndef POLYCALL_FFI_MEMORY_BRIDGE_H_H
 #define POLYCALL_FFI_MEMORY_BRIDGE_H_H
 
 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
// Memory region descriptor
struct memory_region_descriptor {
    void* ptr;                           // Memory region pointer
    size_t size;                         // Memory region size
    const char* owner;                   // Current owner (language)
    uint32_t ref_count;                  // Reference count
    polycall_memory_permissions_t permissions; // Memory permissions
    polycall_memory_share_flags_t flags;  // Sharing flags
};

// Ownership registry
struct ownership_registry {
    memory_region_descriptor_t** regions; // Array of region descriptors
    size_t count;                        // Number of regions
    size_t capacity;                     // Maximum number of regions
    polycall_core_context_t* ctx;        // Core context
    pthread_mutex_t mutex;               // Thread safety mutex
};

// Reference counter
struct reference_counter {
    void** ptrs;                         // Pointers being tracked
    uint32_t* counts;                    // Reference counts
    size_t count;                        // Number of tracked pointers
    size_t capacity;                     // Maximum number of tracked pointers
    pthread_mutex_t mutex;               // Thread safety mutex
};

// GC callback registry entry
typedef struct {
    const char* language;                // Language identifier
    gc_notification_callback_t callback; // Callback function
    void* user_data;                     // User data
} gc_callback_entry_t;

// Memory manager configuration
typedef struct {
    size_t shared_pool_size;             // Size of shared memory pool
    size_t ownership_capacity;           // Capacity of ownership tracking
    size_t reference_capacity;           // Capacity of reference tracking
    bool enable_gc_notification;         // Enable GC notifications
    bool (*is_compatible_language)(      // Language compatibility check
        const char* lang1, 
        const char* lang2
    );
    uint32_t flags;                      // Configuration flags
} memory_manager_config_t;

// Memory manager
struct memory_manager {
    polycall_memory_pool_t* shared_pool; // Shared memory pool
    ownership_registry_t* ownership;     // Ownership registry
    reference_counter_t* ref_counts;     // Reference counter
    gc_callback_entry_t* gc_callbacks;   // GC callbacks
    size_t gc_callback_count;            // Number of GC callbacks
    size_t gc_callback_capacity;         // Maximum number of GC callbacks
    memory_manager_config_t config;      // Configuration
    pthread_mutex_t gc_mutex;            // GC synchronization mutex
    uint32_t snapshot_counter;           // Snapshot counter
    polycall_core_context_t* ctx;        // Core context
};

// Memory region information
typedef struct {
    void* ptr;                           // Memory region pointer
    size_t size;                         // Memory region size
    const char* owner;                   // Current owner (language)
    uint32_t ref_count;                  // Reference count
    polycall_memory_permissions_t permissions; // Memory permissions
    polycall_memory_share_flags_t flags;  // Sharing flags
} memory_region_info_t;

// Memory access flags
typedef enum {
    POLYCALL_MEMORY_ACCESS_NONE = 0,
    POLYCALL_MEMORY_ACCESS_READ = (1 << 0),     // Read access
    POLYCALL_MEMORY_ACCESS_WRITE = (1 << 1),    // Write access
    POLYCALL_MEMORY_ACCESS_INCREMENT_REF = (1 << 2) // Increment reference count
} polycall_memory_access_flags_t;

// Memory configuration flags
typedef enum {
    POLYCALL_MEMORY_CONFIG_NONE = 0,
    POLYCALL_MEMORY_CONFIG_STRICT_OWNERSHIP = (1 << 0), // Strict ownership checking
    POLYCALL_MEMORY_CONFIG_AUTO_GC = (1 << 1),         // Automatic GC integration
    POLYCALL_MEMORY_CONFIG_THREAD_SAFE = (1 << 2)      // Thread safety enabled
} polycall_memory_config_flags_t;

// Memory flags
typedef enum {
    POLYCALL_MEMORY_FLAG_NONE = 0,
    POLYCALL_MEMORY_FLAG_IN_GC = (1 << 0),              // In GC phase
    POLYCALL_MEMORY_FLAG_MARKED_FOR_COLLECTION = (1 << 1), // Marked for collection
    POLYCALL_MEMORY_FLAG_AUTO_FREE = (1 << 2)           // Auto-free when ref count = 0
} polycall_memory_flags_t;

// GC phase
typedef enum {
    POLYCALL_MEMORY_GC_PHASE_START = 0,   // Start of GC
    POLYCALL_MEMORY_GC_PHASE_END          // End of GC
} polycall_memory_gc_phase_t;

// Error codes specific to memory module
#define POLYCALL_FFI_MEMORY_BRIDGE_H_H
#define POLYCALL_FFI_MEMORY_BRIDGE_H_H
#define POLYCALL_FFI_MEMORY_BRIDGE_H_H

// Forward declarations
static bool ownership_registry_add(ownership_registry_t* registry, memory_region_descriptor_t* region_desc);
static memory_region_descriptor_t* ownership_registry_find(ownership_registry_t* registry, void* ptr);
static void ownership_registry_lock(ownership_registry_t* registry);
static void ownership_registry_unlock(ownership_registry_t* registry);
 /**
  * @brief Memory manager (opaque)
  */
 typedef struct memory_manager memory_manager_t;
 
 /**
  * @brief Ownership registry (opaque)
  */
 typedef struct ownership_registry ownership_registry_t;
 
 /**
  * @brief Reference counter (opaque)
  */
 typedef struct reference_counter reference_counter_t;
 
 /**
  * @brief GC notification callback type
  */
 typedef void (*gc_notification_callback_t)(
     polycall_core_context_t* ctx,
     const char* language,
     void* ptr,
     size_t size,
     void* user_data
 );
 
 /**
  * @brief Memory sharing flags
  */
 typedef enum {
     POLYCALL_MEMORY_SHARE_NONE = 0,
     POLYCALL_MEMORY_SHARE_READ_ONLY = (1 << 0),    /**< Read-only sharing */
     POLYCALL_MEMORY_SHARE_COPY = (1 << 1),         /**< Share a copy */
     POLYCALL_MEMORY_SHARE_TRANSFER = (1 << 2),     /**< Transfer ownership */
     POLYCALL_MEMORY_SHARE_REFERENCE = (1 << 3),    /**< Share by reference */
     POLYCALL_MEMORY_SHARE_TEMPORARY = (1 << 4),    /**< Temporary sharing */
     POLYCALL_MEMORY_SHARE_PERSISTENT = (1 << 5),   /**< Persistent sharing */
     POLYCALL_MEMORY_SHARE_USER = (1 << 16)         /**< Start of user-defined flags */
 } polycall_memory_share_flags_t;
 
 /**
  * @brief Memory bridge configuration
  */
 typedef struct {
     size_t shared_pool_size;           /**< Size of shared memory pool */
     size_t ownership_capacity;         /**< Capacity of ownership tracking */
     size_t reference_capacity;         /**< Capacity of reference tracking */
     bool enable_gc_notification;       /**< Enable GC notifications */
     gc_notification_callback_t gc_callback; /**< GC notification callback */
     void* user_data;                   /**< User data */
 } memory_bridge_config_t;
 
 /**
  * @brief Memory region descriptor
  */
 typedef struct {
     void* ptr;                         /**< Memory region pointer */
     size_t size;                       /**< Memory region size */
     const char* owner;                 /**< Current owner (language) */
     uint32_t ref_count;                /**< Reference count */
     polycall_memory_permissions_t permissions; /**< Memory permissions */
     polycall_memory_share_flags_t flags; /**< Sharing flags */
 } memory_region_descriptor_t;
 
 /**
  * @brief Initialize memory bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Pointer to receive memory manager
  * @param config Memory bridge configuration
  * @return Error code
  */
 polycall_core_error_t polycall_memory_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t** memory_mgr,
     const memory_bridge_config_t* config
 );
 
 /**
  * @brief Clean up memory bridge
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager to clean up
  */
 void polycall_memory_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr
 );
 
 /**
  * @brief Share memory across language boundaries
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param size Memory size
  * @param source_language Source language
  * @param target_language Target language
  * @param flags Sharing flags
  * @param region_desc Pointer to receive region descriptor
  * @return Error code
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
 );
 
 /**
  * @brief Track a memory reference
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param size Memory size
  * @param language Language owning the reference
  * @return Error code
  */
 polycall_core_error_t polycall_memory_track_reference(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     void* ptr,
     size_t size,
     const char* language
 );
 
 /**
  * @brief Acquire shared memory
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param language Language acquiring the memory
  * @param permissions Requested permissions
  * @return Error code
  */
 polycall_core_error_t polycall_memory_acquire(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     void* ptr,
     const char* language,
     polycall_memory_permissions_t permissions
 );
 
 /**
  * @brief Release shared memory
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param language Language releasing the memory
  * @return Error code
  */
 polycall_core_error_t polycall_memory_release(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     void* ptr,
     const char* language
 );
 
 /**
  * @brief Allocate memory from shared pool
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param size Memory size
  * @param language Owner language
  * @param flags Memory flags
  * @return Allocated memory pointer
  */
 void* polycall_memory_alloc_shared(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     size_t size,
     const char* language,
     polycall_memory_flags_t flags
 );
 
 /**
  * @brief Free memory from shared pool
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param language Language requesting free
  * @return Error code
  */
 polycall_core_error_t polycall_memory_free_shared(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     void* ptr,
     const char* language
 );
 
 /**
  * @brief Synchronize memory changes
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param size Memory size
  * @param source_language Source language
  * @return Error code
  */
 polycall_core_error_t polycall_memory_synchronize(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     void* ptr,
     size_t size,
     const char* source_language
 );
 
 /**
  * @brief Register GC notification callback
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param language Language for notifications
  * @param callback Notification callback
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_memory_register_gc_callback(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     const char* language,
     gc_notification_callback_t callback,
     void* user_data
 );
 
 /**
  * @brief Notify of GC event
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param language Language triggering GC
  * @return Error code
  */
 polycall_core_error_t polycall_memory_notify_gc(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     const char* language
 );
 
 /**
  * @brief Get memory region information
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param ptr Memory pointer
  * @param region_desc Pointer to receive region descriptor
  * @return Error code
  */
 polycall_core_error_t polycall_memory_get_region_info(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     void* ptr,
     memory_region_descriptor_t** region_desc
 );
 
 /**
  * @brief Create a memory snapshot
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param language Language initiating snapshot
  * @param snapshot_id Pointer to receive snapshot ID
  * @return Error code
  */
 polycall_core_error_t polycall_memory_create_snapshot(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     const char* language,
     uint32_t* snapshot_id
 );
 
 /**
  * @brief Restore a memory snapshot
  *
  * @param ctx Core context
  * @param ffi_ctx FFI context
  * @param memory_mgr Memory manager
  * @param snapshot_id Snapshot ID
  * @param language Language initiating restore
  * @return Error code
  */
 polycall_core_error_t polycall_memory_restore_snapshot(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     memory_manager_t* memory_mgr,
     uint32_t snapshot_id,
     const char* language
 );
 
 /**
  * @brief Create a default memory bridge configuration
  *
  * @return Default configuration
  */
 memory_bridge_config_t polycall_memory_bridge_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_MEMORY_BRIDGE_H_H */