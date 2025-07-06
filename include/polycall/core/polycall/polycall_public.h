/**
 * @file polycall_public.h
 * @brief Public API for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This header defines the public API for the LibPolyCall library,
 * providing a clean interface for applications utilizing the library.
 */

#ifndef POLYCALL_POLYCALL_POLYCALL_PUBLIC_H_H
#define POLYCALL_POLYCALL_POLYCALL_PUBLIC_H_H

#include "polycall/core/polycall/polycall_config.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declaration for polycall_context_t (opaque handle)
typedef struct polycall_context polycall_context_t;

// Error code type (should match the definition in polycall_error.h)
typedef int polycall_error_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Protocol types
 */
typedef enum {
  POLYCALL_PROTOCOL_NONE = 0,
  POLYCALL_PROTOCOL_HTTP,      /**< HTTP/HTTPS protocol */
  POLYCALL_PROTOCOL_WEBSOCKET, /**< WebSocket protocol */
  POLYCALL_PROTOCOL_MQTT,      /**< MQTT protocol */
  POLYCALL_PROTOCOL_GRPC,      /**< gRPC protocol */
  POLYCALL_PROTOCOL_CUSTOM     /**< Custom protocol */
} polycall_protocol_type_t;

/**
 * @brief Security levels
 */
typedef enum {
  POLYCALL_SECURITY_NONE = 0,
  POLYCALL_SECURITY_BASIC,  /**< Basic security */
  POLYCALL_SECURITY_MEDIUM, /**< Medium security */
  POLYCALL_SECURITY_HIGH    /**< High security */
} polycall_security_level_t;

/**
 * @brief Transport types
 */
typedef enum {
  POLYCALL_TRANSPORT_TCP = 0, /**< TCP transport */
  POLYCALL_TRANSPORT_UDP,     /**< UDP transport */
  POLYCALL_TRANSPORT_UNIX,    /**< Unix domain socket */
  POLYCALL_TRANSPORT_MEMORY   /**< In-memory transport */
} polycall_transport_type_t;
/**
 * @brief API flags
 */
typedef enum {
  POLYCALL_FLAG_NONE = 0,
  POLYCALL_FLAG_SECURE = (1 << 0), /**< Enable security features */
  POLYCALL_FLAG_DEBUG = (1 << 1),  /**< Enable debug mode */
  POLYCALL_FLAG_ASYNC = (1 << 2),  /**< Enable asynchronous operations */
  POLYCALL_FLAG_MICRO_ENABLED =
      (1 << 3) /**< Enable microservice infrastructure */
} polycall_flags_t;

/**
 * @brief Message types
 */
typedef enum {
  POLYCALL_MESSAGE_COMMAND = 0, /**< Command message */
  POLYCALL_MESSAGE_RESPONSE,    /**< Response message */
  POLYCALL_MESSAGE_EVENT,       /**< Event message */
  POLYCALL_MESSAGE_DATA,        /**< Data message */
  POLYCALL_MESSAGE_ERROR        /**< Error message */
} polycall_message_type_t;

/**
 * @brief Connection information
 */
typedef struct {
  const char *host;        /**< Host address */
  uint16_t port;           /**< Port number */
  uint32_t timeout_ms;     /**< Connection timeout */
  const char *credentials; /**< Authentication credentials */
  bool use_tls;            /**< Whether to use TLS */
} polycall_connection_info_t;

/**
 * @brief Version information
 */
typedef struct {
  uint32_t major;     /**< Major version */
  uint32_t minor;     /**< Minor version */
  uint32_t patch;     /**< Patch version */
  const char *string; /**< Version string */
} polycall_version_t;

/**
 * @brief Session handle (opaque)
 */
typedef struct polycall_session polycall_session_t;

/**
 * @brief Message handle (opaque)
 */
typedef struct polycall_message polycall_message_t;

/**
 * @brief Configuration
 */
