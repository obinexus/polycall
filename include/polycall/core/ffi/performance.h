
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 #include <time.h>
#include "polycall/core/ffi/ffi_types.h" // for polycall_ffi_type_t
/* Forward declarations */

/**
 * @file performance.h
 * @brief Performance optimization module for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the performance optimization module for LibPolyCall FFI,
 * providing mechanisms for optimizing cross-language function calls through
 * caching, batching, and other techniques.
 */

 #ifndef POLYCALL_FFI_PERFORMANCE_H
 #define POLYCALL_FFI_PERFORMANCE_H


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
typedef struct polycall_ffi_context polycall_ffi_context_t;
typedef struct ffi_value ffi_value_t;
typedef struct performance_manager performance_manager_t;
typedef struct type_cache type_cache_t;
typedef struct call_cache call_cache_t;

// Forward declare functions
polycall_core_error_t polycall_performance_queue_call(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    const char *function_name,
    ffi_value_t *args,
    size_t arg_count,
    const char *target_language,
    uint32_t *batch_id);

polycall_core_error_t polycall_performance_execute_batch(
    polycall_core_context_t *ctx,
    polycall_ffi_context_t *ffi_ctx,
    performance_manager_t *perf_mgr,
    ffi_value_t ***results,
    size_t *result_count);

// Define error sources if not already defined
#ifndef POLYCALL_FFI_PERFORMANCE_H_H
#define POLYCALL_FFI_PERFORMANCE_H_H
#endif

    // Function signature hash entry
    typedef struct
    {
        char *function_name;
        size_t arg_count;
        uint64_t hash;
        uint64_t result_hash;
        ffi_value_t *cached_result;
        uint64_t cache_time;
        uint32_t access_count;
    } cache_entry_t;

