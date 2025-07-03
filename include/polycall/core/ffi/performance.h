/**
 * @file performance.h
 * @brief Performance optimization module for LibPolyCall FFI
 * @author OBINexus Engineering - Aegis Project Phase 2
 *
 * This header has been refactored to eliminate type conflicts with
 * the core FFI type system while maintaining performance optimization capabilities.
 */

#ifndef POLYCALL_FFI_PERFORMANCE_H
#define POLYCALL_FFI_PERFORMANCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/* Critical: Include FFI types before using them */
#include "polycall/core/polycall/polycall_types.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_context.h"

/* Forward declarations to avoid circular dependencies */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_ffi_context polycall_ffi_context_t;

/* Include FFI type definitions */
#include "polycall/core/ffi/ffi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for performance module types */
typedef struct performance_manager performance_manager_t;
typedef struct perf_type_cache perf_type_cache_t;
typedef struct perf_call_cache perf_call_cache_t;
typedef struct call_cache call_cache_t;
typedef struct type_cache type_cache_t;

/**
 * @brief Performance-specific type cache entry
 * Note: Renamed to avoid conflict with polycall_memory.h
 */
typedef struct {
    polycall_ffi_type_t source_type;
    polycall_ffi_type_t target_type;
    const char *source_language;
    const char *target_language;
    void *converter_data;
    uint32_t access_count;
    uint64_t last_access_time;
} perf_type_cache_entry_t;

/**
 * @brief Performance call cache entry
 */
typedef struct {
    char *function_name;
    size_t arg_count;
    uint64_t hash;
    uint64_t result_hash;
    polycall_ffi_value_t *cached_result;
    uint64_t cache_time;
    uint32_t access_count;
} perf_cache_entry_t;

/**
 * @brief Call batch entry
 */
typedef struct {
    char *function_name;
    polycall_ffi_value_t *args;
    size_t arg_count;
    const char *target_language;
    uint32_t batch_id;
    uint32_t call_index;
} batch_entry_t;

/**
 * @brief Performance type cache structure
 */
struct perf_type_cache {
    perf_type_cache_entry_t *entries;
    size_t count;
    size_t capacity;
    pthread_mutex_t mutex;
};

/**
 * @brief Performance call cache structure
 */
struct perf_call_cache {
    perf_cache_entry_t *entries;
    size_t count;
    size_t capacity;
    uint32_t ttl_ms;
    pthread_mutex_t mutex;
};

/**
 * @brief Call optimization level
 */
typedef enum {
    POLYCALL_OPT_LEVEL_NONE = 0,
    POLYCALL_OPT_LEVEL_BASIC,
    POLYCALL_OPT_LEVEL_MODERATE,
    POLYCALL_OPT_LEVEL_AGGRESSIVE
} polycall_optimization_level_t;

/**
 * @brief Performance metrics
 */
typedef struct {
    uint64_t total_calls;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t type_conversions;
    uint64_t batched_calls;
    double avg_call_time_ms;
    double avg_conversion_time_ms;
} polycall_performance_metrics_t;

/**
 * @brief Performance configuration
 */
typedef struct {
    bool enable_call_cache;
    bool enable_type_cache;
    bool enable_batch_optimization;
    bool enable_profiling;
    size_t call_cache_size;
    size_t type_cache_size;
    uint32_t cache_ttl_ms;
    polycall_optimization_level_t opt_level;
} polycall_performance_config_t;

/**
 * @brief Performance manager structure
 */
struct performance_manager {
    polycall_ffi_context_t *ffi_ctx;
    perf_call_cache_t *call_cache;
    perf_type_cache_t *type_cache;
    polycall_performance_config_t config;
    polycall_performance_metrics_t metrics;
    pthread_mutex_t metrics_mutex;
};

/**
 * @brief Performance trace entry
 */
typedef struct {
    char *function_name;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    size_t memory_allocated;
    size_t memory_freed;
    bool cache_hit;
} polycall_trace_entry_t;

/**
 * @brief Initialize performance optimization
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config Performance configuration
 * @return Error code
 */
polycall_core_error_t polycall_performance_init(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    const polycall_performance_config_t *config);

/**
 * @brief Cleanup performance optimization
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 */
void polycall_performance_cleanup(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx);

/**
 * @brief Enable performance profiling
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param enable Enable/disable profiling
 * @return Error code
 */
polycall_core_error_t polycall_performance_enable_profiling(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    bool enable);

/**
 * @brief Get performance metrics
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param metrics Pointer to receive metrics
 * @return Error code
 */
polycall_core_error_t polycall_performance_get_metrics(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    polycall_performance_metrics_t *metrics);

/**
 * @brief Reset performance metrics
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @return Error code
 */
polycall_core_error_t polycall_performance_reset_metrics(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx);

/**
 * @brief Cache function call result
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param function_name Function name
 * @param args Function arguments
 * @param arg_count Argument count
 * @param result Function result
 * @param cached_result Pointer to receive cached result
 * @return Error code
 */
polycall_core_error_t polycall_performance_cache_call(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    const char *function_name,
    polycall_ffi_value_t *args,
    size_t arg_count,
    polycall_ffi_value_t **cached_result);

/**
 * @brief Lookup cached function call result
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param function_name Function name
 * @param args Function arguments
 * @param arg_count Argument count
 * @param result Pointer to receive result
 * @return Error code
 */
polycall_core_error_t polycall_performance_lookup_call(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    const char *function_name,
    polycall_ffi_value_t *args,
    size_t arg_count,
    polycall_ffi_value_t *result);

/**
 * @brief Begin batch optimization
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param batch_id Batch identifier
 * @param expected_calls Expected number of calls
 * @return Error code
 */
polycall_core_error_t polycall_performance_begin_batch(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    uint32_t batch_id,
    size_t expected_calls);

/**
 * @brief Execute batched calls
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param batch_id Batch identifier
 * @param results Pointer to receive results array
 * @param result_count Pointer to receive result count
 * @return Error code
 */
polycall_core_error_t polycall_performance_execute_batch(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    uint32_t batch_id,
    polycall_ffi_value_t ***results,
    size_t *result_count);

/**
 * @brief Clear performance caches
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @return Error code
 */
polycall_core_error_t polycall_performance_clear_caches(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx);

/**
 * @brief Set cache TTL
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param ttl_ms TTL in milliseconds
 * @return Error code
 */
polycall_core_error_t polycall_performance_set_cache_ttl(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    uint32_t ttl_ms);

/**
 * @brief Set optimization level
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param level Optimization level
 * @return Error code
 */
polycall_core_error_t polycall_performance_set_opt_level(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    polycall_optimization_level_t level);

/**
 * @brief Get performance trace
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param entries Pointer to receive trace entries
 * @param entry_count Pointer to receive entry count
 * @return Error code
 */
polycall_core_error_t polycall_performance_get_trace(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    polycall_trace_entry_t **entries,
    size_t *entry_count);

/**
 * @brief Clear performance trace
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @return Error code
 */
polycall_core_error_t polycall_performance_clear_trace(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx);

/**
 * @brief Export performance data
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param format Export format (e.g., "json", "csv")
 * @param buffer Buffer to receive exported data
 * @param buffer_size Pointer to buffer size (in/out)
 * @return Error code
 */
polycall_core_error_t polycall_performance_export(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    const char *format,
    void *buffer,
    size_t *buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_PERFORMANCE_H */