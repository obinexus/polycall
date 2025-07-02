/**
#include "polycall/core/protocol/communication.h"

 * @file communication.c
 * @brief Duplex communication stream implementation for LibPolyCall protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the bidirectional communication interface for the LibPolyCall
 * protocol, enabling efficient duplex streams for data transfer, observation,
 * and polling between endpoints.
 */

 #include "polycall/core/protocol/communication.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <string.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <time.h>
 #include <pthread.h>
 
 #define DEFAULT_BUFFER_SIZE 8192
 #define DEFAULT_MAX_MESSAGE_SIZE 65536
 #define DEFAULT_POLL_INTERVAL_MS 100
 #define DEFAULT_IDLE_TIMEOUT_MS 30000
 #define DEFAULT_RECONNECT_TIMEOUT_MS 5000
 #define MAX_OBSERVER_COUNT 16
 #define STREAM_MAGIC 0x53545244 /* "STRD" in ASCII */
 
 /**
  * @brief Circular buffer structure for stream data
  */
 typedef struct {
     uint8_t* data;                  /**< Buffer data */
     size_t capacity;                /**< Buffer capacity */
     size_t read_pos;                /**< Current read position */
     size_t write_pos;               /**< Current write position */
     size_t size;                    /**< Current data size */
     pthread_mutex_t mutex;          /**< Mutex for thread safety */
 } circular_buffer_t;
 
 /**
  * @brief Communication observer structure
  */
 struct polycall_comm_observer {
     polycall_comm_stream_t* stream;            /**< Associated stream */
     polycall_comm_observer_config_t config;    /**< Observer configuration */
     bool is_active;                           /**< Whether observer is active */
     uint32_t id;                              /**< Observer ID */
 };
 
 /**
  * @brief Communication stream structure
  */
 struct polycall_comm_stream {
     uint32_t magic;                           /**< Magic number for validation */
     polycall_core_context_t* core_ctx;        /**< Core context */
     polycall_protocol_context_t* proto_ctx;   /**< Protocol context */
     polycall_comm_config_t config;            /**< Stream configuration */
     polycall_comm_state_t state;              /**< Current state */
     circular_buffer_t send_buffer;            /**< Buffer for sending data */
     circular_buffer_t recv_buffer;            /**< Buffer for receiving data */
     polycall_comm_stats_t stats;              /**< Stream statistics */
     uint64_t last_activity_time;              /**< Last activity timestamp */
     uint32_t buffer_threshold;                /**< Buffer threshold for notifications */
     pthread_mutex_t mutex;                    /**< Mutex for thread safety */
     pthread_t polling_thread;                 /**< Polling thread */
     bool polling_active;                      /**< Whether polling is active */
     polycall_comm_observer_t* observers[MAX_OBSERVER_COUNT]; /**< Stream observers */
     size_t observer_count;                    /**< Number of observers */
 };
 
 // Forward declarations for internal functions
 static bool validate_stream(polycall_comm_stream_t* stream);
 static polycall_core_error_t initialize_circular_buffer(polycall_core_context_t* ctx, circular_buffer_t* buffer, size_t capacity);
 static void cleanup_circular_buffer(polycall_core_context_t* ctx, circular_buffer_t* buffer);
 static size_t circular_buffer_write(circular_buffer_t* buffer, const void* data, size_t size);
 static size_t circular_buffer_read(circular_buffer_t* buffer, void* data, size_t size);
 static size_t circular_buffer_available(circular_buffer_t* buffer);
 static size_t circular_buffer_used(circular_buffer_t* buffer);
 static void update_stream_state(polycall_comm_stream_t* stream, polycall_comm_state_t new_state);
 static void notify_observers(polycall_comm_stream_t* stream, const void* data, size_t size);
 static void* polling_thread_func(void* arg);
 static uint64_t get_current_time_ms(void);
 
 /**
  * @brief Create a default communication configuration
  */
 polycall_comm_config_t polycall_comm_create_default_config(void) {
     polycall_comm_config_t config;
     memset(&config, 0, sizeof(config));
     
     config.type = POLYCALL_COMM_STREAM_STANDARD;
     config.flags = POLYCALL_COMM_FLAG_BUFFERED;
     config.buffer_size = DEFAULT_BUFFER_SIZE;
     config.max_message_size = DEFAULT_MAX_MESSAGE_SIZE;
     config.poll_interval_ms = DEFAULT_POLL_INTERVAL_MS;
     config.idle_timeout_ms = DEFAULT_IDLE_TIMEOUT_MS;
     config.reconnect_timeout_ms = DEFAULT_RECONNECT_TIMEOUT_MS;
     
     return config;
 }
 
 /**
  * @brief Create a communication stream
  */
 polycall_core_error_t polycall_comm_create_stream(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_comm_stream_t** stream,
     const polycall_comm_config_t* config
 ) {
     if (!ctx || !proto_ctx || !stream) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default config if not provided
     polycall_comm_config_t default_config;
     if (!config) {
         default_config = polycall_comm_create_default_config();
         config = &default_config;
     }
     
     // Allocate stream structure
     polycall_comm_stream_t* new_stream = polycall_core_malloc(ctx, sizeof(polycall_comm_stream_t));
     if (!new_stream) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate communication stream");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize stream
     memset(new_stream, 0, sizeof(polycall_comm_stream_t));
     new_stream->magic = STREAM_MAGIC;
     new_stream->core_ctx = ctx;
     new_stream->proto_ctx = proto_ctx;
     memcpy(&new_stream->config, config, sizeof(polycall_comm_config_t));
     new_stream->state = POLYCALL_COMM_STATE_INIT;
     new_stream->last_activity_time = get_current_time_ms();
     new_stream->buffer_threshold = config->buffer_size / 2; // Default threshold is half buffer
     
     // Initialize mutexes
     pthread_mutex_init(&new_stream->mutex, NULL);
     
     // Initialize circular buffers
     polycall_core_error_t result = initialize_circular_buffer(ctx, &new_stream->send_buffer, config->buffer_size);
     if (result != POLYCALL_CORE_SUCCESS) {
         pthread_mutex_destroy(&new_stream->mutex);
         polycall_core_free(ctx, new_stream);
         return result;
     }
     
     result = initialize_circular_buffer(ctx, &new_stream->recv_buffer, config->buffer_size);
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_circular_buffer(ctx, &new_stream->send_buffer);
         pthread_mutex_destroy(&new_stream->mutex);
         polycall_core_free(ctx, new_stream);
         return result;
     }
     
     *stream = new_stream;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Destroy a communication stream
  */
 void polycall_comm_destroy_stream(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !stream || stream->magic != STREAM_MAGIC) {
         return;
     }
     
     // Close stream if it's open
     if (stream->state != POLYCALL_COMM_STATE_CLOSED && stream->state != POLYCALL_COMM_STATE_INIT) {
         polycall_comm_close_stream(ctx, stream);
     }
     
     // Wait for polling thread to terminate
     if (stream->polling_active) {
         stream->polling_active = false;
         pthread_join(stream->polling_thread, NULL);
     }
     
     // Clean up observers
     for (size_t i = 0; i < stream->observer_count; i++) {
         if (stream->observers[i]) {
             stream->observers[i]->stream = NULL; // Detach from stream
             polycall_core_free(ctx, stream->observers[i]);
             stream->observers[i] = NULL;
         }
     }
     
     // Clean up buffers
     cleanup_circular_buffer(ctx, &stream->send_buffer);
     cleanup_circular_buffer(ctx, &stream->recv_buffer);
     
     // Destroy mutex
     pthread_mutex_destroy(&stream->mutex);
     
     // Invalidate and free
     stream->magic = 0;
     polycall_core_free(ctx, stream);
 }
 
 /**
  * @brief Open a communication stream
  */
 polycall_core_error_t polycall_comm_open_stream(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_INIT && stream->state != POLYCALL_COMM_STATE_CLOSED) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot open stream in current state: %d", stream->state);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Start polling thread if non-blocking
     if (stream->config.flags & POLYCALL_COMM_FLAG_NONBLOCKING) {
         stream->polling_active = true;
         
         // Create polling thread
         int result = pthread_create(&stream->polling_thread, NULL, polling_thread_func, stream);
         if (result != 0) {
             pthread_mutex_unlock(&stream->mutex);
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                              POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to create polling thread: %d", result);
             return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
         }
     }
     
     // Update state
     update_stream_state(stream, POLYCALL_COMM_STATE_OPEN);
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Receive data from a communication stream
  */
 polycall_core_error_t polycall_comm_receive(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream,
     void* buffer,
     size_t size,
     size_t* bytes_read,
     uint32_t flags
 ) {
     if (!ctx || !validate_stream(stream) || !buffer || size == 0 || !bytes_read) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *bytes_read = 0;
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_OPEN && 
         stream->state != POLYCALL_COMM_STATE_ACTIVE) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot receive data in current state: %d", stream->state);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check if data is available
     if (circular_buffer_used(&stream->recv_buffer) == 0) {
         // If non-blocking, return immediately
         if (flags & POLYCALL_COMM_FLAG_NONBLOCKING) {
             pthread_mutex_unlock(&stream->mutex);
             return POLYCALL_CORE_SUCCESS;
         }
         
         // Otherwise, we'd block which isn't implemented in this version
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Blocking receive not supported in this version");
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     // Read data from buffer
     *bytes_read = circular_buffer_read(&stream->recv_buffer, buffer, size);
     
     // Update stats
     stream->stats.bytes_received += *bytes_read;
     if (*bytes_read > 0) {
         stream->stats.messages_received++;
         stream->last_activity_time = get_current_time_ms();
     }
     
     // Update stream state if this is the first data
     if (stream->state == POLYCALL_COMM_STATE_OPEN && *bytes_read > 0) {
         update_stream_state(stream, POLYCALL_COMM_STATE_ACTIVE);
     }
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Poll a communication stream for data
  */
 bool polycall_comm_poll(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream,
     uint32_t timeout_ms
 ) {
     if (!ctx || !validate_stream(stream)) {
         return false;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_OPEN && 
         stream->state != POLYCALL_COMM_STATE_ACTIVE) {
         pthread_mutex_unlock(&stream->mutex);
         return false;
     }
     
     // Check if data is already available
     bool has_data = circular_buffer_used(&stream->recv_buffer) > 0;
     
     // If data available or no timeout, return immediately
     if (has_data || timeout_ms == 0) {
         pthread_mutex_unlock(&stream->mutex);
         return has_data;
     }
     
     // Otherwise, we'd need to wait which isn't implemented in this simple version
     // In a real implementation, you would use condition variables or select/poll here
     
     pthread_mutex_unlock(&stream->mutex);
     
     // Simple polling implementation for demonstration
     uint64_t start_time = get_current_time_ms();
     uint64_t current_time;
     
     do {
         // Sleep a bit to avoid spinning
         struct timespec sleep_time = {0, 10000000}; // 10ms
         nanosleep(&sleep_time, NULL);
         
         pthread_mutex_lock(&stream->mutex);
         has_data = circular_buffer_used(&stream->recv_buffer) > 0;
         pthread_mutex_unlock(&stream->mutex);
         
         if (has_data) {
             return true;
         }
         
         current_time = get_current_time_ms();
     } while (current_time - start_time < timeout_ms);
     
     return false;
 }
 
 /**
  * @brief Create an observer for a communication stream
  */
 polycall_core_error_t polycall_comm_create_observer(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream,
     polycall_comm_observer_t** observer,
     const polycall_comm_observer_config_t* config
 ) {
     if (!ctx || !validate_stream(stream) || !observer || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check if we've reached the maximum number of observers
     if (stream->observer_count >= MAX_OBSERVER_COUNT) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Maximum number of observers reached");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Allocate observer
     polycall_comm_observer_t* new_observer = polycall_core_malloc(ctx, sizeof(polycall_comm_observer_t));
     if (!new_observer) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate observer");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize observer
     memset(new_observer, 0, sizeof(polycall_comm_observer_t));
     new_observer->stream = stream;
     memcpy(&new_observer->config, config, sizeof(polycall_comm_observer_config_t));
     new_observer->is_active = true;
     new_observer->id = (uint32_t)stream->observer_count + 1;
     
     // Add to stream's observer list
     stream->observers[stream->observer_count++] = new_observer;
     
     *observer = new_observer;
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Destroy an observer
  */
 void polycall_comm_destroy_observer(
     polycall_core_context_t* ctx,
     polycall_comm_observer_t* observer
 ) {
     if (!ctx || !observer) {
         return;
     }
     
     polycall_comm_stream_t* stream = observer->stream;
     
     // Stream might already be destroyed
     if (!stream || !validate_stream(stream)) {
         polycall_core_free(ctx, observer);
         return;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Find observer in stream's list
     for (size_t i = 0; i < stream->observer_count; i++) {
         if (stream->observers[i] == observer) {
             // Remove from list by shifting elements
             for (size_t j = i; j < stream->observer_count - 1; j++) {
                 stream->observers[j] = stream->observers[j + 1];
             }
             stream->observers[stream->observer_count - 1] = NULL;
             stream->observer_count--;
             break;
         }
     }
     
     pthread_mutex_unlock(&stream->mutex);
     
     // Free observer
     polycall_core_free(ctx, observer);
 }
 
 /**
  * @brief Pause a communication stream
  */
 polycall_core_error_t polycall_comm_pause_stream(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_OPEN && 
         stream->state != POLYCALL_COMM_STATE_ACTIVE) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot pause stream in current state: %d", stream->state);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update state
     update_stream_state(stream, POLYCALL_COMM_STATE_PAUSED);
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Resume a paused communication stream
  */
 polycall_core_error_t polycall_comm_resume_stream(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_PAUSED) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot resume stream in current state: %d", stream->state);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Determine new state based on buffer contents
     polycall_comm_state_t new_state = POLYCALL_COMM_STATE_OPEN;
     if (circular_buffer_used(&stream->send_buffer) > 0 || 
         circular_buffer_used(&stream->recv_buffer) > 0) {
         new_state = POLYCALL_COMM_STATE_ACTIVE;
     }
     
     // Update state
     update_stream_state(stream, new_state);
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get communication stream statistics
  */
 polycall_core_error_t polycall_comm_get_stats(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream,
     polycall_comm_stats_t* stats
 ) {
     if (!ctx || !validate_stream(stream) || !stats) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Update dynamic stats
     uint64_t current_time = get_current_time_ms();
     uint64_t elapsed_sec = (current_time - stream->stats.last_activity_time) / 1000;
     
     if (elapsed_sec > 0) {
         stream->stats.throughput_send = (double)stream->stats.bytes_sent / elapsed_sec;
         stream->stats.throughput_receive = (double)stream->stats.bytes_received / elapsed_sec;
     }
     
     stream->stats.current_buffer_usage = circular_buffer_used(&stream->send_buffer) + 
                                      circular_buffer_used(&stream->recv_buffer);
     
     if (stream->stats.current_buffer_usage > stream->stats.max_buffer_usage) {
         stream->stats.max_buffer_usage = stream->stats.current_buffer_usage;
     }
     
     // Copy stats
     memcpy(stats, &stream->stats, sizeof(polycall_comm_stats_t));
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get communication stream state
  */
 polycall_comm_state_t polycall_comm_get_state(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_COMM_STATE_ERROR;
     }
     
     return stream->state;
 }
 
 /**
  * @brief Set buffer threshold for notifications
  */
 polycall_core_error_t polycall_comm_set_buffer_threshold(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream,
     uint32_t threshold
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check threshold
     if (threshold > stream->config.buffer_size) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Threshold exceeds buffer size: %u > %u", 
                          threshold, stream->config.buffer_size);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     stream->buffer_threshold = threshold;
     pthread_mutex_unlock(&stream->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Flush a communication stream buffer
  */
 polycall_core_error_t polycall_comm_flush(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_OPEN && 
         stream->state != POLYCALL_COMM_STATE_ACTIVE && 
         stream->state != POLYCALL_COMM_STATE_PAUSED) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot flush stream in current state: %d", stream->state);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Reset buffers
     pthread_mutex_lock(&stream->send_buffer.mutex);
     stream->send_buffer.read_pos = 0;
     stream->send_buffer.write_pos = 0;
     stream->send_buffer.size = 0;
     pthread_mutex_unlock(&stream->send_buffer.mutex);
     
     pthread_mutex_lock(&stream->recv_buffer.mutex);
     stream->recv_buffer.read_pos = 0;
     stream->recv_buffer.write_pos = 0;
     stream->recv_buffer.size = 0;
     pthread_mutex_unlock(&stream->recv_buffer.mutex);
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register with protocol for bidirectional data handling
  */
 polycall_core_error_t polycall_comm_register_with_protocol(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !proto_ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // This function would integrate with the protocol context to handle
     // bidirectional data flow. In a real implementation, it would register
     // callbacks with the protocol layer to handle incoming data.
     
     // Since protocol integration details are not fully specified, this is a placeholder.
     // In a complete implementation, it would:
     // 1. Register with protocol message handlers
     // 2. Set up data forwarding from network to stream
     // 3. Configure protocol state change notifications
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /* Helper Functions Implementation */
 
 /**
  * @brief Validate a stream object
  */
 static bool validate_stream(polycall_comm_stream_t* stream) {
     return stream && stream->magic == STREAM_MAGIC;
 }
 
 /**
  * @brief Initialize a circular buffer
  */
 static polycall_core_error_t initialize_circular_buffer(
     polycall_core_context_t* ctx,
     circular_buffer_t* buffer,
     size_t capacity
 ) {
     if (!ctx || !buffer || capacity == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate buffer memory
     buffer->data = polycall_core_malloc(ctx, capacity);
     if (!buffer->data) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MEMORY, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate circular buffer");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize buffer
     buffer->capacity = capacity;
     buffer->read_pos = 0;
     buffer->write_pos = 0;
     buffer->size = 0;
     
     // Initialize mutex
     pthread_mutex_init(&buffer->mutex, NULL);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up a circular buffer
  */
 static void cleanup_circular_buffer(
     polycall_core_context_t* ctx,
     circular_buffer_t* buffer
 ) {
     if (!ctx || !buffer) {
         return;
     }
     
     // Free buffer data
     if (buffer->data) {
         polycall_core_free(ctx, buffer->data);
         buffer->data = NULL;
     }
     
     // Destroy mutex
     pthread_mutex_destroy(&buffer->mutex);
     
     // Reset fields
     buffer->capacity = 0;
     buffer->read_pos = 0;
     buffer->write_pos = 0;
     buffer->size = 0;
 }
 
 /**
  * @brief Write data to a circular buffer
  */
 static size_t circular_buffer_write(
     circular_buffer_t* buffer,
     const void* data,
     size_t size
 ) {
     if (!buffer || !buffer->data || !data || size == 0) {
         return 0;
     }
     
     const uint8_t* src = (const uint8_t*)data;
     size_t bytes_to_write = size;
     size_t bytes_written = 0;
     
     pthread_mutex_lock(&buffer->mutex);
     
     // Calculate available space
     size_t available = buffer->capacity - buffer->size;
     if (bytes_to_write > available) {
         bytes_to_write = available;
     }
     
     if (bytes_to_write == 0) {
         pthread_mutex_unlock(&buffer->mutex);
         return 0;
     }
     
     // Write data
     while (bytes_written < bytes_to_write) {
         buffer->data[buffer->write_pos] = src[bytes_written];
         buffer->write_pos = (buffer->write_pos + 1) % buffer->capacity;
         bytes_written++;
     }
     
     // Update size
     buffer->size += bytes_written;
     
     pthread_mutex_unlock(&buffer->mutex);
     return bytes_written;
 }
 
 /**
  * @brief Read data from a circular buffer
  */
 static size_t circular_buffer_read(
     circular_buffer_t* buffer,
     void* data,
     size_t size
 ) {
     if (!buffer || !buffer->data || !data || size == 0) {
         return 0;
     }
     
     uint8_t* dst = (uint8_t*)data;
     size_t bytes_to_read = size;
     size_t bytes_read = 0;
     
     pthread_mutex_lock(&buffer->mutex);
     
     // Calculate available data
     if (bytes_to_read > buffer->size) {
         bytes_to_read = buffer->size;
     }
     
     if (bytes_to_read == 0) {
         pthread_mutex_unlock(&buffer->mutex);
         return 0;
     }
     
     // Read data
     while (bytes_read < bytes_to_read) {
         dst[bytes_read] = buffer->data[buffer->read_pos];
         buffer->read_pos = (buffer->read_pos + 1) % buffer->capacity;
         bytes_read++;
     }
     
     // Update size
     buffer->size -= bytes_read;
     
     pthread_mutex_unlock(&buffer->mutex);
     return bytes_read;
 }
 
 /**
  * @brief Get available space in a circular buffer
  */
 static size_t circular_buffer_available(circular_buffer_t* buffer) {
     if (!buffer) {
         return 0;
     }
     
     pthread_mutex_lock(&buffer->mutex);
     size_t available = buffer->capacity - buffer->size;
     pthread_mutex_unlock(&buffer->mutex);
     
     return available;
 }
 
 /**
  * @brief Get used space in a circular buffer
  */
 static size_t circular_buffer_used(circular_buffer_t* buffer) {
     if (!buffer) {
         return 0;
     }
     
     pthread_mutex_lock(&buffer->mutex);
     size_t used = buffer->size;
     pthread_mutex_unlock(&buffer->mutex);
     
     return used;
 }
 
 /**
  * @brief Update stream state with proper notifications
  */
 static void update_stream_state(
     polycall_comm_stream_t* stream,
     polycall_comm_state_t new_state
 ) {
     if (!stream || stream->state == new_state) {
         return;
     }
     
     polycall_comm_state_t old_state = stream->state;
     stream->state = new_state;
     
     // Notify state change callback if provided
     if (stream->config.callbacks.on_state_change) {
         stream->config.callbacks.on_state_change(
             stream, old_state, new_state, stream->config.user_data
         );
     }
 }
 
 /**
  * @brief Notify observers of new data
  */
 static void notify_observers(
     polycall_comm_stream_t* stream,
     const void* data,
     size_t size
 ) {
     if (!stream || !data || size == 0) {
         return;
     }
     
     for (size_t i = 0; i < stream->observer_count; i++) {
         polycall_comm_observer_t* observer = stream->observers[i];
         if (observer && observer->is_active && observer->config.on_next) {
             observer->config.on_next(observer, data, size, observer->config.user_data);
         }
     }
 }
 
 /**
  * @brief Polling thread function
  */
 static void* polling_thread_func(void* arg) {
     polycall_comm_stream_t* stream = (polycall_comm_stream_t*)arg;
     if (!stream) {
         return NULL;
     }
     
     // Buffer for data processing
     uint8_t buffer[4096];
     size_t bytes_read;
     
     while (stream->polling_active) {
         // Check for incoming data from protocol/network layer
         // In a real implementation, this would interact with the protocol context
         
         // Read data from recv buffer and notify observers
         pthread_mutex_lock(&stream->mutex);
         bytes_read = circular_buffer_read(&stream->recv_buffer, buffer, sizeof(buffer));
         if (bytes_read > 0) {
             notify_observers(stream, buffer, bytes_read);
             
             // Notify data received callback
             if (stream->config.callbacks.on_data_received) {
                 stream->config.callbacks.on_data_received(
                     stream, buffer, bytes_read, stream->config.user_data
                 );
             }
         }
         pthread_mutex_unlock(&stream->mutex);
         
         // Check for idle timeout
         pthread_mutex_lock(&stream->mutex);
         uint64_t current_time = get_current_time_ms();
         if (stream->config.idle_timeout_ms > 0 && 
             current_time - stream->last_activity_time > stream->config.idle_timeout_ms) {
             
             // Notify error
             if (stream->config.callbacks.on_error) {
                 stream->config.callbacks.on_error(
                     stream, 
                     POLYCALL_CORE_ERROR_TIMEOUT, 
                     "Stream idle timeout", 
                     stream->config.user_data
                 );
             }
             
             // Update state
             update_stream_state(stream, POLYCALL_COMM_STATE_ERROR);
         }
         pthread_mutex_unlock(&stream->mutex);
         
         // Sleep for poll interval
         struct timespec sleep_time = {
             .tv_sec = stream->config.poll_interval_ms / 1000,
             .tv_nsec = (stream->config.poll_interval_ms % 1000) * 1000000
         };
         nanosleep(&sleep_time, NULL);
     }
     
     return NULL;
 }
 
 /**
  * @brief Get current time in milliseconds
  */
 static uint64_t get_current_time_ms(void) {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
 }
 
 /**
  * @brief Close a communication stream
  */
 polycall_core_error_t polycall_comm_close_stream(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream
 ) {
     if (!ctx || !validate_stream(stream)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state == POLYCALL_COMM_STATE_CLOSED) {
         pthread_mutex_unlock(&stream->mutex);
         return POLYCALL_CORE_SUCCESS; // Already closed
     }
     
     // Set closing state
     update_stream_state(stream, POLYCALL_COMM_STATE_CLOSING);
     
     // Stop polling thread
     if (stream->polling_active) {
         stream->polling_active = false;
         pthread_mutex_unlock(&stream->mutex);
         pthread_join(stream->polling_thread, NULL);
         pthread_mutex_lock(&stream->mutex);
     }
     
     // Notify observers of completion
     for (size_t i = 0; i < stream->observer_count; i++) {
         if (stream->observers[i] && stream->observers[i]->is_active && 
             stream->observers[i]->config.on_complete) {
             stream->observers[i]->config.on_complete(
                 stream->observers[i], 
                 stream->observers[i]->config.user_data
             );
         }
     }
     
     // Update state
     update_stream_state(stream, POLYCALL_COMM_STATE_CLOSED);
     
     pthread_mutex_unlock(&stream->mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Send data through a communication stream
  */
 polycall_core_error_t polycall_comm_send(
     polycall_core_context_t* ctx,
     polycall_comm_stream_t* stream,
     const void* data,
     size_t size,
     uint32_t flags
 ) {
     if (!ctx || !validate_stream(stream) || !data || size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&stream->mutex);
     
     // Check state
     if (stream->state != POLYCALL_COMM_STATE_OPEN && 
         stream->state != POLYCALL_COMM_STATE_ACTIVE) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Cannot send data in current state: %d", stream->state);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check size limits
     if (size > stream->config.max_message_size) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Message exceeds maximum size: %zu > %u", 
                          size, stream->config.max_message_size);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure buffer has enough space
     if (circular_buffer_available(&stream->send_buffer) < size) {
         pthread_mutex_unlock(&stream->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Send buffer full, available: %zu, required: %zu",
                          circular_buffer_available(&stream->send_buffer), size);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Write data to buffer
     size_t written = circular_buffer_write(&stream->send_buffer, data, size);
     
     // Update stats
     stream->stats.bytes_sent += written;
     stream->stats.messages_sent++;
     stream->last_activity_time = get_current_time_ms();
     
     // Update stream state if this is the first data
     if (stream->state == POLYCALL_COMM_STATE_OPEN) {
         update_stream_state(stream, POLYCALL_COMM_STATE_ACTIVE);
     }
     
     // Check buffer threshold
     if (stream->config.callbacks.on_buffer_threshold && 
         circular_buffer_used(&stream->send_buffer) >= stream->buffer_threshold) {
         stream->config.callbacks.on_buffer_threshold(
             stream, 
             circular_buffer_used(&stream->send_buffer), 
             stream->buffer_threshold,
             stream->config.user_data
         );
     }
     
     // Notify data sent callback
     if (stream->config.callbacks.on_data_sent) {
         stream->config.callbacks.on_data_sent(stream, written, stream->config.user_data);
     }