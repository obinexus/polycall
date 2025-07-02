/**
 * @file network.h
 * @brief Main network module interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the main interface for LibPolyCall's network module,
 * integrating all networking components and providing a unified API.
 */

 #ifndef POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #include "polycall/core/network/network_internal.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/network/network_client.h"
 #include "polycall/core/network/network_server.h"
 #include "network_endpoint.h"
 #include "network_packet.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <pthread.h>
 
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct polycall_network_context polycall_network_context_t;
 typedef struct polycall_network_config polycall_network_config_t;
 
 // Platform-specific socket includes
 #ifdef _WIN32
 #include <winsock2.h>
 #include <ws2tcpip.h>
 typedef SOCKET socket_handle_t;
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #else
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <fcntl.h>
 #include <unistd.h>
 typedef int socket_handle_t;
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #endif
 
 // Network version string
 #define POLYCALL_NETWORK_NETWORK_H_H
 
 // Maximum number of event handlers per type
 #define POLYCALL_NETWORK_NETWORK_H_H
 
 // Network module flags
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 #define POLYCALL_NETWORK_NETWORK_H_H
 
 // Thread pool worker
 typedef struct worker_thread {
     pthread_t thread_id;
     bool active;
     void* thread_data;
 } worker_thread_t;
 
 // Event handler entry
 typedef struct {
     polycall_network_event_t event_type;
     void (*handler)(
         polycall_network_context_t* ctx,
         polycall_endpoint_t* endpoint,
         void* event_data,
         void* user_data
     );
     void* user_data;
 } event_handler_t;
 
 // Client entry in the registry
 typedef struct client_entry {
     polycall_network_client_t* client;
     struct client_entry* next;
 } client_entry_t;
 
 // Server entry in the registry
 typedef struct server_entry {
     polycall_network_server_t* server;
     struct server_entry* next;
 } server_entry_t;
 
 // Endpoint entry in the registry
 typedef struct endpoint_entry {
     polycall_endpoint_t* endpoint;
     struct endpoint_entry* next;
 } endpoint_entry_t;
 
 // Network context structure
 struct polycall_network_context {
     polycall_core_context_t* core_ctx;
     uint32_t flags;
     polycall_network_config_t config;
     
     // Thread pool
     worker_thread_t* worker_threads;
     uint32_t worker_thread_count;
     pthread_mutex_t thread_mutex;
     pthread_cond_t thread_cond;
     
     // Client registry
     client_entry_t* clients;
     uint32_t client_count;
     pthread_mutex_t client_mutex;
     
     // Server registry
     server_entry_t* servers;
     uint32_t server_count;
     pthread_mutex_t server_mutex;
     
     // Endpoint registry
     endpoint_entry_t* endpoints;
     uint32_t endpoint_count;
     pthread_mutex_t endpoint_mutex;
     
     // Event handlers
     event_handler_t event_handlers[POLYCALL_NETWORK_EVENT_COUNT][MAX_EVENT_HANDLERS];
     uint32_t event_handler_counts[POLYCALL_NETWORK_EVENT_COUNT];
     pthread_mutex_t event_mutex;
     
     // Statistics
     polycall_network_stats_t stats;
     time_t start_time;
     pthread_mutex_t stats_mutex;
     
     // TLS context
     void* tls_context;
 };
 
 // Forward declarations of internal functions
 static void* worker_thread_func(void* arg);
 static void trigger_event(
     polycall_network_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_network_event_t event_type,
     void* event_data
 );
 static polycall_core_error_t initialize_thread_pool(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     uint32_t thread_count
 );
 static void cleanup_thread_pool(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx
 );
 static polycall_core_error_t initialize_tls_context(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx
 );
 static void cleanup_tls_context(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx
 );
 static polycall_core_error_t register_endpoint(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_endpoint_t* endpoint
 );
 static polycall_core_error_t unregister_endpoint(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_endpoint_t* endpoint
 );
 static polycall_core_error_t register_client(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_client_t* client
 );
 static polycall_core_error_t unregister_client(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_client_t* client
 );
 static polycall_core_error_t register_server(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_server_t* server
 );
 static polycall_core_error_t unregister_server(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_server_t* server
 );
 static void update_statistics(
     polycall_network_context_t* network_ctx,
     polycall_network_event_t event_type
 );
 

 /**
  * @brief Network module configuration
  */
 struct polycall_network_config {
     uint32_t thread_pool_size;         // Size of shared thread pool (0 for default)
     uint32_t max_connections;          // Maximum simultaneous connections
     uint32_t max_endpoints;            // Maximum tracked endpoints
     uint32_t connection_timeout_ms;    // Default connection timeout
     uint32_t operation_timeout_ms;     // Default operation timeout
     uint32_t max_message_size;         // Maximum message size
     bool enable_compression;           // Enable packet compression
     bool enable_encryption;            // Enable packet encryption
     bool enable_tls;                   // Enable TLS security
     const char* tls_cert_file;         // TLS certificate file
     const char* tls_key_file;          // TLS key file
     const char* tls_ca_file;           // TLS CA certificate file
     uint32_t io_buffer_size;           // I/O buffer size
     uint32_t flags;                    // Configuration flags
     void* user_data;                   // User data
     void (*error_callback)(            // Error callback
         polycall_network_context_t* ctx,
         polycall_core_error_t error,
         const char* message,
         void* user_data
     );
 };
 
 /**
  * @brief Initialize network module
  * 
  * @param core_ctx Core context
  * @param network_ctx Pointer to receive network context
  * @param config Network configuration
  * @return Error code
  */
 polycall_core_error_t polycall_network_init(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t** network_ctx,
     const polycall_network_config_t* config
 );
 
 /**
  * @brief Clean up network module
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  */
 void polycall_network_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx
 );
 
 /**
  * @brief Create a client for a specific protocol
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param proto_ctx Protocol context
  * @param client Pointer to receive client
  * @param config Client configuration
  * @return Error code
  */
 polycall_core_error_t polycall_network_create_client(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_client_t** client,
     const polycall_network_client_config_t* config
 );
 
 /**
  * @brief Create a server for a specific protocol
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param proto_ctx Protocol context
  * @param server Pointer to receive server
  * @param config Server configuration
  * @return Error code
  */
 polycall_core_error_t polycall_network_create_server(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_server_t** server,
     const polycall_network_server_config_t* config
 );
 
 /**
  * @brief Get network statistics
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param stats Pointer to receive statistics
  * @return Error code
  */
 polycall_core_error_t polycall_network_get_stats(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_stats_t* stats
 );
 
 /**
  * @brief Process network events
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param timeout_ms Processing timeout in milliseconds (0 for non-blocking)
  * @return Error code
  */
 polycall_core_error_t polycall_network_process_events(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Register a global network event handler
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param event_type Event type
  * @param handler Event handler callback
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_network_register_event_handler(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_event_t event_type,
     void (*handler)(
         polycall_network_context_t* ctx,
         polycall_endpoint_t* endpoint,
         void* event_data,
         void* user_data
     ),
     void* user_data
 );
 
 /**
  * @brief Set network option
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param option Option to set
  * @param value Option value
  * @param size Option value size
  * @return Error code
  */
 polycall_core_error_t polycall_network_set_option(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_option_t option,
     const void* value,
     size_t size
 );
 
 /**
  * @brief Get network option
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param option Option to get
  * @param value Pointer to receive option value
  * @param size Pointer to option value size (in/out)
  * @return Error code
  */
 polycall_core_error_t polycall_network_get_option(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_option_t option,
     void* value,
     size_t* size
 );
 
 /**
  * @brief Create network packet
  * 
  * Convenience function that wraps polycall_network_packet_create
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param packet Pointer to receive packet
  * @param initial_capacity Initial capacity (0 for default)
  * @return Error code
  */
 polycall_core_error_t polycall_network_create_packet(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_packet_t** packet,
     size_t initial_capacity
 );
 
 /**
  * @brief Connect to remote endpoint
  * 
  * Convenience function that creates a client and connects to remote endpoint
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param proto_ctx Protocol context
  * @param address Remote address
  * @param port Remote port
  * @param timeout_ms Connection timeout (0 for default)
  * @param endpoint Pointer to receive endpoint
  * @return Error code
  */
 polycall_core_error_t polycall_network_connect(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     const char* address,
     uint16_t port,
     uint32_t timeout_ms,
     polycall_endpoint_t** endpoint
 );
 
 /**
  * @brief Start listening for connections
  * 
  * Convenience function that creates a server and starts listening
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param proto_ctx Protocol context
  * @param port Port to listen on
  * @param backlog Connection backlog
  * @param server Pointer to receive server
  * @return Error code
  */
 polycall_core_error_t polycall_network_listen(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     uint16_t port,
     uint32_t backlog,
     polycall_network_server_t** server
 );
 
 /**
  * @brief Send message to endpoint
  * 
  * Convenience function for sending a protocol message to an endpoint
  * 
  * @param core_ctx Core context
  * @param network_ctx Network context
  * @param proto_ctx Protocol context
  * @param endpoint Target endpoint
  * @param message Message to send
  * @param timeout_ms Send timeout (0 for default)
  * @param response Pointer to receive response message (NULL if no response expected)
  * @return Error code
  */
 polycall_core_error_t polycall_network_send_message(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_endpoint_t* endpoint,
     polycall_message_t* message,
     uint32_t timeout_ms,
     polycall_message_t** response
 );
 
 /**
  * @brief Create default network configuration
  * 
  * @return Default configuration
  */
 polycall_network_config_t polycall_network_create_default_config(void);
 
 /**
  * @brief Initialize network subsystem
  * 
  * Initializes the global network subsystem (sockets, etc.)
  * Must be called once before any other network functions
  * 
  * @return Error code
  */
 polycall_core_error_t polycall_network_subsystem_init(void);
 
 /**
  * @brief Clean up network subsystem
  * 
  * Cleans up the global network subsystem
  * Should be called at program exit
  */
 void polycall_network_subsystem_cleanup(void);
 
 /**
  * @brief Get network module version
  * 
  * @return Version string
  */
 const char* polycall_network_get_version(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_NETWORK_NETWORK_H_H */