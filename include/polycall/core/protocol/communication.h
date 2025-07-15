/**
 * @file communication.h
 * @brief Duplex communication stream for LibPolyCall protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the bidirectional communication interface for the
 * LibPolyCall protocol, enabling efficient duplex streams for data transfer,
 * observation, and polling between endpoints.
 */

#ifndef POLYCALL_PROTOCOL_COMMUNICATION_H_H
#define POLYCALL_PROTOCOL_COMMUNICATION_H_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/protocol/polycall_protocol_context.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Communication stream types
 */
typedef enum {
  POLYCALL_COMM_STREAM_STANDARD = 0, /**< Standard bidirectional stream */
  POLYCALL_COMM_STREAM_SECURE,       /**< Encrypted bidirectional stream */
  POLYCALL_COMM_STREAM_BULK,         /**< High-volume data stream */
  POLYCALL_COMM_STREAM_REACTIVE,     /**< Event-driven stream */
  POLYCALL_COMM_STREAM_USER = 0x1000 /**< Start of user-defined stream types */
} polycall_comm_stream_type_t;

/**
 * @brief Communication stream states
 */
typedef enum {
  POLYCALL_COMM_STATE_INIT = 0, /**< Stream initialized but not open */
  POLYCALL_COMM_STATE_OPEN,     /**< Stream is open and ready for data */
  POLYCALL_COMM_STATE_ACTIVE,   /**< Stream is actively transferring data */
  POLYCALL_COMM_STATE_PAUSED,   /**< Stream is paused */
  POLYCALL_COMM_STATE_CLOSING,  /**< Stream is in process of closing */
  POLYCALL_COMM_STATE_CLOSED,   /**< Stream is closed */
  POLYCALL_COMM_STATE_ERROR     /**< Stream encountered an error */
} polycall_comm_state_t;

/**
 * @brief Communication stream flags
 */
typedef enum {
  POLYCALL_COMM_FLAG_NONE = 0,
  POLYCALL_COMM_FLAG_NONBLOCKING = (1 << 0),   /**< Non-blocking operations */
  POLYCALL_COMM_FLAG_BUFFERED = (1 << 1),      /**< Buffered I/O */
  POLYCALL_COMM_FLAG_COMPRESSED = (1 << 2),    /**< Data compression enabled */
  POLYCALL_COMM_FLAG_ENCRYPTED = (1 << 3),     /**< Data encryption enabled */
  POLYCALL_COMM_FLAG_PRIORITY = (1 << 4),      /**< High-priority stream */
  POLYCALL_COMM_FLAG_OBSERVABLE = (1 << 5),    /**< Stream can be observed */
  POLYCALL_COMM_FLAG_AUTO_RECONNECT = (1 << 6) /**< Auto reconnect on failure */
} polycall_comm_flags_t;

/**
 * @brief Communication stream statistics
 */
typedef struct {
  uint64_t bytes_sent;           /**< Total bytes sent */
  uint64_t bytes_received;       /**< Total bytes received */
  uint64_t messages_sent;        /**< Total messages sent */
  uint64_t messages_received;    /**< Total messages received */
  uint64_t errors;               /**< Error count */
  uint64_t reconnects;           /**< Reconnection count */
  uint64_t last_activity_time;   /**< Last activity timestamp */
  double throughput_send;        /**< Send throughput (bytes/sec) */
  double throughput_receive;     /**< Receive throughput (bytes/sec) */
  uint32_t current_buffer_usage; /**< Current buffer usage in bytes */
  uint32_t max_buffer_usage;     /**< Maximum recorded buffer usage */
} polycall_comm_stats_t;

/**
 * @brief Communication stream context (opaque)
 */
typedef struct polycall_comm_stream polycall_comm_stream_t;

/**
 * @brief Communication observer context (opaque)
 */
typedef struct polycall_comm_observer polycall_comm_observer_t;

/**
 * @brief Communication data event callbacks
 */
typedef struct {
  void (*on_data_received)(polycall_comm_stream_t *stream, const void *data,
                           size_t size, void *user_data);
  void (*on_data_sent)(polycall_comm_stream_t *stream, size_t size,
                       void *user_data);
  void (*on_state_change)(polycall_comm_stream_t *stream,
                          polycall_comm_state_t old_state,
                          polycall_comm_state_t new_state, void *user_data);
  void (*on_error)(polycall_comm_stream_t *stream, int error_code,
                   const char *message, void *user_data);
  void (*on_buffer_threshold)(polycall_comm_stream_t *stream,
                              uint32_t buffer_size, uint32_t threshold,
                              void *user_data);
} polycall_comm_callbacks_t;

/**
 * @brief Communication stream configuration
 */
typedef struct {
  polycall_comm_stream_type_t type;    /**< Stream type */
  polycall_comm_flags_t flags;         /**< Stream flags */
  uint32_t buffer_size;                /**< Buffer size in bytes */
  uint32_t max_message_size;           /**< Maximum message size */
  uint32_t poll_interval_ms;           /**< Polling interval in milliseconds */
  uint32_t idle_timeout_ms;            /**< Idle timeout in milliseconds */
  uint32_t reconnect_timeout_ms;       /**< Reconnect timeout (if enabled) */
  polycall_comm_callbacks_t callbacks; /**< Event callbacks */
  void *user_data;                     /**< User-defined data */
} polycall_comm_config_t;

/**
 * @brief Observer configuration
 */
