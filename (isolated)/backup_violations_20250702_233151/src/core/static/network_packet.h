/**
 * @file network_server.h
 * @brief Network server interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the server-side networking interface for LibPolyCall,
 * enabling listening for and accepting connections from remote clients
 * with protocol-aware communication.
 */

 #ifndef POLYCALL_NETWORK_NETWORK_PACKET_H_H
 #define POLYCALL_NETWORK_NETWORK_PACKET_H_H
 
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "network_endpoint.h"
 #include "network_packet.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct polycall_network_server polycall_network_server_t;
 typedef struct polycall_network_server_config polycall_network_server_config_t;
 
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
/* Forward declarations */
typedef struct polycall_network_packet polycall_network_packet_t;


 // Default initial capacity for packet buffer
 #define POLYCALL_NETWORK_NETWORK_PACKET_H_H
 
 // Packet header size (in bytes)
 #define POLYCALL_NETWORK_NETWORK_PACKET_H_H
 
 // Packet metadata entry structure
 typedef struct {
     char key[32];
     void* value;
     size_t value_size;
 } packet_metadata_t;
 
 // Maximum number of metadata entries per packet
 #define POLYCALL_NETWORK_NETWORK_PACKET_H_H
 
 // Network packet structure
 struct polycall_network_packet {
     uint16_t type;
     uint32_t id;
     uint32_t sequence;
     uint64_t timestamp;
     polycall_packet_flags_t flags;
     uint32_t checksum;
     uint8_t priority;
     
     void* data;
     size_t data_size;
     size_t buffer_capacity;
     bool owns_data;
     
     packet_metadata_t metadata[MAX_METADATA_ENTRIES];
     size_t metadata_count;
 };
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
 
 #endif /* POLYCALL_NETWORK_NETWORK_PACKET_H_H */