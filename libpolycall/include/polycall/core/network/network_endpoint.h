/**
 * @file network_server.h
 * @brief Network server interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the server-side networking interface for LibPolyCall,
 * enabling listening for and accepting connections from remote clients
 * with protocol-aware communication.
 */

 #ifndef POLYCALL_NETWORK_NETWORK_ENDPOINT_H_H
 #define POLYCALL_NETWORK_NETWORK_ENDPOINT_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/protocol/message.h"
 #include "network_endpoint.h"
 #include "polycall/core/network/network_packet.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
    #include <stdint.h>
    #include <stdbool.h>
    
 #ifdef __cplusplus
 extern "C" {
 #endif
 
// Forward declarations
typedef struct polycall_network_server polycall_network_server_t;
typedef struct polycall_network_server_config polycall_network_server_config_t;
typedef struct polycall_endpoint polycall_endpoint_t;

 
 // Platform-specific socket includes
 #ifdef _WIN32
 #include <winsock2.h>
 #include <ws2tcpip.h>
 typedef SOCKET socket_handle_t;
 #else
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <fcntl.h>
 #include <unistd.h>
 typedef int socket_handle_t;
 #endif
 /**
 * @brief Network context structure
 */
/**
 * @brief Network configuration options
 */
typedef enum polycall_network_option {
    POLYCALL_NETWORK_OPTION_TIMEOUT,         // Connection timeout
    POLYCALL_NETWORK_OPTION_KEEPALIVE,       // Keep-alive settings
    POLYCALL_NETWORK_OPTION_BUFFER_SIZE,     // Socket buffer size
    POLYCALL_NETWORK_OPTION_TCP_NODELAY,     // TCP no delay flag
    POLYCALL_NETWORK_OPTION_MAX_CONNECTIONS  // Maximum connections
} polycall_network_option_t;

/**
 * @brief Network statistics structure
 */
typedef struct polycall_network_stats {
    uint64_t bytes_sent;                              // Total bytes sent
    uint64_t bytes_received;                          // Total bytes received
    uint32_t packets_sent;                            // Total packets sent
    uint32_t packets_received;                        // Total packets received
    uint32_t active_connections;                      // Current active connections
    uint32_t total_connections;                       // Total connections handled
    uint32_t errors;                                  // Total errors encountered
    time_t uptime;                                    // Server uptime
} polycall_network_stats_t;

typedef struct polycall_network_context {
    polycall_core_context_t* core_ctx;                // Core context
    polycall_network_config_t config;                 // Configuration
    polycall_network_flags_t flags;                   // Module flags
    polycall_network_stats_t stats;                   // Statistics
    time_t start_time;                               // Module start time
    void* tls_context;                               // TLS context

    // Thread management
    pthread_mutex_t thread_mutex;                     // Thread mutex
    pthread_cond_t thread_cond;                       // Thread condition
    worker_thread_t* worker_threads;                  // Worker threads
    uint32_t worker_thread_count;                     // Number of worker threads

    // Client management  
    pthread_mutex_t client_mutex;                     // Client mutex
    client_entry_t* clients;                         // Client list
    uint32_t client_count;                           // Number of clients

    // Server management
    pthread_mutex_t server_mutex;                     // Server mutex 
    server_entry_t* servers;                         // Server list
    uint32_t server_count;                           // Number of servers

    // Endpoint management
    pthread_mutex_t endpoint_mutex;                   // Endpoint mutex
    endpoint_entry_t* endpoints;                     // Endpoint list 
    uint32_t endpoint_count;                         // Number of endpoints

    // Event management
    pthread_mutex_t event_mutex;                      // Event mutex
    event_handler_t event_handlers[POLYCALL_NETWORK_EVENT_COUNT][MAX_EVENT_HANDLERS];
    uint32_t event_handler_counts[POLYCALL_NETWORK_EVENT_COUNT];
    
    // Statistics mutex
    pthread_mutex_t stats_mutex;                      // Statistics mutex
} polycall_network_context_t;

/**
 * @brief Client registry entry
 */
typedef struct client_entry {
    polycall_network_client_t* client;              // Client instance
    struct client_entry* next;                      // Next entry
} client_entry_t;

/**
 * @brief Server registry entry  
 */
typedef struct server_entry {
    polycall_network_server_t* server;              // Server instance
    struct server_entry* next;                      // Next entry
} server_entry_t;

/**
 * @brief Endpoint registry entry
 */
typedef struct endpoint_entry {
    polycall_endpoint_t* endpoint;                  // Endpoint instance
    struct endpoint_entry* next;                    // Next entry
} endpoint_entry_t;

/**
 * @brief Event handler entry
 */
typedef struct event_handler {
    polycall_network_event_t event_type;            // Event type
    void (*handler)(                                // Handler function
        polycall_network_context_t* ctx,
        polycall_endpoint_t* endpoint, 
        void* event_data,
        void* user_data
    );
    void* user_data;                               // User data
} event_handler_t;

/**
 * @brief Worker thread 
 */
typedef struct worker_thread {
    pthread_t thread_id;                           // Thread ID
    bool active;                                   // Thread active flag
    void* thread_data;                            // Thread data
} worker_thread_t;
 // Event callback structure
 typedef struct {
     polycall_network_event_t event_type;
     void (*callback)(polycall_endpoint_t* endpoint, void* event_data, void* user_data);
     void* user_data;
 } endpoint_callback_t;
 
 // Maximum number of event callbacks per endpoint
 #define POLYCALL_NETWORK_NETWORK_ENDPOINT_H_H
 
 // Network endpoint structure
 struct polycall_endpoint {
     polycall_endpoint_type_t type;
     polycall_endpoint_state_t state;
     char address[256];
     uint16_t port;
     char local_address[256];
     uint16_t local_port;
     bool secure;
     time_t connected_time;
     uint64_t bytes_sent;
     uint64_t bytes_received;
     uint64_t latency_ms;
     char peer_id[64];
     uint32_t timeout_ms;
     socket_handle_t socket;
     void* tls_context;
     void* user_data;
     endpoint_callback_t callbacks[MAX_CALLBACKS];
     size_t callback_count;
     polycall_network_stats_t stats;
 };
 
 /**
  * @brief Message handler callback type
  */
 typedef polycall_core_error_t (*polycall_message_handler_t)(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_endpoint_t* endpoint,
     polycall_message_t* message,
     polycall_message_t** response,
     void* user_data
 );
 
 /**
  * @brief Network server configuration
  */
 struct polycall_network_server_config {
     uint16_t port;                       // Listening port
     const char* bind_address;            // Bind address (NULL for any)
     uint32_t backlog;                    // Connection backlog
     uint32_t max_connections;            // Maximum simultaneous connections
     uint32_t connection_timeout_ms;      // Connection timeout in milliseconds
     uint32_t operation_timeout_ms;       // Operation timeout in milliseconds
     uint32_t idle_timeout_ms;            // Connection idle timeout
     bool enable_tls;                     // Enable TLS encryption
     const char* tls_cert_file;           // TLS certificate file path
     const char* tls_key_file;            // TLS key file path
     const char* tls_ca_file;             // TLS CA certificate file path
     uint32_t max_message_size;           // Maximum message size
     uint32_t io_thread_count;            // Number of I/O threads (0 for auto)
     uint32_t worker_thread_count;        // Number of worker threads (0 for auto)
     bool enable_protocol_dispatch;       // Enable automatic protocol message dispatching
     polycall_message_handler_t message_handler; // Protocol message handler
     void* user_data;                     // User data
     void (*connection_callback)(         // Connection state change callback
         polycall_network_server_t* server,
         polycall_endpoint_t* endpoint,
         bool connected,
         void* user_data
     );
     void (*error_callback)(              // Error callback
         polycall_network_server_t* server,
         polycall_core_error_t error,
         const char* message,
         void* user_data
     );
 };
 
 /**
  * @brief Create a network server
  * 
  * @param ctx Core context
  * @param proto_ctx Protocol context
  * @param server Pointer to receive created server
  * @param config Server configuration
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_create(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_server_t** server,
     const polycall_network_server_config_t* config
 );
 
 /**
  * @brief Start the server
  * 
  * @param ctx Core context
  * @param server Network server
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_start(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server
 );
 
 /**
  * @brief Stop the server
  * 
  * @param ctx Core context
  * @param server Network server
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_stop(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server
 );
 
 /**
  * @brief Accept a new connection
  * 
  * Manual connection acceptance when not using automatic accept.
  * 
  * @param ctx Core context
  * @param server Network server
  * @param endpoint Pointer to receive accepted endpoint
  * @param timeout_ms Accept timeout in milliseconds (0 for non-blocking)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_accept(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_endpoint_t** endpoint,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Disconnect a client
  * 
  * @param ctx Core context
  * @param server Network server
  * @param endpoint Endpoint to disconnect
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_disconnect(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_endpoint_t* endpoint
 );
 
 /**
  * @brief Send a packet to a client
  * 
  * @param ctx Core context
  * @param server Network server
  * @param endpoint Target endpoint
  * @param packet Packet to send
  * @param timeout_ms Send timeout in milliseconds (0 for default)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_send(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_endpoint_t* endpoint,
     polycall_network_packet_t* packet,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Receive a packet from a client
  * 
  * @param ctx Core context
  * @param server Network server
  * @param endpoint Source endpoint
  * @param packet Pointer to receive packet
  * @param timeout_ms Receive timeout in milliseconds (0 for default)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_receive(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_endpoint_t* endpoint,
     polycall_network_packet_t** packet,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Send a protocol message to a client
  * 
  * @param ctx Core context
  * @param server Network server
  * @param proto_ctx Protocol context
  * @param endpoint Target endpoint
  * @param message Protocol message to send
  * @param timeout_ms Send timeout in milliseconds (0 for default)
  * @param response Pointer to receive response message (NULL if no response expected)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_send_message(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_protocol_context_t* proto_ctx,
     polycall_endpoint_t* endpoint,
     polycall_message_t* message,
     uint32_t timeout_ms,
     polycall_message_t** response
 );
 
 /**
  * @brief Broadcast a packet to all connected clients
  * 
  * @param ctx Core context
  * @param server Network server
  * @param packet Packet to broadcast
  * @param timeout_ms Send timeout in milliseconds (0 for default)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_broadcast(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_network_packet_t* packet,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Register a message handler for specific message types
  * 
  * @param ctx Core context
  * @param server Network server
  * @param message_type Message type to handle
  * @param handler Message handler callback
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_register_handler(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     uint32_t message_type,
     polycall_message_handler_t handler,
     void* user_data
 );
 
 /**
  * @brief Get all connected endpoints
  * 
  * @param ctx Core context
  * @param server Network server
  * @param endpoints Array to receive endpoints
  * @param count Pointer to array size (in/out)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_get_endpoints(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_endpoint_t** endpoints,
     size_t* count
 );
 
 /**
  * @brief Get server statistics
  * 
  * @param ctx Core context
  * @param server Network server
  * @param stats Pointer to receive statistics
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_get_stats(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_network_stats_t* stats
 );
 
 /**
  * @brief Set server option
  * 
  * @param ctx Core context
  * @param server Network server
  * @param option Option to set
  * @param value Option value
  * @param size Option value size
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_set_option(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_network_option_t option,
     const void* value,
     size_t size
 );
 
 /**
  * @brief Get server option
  * 
  * @param ctx Core context
  * @param server Network server
  * @param option Option to get
  * @param value Pointer to receive option value
  * @param size Pointer to option value size (in/out)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_get_option(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_network_option_t option,
     void* value,
     size_t* size
 );
 
 /**
  * @brief Set server event callback
  * 
  * @param ctx Core context
  * @param server Network server
  * @param event_type Event type
  * @param callback Event callback
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_set_event_callback(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     polycall_network_event_t event_type,
     void (*callback)(polycall_network_server_t* server, polycall_endpoint_t* endpoint, void* event_data, void* user_data),
     void* user_data
 );
 
 /**
  * @brief Process pending events
  * 
  * @param ctx Core context
  * @param server Network server
  * @param timeout_ms Processing timeout in milliseconds (0 for non-blocking)
  * @return Error code
  */
 polycall_core_error_t polycall_network_server_process_events(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Clean up server resources
  * 
  * @param ctx Core context
  * @param server Network server
  */
 void polycall_network_server_cleanup(
     polycall_core_context_t* ctx,
     polycall_network_server_t* server
 );
 
 /**
  * @brief Create default server configuration
  * 
  * @return Default configuration
  */
 polycall_network_server_config_t polycall_network_server_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_NETWORK_NETWORK_ENDPOINT_H_H */