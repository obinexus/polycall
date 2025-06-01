/**
#include "polycall/core/network/network_client.h"

 * @file client.c
 * @brief Network client implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the client-side networking interface for LibPolyCall,
 * enabling connections to remote endpoints with protocol-aware communication.
 */

 #include "polycall/core/network/network_client.h"
 
 polycall_core_error_t polycall_network_client_create(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_client_t** client,
     const polycall_network_client_config_t* config
 ) {
     if (!ctx || !proto_ctx || !client) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default config if not provided
     polycall_network_client_config_t default_config;
     if (!config) {
         default_config = polycall_network_client_create_default_config();
         config = &default_config;
     }
     
     // Allocate client structure
     polycall_network_client_t* new_client = polycall_core_malloc(ctx, sizeof(polycall_network_client_t));
     if (!new_client) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize client structure
     memset(new_client, 0, sizeof(polycall_network_client_t));
     new_client->core_ctx = ctx;
     new_client->proto_ctx = proto_ctx;
     new_client->config = *config;
     new_client->user_data = config->user_data;
     new_client->connection_callback = config->connection_callback;
     new_client->error_callback = config->error_callback;
     new_client->request_id_counter = 1; // Start from 1
     new_client->initialized = true;
     
     // Initialize statistics
     memset(&new_client->stats, 0, sizeof(polycall_network_stats_t));
     new_client->stats.start_time = time(NULL);
     
     *client = new_client;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_client_connect(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     const char* address,
     uint16_t port,
     uint32_t timeout_ms,
     polycall_endpoint_t** endpoint
 ) {
     if (!ctx || !client || !address || !endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default timeout if not specified
     if (timeout_ms == 0) {
         timeout_ms = client->config.connect_timeout_ms;
     }
     
     // Create socket
     socket_handle_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (sock == SOCKET_ERROR_VALUE) {
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to create socket", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     
     // Set socket options
     // In a real implementation, we would set socket options based on client configuration
     
     // Set non-blocking mode for connection with timeout
     #ifdef _WIN32
     u_long mode = 1; // Non-blocking
     if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
         closesocket(sock);
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set non-blocking mode", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     #else
     int flags = fcntl(sock, F_GETFL, 0);
     if (flags == -1) {
         close(sock);
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to get socket flags", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     
     flags |= O_NONBLOCK;
     if (fcntl(sock, F_SETFL, flags) == -1) {
         close(sock);
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set non-blocking mode", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     #endif
     
     // Set up address structure
     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     
     // Convert address
     if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
         #ifdef _WIN32
         closesocket(sock);
         #else
         close(sock);
         #endif
         
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_INVALID_PARAMETERS, "Invalid address", client->user_data);
         }
         
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initiate connection
     int connect_result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
     
     // Check for immediate success/failure
     if (connect_result == 0) {
         // Connected immediately (rare for non-blocking socket)
     } else {
         #ifdef _WIN32
         int error_code = WSAGetLastError();
         if (error_code != WSAEWOULDBLOCK && error_code != WSAEINPROGRESS) {
             closesocket(sock);
             
             if (client->error_callback) {
                 char error_msg[256];
                 snprintf(error_msg, sizeof(error_msg), "Connection failed with error code %d", error_code);
                 client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, error_msg, client->user_data);
             }
             
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
         #else
         if (errno != EINPROGRESS) {
             close(sock);
             
             if (client->error_callback) {
                 char error_msg[256];
                 snprintf(error_msg, sizeof(error_msg), "Connection failed with error: %s", strerror(errno));
                 client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, error_msg, client->user_data);
             }
             
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
         #endif
         
         // Wait for connection completion with timeout
         fd_set write_set, error_set;
         struct timeval tv;
         
         FD_ZERO(&write_set);
         FD_ZERO(&error_set);
         FD_SET(sock, &write_set);
         FD_SET(sock, &error_set);
         
         tv.tv_sec = timeout_ms / 1000;
         tv.tv_usec = (timeout_ms % 1000) * 1000;
         
         int select_result = select(sock + 1, NULL, &write_set, &error_set, &tv);
         
         if (select_result <= 0) {
             // Timeout or error
             #ifdef _WIN32
             closesocket(sock);
             #else
             close(sock);
             #endif
             
             if (client->error_callback) {
                 client->error_callback(client, POLYCALL_CORE_ERROR_TIMEOUT, "Connection timed out", client->user_data);
             }
             
             return POLYCALL_CORE_ERROR_TIMEOUT;
         }
         
         if (FD_ISSET(sock, &error_set)) {
             // Connection failed
             #ifdef _WIN32
             closesocket(sock);
             #else
             close(sock);
             #endif
             
             if (client->error_callback) {
                 client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Connection failed", client->user_data);
             }
             
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
         
         // Check if connection was successful
         if (FD_ISSET(sock, &write_set)) {
             // Connection succeeded, but verify with getsockopt on some platforms
             int error_code = 0;
             socklen_t error_code_size = sizeof(error_code);
             
             if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error_code, &error_code_size) < 0 || error_code != 0) {
                 // Connection failed
                 #ifdef _WIN32
                 closesocket(sock);
                 #else
                 close(sock);
                 #endif
                 
                 if (client->error_callback) {
                     client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Connection failed", client->user_data);
                 }
                 
                 return POLYCALL_CORE_ERROR_OPERATION_FAILED;
             }
         } else {
             // Unexpected state
             #ifdef _WIN32
             closesocket(sock);
             #else
             close(sock);
             #endif
             
             if (client->error_callback) {
                 client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Connection failed", client->user_data);
             }
             
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
     }
     
     // Set socket back to blocking mode
     #ifdef _WIN32
     mode = 0; // Blocking
     if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
         closesocket(sock);
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set blocking mode", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     #else
     flags = fcntl(sock, F_GETFL, 0);
     if (flags == -1) {
         close(sock);
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to get socket flags", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     
     flags &= ~O_NONBLOCK;
     if (fcntl(sock, F_SETFL, flags) == -1) {
         close(sock);
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set blocking mode", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     #endif
     
     // Create endpoint
     polycall_endpoint_t* new_endpoint = NULL;
     polycall_core_error_t result = polycall_endpoint_create_from_socket(
         ctx, (void*)(uintptr_t)sock, POLYCALL_ENDPOINT_TYPE_TCP, &new_endpoint);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         #ifdef _WIN32
         closesocket(sock);
         #else
         close(sock);
         #endif
         
         if (client->error_callback) {
             client->error_callback(client, result, "Failed to create endpoint", client->user_data);
         }
         
         return result;
     }
     
     // Set up TLS if enabled
     if (client->config.enable_tls) {
         // In a real implementation, we would initialize TLS here
         
         // Set TLS context on the endpoint
         void* tls_context = NULL; // Placeholder
         polycall_endpoint_set_option(
             ctx, new_endpoint, POLYCALL_NETWORK_OPTION_TLS_CONTEXT, 
             &tls_context, sizeof(tls_context));
     }
     
     // Register event callback
     polycall_endpoint_set_event_callback(
         ctx, new_endpoint, POLYCALL_NETWORK_EVENT_CONNECT, 
         handle_endpoint_event, client);
     
     polycall_endpoint_set_event_callback(
         ctx, new_endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, 
         handle_endpoint_event, client);
     
     polycall_endpoint_set_event_callback(
         ctx, new_endpoint, POLYCALL_NETWORK_EVENT_ERROR, 
         handle_endpoint_event, client);
     
     polycall_endpoint_set_event_callback(
         ctx, new_endpoint, POLYCALL_NETWORK_EVENT_DATA_RECEIVED, 
         handle_endpoint_event, client);
     
     // Create client endpoint entry
     client_endpoint_t* client_endpoint = polycall_core_malloc(ctx, sizeof(client_endpoint_t));
     if (!client_endpoint) {
         polycall_endpoint_close(ctx, new_endpoint);
         
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OUT_OF_MEMORY, "Failed to allocate client endpoint", client->user_data);
         }
         
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize client endpoint
     memset(client_endpoint, 0, sizeof(client_endpoint_t));
     client_endpoint->endpoint = new_endpoint;
     client_endpoint->connected = true;
     client_endpoint->last_activity = time(NULL);
     client_endpoint->auto_reconnect = client->config.enable_auto_reconnect;
     
     // Add to client endpoint list
     client_endpoint->next = client->endpoints;
     client->endpoints = client_endpoint;
     
     // Update statistics
     client->stats.connection_attempts++;
     client->stats.successful_connections++;
     client->stats.active_connections++;
     
     // Notify connection callback
     if (client->connection_callback) {
         client->connection_callback(client, new_endpoint, true, client->user_data);
     }
     
     // Trigger connect event
     trigger_client_event(client, new_endpoint, POLYCALL_NETWORK_EVENT_CONNECT, NULL);
     
     *endpoint = new_endpoint;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_client_disconnect(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint
 ) {
     if (!ctx || !client || !endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find client endpoint
     client_endpoint_t* prev = NULL;
     client_endpoint_t* current = client->endpoints;
     
     while (current) {
         if (current->endpoint == endpoint) {
             // Found the endpoint
             break;
         }
         
         prev = current;
         current = current->next;
     }
     
     if (!current) {
         // Endpoint not found
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Clean up pending requests
     pending_request_t* request = current->pending_requests;
     while (request) {
         pending_request_t* next = request->next;
         
         // Free response if any
         if (request->response) {
             // In a real implementation, we would destroy the protocol message here
         }
         
         polycall_core_free(ctx, request);
         request = next;
     }
     
     // Remove from linked list
     if (prev) {
         prev->next = current->next;
     } else {
         client->endpoints = current->next;
     }
     
     // Update statistics
     client->stats.disconnections++;
     client->stats.active_connections--;
     
     // Notify connection callback
     if (client->connection_callback) {
         client->connection_callback(client, endpoint, false, client->user_data);
     }
     
     // Trigger disconnect event
     trigger_client_event(client, endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
     
     // Close endpoint
     polycall_core_error_t result = polycall_endpoint_close(ctx, endpoint);
     
     // Free client endpoint
     polycall_core_free(ctx, current);
     
     return result;
 }
 
 polycall_core_error_t polycall_network_client_send(
     polycall_core_context_t* ctx,
     polycall_network_client_t* client,
     polycall_endpoint_t* endpoint,
     polycall_network_packet_t* packet,
     uint32_t timeout_ms
 ) {
     if (!ctx || !client || !endpoint || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find client endpoint
     client_endpoint_t* client_endpoint = client->endpoints;
     while (client_endpoint) {
         if (client_endpoint->endpoint == endpoint) {
             break;
         }
         
         client_endpoint = client_endpoint->next;
     }
     
     if (!client_endpoint || !client_endpoint->connected) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Use default timeout if not specified
     if (timeout_ms == 0) {
         timeout_ms = client->config.operation_timeout_ms;
     }
     
     // Get packet data
     void* data = NULL;
     size_t data_size = 0;
     
     polycall_core_error_t result = polycall_network_packet_get_data(
         ctx, packet, &data, &data_size);
         
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     if (!data || data_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get endpoint info for socket handle
     polycall_endpoint_info_t info;
     result = polycall_endpoint_get_info(ctx, endpoint, &info);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     socket_handle_t sock = (socket_handle_t)(uintptr_t)info.socket_handle;
     
     // Set socket timeout
     #ifdef _WIN32
     DWORD send_timeout = timeout_ms;
     if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeout, sizeof(send_timeout)) != 0) {
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set send timeout", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     #else
     struct timeval send_timeout;
     send_timeout.tv_sec = timeout_ms / 1000;
     send_timeout.tv_usec = (timeout_ms % 1000) * 1000;
     
     if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) != 0) {
         if (client->error_callback) {
             client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set send timeout", client->user_data);
         }
         return POLYCALL_CORE_ERROR_OPERATION_FAILED;
     }
     #endif
     
     // Send data
     ssize_t sent = 0;
     while (sent < (ssize_t)data_size) {
         ssize_t n = send(sock, (const char*)data + sent, data_size - sent, 0);
         
         if (n <= 0) {
             // Send error
             if (client->error_callback) {
                 client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Send error", client->user_data);
             }
             
             // Mark endpoint as disconnected
             client_endpoint->connected = false;
             
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
         
         sent += n;
     }
     
     // Update statistics
     client->stats.bytes_sent += data_size;
     client->stats.packets_sent++;
     
     // Update last activity time
     client_endpoint->last_activity = time(NULL);
     
     // Trigger data sent event
     trigger_client_event(client, endpoint, POLYCALL_NETWORK_EVENT_DATA_SENT, NULL);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_client_receive(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    polycall_endpoint_t* endpoint,
    polycall_network_packet_t** packet,
    uint32_t timeout_ms
) {
    if (!ctx || !client || !endpoint || !packet) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find client endpoint
    client_endpoint_t* client_endpoint = client->endpoints;
    while (client_endpoint) {
        if (client_endpoint->endpoint == endpoint) {
            break;
        }
        
        client_endpoint = client_endpoint->next;
    }
    
    if (!client_endpoint || !client_endpoint->connected) {
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Use default timeout if not specified
    if (timeout_ms == 0) {
        timeout_ms = client->config.operation_timeout_ms;
    }
    
    // Get endpoint info for socket handle
    polycall_endpoint_info_t info;
    polycall_core_error_t result = polycall_endpoint_get_info(ctx, endpoint, &info);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    socket_handle_t sock = (socket_handle_t)(uintptr_t)info.socket_handle;
    
    // Set socket timeout
    #ifdef _WIN32
    DWORD recv_timeout = timeout_ms;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeout, sizeof(recv_timeout)) != 0) {
        if (client->error_callback) {
            client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set receive timeout", client->user_data);
        }
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    #else
    struct timeval recv_timeout;
    recv_timeout.tv_sec = timeout_ms / 1000;
    recv_timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) != 0) {
        if (client->error_callback) {
            client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set receive timeout", client->user_data);
        }
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    #endif
    
    // Read header first to get packet size
    uint32_t packet_size = 0;
    ssize_t n = recv(sock, (char*)&packet_size, sizeof(packet_size), MSG_PEEK);
    
    if (n <= 0) {
        // Receive error or connection closed
        if (n == 0) {
            // Connection closed
            client_endpoint->connected = false;
            if (client->error_callback) {
                client->error_callback(client, POLYCALL_CORE_ERROR_CONNECTION_CLOSED, "Connection closed by peer", client->user_data);
            }
            return POLYCALL_CORE_ERROR_CONNECTION_CLOSED;
        } else {
            // Receive error
            if (client->error_callback) {
                client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Receive error", client->user_data);
            }
            return POLYCALL_CORE_ERROR_OPERATION_FAILED;
        }
    }
    
    // Create packet with appropriate size
    polycall_network_packet_t* new_packet = NULL;
    result = polycall_network_packet_create(ctx, &new_packet, packet_size + sizeof(packet_size));
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Read actual data
    char* buffer = polycall_core_malloc(ctx, packet_size + sizeof(packet_size));
    if (!buffer) {
        polycall_network_packet_destroy(ctx, new_packet);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    ssize_t total_received = 0;
    while (total_received < (ssize_t)(packet_size + sizeof(packet_size))) {
        n = recv(sock, buffer + total_received, packet_size + sizeof(packet_size) - total_received, 0);
        
        if (n <= 0) {
            // Receive error or connection closed
            polycall_core_free(ctx, buffer);
            polycall_network_packet_destroy(ctx, new_packet);
            
            if (n == 0) {
                // Connection closed
                client_endpoint->connected = false;
                if (client->error_callback) {
                    client->error_callback(client, POLYCALL_CORE_ERROR_CONNECTION_CLOSED, "Connection closed by peer", client->user_data);
                }
                return POLYCALL_CORE_ERROR_CONNECTION_CLOSED;
            } else {
                // Receive error
                if (client->error_callback) {
                    client->error_callback(client, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Receive error", client->user_data);
                }
                return POLYCALL_CORE_ERROR_OPERATION_FAILED;
            }
        }
        
        total_received += n;
    }
    
    // Set packet data
    result = polycall_network_packet_set_data(ctx, new_packet, buffer + sizeof(packet_size), packet_size);
    polycall_core_free(ctx, buffer);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, new_packet);
        return result;
    }
    
    // Update statistics
    client->stats.bytes_received += packet_size;
    client->stats.packets_received++;
    
    // Update last activity time
    client_endpoint->last_activity = time(NULL);
    
    // Trigger data received event
    trigger_client_event(client, endpoint, POLYCALL_NETWORK_EVENT_DATA_RECEIVED, NULL);
    
    *packet = new_packet;
    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_network_client_send_message(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    polycall_protocol_context_t* proto_ctx,
    polycall_endpoint_t* endpoint,
    polycall_message_t* message,
    uint32_t timeout_ms,
    polycall_message_t** response
) {
    if (!ctx || !client || !proto_ctx || !endpoint || !message) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find client endpoint
    client_endpoint_t* client_endpoint = client->endpoints;
    while (client_endpoint) {
        if (client_endpoint->endpoint == endpoint) {
            break;
        }
        
        client_endpoint = client_endpoint->next;
    }
    
    if (!client_endpoint || !client_endpoint->connected) {
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Use default timeout if not specified
    if (timeout_ms == 0) {
        timeout_ms = client->config.operation_timeout_ms;
    }
    
    // Serialize the protocol message into a packet
    polycall_network_packet_t* packet = NULL;
    polycall_core_error_t result = polycall_network_packet_create(ctx, &packet, 0);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Get message data
    void* message_data = NULL;
    size_t message_size = 0;
    
    result = polycall_protocol_serialize_message(ctx, proto_ctx, message, &message_data, &message_size);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    // Set packet data
    result = polycall_network_packet_set_data(ctx, packet, message_data, message_size);
    polycall_core_free(ctx, message_data);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    // Mark as protocol packet
    polycall_packet_flags_t flags;
    result = polycall_network_packet_get_flags(ctx, packet, &flags);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    flags |= POLYCALL_PACKET_FLAG_PROTOCOL;
    
    result = polycall_network_packet_set_flags(ctx, packet, flags);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    // Generate request ID
    uint32_t request_id = client->request_id_counter++;
    result = polycall_network_packet_set_id(ctx, packet, request_id);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    // If response is expected, register a pending request
    if (response) {
        result = add_pending_request(ctx, client_endpoint, request_id, timeout_ms);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_network_packet_destroy(ctx, packet);
            return result;
        }
    }
    
    // Send the packet
    result = polycall_network_client_send(ctx, client, endpoint, packet, timeout_ms);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        // Clean up pending request if registered
        if (response) {
            pending_request_t* request = find_pending_request(client_endpoint, request_id);
            if (request) {
                remove_pending_request(ctx, client_endpoint, request);
            }
        }
        
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    polycall_network_packet_destroy(ctx, packet);
    
    // If response is expected, wait for it
    if (response) {
        // Wait for response
        time_t start_time = time(NULL);
        pending_request_t* request = find_pending_request(client_endpoint, request_id);
        
        while (request && !request->completed) {
            // Process incoming packets
            result = polycall_network_client_process_events(ctx, client, 100); // 100ms polling
            
            if (result != POLYCALL_CORE_SUCCESS && result != POLYCALL_CORE_ERROR_TIMEOUT) {
                remove_pending_request(ctx, client_endpoint, request);
                return result;
            }
            
            // Check timeout
            if ((time(NULL) - start_time) * 1000 >= timeout_ms) {
                remove_pending_request(ctx, client_endpoint, request);
                return POLYCALL_CORE_ERROR_TIMEOUT;
            }
            
            // Check if request was completed
            request = find_pending_request(client_endpoint, request_id);
        }
        
        // Return response if request was completed
        if (request && request->completed && request->response) {
            *response = request->response;
            request->response = NULL; // Prevent response from being freed
            remove_pending_request(ctx, client_endpoint, request);
            return POLYCALL_CORE_SUCCESS;
        }
        
        // Request not found or not completed properly
        if (request) {
            remove_pending_request(ctx, client_endpoint, request);
        }
        
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_network_client_get_stats(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    polycall_network_stats_t* stats
) {
    if (!ctx || !client || !stats) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Update uptime
    client->stats.uptime_seconds = time(NULL) - client->stats.start_time;
    
    // Count active connections
    client->stats.active_connections = 0;
    client_endpoint_t* endpoint = client->endpoints;
    while (endpoint) {
        if (endpoint->connected) {
            client->stats.active_connections++;
        }
        endpoint = endpoint->next;
    }
    
    // Copy stats
    memcpy(stats, &client->stats, sizeof(polycall_network_stats_t));
    
    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_network_client_set_option(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    polycall_network_option_t option,
    const void* value,
    size_t size
) {
    if (!ctx || !client || !value) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    switch (option) {
        case POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE:
        case POLYCALL_NETWORK_OPTION_SOCKET_TIMEOUT:
        case POLYCALL_NETWORK_OPTION_KEEP_ALIVE:
        case POLYCALL_NETWORK_OPTION_NAGLE_ALGORITHM:
        case POLYCALL_NETWORK_OPTION_REUSE_ADDRESS:
        case POLYCALL_NETWORK_OPTION_LINGER:
        case POLYCALL_NETWORK_OPTION_MAX_SEGMENT_SIZE:
        case POLYCALL_NETWORK_OPTION_IP_TTL:
        case POLYCALL_NETWORK_OPTION_TLS_CONTEXT:
        case POLYCALL_NETWORK_OPTION_NON_BLOCKING:
            // These options need to be applied to all endpoints
            {
                client_endpoint_t* endpoint = client->endpoints;
                while (endpoint) {
                    polycall_endpoint_set_option(ctx, endpoint->endpoint, option, value, size);
                    endpoint = endpoint->next;
                }
            }
            break;
            
        default:
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_network_client_get_option(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    polycall_network_option_t option,
    void* value,
    size_t* size
) {
    if (!ctx || !client || !value || !size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // For client options, we'll use the first endpoint if available
    if (client->endpoints) {
        return polycall_endpoint_get_option(ctx, client->endpoints->endpoint, option, value, size);
    }
    
    return POLYCALL_CORE_ERROR_INVALID_STATE;
}

polycall_core_error_t polycall_network_client_set_event_callback(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    polycall_network_event_t event_type,
    void (*callback)(polycall_network_client_t* client, polycall_endpoint_t* endpoint, void* event_data, void* user_data),
    void* user_data
) {
    if (!ctx || !client || !callback) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if we already have a handler for this event type
    for (size_t i = 0; i < client->handler_count; i++) {
        if (client->event_handlers[i].event_type == event_type) {
            // Update existing handler
            client->event_handlers[i].handler = callback;
            client->event_handlers[i].user_data = user_data;
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    // Add new handler if we have space
    if (client->handler_count < sizeof(client->event_handlers) / sizeof(client->event_handlers[0])) {
        client->event_handlers[client->handler_count].event_type = event_type;
        client->event_handlers[client->handler_count].handler = callback;
        client->event_handlers[client->handler_count].user_data = user_data;
        client->handler_count++;
        return POLYCALL_CORE_SUCCESS;
    }
    
    return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
}

polycall_core_error_t polycall_network_client_process_events(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    uint32_t timeout_ms
) {
    if (!ctx || !client) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Process each endpoint
    client_endpoint_t* endpoint = client->endpoints;
    bool activity = false;
    
    while (endpoint) {
        // Skip disconnected endpoints
        if (!endpoint->connected) {
            endpoint = endpoint->next;
            continue;
        }
        
        // Process pending requests
        polycall_core_error_t result = process_pending_requests(ctx, client, endpoint);
        
        if (result != POLYCALL_CORE_SUCCESS && result != POLYCALL_CORE_ERROR_TIMEOUT) {
            return result;
        }
        
        // Check for incoming data
        polycall_endpoint_info_t info;
        result = polycall_endpoint_get_info(ctx, endpoint->endpoint, &info);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            endpoint = endpoint->next;
            continue;
        }
        
        socket_handle_t sock = (socket_handle_t)(uintptr_t)info.socket_handle;
        
        // Check if data is available
        fd_set read_set;
        struct timeval tv;
        
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);
        
        tv.tv_sec = 0;
        tv.tv_usec = 0; // Non-blocking check
        
        int select_result = select(sock + 1, &read_set, NULL, NULL, &tv);
        
        if (select_result > 0 && FD_ISSET(sock, &read_set)) {
            // Data available, receive it
            polycall_network_packet_t* packet = NULL;
            result = polycall_network_client_receive(ctx, client, endpoint->endpoint, &packet, 0);
            
            if (result == POLYCALL_CORE_SUCCESS && packet) {
                // Process packet
                uint32_t packet_id;
                polycall_network_packet_get_id(ctx, packet, &packet_id);
                
                // Check if this is a response to a pending request
                pending_request_t* request = find_pending_request(endpoint, packet_id);
                
                if (request) {
                    // Handle protocol response
                    polycall_packet_flags_t flags;
                    polycall_network_packet_get_flags(ctx, packet, &flags);
                    
                    if (flags & POLYCALL_PACKET_FLAG_PROTOCOL) {
                        // Extract protocol message
                        void* data = NULL;
                        size_t data_size = 0;
                        
                        result = polycall_network_packet_get_data(ctx, packet, &data, &data_size);
                        
                        if (result == POLYCALL_CORE_SUCCESS && data && data_size > 0) {
                            // Deserialize protocol message
                            polycall_message_t* response_message = NULL;
                            result = polycall_protocol_deserialize_message(ctx, client->proto_ctx, data, data_size, &response_message);
                            
                            if (result == POLYCALL_CORE_SUCCESS && response_message) {
                                // Set as response
                                request->response = response_message;
                                request->completed = true;
                            }
                        }
                    }
                }
                
                // Clean up packet
                polycall_network_packet_destroy(ctx, packet);
                
                activity = true;
            }
        }
        
        endpoint = endpoint->next;
    }
    
    // If no activity and non-zero timeout, sleep
    if (!activity && timeout_ms > 0) {
        // Use platform-specific sleep
        #ifdef _WIN32
        Sleep(timeout_ms > 100 ? 100 : timeout_ms);
        #else
        struct timespec ts;
        ts.tv_sec = (timeout_ms > 100 ? 100 : timeout_ms) / 1000;
        ts.tv_nsec = ((timeout_ms > 100 ? 100 : timeout_ms) % 1000) * 1000000;
        nanosleep(&ts, NULL);
        #endif
        
        return POLYCALL_CORE_ERROR_TIMEOUT;
    }
    
    return activity ? POLYCALL_CORE_SUCCESS : POLYCALL_CORE_ERROR_TIMEOUT;
}

void polycall_network_client_cleanup(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client
) {
    if (!ctx || !client) {
        return;
    }
    
    // Mark as shutting down
    client->shutting_down = true;
    
    // Disconnect all endpoints
    while (client->endpoints) {
        polycall_endpoint_t* endpoint = client->endpoints->endpoint;
        polycall_network_client_disconnect(ctx, client, endpoint);
    }
    
    // Free client structure
    polycall_core_free(ctx, client);
}

polycall_network_client_config_t polycall_network_client_create_default_config(void) {
    polycall_network_client_config_t config;
    
    config.connect_timeout_ms = 30000; // 30 seconds
    config.operation_timeout_ms = 30000; // 30 seconds
    config.keep_alive_interval_ms = 60000; // 60 seconds
    config.max_reconnect_attempts = 5;
    config.reconnect_delay_ms = 5000; // 5 seconds
    config.enable_auto_reconnect = true;
    config.enable_tls = false;
    config.tls_cert_file = NULL;
    config.tls_key_file = NULL;
    config.tls_ca_file = NULL;
    config.max_pending_requests = DEFAULT_MAX_PENDING_REQUESTS;
    config.max_message_size = 1024 * 1024; // 1MB
    config.user_data = NULL;
    config.connection_callback = NULL;
    config.error_callback = NULL;
    
    return config;
}

// Internal helper functions
static void handle_endpoint_event(
    polycall_endpoint_t* endpoint,
    void* event_data,
    void* user_data
) {
    polycall_network_client_t* client = (polycall_network_client_t*)user_data;
    if (!client) {
        return;
    }
    
    // Get endpoint info for event type
    polycall_endpoint_info_t info;
    if (polycall_endpoint_get_info(client->core_ctx, endpoint, &info) != POLYCALL_CORE_SUCCESS) {
        return;
    }
    
    // Find client endpoint
    client_endpoint_t* client_endpoint = client->endpoints;
    while (client_endpoint) {
        if (client_endpoint->endpoint == endpoint) {
            break;
        }
        
        client_endpoint = client_endpoint->next;
    }
    
    if (!client_endpoint) {
        return;
    }
    
    // Handle specific events
    if (info.state == POLYCALL_ENDPOINT_STATE_DISCONNECTED) {
        // Mark as disconnected
        client_endpoint->connected = false;
        
        // Trigger disconnect event
        trigger_client_event(client, endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
        
        // Try auto-reconnect if enabled
        if (client_endpoint->auto_reconnect && !client->shutting_down) {
            // In a real implementation, we would initiate reconnection
        }
    }
}

static polycall_core_error_t process_pending_requests(
    polycall_core_context_t* ctx,
    polycall_network_client_t* client,
    client_endpoint_t* client_endpoint
) {
    if (!ctx || !client || !client_endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check for timed out requests
    pending_request_t* request = client_endpoint->pending_requests;
    pending_request_t* prev = NULL;
    
    while (request) {
        // Get current time
        time_t now = time(NULL);
        uint32_t elapsed_ms = (uint32_t)((now - request->created_time) * 1000);
        
        // Check if request timed out
        if (elapsed_ms >= request->timeout_ms && !request->completed) {
            // Request timed out
            pending_request_t* to_remove = request;
            request = request->next;
            
            // Remove from list
            if (prev) {
                prev->next = to_remove->next;
            } else {
                client_endpoint->pending_requests = to_remove->next;
            }
            
            // Free request
            if (to_remove->response) {
                // In a real implementation, we would destroy the protocol message here
            }
            
            polycall_core_free(ctx, to_remove);
            
            // Continue with next request
            continue;
        }
        
        prev = request;
        request = request->next;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

static pending_request_t* find_pending_request(
    client_endpoint_t* client_endpoint,
    uint32_t request_id
) {
    if (!client_endpoint) {
        return NULL;
    }
    
    pending_request_t* request = client_endpoint->pending_requests;
    
    while (request) {
        if (request->id == request_id) {
            return request;
        }
        
        request = request->next;
    }
    
    return NULL;
}

static void remove_pending_request(
    polycall_core_context_t* ctx,
    client_endpoint_t* client_endpoint,
    pending_request_t* request
) {
    if (!ctx || !client_endpoint || !request) {
        return;
    }
    
    // Find request in list
    pending_request_t* prev = NULL;
    pending_request_t* current = client_endpoint->pending_requests;
    
    while (current) {
        if (current == request) {
            // Remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                client_endpoint->pending_requests = current->next;
            }
            
            // Free response if any
            if (current->response) {
                // In a real implementation, we would destroy the protocol message here
            }
            
            // Free request
            polycall_core_free(ctx, current);
            return;
        }
        
        prev = current;
        current = current->next;
    }
}

static void trigger_client_event(
    polycall_network_client_t* client,
    polycall_endpoint_t* endpoint,
    polycall_network_event_t event_type,
    void* event_data
) {
    if (!client || !endpoint) {
        return;
    }
    
    // Call handlers for this event type
    for (size_t i = 0; i < client->handler_count; i++) {
        if (client->event_handlers[i].event_type == event_type && 
            client->event_handlers[i].handler != NULL) {
            client->event_handlers[i].handler(
                client, endpoint, event_data, client->event_handlers[i].user_data);
        }
    }
}

static polycall_core_error_t add_pending_request(
    polycall_core_context_t* ctx,
    client_endpoint_t* client_endpoint,
    uint32_t request_id,
    uint32_t timeout_ms
) {
    if (!ctx || !client_endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create new request
    pending_request_t* request = polycall_core_malloc(ctx, sizeof(pending_request_t));
    if (!request) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize request
    memset(request, 0, sizeof(pending_request_t));
    request->id = request_id;
    request->created_time = time(NULL);
    request->timeout_ms = timeout_ms;
    request->completed = false;
    request->response = NULL;
    
    // Add to list
    request->next = client_endpoint->pending_requests;
    client_endpoint->pending_requests = request;
    
    return POLYCALL_CORE_SUCCESS;
}