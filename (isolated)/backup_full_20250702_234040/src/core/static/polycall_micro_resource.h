/**
 * @file polycall_micro_resource.h
 * @brief Resource limitation and quota management for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the resource limitation and quota management interfaces
 * for the LibPolyCall micro command system.
 */

 #ifndef POLYCALL_MICRO_POLYCALL_MICRO_RESOURCE_H_H
 #define POLYCALL_MICRO_POLYCALL_MICRO_RESOURCE_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_micro_context.h"
 #include "polycall/core/polycall/polycall_micro_component.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct resource_limiter resource_limiter_t;
 typedef struct resource_quota resource_quota_t;
 
 /**
  * @brief Resource types that can be limited
  */
 typedef enum {
     POLYCALL_RESOURCE_MEMORY = 0,   // Memory resources
     POLYCALL_RESOURCE_CPU = 1,      // CPU resources
     POLYCALL_RESOURCE_IO = 2,       // I/O resources
     POLYCALL_RESOURCE_COUNT = 3     // Number of resource types
 } polycall_resource_type_t;
 
 /**
  * @brief Resource limit configuration
  */
 typedef struct {
     size_t memory_quota;            // Memory quota in bytes
     uint32_t cpu_quota;             // CPU quota in milliseconds
     uint32_t io_quota;              // I/O quota in operations
     bool enforce_limits;            // Whether to enforce limits
     bool track_usage;               // Whether to track usage
 } resource_limiter_config_t;
 
 /**
  * @brief Resource usage statistics
  */
 typedef struct {
     size_t memory_usage;            // Current memory usage in bytes
     size_t peak_memory_usage;       // Peak memory usage in bytes
     uint32_t cpu_usage;             // Current CPU usage in milliseconds
     uint32_t peak_cpu_usage;        // Peak CPU usage in milliseconds
     uint32_t io_usage;              // Current I/O usage in operations
     uint32_t peak_io_usage;         // Peak I/O usage in operations
     uint32_t limit_violations;      // Number of limit violations
     uint32_t memory_allocations;    // Number of memory allocations
     uint32_t memory_frees;          // Number of memory frees
 } polycall_resource_usage_t;
 
 /**
  * @brief Resource threshold callback function type
  */
 typedef void (*resource_threshold_callback_t)(
     polycall_core_context_t* ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     polycall_resource_type_t resource_type,
     size_t current_usage,
     size_t quota,
     void* user_data
 );
 
 /**
  * @brief Initialize resource limiter
  * 
  * @param ctx Core context
  * @param limiter Pointer to receive limiter
  * @param config Limiter configuration
  * @return Error code
  */
 polycall_core_error_t resource_limiter_init(
     polycall_core_context_t* ctx,
     resource_limiter_t** limiter,
     const resource_limiter_config_t* config
 );
 
 /**
  * @brief Clean up resource limiter
  * 
  * @param ctx Core context
  * @param limiter Limiter to clean up
  */
 void resource_limiter_cleanup(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter
 );
 
 /**
  * @brief Set resource quota
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param resource_type Resource type
  * @param quota Resource quota
  * @return Error code
  */
 polycall_core_error_t resource_limiter_set_quota(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     polycall_resource_type_t resource_type,
     size_t quota
 );
 
 /**
  * @brief Get resource quota
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param resource_type Resource type
  * @param quota Pointer to receive quota
  * @return Error code
  */
 polycall_core_error_t resource_limiter_get_quota(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     polycall_resource_type_t resource_type,
     size_t* quota
 );
 
 /**
  * @brief Allocate resource
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param resource_type Resource type
  * @param amount Amount to allocate
  * @return Error code (POLYCALL_CORE_ERROR_QUOTA_EXCEEDED if exceeds quota)
  */
 polycall_core_error_t resource_limiter_allocate(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     polycall_resource_type_t resource_type,
     size_t amount
 );
 
 /**
  * @brief Release resource
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param resource_type Resource type
  * @param amount Amount to release
  * @return Error code
  */
 polycall_core_error_t resource_limiter_release(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     polycall_resource_type_t resource_type,
     size_t amount
 );
 
 /**
  * @brief Get current resource usage
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param usage Pointer to receive usage statistics
  * @return Error code
  */
 polycall_core_error_t resource_limiter_get_usage(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     polycall_resource_usage_t* usage
 );
 
 /**
  * @brief Reset resource usage counters
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @return Error code
  */
 polycall_core_error_t resource_limiter_reset_usage(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter
 );
 
 /**
  * @brief Register resource threshold callback
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param resource_type Resource type
  * @param threshold Threshold percentage (0-100)
  * @param callback Callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t resource_limiter_register_threshold(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     polycall_resource_type_t resource_type,
     uint8_t threshold,
     resource_threshold_callback_t callback,
     void* user_data
 );
 
 /**
  * @brief Memory allocation wrapper for resource-limited components
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param size Size to allocate
  * @return Allocated memory or NULL if quota exceeded
  */
 void* resource_limiter_malloc(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     size_t size
 );
 
 /**
  * @brief Memory free wrapper for resource-limited components
  * 
  * @param ctx Core context
  * @param limiter Resource limiter
  * @param ptr Pointer to free
  */
 void resource_limiter_free(
     polycall_core_context_t* ctx,
     resource_limiter_t* limiter,
     void* ptr
 );
 
 /**
  * @brief Create default resource limiter configuration
  * 
  * @return Default configuration
  */
 resource_limiter_config_t resource_limiter_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_MICRO_POLYCALL_MICRO_RESOURCE_H_H */