typedef struct {
  polycall_flags_t flags;  /**< Configuration flags */
  size_t memory_pool_size; /**< Memory pool size in bytes */
  const char *config_file; /**< Configuration file path */
  void *user_data;         /**< User data */
  void (*error_callback)(int level, const char *message,
                         void *user_data); /**< Error callback */
  void (*log_callback)(int level, const char *message,
                       void *user_data); /**< Log callback */
} polycall_config_t;

/**
 * @brief Initialize the LibPolyCall library
 *
 * @param ctx Pointer to receive context
 * @param config Configuration (NULL for defaults)
 * @return Error code
 */
polycall_error_t polycall_init(polycall_context_t **ctx,
                               const polycall_config_t *config);

/**
 * @brief Clean up and release LibPolyCall resources
 *
 * @param ctx Context to clean up
 */
void polycall_cleanup(polycall_context_t *ctx);

/**
 * @brief Get the LibPolyCall version
 *
 * @return Version information
 */
polycall_version_t polycall_get_version(void);

/**
 * @brief Get the last error message
 *
 * @param ctx Context
 * @return Error message string
 */
const char *polycall_get_error_message(polycall_context_t *ctx);

/**
 * @brief Get the last error code
 *
 * @param ctx Context
 * @return Error code
 */
polycall_error_t polycall_get_error_code(polycall_context_t *ctx);

/**
 * @brief Connect to a server
 *
 * @param ctx Context
 * @param session Pointer to receive session handle
 * @param info Connection information
 * @return Error code
 */
polycall_error_t polycall_connect(polycall_context_t *ctx,
                                  polycall_session_t **session,
                                  const polycall_connection_info_t *info);

/**
 * @brief Disconnect from a server
 *
 * @param ctx Context
 * @param session Session to disconnect
 * @return Error code
 */
polycall_error_t polycall_disconnect(polycall_context_t *ctx,
                                     polycall_session_t *session);

/**
 * @brief Create a message
 *
 * @param ctx Context
 * @param message Pointer to receive message handle
 * @param type Message type
 * @return Error code
 */
polycall_error_t polycall_create_message(polycall_context_t *ctx,
                                         polycall_message_t **message,
                                         polycall_message_type_t type);

/**
 * @brief Destroy a message
 *
 * @param ctx Context
 * @param message Message to destroy
 * @return Error code
 */
polycall_error_t polycall_destroy_message(polycall_context_t *ctx,
                                          polycall_message_t *message);

/**
 * @brief Set message path
 *
 * @param ctx Context
 * @param message Message to modify
 * @param path Path to set
 * @return Error code
 */
polycall_error_t polycall_message_set_path(polycall_context_t *ctx,
                                           polycall_message_t *message,
                                           const char *path);

/**
 * @brief Set message data
 *
 * @param ctx Context
 * @param message Message to modify
 * @param data Data to set
 * @param size Size of data
 * @return Error code
 */
polycall_error_t polycall_message_set_data(polycall_context_t *ctx,
                                           polycall_message_t *message,
                                           const void *data, size_t size);

/**
 * @brief Set message string
 *
 * @param ctx Context
 * @param message Message to modify
 * @param string String to set
 * @return Error code
 */
polycall_error_t polycall_message_set_string(polycall_context_t *ctx,
                                             polycall_message_t *message,
                                             const char *string);

/**
 * @brief Set message JSON
 *
 * @param ctx Context
 * @param message Message to modify
 * @param json JSON to set
 * @return Error code
 */
polycall_error_t polycall_message_set_json(polycall_context_t *ctx,
                                           polycall_message_t *message,
                                           const char *json);

/**
 * @brief Send a message
 *
 * @param ctx Context
 * @param session Session to use
 * @param message Message to send
 * @param response Pointer to receive response (NULL to ignore)
 * @return Error code
 */
polycall_error_t polycall_send_message(polycall_context_t *ctx,
                                       polycall_session_t *session,
                                       polycall_message_t *message,
                                       polycall_message_t **response);

