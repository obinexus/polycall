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
#include "polycall/core/protocol/enhancements/message_optimization.h"

 * @file message_optimization.c
 * @brief Message Optimization Implementation for LibPolyCall Protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements advanced message optimization techniques including compression,
 * batching, prioritization, and adaptive scaling for efficient transmission.
 */

 #include "polycall/core/protocol/enhancements/message_optimization.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 #define POLYCALL_MESSAGE_OPTIMIZATION_MAGIC 0xD3A70613
 #define POLYCALL_BATCH_HEADER_MAGIC 0xBA7C8412
 
 /**
  * @brief Message batch entry
  */
 typedef struct {
     void* data;                             // Message data
     size_t size;                            // Message size
     polycall_msg_priority_t priority;       // Message priority
     polycall_protocol_msg_type_t type;      // Message type
     uint64_t timestamp;                     // Entry timestamp
 } polycall_batch_entry_t;
 
 /**
  * @brief Message batch header
  */
 typedef struct {
     uint32_t magic;                         // Magic number for validation
     uint32_t message_count;                 // Number of messages
     uint32_t batch_strategy;                // Strategy used
     uint32_t compression_level;             // Compression level
     uint64_t batch_timestamp;               // Batch creation time
 } polycall_batch_header_t;
 
 /**
  * @brief Message optimization context
  */
 struct polycall_message_optimization_context {
     uint32_t magic;                         // Magic number for validation
     polycall_message_optimization_config_t config; // Configuration
     polycall_protocol_context_t* proto_ctx; // Protocol context
     
     // Batching
     polycall_batch_entry_t batch_queue[POLYCALL_MAX_BATCH_SIZE]; // Batch queue
     uint32_t batch_queue_count;             // Number of messages in queue
     uint64_t first_batch_timestamp;         // First message timestamp
     
     // Prioritization
     struct {
         polycall_batch_entry_t entries[POLYCALL_MAX_BATCH_SIZE]; // Queue entries
         uint32_t count;                     // Entry count
     } priority_queues[POLYCALL_MAX_PRIORITY_QUEUES]; // Priority queues
     
     // Statistics
     polycall_message_optimization_stats_t stats; // Performance statistics
     
     // Core context reference
     polycall_core_context_t* core_ctx;      // Core context
 };
 
 /**
  * @brief Get current timestamp in milliseconds
  */
 static uint64_t get_timestamp_ms() {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
 }
 
 /**
  * @brief Check if optimization context is valid
  */
 static bool validate_optimization_context(polycall_message_optimization_context_t* opt_ctx) {
     return opt_ctx && opt_ctx->magic == POLYCALL_MESSAGE_OPTIMIZATION_MAGIC;
 }
 
 /**
  * @brief Compress message data using the specified level
  * 
  * This is a simple placeholder implementation. In a real system, this would use
  * a proper compression library like zlib, lz4, etc.
  */
 static polycall_core_error_t compress_message_data(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* data,
     size_t data_size,
     void* compressed_buffer,
     size_t buffer_size,
     size_t* compressed_size,
     polycall_msg_compression_level_t level
 ) {
     if (!data || data_size == 0 || !compressed_buffer || buffer_size == 0 || !compressed_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Start timer for compression stats
     uint64_t start_time = get_timestamp_ms();
     
     // Simple placeholder for compression
     // In a real implementation, this would use an actual compression algorithm
     // with the compression level determining the parameters
     
     // For now, just copy the data (no actual compression)
     if (data_size > buffer_size) {
         return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
     }
     
     memcpy(compressed_buffer, data, data_size);
     *compressed_size = data_size;
     
     // End timer and update stats
     uint64_t end_time = get_timestamp_ms();
     opt_ctx->stats.compression_time_ms += (uint32_t)(end_time - start_time);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Decompress message data
  * 
  * This is a simple placeholder implementation. In a real system, this would use
  * a proper decompression function corresponding to the compression method.
  */
 static polycall_core_error_t decompress_message_data(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* compressed_data,
     size_t compressed_size,
     void* decompressed_buffer,
     size_t buffer_size,
     size_t* decompressed_size
 ) {
     if (!compressed_data || compressed_size == 0 || !decompressed_buffer || 
         buffer_size == 0 || !decompressed_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Start timer for decompression stats
     uint64_t start_time = get_timestamp_ms();
     
     // Simple placeholder for decompression
     // In a real implementation, this would use an actual decompression algorithm
     
     // For now, just copy the data (no actual decompression)
     if (compressed_size > buffer_size) {
         return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
     }
     
     memcpy(decompressed_buffer, compressed_data, compressed_size);
     *decompressed_size = compressed_size;
     
     // End timer and update stats
     uint64_t end_time = get_timestamp_ms();
     opt_ctx->stats.decompression_time_ms += (uint32_t)(end_time - start_time);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Initialize message optimization
  */
 polycall_core_error_t polycall_message_optimization_init(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_message_optimization_context_t** opt_ctx,
     const polycall_message_optimization_config_t* config
 ) {
     if (!core_ctx || !proto_ctx || !opt_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_message_optimization_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_message_optimization_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_message_optimization_context_t));
     new_ctx->magic = POLYCALL_MESSAGE_OPTIMIZATION_MAGIC;
     memcpy(&new_ctx->config, config, sizeof(polycall_message_optimization_config_t));
     new_ctx->proto_ctx = proto_ctx;
     new_ctx->core_ctx = core_ctx;
     
     // Validate config
     if (config->priority_queue_count > POLYCALL_MAX_PRIORITY_QUEUES) {
         new_ctx->config.priority_queue_count = POLYCALL_MAX_PRIORITY_QUEUES;
     }
     
     if (config->batch_size > POLYCALL_MAX_BATCH_SIZE) {
         new_ctx->config.batch_size = POLYCALL_MAX_BATCH_SIZE;
     }
     
     *opt_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up message optimization
  */
 void polycall_message_optimization_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx)) {
         return;
     }
     
     // Clean up batch queue
     for (uint32_t i = 0; i < opt_ctx->batch_queue_count; i++) {
         if (opt_ctx->batch_queue[i].data) {
             polycall_core_free(core_ctx, opt_ctx->batch_queue[i].data);
         }
     }
     
     // Clean up priority queues
     for (uint32_t q = 0; q < opt_ctx->config.priority_queue_count; q++) {
         for (uint32_t i = 0; i < opt_ctx->priority_queues[q].count; i++) {
             if (opt_ctx->priority_queues[q].entries[i].data) {
                 polycall_core_free(core_ctx, opt_ctx->priority_queues[q].entries[i].data);
             }
         }
     }
     
     // Clear magic number
     opt_ctx->magic = 0;
     
     // Free context
     polycall_core_free(core_ctx, opt_ctx);
 }
 
 /**
  * @brief Optimize a message for transmission
  */
 polycall_core_error_t polycall_message_optimize(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* message,
     size_t message_size,
     void* optimized_buffer,
     size_t buffer_size,
     size_t* optimized_size,
     polycall_msg_priority_t priority
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx) || 
         !message || message_size == 0 || 
         !optimized_buffer || buffer_size == 0 || !optimized_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Update statistics
     opt_ctx->stats.total_messages++;
     opt_ctx->stats.total_original_bytes += message_size;
     
     // Check if message should be compressed
     if (opt_ctx->config.compression_level != POLYCALL_MSG_COMPRESSION_NONE &&
         message_size >= opt_ctx->config.min_message_size_for_compression) {
         
         // Apply compression
         polycall_core_error_t result = compress_message_data(
             core_ctx,
             opt_ctx,
             message,
             message_size,
             optimized_buffer,
             buffer_size,
             optimized_size,
             opt_ctx->config.compression_level
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     } else {
         // No compression, just copy the message
         if (message_size > buffer_size) {
             return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
         }
         
         memcpy(optimized_buffer, message, message_size);
         *optimized_size = message_size;
     }
     
     // Update statistics
     opt_ctx->stats.total_optimized_bytes += *optimized_size;
     
     // Calculate compression ratio
     float current_ratio = (float)message_size / *optimized_size;
     float total_ratio = (float)opt_ctx->stats.total_original_bytes / opt_ctx->stats.total_optimized_bytes;
     
     // Update average compression ratio (using a weighted average)
     opt_ctx->stats.average_compression_ratio = 
         (opt_ctx->stats.average_compression_ratio * (opt_ctx->stats.total_messages - 1) + 
          current_ratio) / opt_ctx->stats.total_messages;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Restore an optimized message
  */
 polycall_core_error_t polycall_message_restore(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* optimized_message,
     size_t optimized_size,
     void* original_buffer,
     size_t buffer_size,
     size_t* original_size
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx) || 
         !optimized_message || optimized_size == 0 || 
         !original_buffer || buffer_size == 0 || !original_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if message was compressed
     if (opt_ctx->config.compression_level != POLYCALL_MSG_COMPRESSION_NONE) {
         // Apply decompression
         polycall_core_error_t result = decompress_message_data(
             core_ctx,
             opt_ctx,
             optimized_message,
             optimized_size,
             original_buffer,
             buffer_size,
             original_size
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     } else {
         // No compression was applied, just copy the message
         if (optimized_size > buffer_size) {
             return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
         }
         
         memcpy(original_buffer, optimized_message, optimized_size);
         *original_size = optimized_size;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Add a message to the batch queue
  */
 polycall_core_error_t polycall_message_batch_add(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* message,
     size_t message_size,
     polycall_msg_priority_t priority,
     polycall_protocol_msg_type_t message_type
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx) || 
         !message || message_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if batching is enabled
     if (!opt_ctx->config.enable_batching) {
         return POLYCALL_CORE_SUCCESS;  // Skip batching
     }
     
     // Check if batch queue is full
     if (opt_ctx->batch_queue_count >= opt_ctx->config.batch_size) {
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Create a copy of the message
     void* message_copy = polycall_core_malloc(core_ctx, message_size);
     if (!message_copy) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(message_copy, message, message_size);
     
     // Get current timestamp
     uint64_t current_time = get_timestamp_ms();
     
     // Store first message timestamp
     if (opt_ctx->batch_queue_count == 0) {
         opt_ctx->first_batch_timestamp = current_time;
     }
     
     // Add to appropriate queue based on strategy
     if (opt_ctx->config.enable_prioritization) {
         // Map priority to queue index
         uint32_t queue_idx = 0;
         
         switch (priority) {
             case POLYCALL_MSG_PRIORITY_LOWEST:
                 queue_idx = 0;
                 break;
             case POLYCALL_MSG_PRIORITY_LOW:
                 queue_idx = 1;
                 break;
             case POLYCALL_MSG_PRIORITY_NORMAL:
                 queue_idx = 2;
                 break;
             case POLYCALL_MSG_PRIORITY_HIGH:
                 queue_idx = 3;
                 break;
             case POLYCALL_MSG_PRIORITY_CRITICAL:
                 queue_idx = 4;
                 break;
         }
         
         // Adjust queue index based on available queues
         if (queue_idx >= opt_ctx->config.priority_queue_count) {
             queue_idx = opt_ctx->config.priority_queue_count - 1;
         }
         
         // Check if priority queue is full
         if (opt_ctx->priority_queues[queue_idx].count >= POLYCALL_MAX_BATCH_SIZE) {
             polycall_core_free(core_ctx, message_copy);
             return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
         }
         
         // Add to priority queue
         polycall_batch_entry_t* entry = 
             &opt_ctx->priority_queues[queue_idx].entries[opt_ctx->priority_queues[queue_idx].count++];
         
         entry->data = message_copy;
         entry->size = message_size;
         entry->priority = priority;
         entry->type = message_type;
         entry->timestamp = current_time;
         
         // Update priority distribution statistics
         opt_ctx->stats.current_priority_distribution[queue_idx]++;
     } else {
         // Add to batch queue
         polycall_batch_entry_t* entry = &opt_ctx->batch_queue[opt_ctx->batch_queue_count++];
         
         entry->data = message_copy;
         entry->size = message_size;
         entry->priority = priority;
         entry->type = message_type;
         entry->timestamp = current_time;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Process the batch queue
  */
 polycall_core_error_t polycall_message_batch_process(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     bool force_flush,
     void* batch_buffer,
     size_t buffer_size,
     size_t* batch_size
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx) || 
         !batch_buffer || buffer_size == 0 || !batch_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if batching is enabled
     if (!opt_ctx->config.enable_batching) {
         *batch_size = 0;
         return POLYCALL_CORE_SUCCESS;  // Skip batching
     }
     
     // Check if batch should be processed
     bool should_process = force_flush;
     
     if (!should_process) {
         // Check batch size threshold
         if (opt_ctx->batch_strategy == POLYCALL_BATCH_STRATEGY_SIZE) {
             should_process = (opt_ctx->batch_queue_count >= opt_ctx->config.batch_size);
         }
         
         // Check time threshold
         else if (opt_ctx->batch_strategy == POLYCALL_BATCH_STRATEGY_TIME) {
             uint64_t current_time = get_timestamp_ms();
             uint64_t elapsed_time = current_time - opt_ctx->first_batch_timestamp;
             
             should_process = (elapsed_time >= opt_ctx->config.batch_timeout_ms);
         }
         
         // For other strategies, we rely on external triggers or force_flush
     }
     
     // Nothing to process
     if (!should_process || (opt_ctx->batch_queue_count == 0 && 
                            !opt_ctx->config.enable_prioritization)) {
         *batch_size = 0;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Prepare batch header
     polycall_batch_header_t header;
     header.magic = POLYCALL_BATCH_HEADER_MAGIC;
     header.message_count = 0;
     header.batch_strategy = opt_ctx->config.batch_strategy;
     header.compression_level = opt_ctx->config.compression_level;
     header.batch_timestamp = get_timestamp_ms();
     
     // Calculate batch size requirements
     size_t required_size = sizeof(polycall_batch_header_t);
     
     // Process priority queues if enabled
     if (opt_ctx->config.enable_prioritization) {
         // Process from highest to lowest priority
         for (int q = opt_ctx->config.priority_queue_count - 1; q >= 0; q--) {
             for (uint32_t i = 0; i < opt_ctx->priority_queues[q].count; i++) {
                 polycall_batch_entry_t* entry = &opt_ctx->priority_queues[q].entries[i];
                 
                 // 8 bytes for message size + 4 bytes for priority + 4 bytes for type
                 size_t entry_overhead = 16;
                 size_t entry_size = entry->size + entry_overhead;
                 
                 // Check if this entry would fit
                 if (required_size + entry_size > buffer_size) {
                     // Skip remaining entries in this queue and lower priority queues
                     break;
                 }
                 
                 // Message fits, add it to the batch
                 uint8_t* ptr = (uint8_t*)batch_buffer + required_size;
                 
                 // Write message size (8 bytes)
                 *((uint64_t*)ptr) = entry->size;
                 ptr += 8;
                 
                 // Write priority (4 bytes)
                 *((uint32_t*)ptr) = (uint32_t)entry->priority;
                 ptr += 4;
                 
                 // Write message type (4 bytes)
                 *((uint32_t*)ptr) = (uint32_t)entry->type;
                 ptr += 4;
                 
                 // Write message data
                 memcpy(ptr, entry->data, entry->size);
                 
                 // Update required size
                 required_size += entry_size;
                 
                 // Increment message count
                 header.message_count++;
                 
                 // Free the message copy
                 polycall_core_free(core_ctx, entry->data);
                 entry->data = NULL;
             }
             
             // Reset this priority queue
             opt_ctx->priority_queues[q].count = 0;
             opt_ctx->stats.current_priority_distribution[q] = 0;
         }
     } else {
         // Process batch queue
         for (uint32_t i = 0; i < opt_ctx->batch_queue_count; i++) {
             polycall_batch_entry_t* entry = &opt_ctx->batch_queue[i];
             
             // 8 bytes for message size + 4 bytes for priority + 4 bytes for type
             size_t entry_overhead = 16;
             size_t entry_size = entry->size + entry_overhead;
             
             // Check if this entry would fit
             if (required_size + entry_size > buffer_size) {
                 // Not enough space for this message
                 polycall_core_free(core_ctx, entry->data);
                 entry->data = NULL;
                 continue;
             }
             
             // Message fits, add it to the batch
             uint8_t* ptr = (uint8_t*)batch_buffer + required_size;
             
             // Write message size (8 bytes)
             *((uint64_t*)ptr) = entry->size;
             ptr += 8;
             
             // Write priority (4 bytes)
             *((uint32_t*)ptr) = (uint32_t)entry->priority;
             ptr += 4;
             
             // Write message type (4 bytes)
             *((uint32_t*)ptr) = (uint32_t)entry->type;
             ptr += 4;
             
             // Write message data
             memcpy(ptr, entry->data, entry->size);
             
             // Update required size
             required_size += entry_size;
             
             // Increment message count
             header.message_count++;
             
             // Free the message copy
             polycall_core_free(core_ctx, entry->data);
             entry->data = NULL;
         }
         
         // Reset batch queue
         opt_ctx->batch_queue_count = 0;
     }
     
     // If no messages were added, return
     if (header.message_count == 0) {
         *batch_size = 0;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Write batch header at the beginning of the buffer
     memcpy(batch_buffer, &header, sizeof(polycall_batch_header_t));
     
     // Update output size
     *batch_size = required_size;
     
     // Update statistics
     opt_ctx->stats.total_batches++;
     opt_ctx->stats.messages_per_batch = 
         (opt_ctx->stats.messages_per_batch * (opt_ctx->stats.total_batches - 1) + 
          header.message_count) / opt_ctx->stats.total_batches;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Unbatch previously batched messages
  */
 polycall_core_error_t polycall_message_unbatch(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* batch_data,
     size_t batch_size,
     void (*message_callback)(
         const void* message,
         size_t message_size,
         polycall_msg_priority_t priority,
         polycall_protocol_msg_type_t message_type,
         void* user_data
     ),
     void* user_data
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx) || 
         !batch_data || batch_size < sizeof(polycall_batch_header_t) || 
         !message_callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Parse batch header
     const polycall_batch_header_t* header = (const polycall_batch_header_t*)batch_data;
     
     // Validate header
     if (header->magic != POLYCALL_BATCH_HEADER_MAGIC) {
         return POLYCALL_CORE_ERROR_INVALID_FORMAT;
     }
     
     // Initialize pointer to first message
     const uint8_t* ptr = (const uint8_t*)batch_data + sizeof(polycall_batch_header_t);
     const uint8_t* end_ptr = (const uint8_t*)batch_data + batch_size;
     
     // Process each message
     for (uint32_t i = 0; i < header->message_count; i++) {
         // Check if there's enough data for message header
         if (ptr + 16 > end_ptr) {
             return POLYCALL_CORE_ERROR_INVALID_FORMAT;
         }
         
         // Read message size
         uint64_t message_size = *((uint64_t*)ptr);
         ptr += 8;
         
         // Read priority
         uint32_t priority_value = *((uint32_t*)ptr);
         polycall_msg_priority_t priority = (polycall_msg_priority_t)priority_value;
         ptr += 4;
         
         // Read message type
         uint32_t type_value = *((uint32_t*)ptr);
         polycall_protocol_msg_type_t message_type = (polycall_protocol_msg_type_t)type_value;
         ptr += 4;
         
         // Check if there's enough data for message content
         if (ptr + message_size > end_ptr) {
             return POLYCALL_CORE_ERROR_INVALID_FORMAT;
         }
         
         // Call user callback
         message_callback(ptr, message_size, priority, message_type, user_data);
         
         // Advance to next message
         ptr += message_size;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set message compression level
  */
 polycall_core_error_t polycall_message_set_compression(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     polycall_msg_compression_level_t level
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     opt_ctx->config.compression_level = level;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set message batch strategy
  */
 polycall_core_error_t polycall_message_set_batch_strategy(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     polycall_msg_batch_strategy_t strategy,
     void* params
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     opt_ctx->config.batch_strategy = strategy;
     
     // Handle strategy-specific parameters
     switch (strategy) {
         case POLYCALL_BATCH_STRATEGY_SIZE:
             if (params) {
                 uint32_t* size_param = (uint32_t*)params;
                 if (*size_param > 0 && *size_param <= POLYCALL_MAX_BATCH_SIZE) {
                     opt_ctx->config.batch_size = *size_param;
                 }
             }
             break;
             
         case POLYCALL_BATCH_STRATEGY_TIME:
             if (params) {
                 uint32_t* time_param = (uint32_t*)params;
                 if (*time_param > 0) {
                     opt_ctx->config.batch_timeout_ms = *time_param;
                 }
             }
             break;
             
         default:
             // Other strategies might have their own parameters
             break;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get message optimization statistics
  */
 polycall_core_error_t polycall_message_get_stats(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     polycall_message_optimization_stats_t* stats
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx) || !stats) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     memcpy(stats, &opt_ctx->stats, sizeof(polycall_message_optimization_stats_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Reset message optimization statistics
  */
 polycall_core_error_t polycall_message_reset_stats(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx
 ) {
     if (!core_ctx || !validate_optimization_context(opt_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     memset(&opt_ctx->stats, 0, sizeof(polycall_message_optimization_stats_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default message optimization configuration
  */
 polycall_message_optimization_config_t polycall_message_default_config(void) {
     polycall_message_optimization_config_t config;
     
     config.compression_level = POLYCALL_MSG_COMPRESSION_BALANCED;
     config.enable_batching = true;
     config.batch_strategy = POLYCALL_BATCH_STRATEGY_SIZE;
     config.batch_size = 16;
     config.batch_timeout_ms = 100;  // 100 milliseconds
     config.enable_prioritization = true;
     config.priority_queue_count = 5;  // One for each priority level
     
     // Set default priority thresholds
     for (uint32_t i = 0; i < POLYCALL_MAX_PRIORITY_QUEUES; i++) {
         config.priority_thresholds[i] = i;
     }
     
     config.enable_adaptive_optimization = true;
     config.optimization_check_interval_ms = 5000;  // 5 seconds
     config.min_message_size_for_compression = 128;  // 128 bytes
     
     return config;
 }
