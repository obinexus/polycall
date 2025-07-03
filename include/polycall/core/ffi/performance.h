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
#include "polycall/core/ffi/ffi_types.h"
#include "polycall/core/polycall/polycall_error.h"




#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for performance module types */
typedef struct performance_manager performance_manager_t;
typedef struct perf_type_cache perf_type_cache_t;
typedef struct perf_call_cache perf_call_cache_t;

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
    uint64_t total_execution_time_ns;
    uint64_t total_marshalling_time_ns;
    uint64_t batched_calls;
    uint64_t type_conversions;
    uint64_t memory_usage_bytes;
} polycall_performance_metrics_t;

/**
 * @brief Performance configuration
 */
typedef struct {
    polycall_optimization_level_t opt_level;
    bool enable_call_caching;
    bool enable_type_caching;
    bool enable_call_batching;
    bool enable_lazy_initialization;
    size_t cache_size;
    size_t batch_size;
    uint32_t cache_ttl_ms;
    void *user_data;
} performance_config_t;

/**
 * @brief Performance trace entry
 */
typedef struct {
    const char *function_name;
    const char *source_language;
    const char *target_language;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    uint64_t marshalling_time_ns;
    uint64_t execution_time_ns;
    size_t arg_count;
    bool cached;
    bool batched;
    uint32_t sequence;
} performance_trace_entry_t;

/**
 * @brief Performance manager structure
 */
struct performance_manager {
    polycall_core_context_t *core_ctx;
    polycall_ffi_context_t *ffi_ctx;
    perf_type_cache_t *type_cache;
    perf_call_cache_t *call_cache;
    batch_entry_t *batch_queue;
    size_t batch_queue_count;
    size_t batch_capacity;
    performance_trace_entry_t *trace_entries;
    size_t trace_count;
    size_t trace_capacity;
    performance_config_t config;
    polycall_performance_metrics_t metrics;
    uint32_t call_sequence;
    uint32_t batch_sequence;
    pthread_mutex_t batch_mutex;
    pthread_mutex_t trace_mutex;
};

/**
 * @brief Initialize performance manager
 */
polycall_core_error_t polycall_performance_init(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t **perf_mgr,
    const performance_config_t *config);

/**
 * @brief Clean up performance manager
 */
void polycall_performance_cleanup(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr);

/**
 * @brief Start tracing a function call
 */
polycall_core_error_t polycall_performance_trace_begin(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *function_name,
    const char *source_language,
    const char *target_language,
    performance_trace_entry_t **trace_entry);

/**
 * @brief End tracing a function call
 */
polycall_core_error_t polycall_performance_trace_end(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    performance_trace_entry_t *trace_entry);

/**
 * @brief Check if a function result is cached
 */
bool polycall_performance_check_cache(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *function_name,
    polycall_ffi_value_t *args,
    size_t arg_count,
    polycall_ffi_value_t **cached_result);

/**
 * @brief Cache a function result
 */
polycall_core_error_t polycall_performance_cache_result(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *function_name,
    polycall_ffi_value_t *args,
    size_t arg_count,
    polycall_ffi_value_t *result);

/**
 * @brief Queue a function call for batching
 */
polycall_core_error_t polycall_performance_queue_call(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *function_name,
    polycall_ffi_value_t *args,
    size_t arg_count,
    const char *target_language,
    uint32_t *batch_id);

/**
 * @brief Execute queued function calls as a batch
 */
polycall_core_error_t polycall_performance_execute_batch(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    polycall_ffi_value_t ***results,
    size_t *result_count);

/**
 * @brief Get performance metrics
 */
polycall_core_error_t polycall_performance_get_metrics(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    polycall_performance_metrics_t *metrics);

/**
 * @brief Reset performance metrics
 */
polycall_core_error_t polycall_performance_reset_metrics(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr);

/**
 * @brief Register a hot function for special optimization
 */
polycall_core_error_t polycall_performance_register_hot_function(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *function_name,
    polycall_optimization_level_t opt_level);

/**
 * @brief Set optimization level for all operations
 */
polycall_core_error_t polycall_performance_set_optimization_level(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    polycall_optimization_level_t opt_level);

/**
 * @brief Enable/disable performance features
 */
polycall_core_error_t polycall_performance_set_feature(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *feature_name,
    bool enabled);

/**
 * @brief Get performance traces
 */
polycall_core_error_t polycall_performance_get_traces(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    performance_trace_entry_t ***traces,
    size_t *trace_count);

/**
 * @brief Clear performance traces
 */
polycall_core_error_t polycall_performance_clear_traces(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr);

/**
 * @brief Export performance data to file
 */
polycall_core_error_t polycall_performance_export_data(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *filename,
    const char *format);

/**
 * @brief Create a default performance configuration
 */
performance_config_t polycall_performance_create_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_PERFORMANCE_H */