/**
 * @brief Get message path
 *
 * @param ctx Context
 * @param message Message
 * @return Path
 */
const char *polycall_message_get_path(polycall_context_t *ctx,
                                      polycall_message_t *message);

/**
 * @brief Get message data
 *
 * @param ctx Context
 * @param message Message
 * @param size Pointer to receive size
 * @return Data
 */
const void *polycall_message_get_data(polycall_context_t *ctx,
                                      polycall_message_t *message,
                                      size_t *size);

/**
 * @brief Get message string
 *
 * @param ctx Context
 * @param message Message
 * @return String
 */
const char *polycall_message_get_string(polycall_context_t *ctx,
                                        polycall_message_t *message);

/**
 * @brief Get message JSON
 *
 * @param ctx Context
 * @param message Message
 * @return JSON
 */
const char *polycall_message_get_json(polycall_context_t *ctx,
                                      polycall_message_t *message);

/**
 * @brief Create default configuration
 *
 * @return Default configuration
 */
polycall_config_t polycall_create_default_config(void);

/**
 * @brief Load configuration from file
 *
 * @param ctx Context
 * @param config Configuration to update
 * @param filename File to load
 * @return Error code
 */
polycall_error_t polycall_load_config_file(polycall_context_t *ctx,
                                           polycall_config_t *config,
                                           const char *filename);

/**
 * @brief Load configuration
 *
 * @param filename File to load
 * @return Configuration
 */
polycall_config_t polycall_load_config(const char *filename);

/**
 * @brief Set user data
 *
 * @param ctx Context
 * @param user_data User data
 * @return Error code
 */
polycall_error_t polycall_set_user_data(polycall_context_t *ctx,
                                        void *user_data);

/**
 * @brief Get user data
 *
 * @param ctx Context
 * @return User data
 */
void *polycall_get_user_data(polycall_context_t *ctx);

/**
 * @brief Register callback
 *
 * @param ctx Context
 * @param event_type Event type
 * @param callback Callback function
 * @param user_data User data
 * @return Error code
 */
polycall_error_t
polycall_register_callback(polycall_context_t *ctx,
                           void (*callback)(polycall_context_t *, void *),
                           void *user_data);

/**
 * @brief Unregister callback
 *
 * @param ctx Context
 * @param event_type Event type
 * @param callback Callback function
 * @return Error code
 */
polycall_error_t
polycall_unregister_callback(polycall_context_t *ctx, uint32_t event_type,
                             void (*callback)(polycall_context_t *, void *));

/**
 * @brief Set log callback
 *
 * @param ctx Context
 * @param callback Callback function
 * @param user_data User data
 * @return Error code
 */
polycall_error_t polycall_set_log_callback(polycall_context_t *ctx,
                                           void (*callback)(int level,
                                                            const char *message,
                                                            void *user_data),
                                           void *user_data);

/**
 * @brief Process messages
 *
 * @param ctx Context
 * @param session Session
 * @param timeout_ms Timeout in milliseconds
 * @return Error code
 */
polycall_error_t polycall_process_messages(polycall_context_t *ctx,
                                           polycall_session_t *session,
                                           uint32_t timeout_ms);

/**
 * @brief Initialize FFI subsystem
 *
 * @param ctx Context
 * @param ffi_config FFI configuration
 * @return Error code
 */
polycall_error_t polycall_init_ffi(polycall_context_t *ctx,
                                   const void *ffi_config);

/**
 * @brief Initialize protocol subsystem
 *
 * @param ctx Context
 * @param protocol_config Protocol configuration
 * @return Error code
 */
polycall_error_t polycall_init_protocol(polycall_context_t *ctx,
                                        const void *protocol_config);

/**
 * @brief Initialize all subsystems
 *
 * @param ctx Pointer to receive context
 * @param config Configuration
 * @return Error code
 */
polycall_error_t polycall_init_all(polycall_context_t **ctx,
                                   const polycall_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_PUBLIC_H_H */