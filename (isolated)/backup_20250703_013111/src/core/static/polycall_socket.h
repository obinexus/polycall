/**
 * @file polycall_socket.h
 * @brief WebSocket Interface Definitions for LibPolyCall
 * @author Implementation based on existing LibPolyCall architecture
 */

#ifndef POLYCALL_SOCKET_H
#define POLYCALL_SOCKET_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/telemetry/polycall_telemetry.h"
#include "polycall/core/auth/polycall_auth.h"
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WebSocket data types
 */
typedef enum {
    POLYCALL_SOCKET_DATA_TEXT = 1,    // Text data (UTF-8)
    POLYCALL_SOCKET_DATA_BINARY = 2,  // Binary data
} polycall_socket_data_type_t;

/**
 * @brief WebSocket message structure
 */
typedef struct {
    polycall_socket_data_type_t data_type;  // Data type
    uint8_t* data;                          // Message data
    size_t data_size;                       // Data size
    bool is_final;                          // Final fragment flag
} polycall_socket_message_t;

/**
 * @brief WebSocket connection options
 */
typedef struct {
    uint32_t timeout_ms;       // Connection timeout (milliseconds)
    bool use_tls;              // Use secure WebSocket (WSS)
    bool auto_reconnect;       // Auto-reconnect on disconnection
    uint32_t reconnect_max_attempts;    // Maximum reconnection attempts
    uint32_t reconnect_base_delay_ms;   // Base delay between reconnection attempts
    const char* protocols;     // Comma-separated list of supported protocols
    const char* auth_token;    // Authentication token (optional)
} polycall_socket_connect_options_t;

// Forward declarations
typedef struct polycall_socket_context polycall_socket_context_t;
typedef struct polycall_socket_server polycall_socket_server_t;
typedef struct polycall_socket_connection polycall_socket_connection_t;
typedef struct polycall_socket_worker polycall_socket_worker_t;

/**
 * @brief WebSocket configuration
 */
typedef struct {
    uint32_t max_connections;         // Maximum number of connections
    uint32_t connection_timeout_ms;   // Connection timeout (milliseconds)
    bool use_tls;                     // Use secure WebSocket (WSS)
    uint32_t ping_interval_ms;        // Ping interval (milliseconds)
    uint32_t max_message_size;        // Maximum message size (bytes)
    uint32_t worker_threads;          // Number of worker threads
    bool enable_compression;          // Enable WebSocket compression (RFC 7692)
    bool auto_reconnect;              // Auto-reconnect on disconnection
    uint32_t reconnect_max_attempts;  // Maximum reconnection attempts
    uint32_t reconnect_base_delay_ms; // Base delay between reconnection attempts
} polycall_socket_config_t;

/**
 * @brief WebSocket server
 */
struct polycall_socket_server {
    polycall_socket_context_t* socket_ctx;  // Socket context
    char bind_address[128];                 // Bind address
    uint16_t port;                          // Port
    bool is_running;                        // Server running flag
    pthread_mutex_t server_mutex;           // Server mutex
    // ... Additional implementation details
};

/**
 * @brief WebSocket connection
 */
struct polycall_socket_connection {
    polycall_socket_context_t* socket_ctx;  // Socket context
    char url[256];                          // Connection URL
    bool is_connected;                      // Connection status
    uint64_t created_time;                  // Creation timestamp
    polycall_socket_connect_options_t options; // Connection options
    char protocol[64];                      // Negotiated protocol
    uint16_t close_code;                    // Close code (if closed)
    char close_reason[128];                 // Close reason (if closed)
    pthread_mutex_t connection_mutex;       // Connection mutex
    
    // Message handling
    void (*message_handler)(
        polycall_socket_connection_t* connection,
        const polycall_socket_message_t* message,
        void* user_data
    );
    void* handler_user_data;                // User data for handler
    
    // ... Additional implementation details
};

/**
 * @brief WebSocket message handler callback type
 */
typedef void (*polycall_socket_message_handler_t)(
    polycall_socket_connection_t* connection,
    const polycall_socket_message_t* message,
    void* user_data
);

/**
 * @brief WebSocket event callback type
 */
typedef void (*polycall_socket_callback_t)(
    polycall_socket_connection_t* connection,
    void* user_data
);

/**
 * @brief Telemetry event IDs for socket operations
 */
#define POLYCALL_TELEMETRY_EVENT_SOCKET_CONNECT       0x1000
#define POLYCALL_TELEMETRY_EVENT_SOCKET_DISCONNECT    0x1001
#define POLYCALL_TELEMETRY_EVENT_SOCKET_MESSAGE       0x1002
#define POLYCALL_TELEMETRY_EVENT_SOCKET_ERROR         0x1003
#define POLYCALL_TELEMETRY_EVENT_SOCKET_SERVER_CREATE 0x1004
#define POLYCALL_TELEMETRY_EVENT_SOCKET_CLOSE         0x1005
#define POLYCALL_TELEMETRY_EVENT_SOCKET_SEND          0x1006

/**
 * @brief Initialize socket system
 */
polycall_core_error_t polycall_socket_init(
    polycall_core_context_t* core_ctx,
    polycall_socket_context_t** socket_ctx,
    const polycall_socket_config_t* config
);

/**
 * @brief Create WebSocket server
 */
polycall_core_error_t polycall_socket_create_server(
    polycall_socket_context_t* socket_ctx,
    const char* bind_address,
    uint16_t port,
    polycall_socket_server_t** server
);

/**
 * @brief Start WebSocket server
 */
polycall_core_error_t polycall_socket_start_server(
    polycall_socket_server_t* server
);

/**
 * @brief Stop WebSocket server
 */
polycall_core_error_t polycall_socket_stop_server(
    polycall_socket_server_t* server
);

/**
 * @brief Connect to WebSocket server
 */
polycall_core_error_t polycall_socket_connect(
    polycall_socket_context_t* socket_ctx,
    const char* url,
    const polycall_socket_connect_options_t* options,
    polycall_socket_connection_t** connection
);

/**
 * @brief Send message over WebSocket connection
 */
polycall_core_error_t polycall_socket_send(
    polycall_socket_connection_t* connection,
    const void* data,
    size_t data_size,
    polycall_socket_data_type_t data_type
);

/**
 * @brief Send text message over WebSocket connection
 */
static inline polycall_core_error_t polycall_socket_send_text(
    polycall_socket_connection_t* connection,
    const char* text
) {
    return polycall_socket_send(
        connection,
        text,
        strlen(text),
        POLYCALL_SOCKET_DATA_TEXT
    );
}

/**
 * @brief Register message handler
 */
polycall_core_error_t polycall_socket_register_handler(
    polycall_socket_connection_t* connection,
    polycall_socket_message_handler_t handler,
    void* user_data
);

/**
 * @brief Close WebSocket connection
 */
polycall_core_error_t polycall_socket_close(
    polycall_socket_connection_t* connection,
    uint16_t close_code,
    const char* reason
);

/**
 * @brief Create default socket configuration
 */
polycall_socket_config_t polycall_socket_create_default_config(void);

/**
 * @brief Cleanup socket system
 */
void polycall_socket_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_socket_context_t* socket_ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_SOCKET_H */
