/**
 * @file message_optimization.h
 * @brief Message Optimization for LibPolyCall Protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides advanced message optimization techniques including compression,
 * batching, prioritization, and adaptive scaling for efficient transmission.
 */

 #ifndef POLYCALL_PROTOCOL_MESSAGE_OPTIMIZATION_H_H
 #define POLYCALL_PROTOCOL_MESSAGE_OPTIMIZATION_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Maximum number of messages in a batch
  */
 #define POLYCALL_PROTOCOL_MESSAGE_OPTIMIZATION_H_H
 
 /**
  * @brief Maximum number of priority queues
  */
 #define POLYCALL_PROTOCOL_MESSAGE_OPTIMIZATION_H_H
 
 /**
  * @brief Message optimization compression levels
  */
 typedef enum {
     POLYCALL_MSG_COMPRESSION_NONE = 0,       /**< No compression */
     POLYCALL_MSG_COMPRESSION_FAST,           /**< Fast compression with modest ratio */
     POLYCALL_MSG_COMPRESSION_BALANCED,       /**< Balanced compression speed/ratio */
     POLYCALL_MSG_COMPRESSION_MAX             /**< Maximum compression ratio */
 } polycall_msg_compression_level_t;
 
 /**
  * @brief Message priority levels
  */
 typedef enum {
     POLYCALL_MSG_PRIORITY_LOWEST = 0,       /**< Lowest priority */
     POLYCALL_MSG_PRIORITY_LOW,              /**< Low priority */
     POLYCALL_MSG_PRIORITY_NORMAL,           /**< Normal priority */
     POLYCALL_MSG_PRIORITY_HIGH,             /**< High priority */
     POLYCALL_MSG_PRIORITY_CRITICAL          /**< Critical priority */
 } polycall_msg_priority_t;
 
 /**
  * @brief Message batching strategies
  */
 typedef enum {
     POLYCALL_BATCH_STRATEGY_SIZE = 0,       /**< Batch by message count */
     POLYCALL_BATCH_STRATEGY_TIME,           /**< Batch by elapsed time */
     POLYCALL_BATCH_STRATEGY_PRIORITY,       /**< Batch by message priority */
     POLYCALL_BATCH_STRATEGY_TYPE,           /**< Batch by message type */
     POLYCALL_BATCH_STRATEGY_ADAPTIVE        /**< Adapt strategy based on metrics */
 } polycall_msg_batch_strategy_t;
 
 /**
  * @brief Message optimization configuration
  */
 typedef struct {
     polycall_msg_compression_level_t compression_level; /**< Compression level */
     bool enable_batching;                     /**< Enable message batching */
     polycall_msg_batch_strategy_t batch_strategy; /**< Batching strategy */
     uint32_t batch_size;                       /**< Maximum batch size */
     uint32_t batch_timeout_ms;                 /**< Batch timeout in milliseconds */
     bool enable_prioritization;               /**< Enable message prioritization */
     uint32_t priority_queue_count;             /**< Number of priority queues */
     uint32_t priority_thresholds[POLYCALL_MAX_PRIORITY_QUEUES]; /**< Priority thresholds */
     bool enable_adaptive_optimization;        /**< Enable adaptive optimization */
     uint32_t optimization_check_interval_ms;   /**< Optimization check interval */
     uint32_t min_message_size_for_compression; /**< Minimum size for compression */
 } polycall_message_optimization_config_t;
 
 /**
  * @brief Message optimization statistics
  */
 typedef struct {
     uint64_t total_messages;                   /**< Total messages processed */
     uint64_t total_batches;                    /**< Total batches created */
     uint64_t total_original_bytes;             /**< Original data size in bytes */
     uint64_t total_optimized_bytes;            /**< Optimized data size in bytes */
     float average_compression_ratio;           /**< Average compression ratio */
     uint32_t compression_time_ms;              /**< Time spent in compression */
     uint32_t decompression_time_ms;            /**< Time spent in decompression */
     uint32_t messages_per_batch;               /**< Average messages per batch */
     uint32_t current_priority_distribution[POLYCALL_MAX_PRIORITY_QUEUES]; /**< Priority distribution */
 } polycall_message_optimization_stats_t;
 
 /**
  * @brief Message optimization context (opaque)
  */
 typedef struct polycall_message_optimization_context polycall_message_optimization_context_t;
 
 /**
  * @brief Initialize message optimization
  *
  * @param core_ctx Core context
  * @param proto_ctx Protocol context
  * @param opt_ctx Pointer to receive optimization context
  * @param config Optimization configuration
  * @return Error code
  */
 polycall_core_error_t polycall_message_optimization_init(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_message_optimization_context_t** opt_ctx,
     const polycall_message_optimization_config_t* config
 );
 
 /**
  * @brief Clean up message optimization
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context to clean up
  */
 void polycall_message_optimization_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx
 );
 
 /**
  * @brief Optimize a message for transmission
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param message Original message data
  * @param message_size Original message size
  * @param optimized_buffer Buffer to receive optimized message
  * @param buffer_size Size of optimization buffer
  * @param optimized_size Pointer to receive optimized size
  * @param priority Message priority
  * @return Error code
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
 );
 
 /**
  * @brief Restore an optimized message
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param optimized_message Optimized message data
  * @param optimized_size Optimized message size
  * @param original_buffer Buffer to receive original message
  * @param buffer_size Size of original buffer
  * @param original_size Pointer to receive original size
  * @return Error code
  */
 polycall_core_error_t polycall_message_restore(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* optimized_message,
     size_t optimized_size,
     void* original_buffer,
     size_t buffer_size,
     size_t* original_size
 );
 
 /**
  * @brief Add a message to the batch queue
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param message Message data
  * @param message_size Message size
  * @param priority Message priority
  * @param message_type Message type 
  * @return Error code
  */
 polycall_core_error_t polycall_message_batch_add(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     const void* message,
     size_t message_size,
     polycall_msg_priority_t priority,
     polycall_protocol_msg_type_t message_type
 );
 
 /**
  * @brief Process the batch queue
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param force_flush Force processing even if thresholds not met
  * @param batch_buffer Buffer to receive batch data
  * @param buffer_size Size of batch buffer
  * @param batch_size Pointer to receive batch size
  * @return Error code
  */
 polycall_core_error_t polycall_message_batch_process(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     bool force_flush,
     void* batch_buffer,
     size_t buffer_size,
     size_t* batch_size
 );
 
 /**
  * @brief Unbatch previously batched messages
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param batch_data Batch data
  * @param batch_size Batch size
  * @param message_callback Callback to receive individual messages
  * @param user_data User data for callback
  * @return Error code
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
 );
 
 /**
  * @brief Set message compression level
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param level Compression level
  * @return Error code
  */
 polycall_core_error_t polycall_message_set_compression(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     polycall_msg_compression_level_t level
 );
 
 /**
  * @brief Set message batch strategy
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param strategy Batch strategy
  * @param params Strategy-specific parameters
  * @return Error code
  */
 polycall_core_error_t polycall_message_set_batch_strategy(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     polycall_msg_batch_strategy_t strategy,
     void* params
 );
 
 /**
  * @brief Get message optimization statistics
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @param stats Pointer to receive statistics
  * @return Error code
  */
 polycall_core_error_t polycall_message_get_stats(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx,
     polycall_message_optimization_stats_t* stats
 );
 
 /**
  * @brief Reset message optimization statistics
  *
  * @param core_ctx Core context
  * @param opt_ctx Optimization context
  * @return Error code
  */
 polycall_core_error_t polycall_message_reset_stats(
     polycall_core_context_t* core_ctx,
     polycall_message_optimization_context_t* opt_ctx
 );
 
 /**
  * @brief Create default message optimization configuration
  *
  * @return Default configuration
  */
 polycall_message_optimization_config_t polycall_message_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_PROTOCOL_MESSAGE_OPTIMIZATION_H_H */