// Type cache entry
typedef struct {
    polycall_ffi_type_t source_type;
    polycall_ffi_type_t target_type;
    const char *source_language;
    const char *target_language;
    void *converter_data;
    uint32_t access_count;
} type_cache_entry_t;

    // Call batch entry
    typedef struct
    {
        char *function_name;
        ffi_value_t *args;
        size_t arg_count;
        const char *target_language;
        uint32_t batch_id;
        uint32_t call_index;
    } batch_entry_t;

    // Type cache
    struct type_cache
    {
        type_cache_entry_t *entries;
        size_t count;
        size_t capacity;
        pthread_mutex_t mutex;
    };

    // Call cache
    struct call_cache
    {
        cache_entry_t *entries;
        size_t count;
        size_t capacity;
        uint32_t ttl_ms;
        pthread_mutex_t mutex;
    };

    // Performance manager structure
    struct performance_manager
    {
        polycall_core_context_t *core_ctx;
        polycall_ffi_context_t *ffi_ctx;
        type_cache_t *type_cache;
        call_cache_t *call_cache;
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
     * @brief Performance manager (opaque)
     */
    typedef struct performance_manager performance_manager_t;

    /**
     * @brief Type cache (opaque)
     */
    typedef struct type_cache type_cache_t;

    /**
     * @brief Call cache (opaque)
     */
    typedef struct call_cache call_cache_t;

    /**
     * @brief Call optimization level
     */
    typedef enum
    {
        POLYCALL_OPT_LEVEL_NONE = 0,  /**< No optimization */
        POLYCALL_OPT_LEVEL_BASIC,     /**< Basic optimization */
        POLYCALL_OPT_LEVEL_MODERATE,  /**< Moderate optimization */
        POLYCALL_OPT_LEVEL_AGGRESSIVE /**< Aggressive optimization */
    } polycall_optimization_level_t;

    /**
     * @brief Performance metrics
     */
    typedef struct
    {
        uint64_t total_calls;               /**< Total function calls */
        uint64_t cache_hits;                /**< Cache hits */
        uint64_t cache_misses;              /**< Cache misses */
        uint64_t total_execution_time_ns;   /**< Total execution time in nanoseconds */
        uint64_t total_marshalling_time_ns; /**< Total marshalling time in nanoseconds */
        uint64_t batched_calls;             /**< Number of batched calls */
        uint64_t type_conversions;          /**< Number of type conversions */
        uint64_t memory_usage_bytes;        /**< Memory usage in bytes */
    } polycall_performance_metrics_t;

    /**
     * @brief Performance configuration
     */
    typedef struct
    {
        polycall_optimization_level_t opt_level; /**< Optimization level */
        bool enable_call_caching;                /**< Enable call result caching */
        bool enable_type_caching;                /**< Enable type conversion caching */
        bool enable_call_batching;               /**< Enable call batching */
        bool enable_lazy_initialization;         /**< Enable lazy initialization */
        size_t cache_size;                       /**< Cache size in entries */
        size_t batch_size;                       /**< Maximum batch size */
        uint32_t cache_ttl_ms;                   /**< Cache entry time-to-live in milliseconds */
        void *user_data;                         /**< User data */
    } performance_config_t;

    /**
     * @brief Performance trace entry
     */
    typedef struct
    {
        const char *function_name;    /**< Function name */
        const char *source_language;  /**< Source language */
        const char *target_language;  /**< Target language */
        uint64_t start_time_ns;       /**< Start time in nanoseconds */
        uint64_t end_time_ns;         /**< End time in nanoseconds */
        uint64_t marshalling_time_ns; /**< Marshalling time in nanoseconds */
        uint64_t execution_time_ns;   /**< Execution time in nanoseconds */
        size_t arg_count;             /**< Argument count */
        bool cached;                  /**< Whether result was cached */
        bool batched;                 /**< Whether call was batched */
        uint32_t sequence;            /**< Call sequence number */
    } performance_trace_entry_t;

    /**
     * @brief Initialize performance manager
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Pointer to receive performance manager
     * @param config Performance configuration
     * @return Error code
     */
    polycall_core_error_t polycall_performance_init(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t **perf_mgr,
        const performance_config_t *config);

    /**
     * @brief Clean up performance manager
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager to clean up
     */
    void polycall_performance_cleanup(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr);

    /**
     * @brief Start tracing a function call
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param function_name Function name
     * @param source_language Source language
     * @param target_language Target language
     * @param trace_entry Pointer to receive trace entry
     * @return Error code
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
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param trace_entry Trace entry to complete
     * @return Error code
     */
    polycall_core_error_t polycall_performance_trace_end(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        performance_trace_entry_t *trace_entry);

    /**
     * @brief Check if a function result is cached
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param function_name Function name
     * @param args Function arguments
     * @param arg_count Argument count
     * @param cached_result Pointer to receive cached result (if available)
     * @return true if result is cached, false otherwise
     */
    _Bool polycall_performance_check_cache(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        const char *function_name,
        ffi_value_t *args,
        size_t arg_count,
        ffi_value_t **cached_result);

    /**
     * @brief Cache a function result
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param function_name Function name
     * @param args Function arguments
     * @param arg_count Argument count
     * @param result Function result
     * @return Error code
     */
    polycall_core_error_t polycall_performance_cache_result(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        const char *function_name,
        ffi_value_t *args,
        size_t arg_count,
        ffi_value_t *result);

    /**
     * @brief Queue a function call for batching
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param function_name Function name
     * @param args Function arguments
     * @param arg_count Argument count
     * @param target_language Target language
     * @param batch_id Pointer to receive batch ID
     * @return Error code
     */
    polycall_core_error_t polycall_performance_queue_call(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        const char *function_name,
        ffi_value_t *args,
        size_t arg_count,
        const char *target_language,
        uint32_t *batch_id);

    /**
     * @brief Execute queued function calls as a batch
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param results Pointer to receive function results
     * @param result_count Pointer to receive result count
     * @return Error code
     */
    polycall_core_error_t polycall_performance_execute_batch(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        ffi_value_t ***results,
        size_t *result_count);

    /**
     * @brief Get performance metrics
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param metrics Pointer to receive performance metrics
     * @return Error code
     */
    polycall_core_error_t polycall_performance_get_metrics(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        polycall_performance_metrics_t *metrics);

    /**
     * @brief Reset performance metrics
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @return Error code
     */
    polycall_core_error_t polycall_performance_reset_metrics(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr);

    /**
     * @brief Register a hot function for special optimization
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param function_name Function name
     * @param opt_level Optimization level for this function
     * @return Error code
     */
    polycall_core_error_t polycall_performance_register_hot_function(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        const char *function_name,
        polycall_optimization_level_t opt_level);

    /**
     * @brief Set optimization level for all operations
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param opt_level Optimization level
     * @return Error code
     */
    polycall_core_error_t polycall_performance_set_optimization_level(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        polycall_optimization_level_t opt_level);

    /**
     * @brief Enable/disable performance features
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param feature_name Feature name ("caching", "batching", "tracing", etc.)
     * @param enabled Whether the feature should be enabled
     * @return Error code
     */
    polycall_core_error_t polycall_performance_set_feature(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        const char *feature_name,
        bool enabled);

    /**
     * @brief Get performance traces
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param traces Pointer to receive trace entries
     * @param trace_count Pointer to receive trace count
     * @return Error code
     */
    polycall_core_error_t polycall_performance_get_traces(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        performance_trace_entry_t ***traces,
        size_t *trace_count);

    /**
     * @brief Clear performance traces
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @return Error code
     */
    polycall_core_error_t polycall_performance_clear_traces(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr);

    /**
     * @brief Export performance data to file
     *
     * @param ctx Core context
     * @param ffi_ctx FFI context
     * @param perf_mgr Performance manager
     * @param filename File path
     * @param format Export format ("json", "csv", "text")
     * @return Error code
     */
    polycall_core_error_t polycall_performance_export_data(
        polycall_core_context_t *ctx,
        polycall_ffi_context_t *ffi_ctx,
        performance_manager_t *perf_mgr,
        const char *filename,
        const char *format);

    /**
     * @brief Create a default performance configuration
     *
     * @return Default configuration
     */
    performance_config_t polycall_performance_create_default_config(void);

#ifdef __cplusplus
}
 #endif
 
 #endif /* POLYCALL_FFI_PERFORMANCE_H*/