typedef struct {
  void (*on_next)(polycall_comm_observer_t *observer, const void *data,
                  size_t size, void *user_data);
  void (*on_error)(polycall_comm_observer_t *observer, int error_code,
                   const char *message, void *user_data);
  void (*on_complete)(polycall_comm_observer_t *observer, void *user_data);
  void *user_data; /**< User-defined data */
} polycall_comm_observer_config_t;

/**
 * @brief Create a communication stream
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param stream Pointer to receive created stream
 * @param config Stream configuration
 * @return Error code
 */
polycall_core_error_t polycall_comm_create_stream(
    polycall_core_context_t *ctx, polycall_protocol_context_t *proto_ctx,
    polycall_comm_stream_t **stream, const polycall_comm_config_t *config);

/**
 * @brief Destroy a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to destroy
 */
void polycall_comm_destroy_stream(polycall_core_context_t *ctx,
                                  polycall_comm_stream_t *stream);

/**
 * @brief Open a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to open
 * @return Error code
 */
polycall_core_error_t polycall_comm_open_stream(polycall_core_context_t *ctx,
                                                polycall_comm_stream_t *stream);

/**
 * @brief Close a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to close
 * @return Error code
 */
polycall_core_error_t
polycall_comm_close_stream(polycall_core_context_t *ctx,
                           polycall_comm_stream_t *stream);

/**
 * @brief Send data through a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to send through
 * @param data Data to send
 * @param size Data size in bytes
 * @param flags Send flags
 * @return Error code
 */
polycall_core_error_t polycall_comm_send(polycall_core_context_t *ctx,
                                         polycall_comm_stream_t *stream,
                                         const void *data, size_t size,
                                         uint32_t flags);

/**
 * @brief Receive data from a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to receive from
 * @param buffer Buffer to store received data
 * @param size Buffer size
 * @param bytes_read Pointer to receive number of bytes read
 * @param flags Receive flags
 * @return Error code
 */
polycall_core_error_t polycall_comm_receive(polycall_core_context_t *ctx,
                                            polycall_comm_stream_t *stream,
                                            void *buffer, size_t size,
                                            size_t *bytes_read, uint32_t flags);

/**
 * @brief Poll a communication stream for data
 *
 * @param ctx Core context
 * @param stream Stream to poll
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
 * @return true if data is available, false otherwise
 */
bool polycall_comm_poll(polycall_core_context_t *ctx,
                        polycall_comm_stream_t *stream, uint32_t timeout_ms);

/**
 * @brief Create an observer for a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to observe
 * @param observer Pointer to receive created observer
 * @param config Observer configuration
 * @return Error code
 */
polycall_core_error_t
polycall_comm_create_observer(polycall_core_context_t *ctx,
                              polycall_comm_stream_t *stream,
                              polycall_comm_observer_t **observer,
                              const polycall_comm_observer_config_t *config);

/**
 * @brief Destroy an observer
 *
 * @param ctx Core context
 * @param observer Observer to destroy
 */
void polycall_comm_destroy_observer(polycall_core_context_t *ctx,
                                    polycall_comm_observer_t *observer);

/**
 * @brief Pause a communication stream
 *
 * @param ctx Core context
 * @param stream Stream to pause
 * @return Error code
 */
polycall_core_error_t
polycall_comm_pause_stream(polycall_core_context_t *ctx,
                           polycall_comm_stream_t *stream);

/**
 * @brief Resume a paused communication stream
 *
 * @param ctx Core context
 * @param stream Stream to resume
 * @return Error code
 */
polycall_core_error_t
polycall_comm_resume_stream(polycall_core_context_t *ctx,
                            polycall_comm_stream_t *stream);

/**
 * @brief Get communication stream statistics
 *
 * @param ctx Core context
 * @param stream Stream to query
 * @param stats Pointer to receive statistics
 * @return Error code
 */
polycall_core_error_t polycall_comm_get_stats(polycall_core_context_t *ctx,
                                              polycall_comm_stream_t *stream,
                                              polycall_comm_stats_t *stats);

/**
 * @brief Get communication stream state
 *
 * @param ctx Core context
 * @param stream Stream to query
 * @return Stream state
 */
polycall_comm_state_t polycall_comm_get_state(polycall_core_context_t *ctx,
                                              polycall_comm_stream_t *stream);

/**
 * @brief Set buffer threshold for notifications
 *
 * @param ctx Core context
 * @param stream Stream to modify
 * @param threshold Threshold in bytes
 * @return Error code
 */
polycall_core_error_t
polycall_comm_set_buffer_threshold(polycall_core_context_t *ctx,
                                   polycall_comm_stream_t *stream,
                                   uint32_t threshold);

/**
 * @brief Flush a communication stream buffer
 *
 * @param ctx Core context
 * @param stream Stream to flush
 * @return Error code
 */
polycall_core_error_t polycall_comm_flush(polycall_core_context_t *ctx,
                                          polycall_comm_stream_t *stream);

/**
 * @brief Register with protocol for bidirectional data handling
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param stream Stream to register
 * @return Error code
 */
polycall_core_error_t
polycall_comm_register_with_protocol(polycall_core_context_t *ctx,
                                     polycall_protocol_context_t *proto_ctx,
                                     polycall_comm_stream_t *stream);

/**
 * @brief Create a default communication configuration
 *
 * @return Default configuration
 */
polycall_comm_config_t polycall_comm_create_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_PROTOCOL_COMMUNICATION_H_H */