/**
#include "polycall/core/ffi/performance.h"
#include "polycall/core/ffi/performance.h"

 * @file performance.c
 * @brief Performance optimization module implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the performance optimization module for LibPolyCall FFI,
 * providing mechanisms for optimizing cross-language function calls through
 * caching, batching, and tracing.
 */




 
 /**
  * @brief Initialize performance manager
  */
 polycall_core_error_t polycall_performance_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t** perf_mgr,
     const performance_config_t* config
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     // Allocate performance manager
     performance_manager_t* mgr = polycall_core_malloc(ctx, sizeof(performance_manager_t));
     if (!mgr) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate performance manager");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     // Initialize fields
     memset(mgr, 0, sizeof(performance_manager_t));
     mgr->core_ctx = ctx;
     mgr->ffi_ctx = ffi_ctx;
     mgr->call_sequence = 0;
     mgr->batch_sequence = 0;
 
     // Initialize configuration
     memcpy(&mgr->config, config, sizeof(performance_config_t));
 
     // Initialize metrics
     memset(&mgr->metrics, 0, sizeof(polycall_performance_metrics_t));
 
     // Initialize mutexes
     if (pthread_mutex_init(&mgr->batch_mutex, NULL) != 0 || 
         pthread_mutex_init(&mgr->trace_mutex, NULL) != 0) {
         if (mgr->batch_mutex) {
             pthread_mutex_destroy(&mgr->batch_mutex);
         }
         if (mgr->trace_mutex) {
             pthread_mutex_destroy(&mgr->trace_mutex);
         }
         polycall_core_free(ctx, mgr);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize performance manager mutexes");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
 
     // Initialize caches if enabled
     if (config->enable_call_caching) {
         polycall_core_error_t result = init_call_cache(ctx, &mgr->call_cache, 
                                                      config->cache_size, 
                                                      config->cache_ttl_ms);
         if (result != POLYCALL_CORE_SUCCESS) {
             pthread_mutex_destroy(&mgr->batch_mutex);
             pthread_mutex_destroy(&mgr->trace_mutex);
             polycall_core_free(ctx, mgr);
             return result;
         }
     }
 
     if (config->enable_type_caching) {
         polycall_core_error_t result = init_type_cache(ctx, &mgr->type_cache, 
                                                      config->cache_size);
         if (result != POLYCALL_CORE_SUCCESS) {
             if (mgr->call_cache) {
                 cleanup_call_cache(ctx, mgr->call_cache);
             }
             pthread_mutex_destroy(&mgr->batch_mutex);
             pthread_mutex_destroy(&mgr->trace_mutex);
             polycall_core_free(ctx, mgr);
             return result;
         }
     }
 
     // Initialize batch queue if enabled
     if (config->enable_call_batching) {
         mgr->batch_capacity = config->batch_size;
         mgr->batch_queue = polycall_core_malloc(ctx, mgr->batch_capacity * sizeof(batch_entry_t));
         if (!mgr->batch_queue) {
             if (mgr->type_cache) {
                 cleanup_type_cache(ctx, mgr->type_cache);
             }
             if (mgr->call_cache) {
                 cleanup_call_cache(ctx, mgr->call_cache);
             }
             pthread_mutex_destroy(&mgr->batch_mutex);
             pthread_mutex_destroy(&mgr->trace_mutex);
             polycall_core_free(ctx, mgr);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate batch queue");
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         memset(mgr->batch_queue, 0, mgr->batch_capacity * sizeof(batch_entry_t));
     }
 
     // Initialize trace buffer
     mgr->trace_capacity = 1024; // Default trace capacity
     mgr->trace_entries = polycall_core_malloc(ctx, mgr->trace_capacity * sizeof(performance_trace_entry_t));
     if (!mgr->trace_entries) {
         if (mgr->batch_queue) {
             polycall_core_free(ctx, mgr->batch_queue);
         }
         if (mgr->type_cache) {
             cleanup_type_cache(ctx, mgr->type_cache);
         }
         if (mgr->call_cache) {
             cleanup_call_cache(ctx, mgr->call_cache);
         }
         pthread_mutex_destroy(&mgr->batch_mutex);
         pthread_mutex_destroy(&mgr->trace_mutex);
         polycall_core_free(ctx, mgr);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate trace buffer");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     memset(mgr->trace_entries, 0, mgr->trace_capacity * sizeof(performance_trace_entry_t));
 
     *perf_mgr = mgr;
     return POLYCALL_CORE_SUCCESS;
 }
 /**
  * @brief Get performance metrics
  */
 polycall_core_error_t polycall_performance_get_metrics(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     polycall_performance_metrics_t* metrics
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !metrics) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy metrics
     memcpy(metrics, &perf_mgr->metrics, sizeof(polycall_performance_metrics_t));
     
     // Calculate memory usage
     size_t memory_usage = sizeof(performance_manager_t);
     
     // Add trace buffer size
     memory_usage += perf_mgr->trace_capacity * sizeof(performance_trace_entry_t);
     
     // Add call cache size
     if (perf_mgr->call_cache) {
         memory_usage += sizeof(call_cache_t);
         memory_usage += perf_mgr->call_cache->capacity * sizeof(cache_entry_t);
     }
     
     // Add type cache size
     if (perf_mgr->type_cache) {
         memory_usage += sizeof(type_cache_t);
         memory_usage += perf_mgr->type_cache->capacity * sizeof(type_cache_entry_t);
     }
     
     // Add batch queue size
     if (perf_mgr->batch_queue) {
         memory_usage += perf_mgr->batch_capacity * sizeof(batch_entry_t);
     }
     
     // Update memory usage metric
     metrics->memory_usage_bytes = memory_usage;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Reset performance metrics
  */
 polycall_core_error_t polycall_performance_reset_metrics(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Reset metrics
     memset(&perf_mgr->metrics, 0, sizeof(polycall_performance_metrics_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register a hot function for special optimization
  */
 polycall_core_error_t polycall_performance_register_hot_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     const char* function_name,
     polycall_optimization_level_t opt_level
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !function_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // This is a placeholder implementation
     // In a complete implementation, we would maintain a list of hot functions
     // with their optimization levels and apply special optimizations
     
     // For now, just log the registration
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_SUCCESS,
                       POLYCALL_ERROR_SEVERITY_INFO, 
                       "Registered hot function: %s (level %d)", 
                       function_name, opt_level);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set optimization level for all operations
  */
 polycall_core_error_t polycall_performance_set_optimization_level(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     polycall_optimization_level_t opt_level
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Update configuration
     perf_mgr->config.opt_level = opt_level;
     
     // Apply optimization level changes
     switch (opt_level) {
         case POLYCALL_OPT_LEVEL_NONE:
             // Disable all optimizations
             perf_mgr->config.enable_call_caching = false;
             perf_mgr->config.enable_type_caching = false;
             perf_mgr->config.enable_call_batching = false;
             break;
             
         case POLYCALL_OPT_LEVEL_BASIC:
             // Enable basic optimizations
             perf_mgr->config.enable_call_caching = true;
             perf_mgr->config.enable_type_caching = true;
             perf_mgr->config.enable_call_batching = false;
             break;
             
         case POLYCALL_OPT_LEVEL_MODERATE:
             // Enable more optimizations
             perf_mgr->config.enable_call_caching = true;
             perf_mgr->config.enable_type_caching = true;
             perf_mgr->config.enable_call_batching = true;
             break;
             
         case POLYCALL_OPT_LEVEL_AGGRESSIVE:
             // Enable all optimizations
             perf_mgr->config.enable_call_caching = true;
             perf_mgr->config.enable_type_caching = true;
             perf_mgr->config.enable_call_batching = true;
             perf_mgr->config.enable_lazy_initialization = true;
             break;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Enable/disable performance features
  */
 polycall_core_error_t polycall_performance_set_feature(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     const char* feature_name,
     bool enabled
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !feature_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Enable/disable feature based on name
     if (strcmp(feature_name, "caching") == 0) {
         perf_mgr->config.enable_call_caching = enabled;
     } else if (strcmp(feature_name, "type_caching") == 0) {
         perf_mgr->config.enable_type_caching = enabled;
     } else if (strcmp(feature_name, "batching") == 0) {
         perf_mgr->config.enable_call_batching = enabled;
     } else if (strcmp(feature_name, "lazy_initialization") == 0) {
         perf_mgr->config.enable_lazy_initialization = enabled;
     } else {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Unknown performance feature: %s", feature_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get performance traces
  */
 polycall_core_error_t polycall_performance_get_traces(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     performance_trace_entry_t*** traces,
     size_t* trace_count
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !traces || !trace_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&perf_mgr->trace_mutex);
     
     // Check if there are any traces
     if (perf_mgr->trace_count == 0) {
         pthread_mutex_unlock(&perf_mgr->trace_mutex);
         *traces = NULL;
         *trace_count = 0;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Allocate memory for trace entry pointers
     performance_trace_entry_t** trace_array = polycall_core_malloc(ctx, 
                                                                perf_mgr->trace_count * sizeof(performance_trace_entry_t*));
     if (!trace_array) {
         pthread_mutex_unlock(&perf_mgr->trace_mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for trace array");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Sort traces by sequence number if needed
     qsort(perf_mgr->trace_entries, perf_mgr->trace_count, 
           sizeof(performance_trace_entry_t), compare_trace_entries);
     
     // Fill array with pointers to trace entries
     for (size_t i = 0; i < perf_mgr->trace_count; i++) {
         trace_array[i] = &perf_mgr->trace_entries[i];
     }
     
     pthread_mutex_unlock(&perf_mgr->trace_mutex);
     
     // Return results
     *traces = trace_array;
     *trace_count = perf_mgr->trace_count;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clear performance traces
  */
 polycall_core_error_t polycall_performance_clear_traces(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&perf_mgr->trace_mutex);
     
     // Free any allocated strings in trace entries
     for (size_t i = 0; i < perf_mgr->trace_count; i++) {
         if (perf_mgr->trace_entries[i].function_name) {
             polycall_core_free(ctx, (void*)perf_mgr->trace_entries[i].function_name);
         }
         if (perf_mgr->trace_entries[i].source_language) {
             polycall_core_free(ctx, (void*)perf_mgr->trace_entries[i].source_language);
         }
         if (perf_mgr->trace_entries[i].target_language) {
             polycall_core_free(ctx, (void*)perf_mgr->trace_entries[i].target_language);
         }
     }
     
     // Reset trace count
     perf_mgr->trace_count = 0;
     
     pthread_mutex_unlock(&perf_mgr->trace_mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Export performance data to file
  */
 polycall_core_error_t polycall_performance_export_data(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     const char* filename,
     const char* format
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !filename || !format) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Open file for writing
     FILE* file = fopen(filename, "w");
     if (!file) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_IO_ERROR,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to open file for writing: %s", filename);
         return POLYCALL_CORE_ERROR_IO_ERROR;
     }
     
     // Write data based on format
     if (strcmp(format, "json") == 0) {
         // Write metrics as JSON
         fprintf(file, "{\n");
         fprintf(file, "  \"metrics\": {\n");
         fprintf(file, "    \"total_calls\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.total_calls);
         fprintf(file, "    \"cache_hits\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.cache_hits);
         fprintf(file, "    \"cache_misses\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.cache_misses);
         fprintf(file, "    \"total_execution_time_ns\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.total_execution_time_ns);
         fprintf(file, "    \"total_marshalling_time_ns\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.total_marshalling_time_ns);
         fprintf(file, "    \"batched_calls\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.batched_calls);
         fprintf(file, "    \"type_conversions\": %llu,\n", 
                 (unsigned long long)perf_mgr->metrics.type_conversions);
         fprintf(file, "    \"memory_usage_bytes\": %llu\n", 
                 (unsigned long long)perf_mgr->metrics.memory_usage_bytes);
         fprintf(file, "  },\n");
         
         // Write traces as JSON
         fprintf(file, "  \"traces\": [\n");
         
         pthread_mutex_lock(&perf_mgr->trace_mutex);
         
         for (size_t i = 0; i < perf_mgr->trace_count; i++) {
             performance_trace_entry_t* entry = &perf_mgr->trace_entries[i];
             
             fprintf(file, "    {\n");
             fprintf(file, "      \"function_name\": \"%s\",\n", entry->function_name);
             fprintf(file, "      \"source_language\": \"%s\",\n", entry->source_language);
             fprintf(file, "      \"target_language\": \"%s\",\n", entry->target_language);
             fprintf(file, "      \"start_time_ns\": %llu,\n", 
                     (unsigned long long)entry->start_time_ns);
             fprintf(file, "      \"end_time_ns\": %llu,\n", 
                     (unsigned long long)entry->end_time_ns);
             fprintf(file, "      \"execution_time_ns\": %llu,\n", 
                     (unsigned long long)entry->execution_time_ns);
             fprintf(file, "      \"marshalling_time_ns\": %llu,\n", 
                     (unsigned long long)entry->marshalling_time_ns);
             fprintf(file, "      \"arg_count\": %zu,\n", entry->arg_count);
             fprintf(file, "      \"cached\": %s,\n", entry->cached ? "true" : "false");
             fprintf(file, "      \"batched\": %s,\n", entry->batched ? "true" : "false");
             fprintf(file, "      \"sequence\": %u\n", entry->sequence);
             fprintf(file, "    }%s\n", i < perf_mgr->trace_count - 1 ? "," : "");
         }
         
         pthread_mutex_unlock(&perf_mgr->trace_mutex);
         
         fprintf(file, "  ]\n");
         fprintf(file, "}\n");
     } else if (strcmp(format, "csv") == 0) {
         // Write metrics as CSV
         fprintf(file, "Metric,Value\n");
         fprintf(file, "total_calls,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.total_calls);
         fprintf(file, "cache_hits,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.cache_hits);
         fprintf(file, "cache_misses,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.cache_misses);
         fprintf(file, "total_execution_time_ns,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.total_execution_time_ns);
         fprintf(file, "total_marshalling_time_ns,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.total_marshalling_time_ns);
         fprintf(file, "batched_calls,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.batched_calls);
         fprintf(file, "type_conversions,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.type_conversions);
         fprintf(file, "memory_usage_bytes,%llu\n", 
                 (unsigned long long)perf_mgr->metrics.memory_usage_bytes);
         
         // Write traces as CSV
         fprintf(file, "\nfunction_name,source_language,target_language,start_time_ns,end_time_ns,execution_time_ns,marshalling_time_ns,arg_count,cached,batched,sequence\n");
         
         pthread_mutex_lock(&perf_mgr->trace_mutex);
         
         for (size_t i = 0; i < perf_mgr->trace_count; i++) {
             performance_trace_entry_t* entry = &perf_mgr->trace_entries[i];
             
             fprintf(file, "\"%s\",\"%s\",\"%s\",%llu,%llu,%llu,%llu,%zu,%s,%s,%u\n",
                     entry->function_name,
                     entry->source_language,
                     entry->target_language,
                     (unsigned long long)entry->start_time_ns,
                     (unsigned long long)entry->end_time_ns,
                     (unsigned long long)entry->execution_time_ns,
                     (unsigned long long)entry->marshalling_time_ns,
                     entry->arg_count,
                     entry->cached ? "true" : "false",
                     entry->batched ? "true" : "false",
                     entry->sequence);
         }
         
         pthread_mutex_unlock(&perf_mgr->trace_mutex);
     } else if (strcmp(format, "text") == 0) {
         // Write metrics as text
         fprintf(file, "Performance Metrics:\n");
         fprintf(file, "--------------------------------------------------------------------------------\n");
         fprintf(file, "Total calls:               %llu\n", 
                 (unsigned long long)perf_mgr->metrics.total_calls);
         fprintf(file, "Cache hits:                %llu\n", 
                 (unsigned long long)perf_mgr->metrics.cache_hits);
         fprintf(file, "Cache misses:              %llu\n", 
                 (unsigned long long)perf_mgr->metrics.cache_misses);
         fprintf(file, "Total execution time:      %llu ns\n", 
                 (unsigned long long)perf_mgr->metrics.total_execution_time_ns);
         fprintf(file, "Total marshalling time:    %llu ns\n", 
                 (unsigned long long)perf_mgr->metrics.total_marshalling_time_ns);
         fprintf(file, "Batched calls:             %llu\n", 
                 (unsigned long long)perf_mgr->metrics.batched_calls);
         fprintf(file, "Type conversions:          %llu\n", 
                 (unsigned long long)perf_mgr->metrics.type_conversions);
         fprintf(file, "Memory usage:              %llu bytes\n", 
                 (unsigned long long)perf_mgr->metrics.memory_usage_bytes);
         
             // Write traces as text
             fprintf(file, "\nPerformance Traces:\n");
             fprintf(file, "--------------------------------------------------------------------------------\n");
             fprintf(file, "%-30s %-15s %-15s %-15s %-15s %-15s %-15s %-10s %-8s %-8s %-10s\n",
                     "Function", "Source", "Target", "Start (ns)", "End (ns)", "Exec (ns)", "Marshal (ns)", "Args", "Cached", "Batched", "Sequence");
             fprintf(file, "--------------------------------------------------------------------------------\n");
             
             pthread_mutex_lock(&perf_mgr->trace_mutex);
             
             for (size_t i = 0; i < perf_mgr->trace_count; i++) {
                 performance_trace_entry_t* entry = &perf_mgr->trace_entries[i];
                 
                 fprintf(file, "%-30.30s %-15.15s %-15.15s %-15llu %-15llu %-15llu %-15llu %-10zu %-8s %-8s %-10u\n",
                         entry->function_name,
                         entry->source_language,
                         entry->target_language,
                         (unsigned long long)entry->start_time_ns,
                         (unsigned long long)entry->end_time_ns,
                         (unsigned long long)entry->execution_time_ns,
                         (unsigned long long)entry->marshalling_time_ns,
                         entry->arg_count,
                         entry->cached ? "Yes" : "No",
                         entry->batched ? "Yes" : "No",
                         entry->sequence);
             }
             
             pthread_mutex_unlock(&perf_mgr->trace_mutex);
         } else {
             // Unknown format
             fclose(file);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                               POLYCALL_ERROR_SEVERITY_WARNING, 
                               "Unsupported export format: %s", format);
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         // Close file
         fclose(file);
         
         return POLYCALL_CORE_SUCCESS;
 
/**
 * @brief Execute queued function calls as a batch
 */
polycall_core_error_t polycall_performance_execute_batch(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    performance_manager_t* perf_mgr,
    ffi_value_t*** results,
    size_t* result_count
) {
    if (!ctx || !ffi_ctx || !perf_mgr || !results || !result_count) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // If batching is disabled, return error
    if (!perf_mgr->config.enable_call_batching || !perf_mgr->batch_queue) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Call batching is disabled");
        return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
    }
    
    pthread_mutex_lock(&perf_mgr->batch_mutex);
    
    // Check if there are any calls to execute
    if (perf_mgr->batch_queue_count == 0) {
        pthread_mutex_unlock(&perf_mgr->batch_mutex);
        *results = NULL;
        *result_count = 0;
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Allocate result array
    ffi_value_t** result_array = polycall_core_malloc(ctx, 
                                                    perf_mgr->batch_queue_count * sizeof(ffi_value_t*));
    if (!result_array) {
        pthread_mutex_unlock(&perf_mgr->batch_mutex);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for batch results");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize result array
    memset(result_array, 0, perf_mgr->batch_queue_count * sizeof(ffi_value_t*));
    
    // Execute each call in the batch
    for (size_t i = 0; i < perf_mgr->batch_queue_count; i++) {
        batch_entry_t* entry = &perf_mgr->batch_queue[i];
        
        // Allocate result
        ffi_value_t* result = polycall_core_malloc(ctx, sizeof(ffi_value_t));
        if (!result) {
            // Clean up allocated results
            for (size_t j = 0; j < i; j++) {
                polycall_core_free(ctx, result_array[j]);
            }
            polycall_core_free(ctx, result_array);
            
            pthread_mutex_unlock(&perf_mgr->batch_mutex);
            
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate memory for batch result entry");
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Initialize result
        memset(result, 0, sizeof(ffi_value_t));
        
        // Create trace entry
        performance_trace_entry_t* trace_entry = NULL;
        polycall_performance_trace_begin(ctx, ffi_ctx, perf_mgr, 
                                        entry->function_name, 
                                        "batch", 
                                        entry->target_language,
                                        &trace_entry);
        
        if (trace_entry) {
            trace_entry->batched = true;
            trace_entry->arg_count = entry->arg_count;
        }
        
        // Call the function
        polycall_core_error_t call_result = polycall_ffi_call_function(
            ctx, ffi_ctx, 
            entry->function_name, 
            entry->args, 
            entry->arg_count, 
            result, 
            entry->target_language);
        
        // End trace
        if (trace_entry) {
            polycall_performance_trace_end(ctx, ffi_ctx, perf_mgr, trace_entry);
        }
        
        // Handle errors
        if (call_result != POLYCALL_CORE_SUCCESS) {
            // Clean up allocated results
            polycall_core_free(ctx, result);
            for (size_t j = 0; j < i; j++) {
                polycall_core_free(ctx, result_array[j]);
            }
            polycall_core_free(ctx, result_array);
            
            pthread_mutex_unlock(&perf_mgr->batch_mutex);
            
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               call_result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Batch call %zu failed: %s", i, entry->function_name);
            return call_result;
        }
        
        // Store result
        result_array[i] = result;
    }
    
    // Clean up batch queue
    size_t executed_count = perf_mgr->batch_queue_count;
    for (size_t i = 0; i < perf_mgr->batch_queue_count; i++) {
        batch_entry_t* entry = &perf_mgr->batch_queue[i];
        polycall_core_free(ctx, entry->function_name);
        if (entry->args) {
            polycall_core_free(ctx, entry->args);
        }
    }
    
    // Reset batch queue
    perf_mgr->batch_queue_count = 0;
    perf_mgr->batch_sequence++;
    
    pthread_mutex_unlock(&perf_mgr->batch_mutex);
    
    // Return results
    *results = result_array;
    *result_count = executed_count;
    
    return POLYCALL_CORE_SUCCESS;
}
 
 /**
  * @brief Clean up performance manager
  */
 void polycall_performance_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr
 ) {
     if (!ctx || !perf_mgr) {
         return;
     }
 
     // Clean up trace entries
     if (perf_mgr->trace_entries) {
         // Free any allocated strings in trace entries
         for (size_t i = 0; i < perf_mgr->trace_count; i++) {
             if (perf_mgr->trace_entries[i].function_name) {
                 polycall_core_free(ctx, (void*)perf_mgr->trace_entries[i].function_name);
             }
             if (perf_mgr->trace_entries[i].source_language) {
                 polycall_core_free(ctx, (void*)perf_mgr->trace_entries[i].source_language);
             }
             if (perf_mgr->trace_entries[i].target_language) {
                 polycall_core_free(ctx, (void*)perf_mgr->trace_entries[i].target_language);
             }
         }
         polycall_core_free(ctx, perf_mgr->trace_entries);
     }
 
     // Clean up batch queue
     if (perf_mgr->batch_queue) {
         for (size_t i = 0; i < perf_mgr->batch_queue_count; i++) {
             if (perf_mgr->batch_queue[i].function_name) {
                 polycall_core_free(ctx, perf_mgr->batch_queue[i].function_name);
             }
             if (perf_mgr->batch_queue[i].args) {
                 for (size_t j = 0; j < perf_mgr->batch_queue[i].arg_count; j++) {
                     free_ffi_value(ctx, &perf_mgr->batch_queue[i].args[j]);
                 }
                 polycall_core_free(ctx, perf_mgr->batch_queue[i].args);
             }
         }
         polycall_core_free(ctx, perf_mgr->batch_queue);
     }
 
     // Clean up type cache
     if (perf_mgr->type_cache) {
         cleanup_type_cache(ctx, perf_mgr->type_cache);
     }
 
     // Clean up call cache
     if (perf_mgr->call_cache) {
         cleanup_call_cache(ctx, perf_mgr->call_cache);
     }
 
     // Destroy mutexes
     pthread_mutex_destroy(&perf_mgr->batch_mutex);
     pthread_mutex_destroy(&perf_mgr->trace_mutex);
 
     // Free performance manager
     polycall_core_free(ctx, perf_mgr);
 }
 
 /**
  * @brief Start tracing a function call
  */
 polycall_core_error_t polycall_performance_trace_begin(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     const char* function_name,
     const char* source_language,
     const char* target_language,
     performance_trace_entry_t** trace_entry
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !function_name || 
         !source_language || !target_language || !trace_entry) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     pthread_mutex_lock(&perf_mgr->trace_mutex);
 
     // Check if we need to expand the trace buffer
     if (perf_mgr->trace_count >= perf_mgr->trace_capacity) {
         size_t new_capacity = perf_mgr->trace_capacity * 2;
         performance_trace_entry_t* new_entries = polycall_core_realloc(
             ctx, perf_mgr->trace_entries,
             new_capacity * sizeof(performance_trace_entry_t));
         
         if (!new_entries) {
             pthread_mutex_unlock(&perf_mgr->trace_mutex);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to expand trace buffer");
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         perf_mgr->trace_entries = new_entries;
         // Initialize new entries
         memset(&perf_mgr->trace_entries[perf_mgr->trace_capacity], 0, 
                (new_capacity - perf_mgr->trace_capacity) * sizeof(performance_trace_entry_t));
         perf_mgr->trace_capacity = new_capacity;
     }
 
     // Create new trace entry
     performance_trace_entry_t* entry = &perf_mgr->trace_entries[perf_mgr->trace_count++];
     
     // Make copies of strings
     char* fn_copy = polycall_core_malloc(ctx, strlen(function_name) + 1);
     char* src_copy = polycall_core_malloc(ctx, strlen(source_language) + 1);
     char* tgt_copy = polycall_core_malloc(ctx, strlen(target_language) + 1);
     
     if (!fn_copy || !src_copy || !tgt_copy) {
         if (fn_copy) polycall_core_free(ctx, fn_copy);
         if (src_copy) polycall_core_free(ctx, src_copy);
         if (tgt_copy) polycall_core_free(ctx, tgt_copy);
         
         perf_mgr->trace_count--;
         pthread_mutex_unlock(&perf_mgr->trace_mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for trace entry strings");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strcpy(fn_copy, function_name);
     strcpy(src_copy, source_language);
     strcpy(tgt_copy, target_language);
     
     // Initialize entry
     entry->function_name = fn_copy;
     entry->source_language = src_copy;
     entry->target_language = tgt_copy;
     entry->start_time_ns = get_current_time_ms() * 1000000; // Convert to nanoseconds
     entry->end_time_ns = 0;
     entry->marshalling_time_ns = 0;
     entry->execution_time_ns = 0;
     entry->arg_count = 0;
     entry->cached = false;
     entry->batched = false;
     entry->sequence = perf_mgr->call_sequence++;
     
     pthread_mutex_unlock(&perf_mgr->trace_mutex);
     
     // Update metrics
     perf_mgr->metrics.total_calls++;
     
     *trace_entry = entry;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief End tracing a function call
  */
 polycall_core_error_t polycall_performance_trace_end(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     performance_trace_entry_t* trace_entry
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !trace_entry) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Record end time
     trace_entry->end_time_ns = get_current_time_ms() * 1000000; // Convert to nanoseconds
     
     // Calculate execution time
     trace_entry->execution_time_ns = trace_entry->end_time_ns - trace_entry->start_time_ns;
     
     // Update metrics
     perf_mgr->metrics.total_execution_time_ns += trace_entry->execution_time_ns;
     perf_mgr->metrics.total_marshalling_time_ns += trace_entry->marshalling_time_ns;
     
     if (trace_entry->cached) {
         perf_mgr->metrics.cache_hits++;
     }
     
     if (trace_entry->batched) {
         perf_mgr->metrics.batched_calls++;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Check if a function result is cached
  */
 bool polycall_performance_check_cache(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t** cached_result
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !function_name || (!args && arg_count > 0) || !cached_result) {
         return false;
     }
     
     // If caching is disabled, return false
     if (!perf_mgr->config.enable_call_caching || !perf_mgr->call_cache) {
         return false;
     }
     
     // Calculate call hash
     uint64_t call_hash = hash_function_call(function_name, args, arg_count);
     
     // Check cache for matching entry
     pthread_mutex_lock(&perf_mgr->call_cache->mutex);
     
     // First, process any expired entries
     process_cache_expiry(ctx, perf_mgr->call_cache);
     
     for (size_t i = 0; i < perf_mgr->call_cache->count; i++) {
         cache_entry_t* entry = &perf_mgr->call_cache->entries[i];
         
         if (entry->hash == call_hash && 
             strcmp(entry->function_name, function_name) == 0 && 
             entry->arg_count == arg_count) {
             
             // Found matching entry, increment access count
             entry->access_count++;
             
             // Make a copy of the cached result
             *cached_result = clone_ffi_value(ctx, entry->cached_result);
             
             pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
             
             // If the result couldn't be cloned, return false
             if (!*cached_result) {
                 return false;
             }
             
             return true;
         }
     }
     
     pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
     
     // No matching entry found
     *cached_result = NULL;
     perf_mgr->metrics.cache_misses++;
     
     return false;
 }
 
 /**
  * @brief Cache a function result
  */
 polycall_core_error_t polycall_performance_cache_result(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     performance_manager_t* perf_mgr,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     if (!ctx || !ffi_ctx || !perf_mgr || !function_name || 
         (!args && arg_count > 0) || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If caching is disabled, return success but do nothing
     if (!perf_mgr->config.enable_call_caching || !perf_mgr->call_cache) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Calculate call hash
     uint64_t call_hash = hash_function_call(function_name, args, arg_count);
     uint64_t result_hash = hash_function_result(result);
     
     pthread_mutex_lock(&perf_mgr->call_cache->mutex);
     
     // First, process any expired entries
     process_cache_expiry(ctx, perf_mgr->call_cache);
     
     // Check if this call is already cached
     for (size_t i = 0; i < perf_mgr->call_cache->count; i++) {
         cache_entry_t* entry = &perf_mgr->call_cache->entries[i];
         
         if (entry->hash == call_hash && 
             strcmp(entry->function_name, function_name) == 0 && 
             entry->arg_count == arg_count) {
             
             // Already cached, update the entry
             if (entry->result_hash != result_hash) {
                 // Result has changed, update it
                 free_ffi_value(ctx, entry->cached_result);
                 polycall_core_free(ctx, entry->cached_result);
                 
                 // Clone the new result
                 entry->cached_result = clone_ffi_value(ctx, result);
                 if (!entry->cached_result) {
                     pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
                     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                       POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                                       POLYCALL_ERROR_SEVERITY_ERROR, 
                                       "Failed to clone result for cache update");
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 entry->result_hash = result_hash;
             }
             
             // Update cache time
             entry->cache_time = get_current_time_ms();
             entry->access_count++;
             
             pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Not cached, check if cache is full
     if (perf_mgr->call_cache->count >= perf_mgr->call_cache->capacity) {
         // Cache is full, find the least recently used entry
         size_t lru_index = 0;
         uint64_t oldest_time = UINT64_MAX;
         uint32_t lowest_access = UINT32_MAX;
         
         for (size_t i = 0; i < perf_mgr->call_cache->count; i++) {
             cache_entry_t* entry = &perf_mgr->call_cache->entries[i];
             
             // Use a combination of access count and time for LRU
             if (entry->access_count < lowest_access || 
                 (entry->access_count == lowest_access && entry->cache_time < oldest_time)) {
                 lru_index = i;
                 oldest_time = entry->cache_time;
                 lowest_access = entry->access_count;
             }
         }
         
         // Free the LRU entry
         cache_entry_t* lru_entry = &perf_mgr->call_cache->entries[lru_index];
         polycall_core_free(ctx, lru_entry->function_name);
         free_ffi_value(ctx, lru_entry->cached_result);
         polycall_core_free(ctx, lru_entry->cached_result);
         
         // Shift entries to fill the gap
         if (lru_index < perf_mgr->call_cache->count - 1) {
             memmove(&perf_mgr->call_cache->entries[lru_index],
                    &perf_mgr->call_cache->entries[lru_index + 1],
                    (perf_mgr->call_cache->count - lru_index - 1) * sizeof(cache_entry_t));
         }
         
         perf_mgr->call_cache->count--;
     }
     
     // Add new cache entry
     cache_entry_t* new_entry = &perf_mgr->call_cache->entries[perf_mgr->call_cache->count];
     
     // Copy function name
     new_entry->function_name = polycall_core_malloc(ctx, strlen(function_name) + 1);
     if (!new_entry->function_name) {
         pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for cache entry function name");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     strcpy(new_entry->function_name, function_name);
     
     // Clone the result
     new_entry->cached_result = clone_ffi_value(ctx, result);
     if (!new_entry->cached_result) {
         polycall_core_free(ctx, new_entry->function_name);
         pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to clone result for new cache entry");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Set other fields
     new_entry->arg_count = arg_count;
     new_entry->hash = call_hash;
     new_entry->result_hash = result_hash;
     new_entry->cache_time = get_current_time_ms();
     new_entry->access_count = 1;
     
     perf_mgr->call_cache->count++;
     
     pthread_mutex_unlock(&perf_mgr->call_cache->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
/**
 * @brief Internal helper functions for performance optimization
 */

static uint64_t calculate_hash(const void* data, size_t size) {
    // FNV-1a hash algorithm
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = 0xcbf29ce484222325ULL; // FNV offset basis
    
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL; // FNV prime
    }
    
    return hash;
}

static uint64_t hash_function_call(const char* function_name, ffi_value_t* args, size_t arg_count) {
    // Hash the function name
    uint64_t hash = calculate_hash(function_name, strlen(function_name));
    
    // Hash each argument
    for (size_t i = 0; i < arg_count; i++) {
        // Hash argument type
        hash ^= args[i].type;
        hash *= 0x100000001b3ULL;
        
        // Hash argument value based on type
        switch (args[i].type) {
            case FFI_TYPE_INT:
                hash ^= calculate_hash(&args[i].value.int_val, sizeof(int));
                break;
            case FFI_TYPE_FLOAT:
                hash ^= calculate_hash(&args[i].value.float_val, sizeof(float));
                break;
            case FFI_TYPE_DOUBLE:
                hash ^= calculate_hash(&args[i].value.double_val, sizeof(double));
                break;
            case FFI_TYPE_BOOL:
                hash ^= calculate_hash(&args[i].value.bool_val, sizeof(bool));
                break;
            case FFI_TYPE_STRING:
                if (args[i].value.string_val) {
                    hash ^= calculate_hash(args[i].value.string_val, strlen(args[i].value.string_val));
                }
                break;
            // Additional types would be handled here
            default:
                // For complex types, just use the pointer value
                hash ^= (uint64_t)args[i].value.ptr_val;
                break;
        }
    }
    
    return hash;
}

static uint64_t hash_function_result(ffi_value_t* result) {
    if (!result) {
        return 0;
    }
    
    // Hash result type
    uint64_t hash = result->type;
    
    // Hash result value based on type
    switch (result->type) {
        case FFI_TYPE_INT:
            hash ^= calculate_hash(&result->value.int_val, sizeof(int));
            break;
        case FFI_TYPE_FLOAT:
            hash ^= calculate_hash(&result->value.float_val, sizeof(float));
            break;
        case FFI_TYPE_DOUBLE:
            hash ^= calculate_hash(&result->value.double_val, sizeof(double));
            break;
        case FFI_TYPE_BOOL:
            hash ^= calculate_hash(&result->value.bool_val, sizeof(bool));
            break;
        case FFI_TYPE_STRING:
            if (result->value.string_val) {
                hash ^= calculate_hash(result->value.string_val, strlen(result->value.string_val));
            }
            break;
        // Additional types would be handled here
        default:
            // For complex types, just use the pointer value
            hash ^= (uint64_t)result->value.ptr_val;
            break;
    }
    
    return hash;
}

static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}


static int compare_trace_entries(const void* a, const void* b) {
    const performance_trace_entry_t* entry_a = (const performance_trace_entry_t*)a;
    const performance_trace_entry_t* entry_b = (const performance_trace_entry_t*)b;
    
    return (entry_a->sequence < entry_b->sequence) ? -1 : 
           (entry_a->sequence > entry_b->sequence) ? 1 : 0;
}

static void process_cache_expiry(polycall_core_context_t* ctx, call_cache_t* cache) {
    uint64_t current_time = get_current_time_ms();
    size_t i = 0;
    
    while (i < cache->count) {
        cache_entry_t* entry = &cache->entries[i];
        
        // Check if entry has expired
        if (cache->ttl_ms > 0 && (current_time - entry->cache_time) > cache->ttl_ms) {
            // Free resources
            polycall_core_free(ctx, entry->function_name);
            free_ffi_value(ctx, entry->cached_result);
            polycall_core_free(ctx, entry->cached_result);
            
            // Remove entry by shifting remaining entries
            if (i < cache->count - 1) {
                memmove(&cache->entries[i], &cache->entries[i+1], 
                        sizeof(cache_entry_t) * (cache->count - i - 1));
            }
            
            cache->count--;
            // Don't increment i, as we've shifted the next entry into this position
        } else {
            i++;
        }
    }
}

static ffi_value_t* clone_ffi_value(polycall_core_context_t* ctx, const ffi_value_t* src) {
    if (!src) {
        return NULL;
    }
    
    ffi_value_t* dest = polycall_core_malloc(ctx, sizeof(ffi_value_t));
    if (!dest) {
        return NULL;
    }
    
    // Copy basic structure
    memcpy(dest, src, sizeof(ffi_value_t));
    
    // Deep copy any allocated resources based on type
    switch (src->type) {
        case FFI_TYPE_STRING:
            if (src->value.string_val) {
                dest->value.string_val = polycall_core_malloc(ctx, strlen(src->value.string_val) + 1);
                if (!dest->value.string_val) {
                    polycall_core_free(ctx, dest);
                    return NULL;
                }
                strcpy(dest->value.string_val, src->value.string_val);
            }
            break;
        // Add handling for other complex types that need deep copies
    }
    
    return dest;
}

static void free_ffi_value(polycall_core_context_t* ctx, ffi_value_t* value) {
    if (!value) {
        return;
    }
    
    // Free any allocated resources based on type
    switch (value->type) {
        case FFI_TYPE_STRING:
            if (value->value.string_val) {
                polycall_core_free(ctx, value->value.string_val);
                value->value.string_val = NULL;
            }
            break;
        // Add handling for other complex types
    }
}

static polycall_core_error_t init_call_cache(polycall_core_context_t* ctx, call_cache_t** cache, size_t capacity, uint32_t ttl_ms) {
    call_cache_t* new_cache = polycall_core_malloc(ctx, sizeof(call_cache_t));
    if (!new_cache) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate call cache");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    new_cache->entries = polycall_core_malloc(ctx, capacity * sizeof(cache_entry_t));
    if (!new_cache->entries) {
        polycall_core_free(ctx, new_cache);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate call cache entries");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_cache->entries, 0, capacity * sizeof(cache_entry_t));
    new_cache->count = 0;
    new_cache->capacity = capacity;
    new_cache->ttl_ms = ttl_ms;
    
    if (pthread_mutex_init(&new_cache->mutex, NULL) != 0) {
        polycall_core_free(ctx, new_cache->entries);
        polycall_core_free(ctx, new_cache);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to initialize call cache mutex");
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    *cache = new_cache;
    return POLYCALL_CORE_SUCCESS;
}

static void cleanup_call_cache(polycall_core_context_t* ctx, call_cache_t* cache) {
    if (!cache) {
        return;
    }
    
    // Free all entries
    for (size_t i = 0; i < cache->count; i++) {
        if (cache->entries[i].function_name) {
            polycall_core_free(ctx, cache->entries[i].function_name);
        }
        if (cache->entries[i].cached_result) {
            free_ffi_value(ctx, cache->entries[i].cached_result);
            polycall_core_free(ctx, cache->entries[i].cached_result);
        }
    }
    
    // Free entries array
    polycall_core_free(ctx, cache->entries);
    
    // Destroy mutex
    pthread_mutex_destroy(&cache->mutex);
    
    // Free cache structure
    polycall_core_free(ctx, cache);
}

static polycall_core_error_t init_type_cache(polycall_core_context_t* ctx, type_cache_t** cache, size_t capacity) {
    type_cache_t* new_cache = polycall_core_malloc(ctx, sizeof(type_cache_t));
    if (!new_cache) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate type cache");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    new_cache->entries = polycall_core_malloc(ctx, capacity * sizeof(type_cache_entry_t));
    if (!new_cache->entries) {
        polycall_core_free(ctx, new_cache);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate type cache entries");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_cache->entries, 0, capacity * sizeof(type_cache_entry_t));
    new_cache->count = 0;
    new_cache->capacity = capacity;
    
    if (pthread_mutex_init(&new_cache->mutex, NULL) != 0) {
        polycall_core_free(ctx, new_cache->entries);
        polycall_core_free(ctx, new_cache);
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to initialize type cache mutex");
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    *cache = new_cache;
    return POLYCALL_CORE_SUCCESS;
}

static void cleanup_type_cache(polycall_core_context_t* ctx, type_cache_t* cache) {
    if (!cache) {
        return;
    }
    
    // Free all entries (assuming converter_data is properly freed elsewhere)
    polycall_core_free(ctx, cache->entries);
    
    // Destroy mutex
    pthread_mutex_destroy(&cache->mutex);
    
    // Free cache structure
    polycall_core_free(ctx, cache);
}