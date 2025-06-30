/**
 * @file network_client.h
 * @brief Network client interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the client-side networking interface for LibPolyCall,
 * enabling connections to remote endpoints with protocol-aware communication.
 */

 #ifndef POLYCALL_NETWORK_NETWORK_CLIENT_H_H
 #define POLYCALL_NETWORK_NETWORK_CLIENT_H_H
 

 #include "network_endpoint.h"
 #include "network_packet.h"
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
// Forward declarations
typedef struct polycall_network_client polycall_network_client_t;
typedef struct polycall_network_client_config polycall_network_client_config_t;
typedef enum polycall_network_event polycall_network_event_t;
 
 // Platform-specific socket includes
 #ifdef _WIN32
 #include <winsock2.h>
 #include <ws2tcpip.h>
 typedef SOCKET socket_handle_t;
 #define POLYCALL_NETWORK_NETWORK_CLIENT_H_H
 #else
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <errno.h>
 typedef int socket_handle_t;
 #define POLYCALL_NETWORK_NETWORK_CLIENT_H_H
 #endif
 
 // Maximum number of pending requests
 #define POLYCALL_NETWORK_NETWORK_CLIENT_H_H
 
 // Connection request structure
 typedef struct pending_request {
     uint32_t id;
     uint32_t created_time;
     uint32_t timeout_ms;
     bool completed;
     polycall_message_t* response;
     struct pending_request* next;
 } pending_request_t;
 
 // Client endpoint reference
 typedef struct client_endpoint {
     polycall_endpoint_t* endpoint;
     bool connected;
     time_t last_activity;
     bool auto_reconnect;
     pending_request_t* pending_requests;
     struct client_endpoint* next;
 } client_endpoint_t;
 
 // Network client structure
 struct polycall_network_client {
     polycall_core_context_t* core_ctx;
     polycall_protocol_context_t* proto_ctx;
     client_endpoint_t* endpoints;
     uint32_t request_id_counter;
     polycall_network_client_config_t config;
     polycall_network_stats_t stats;
     void* user_data;
     bool initialized;
     bool shutting_down;
     
     // Callbacks
     void (*connection_callback)(
         polycall_network_client_t* client,
         polycall_endpoint_t* endpoint,
         bool connected,
         void* user_data
     );
     
     void (*error_callback)(
         polycall_network_client_t* client,
         polycall_core_error_t error,
         const char* message,
         void* user_data
     );
     
     // Event handlers
     struct {
         polycall_network_event_t event_type;
         void (*handler)(
             polycall_network_client_t* client,
             polycall_endpoint_t* endpoint,
             void* event_data,
             void* user_data
         );
         void* user_data;
     } event_handlers[8]; // Support for up to 8 event handlers
     
     size_t handler_count;
 };
 
 // Forward declarations of internal functions
 static void handle_endpoint_event(
     polycall_endpoint_t* endpoint,
     void* event_data,
     void* user_data
 );
 
 static polycall_core_error_t process_pending_requests(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     client_endpoint_t* client_endpoint
 );
 
 static pending_request_t* find_pending_request(
     client_endpoint_t* client_endpoint,
     uint32_t request_id
 );
 
 static void remove_pending_request(
     polycall_core_context_t* ctx,
     client_endpoint_t* client_endpoint,
     pending_request_t* request
 );
 
 static void trigger_client_event(
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint,
     polycall_network_event_t event_type,
     void* event_data
 );
 
 static polycall_core_error_t add_pending_request(
     polycall_core_context_t* ctx,
     client_endpoint_t* client_endpoint,
     uint32_t request_id,
     uint32_t timeout_ms
 );
 /**
  * @brief Network client configuration
  */
 struct polycall_network_client_config {
     uint32_t connect_timeout_ms;          // Connection timeout in milliseconds
     uint32_t operation_timeout_ms;        // Operation timeout in milliseconds
     uint32_t keep_alive_interval_ms;      // Keep-alive interval in milliseconds
     uint32_t max_reconnect_attempts;      // Maximum reconnection attempts
     uint32_t reconnect_delay_ms;          // Delay between reconnection attempts
     bool enable_auto_reconnect;           // Enable automatic reconnection
     bool enable_tls;                      // Enable TLS encryption
     const char* tls_cert_file;            // TLS certificate file path
     const char* tls_key_file;             // TLS key file path
     const char* tls_ca_file;              // TLS CA certificate file path
     uint32_t max_pending_requests;        // Maximum pending requests
     uint32_t max_message_size;            // Maximum message size
     void* user_data;                      // User data
     void (*connection_callback)(          // Connection state change callback
         polycall_network_client_t* client,
         polycall_endpoint_t* endpoint,
         bool connected,
         void* user_data
     );
     void (*error_callback)(               // Error callback
         polycall_network_client_t* client,
         polycall_core_error_t error,
         const char* message,
         void* user_data
     );
 };
 
 /**
  * @brief Create a network client
  * 
  * @param ctx Core context
  * @param proto_ctx Protocol context
  * @param client Pointer to receive created client
  * @param config Client configuration
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_create(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_client_t** client,
     const polycall_network_client_config_t* config
 );
 
 /**
  * @brief Connect to a remote endpoint
  * 
  * @param ctx Core context
  * @param client Network client
  * @param address Server address
  * @param port Server port
  * @param timeout_ms Connection timeout in milliseconds (0 for default)
  * @param endpoint Pointer to receive the connected endpoint
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_connect(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     const char* address,
     uint16_t port,
     uint32_t timeout_ms,
     polycall_endpoint_t** endpoint
 );
 
 /**
  * @brief Disconnect from a remote endpoint
  * 
  * @param ctx Core context
  * @param client Network client
  * @param endpoint Endpoint to disconnect
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_disconnect(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint
 );
 
 /**
  * @brief Send a packet to a remote endpoint
  * 
  * @param ctx Core context
  * @param client Network client
  * @param endpoint Target endpoint
  * @param packet Packet to send
  * @param timeout_ms Send timeout in milliseconds (0 for default)
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_send(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint,
     polycall_network_packet_t* packet,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Receive a packet from a remote endpoint
  * 
  * @param ctx Core context
  * @param client Network client
  * @param endpoint Source endpoint
  * @param packet Pointer to receive packet
  * @param timeout_ms Receive timeout in milliseconds (0 for default)
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_receive(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint,
     polycall_network_packet_t** packet,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Send a protocol message to a remote endpoint
  * 
  * @param ctx Core context
  * @param client Network client
  * @param proto_ctx Protocol context
  * @param endpoint Target endpoint
  * @param message Protocol message to send
  * @param timeout_ms Send timeout in milliseconds (0 for default)
  * @param response Pointer to receive response message (NULL if no response expected)
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_send_message(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_protocol_context_t* proto_ctx,
     polycall_endpoint_t* endpoint,
     polycall_message_t* message,
     uint32_t timeout_ms,
     polycall_message_t** response
 );
 
 /**
  * @brief Get client statistics
  * 
  * @param ctx Core context
  * @param client Network client
  * @param stats Pointer to receive statistics
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_get_stats(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_network_stats_t* stats
 );
 
 /**
  * @brief Set client option
  * 
  * @param ctx Core context
  * @param client Network client
  * @param option Option to set
  * @param value Option value
  * @param size Option value size
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_set_option(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_network_option_t option,
     const void* value,
     size_t size
 );
 
 /**
  * @brief Get client option
  * 
  * @param ctx Core context
  * @param client Network client
  * @param option Option to get
  * @param value Pointer to receive option value
  * @param size Pointer to option value size (in/out)
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_get_option(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_network_option_t option,
     void* value,
     size_t* size
 );
 
 /**
  * @brief Set client event callback
  * 
  * @param ctx Core context
  * @param client Network client
  * @param event_type Event type
  * @param callback Event callback
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_set_event_callback(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_network_event_t event_type,
     void (*callback)(polycall_network_client_t* client, polycall_endpoint_t* endpoint, void* event_data, void* user_data),
     void* user_data
 );
 
 /**
  * @brief Process pending events
  * 
  * @param ctx Core context
  * @param client Network client
  * @param timeout_ms Processing timeout in milliseconds (0 for non-blocking)
  * @return Error code
  */
 polycall_core_error_t polycall_network_client_process_events(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     uint32_t timeout_ms
 );
 
 /**
  * @brief Clean up client resources
  * 
  * @param ctx Core context
  * @param client Network client
  */
 void polycall_network_client_cleanup(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client
 );
 
 /**
  * @brief Create default client configuration
  * 
  * @return Default configuration
  */
 polycall_network_client_config_t polycall_network_client_create_default_config(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_NETWORK_NETWORK_CLIENT_H_H */