/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/network/network_endpoint.h"

 * @file endpoint.c
 * @brief Network endpoint implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the endpoint interface for LibPolyCall networking,
 * representing a remote connection point for communication.
 */

 #include "polycall/core/network/network_endpoint.h"
 
 // Helper function to trigger event callbacks
 static void trigger_event(
     polycall_endpoint_t* endpoint,
     polycall_network_event_t event_type,
     void* event_data
 ) {
     if (!endpoint) {
         return;
     }
     
     for (size_t i = 0; i < endpoint->callback_count; i++) {
         if (endpoint->callbacks[i].event_type == event_type && 
             endpoint->callbacks[i].callback != NULL) {
             endpoint->callbacks[i].callback(endpoint, event_data, endpoint->callbacks[i].user_data);
         }
     }
 }
 
 polycall_core_error_t polycall_endpoint_get_info(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_endpoint_info_t* info
 ) {
     if (!ctx || !endpoint || !info) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy endpoint information to info structure
     info->type = endpoint->type;
     info->state = endpoint->state;
     strcpy(info->address, endpoint->address);
     info->port = endpoint->port;
     strcpy(info->local_address, endpoint->local_address);
     info->local_port = endpoint->local_port;
     info->secure = endpoint->secure;
     info->connected_time = endpoint->connected_time;
     info->bytes_sent = endpoint->bytes_sent;
     info->bytes_received = endpoint->bytes_received;
     info->latency_ms = endpoint->latency_ms;
     strcpy(info->peer_id, endpoint->peer_id);
     info->timeout_ms = endpoint->timeout_ms;
     info->socket_handle = (void*)(uintptr_t)endpoint->socket;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_get_state(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_endpoint_state_t* state
 ) {
     if (!ctx || !endpoint || !state) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *state = endpoint->state;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_set_option(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_network_option_t option,
     const void* value,
     size_t size
 ) {
     if (!ctx || !endpoint || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Platform-specific socket option handling
     int socket_opt = 0;
     int level = SOL_SOCKET;
     bool is_socket_option = true;
     
     switch (option) {
         case POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_RCVBUF; // For receive buffer, would need to handle send buffer separately
             break;
             
         case POLYCALL_NETWORK_OPTION_SOCKET_TIMEOUT:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_RCVTIMEO; // For receive timeout, would need to handle send timeout separately
             break;
             
         case POLYCALL_NETWORK_OPTION_KEEP_ALIVE:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_KEEPALIVE;
             break;
             
         case POLYCALL_NETWORK_OPTION_NAGLE_ALGORITHM:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             level = IPPROTO_TCP;
             socket_opt = TCP_NODELAY;
             break;
             
         case POLYCALL_NETWORK_OPTION_REUSE_ADDRESS:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_REUSEADDR;
             break;
             
         case POLYCALL_NETWORK_OPTION_LINGER:
             if (size != sizeof(struct linger)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_LINGER;
             break;
             
         case POLYCALL_NETWORK_OPTION_MAX_SEGMENT_SIZE:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             level = IPPROTO_TCP;
             socket_opt = TCP_MAXSEG;
             break;
             
         case POLYCALL_NETWORK_OPTION_IP_TTL:
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             level = IPPROTO_IP;
             socket_opt = IP_TTL;
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_CONTEXT:
             // This is not a socket option, handle specially
             is_socket_option = false;
             endpoint->tls_context = *(void**)value;
             endpoint->secure = (endpoint->tls_context != NULL);
             break;
             
         case POLYCALL_NETWORK_OPTION_NON_BLOCKING:
             // This requires platform-specific handling
             is_socket_option = false;
             if (size != sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             int non_blocking = *(int*)value;
             
 #ifdef _WIN32
             u_long mode = non_blocking ? 1 : 0;
             if (ioctlsocket(endpoint->socket, FIONBIO, &mode) != 0) {
                 return POLYCALL_CORE_ERROR_OPERATION_FAILED;
             }
 #else
             int flags = fcntl(endpoint->socket, F_GETFL, 0);
             if (flags == -1) {
                 return POLYCALL_CORE_ERROR_OPERATION_FAILED;
             }
             
             if (non_blocking) {
                 flags |= O_NONBLOCK;
             } else {
                 flags &= ~O_NONBLOCK;
             }
             
             if (fcntl(endpoint->socket, F_SETFL, flags) == -1) {
                 return POLYCALL_CORE_ERROR_OPERATION_FAILED;
             }
 #endif
             break;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Apply socket option if applicable
     if (is_socket_option) {
 #ifdef _WIN32
         if (setsockopt(endpoint->socket, level, socket_opt, (const char*)value, (int)size) != 0) {
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
 #else
         if (setsockopt(endpoint->socket, level, socket_opt, value, size) != 0) {
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
 #endif
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_get_option(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_network_option_t option,
     void* value,
     size_t* size
 ) {
     if (!ctx || !endpoint || !value || !size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Platform-specific socket option handling
     int socket_opt = 0;
     int level = SOL_SOCKET;
     bool is_socket_option = true;
     
     switch (option) {
         case POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_RCVBUF;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_SOCKET_TIMEOUT:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_RCVTIMEO;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_KEEP_ALIVE:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_KEEPALIVE;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_NAGLE_ALGORITHM:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             level = IPPROTO_TCP;
             socket_opt = TCP_NODELAY;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_REUSE_ADDRESS:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_REUSEADDR;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_LINGER:
             if (*size < sizeof(struct linger)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             socket_opt = SO_LINGER;
             *size = sizeof(struct linger);
             break;
             
         case POLYCALL_NETWORK_OPTION_MAX_SEGMENT_SIZE:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             level = IPPROTO_TCP;
             socket_opt = TCP_MAXSEG;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_IP_TTL:
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             level = IPPROTO_IP;
             socket_opt = IP_TTL;
             *size = sizeof(int);
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_CONTEXT:
             // This is not a socket option, handle specially
             is_socket_option = false;
             if (*size < sizeof(void*)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             *(void**)value = endpoint->tls_context;
             *size = sizeof(void*);
             break;
             
         case POLYCALL_NETWORK_OPTION_NON_BLOCKING:
             // This requires platform-specific handling
             is_socket_option = false;
             if (*size < sizeof(int)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
 #ifdef _WIN32
             u_long mode = 0;
             if (ioctlsocket(endpoint->socket, FIONBIO, &mode) != 0) {
                 return POLYCALL_CORE_ERROR_OPERATION_FAILED;
             }
             *(int*)value = (mode != 0) ? 1 : 0;
 #else
             int flags = fcntl(endpoint->socket, F_GETFL, 0);
             if (flags == -1) {
                 return POLYCALL_CORE_ERROR_OPERATION_FAILED;
             }
             *(int*)value = (flags & O_NONBLOCK) ? 1 : 0;
 #endif
             *size = sizeof(int);
             break;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get socket option if applicable
     if (is_socket_option) {
 #ifdef _WIN32
         int optlen = (int)*size;
         if (getsockopt(endpoint->socket, level, socket_opt, (char*)value, &optlen) != 0) {
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
         *size = optlen;
 #else
         socklen_t optlen = (socklen_t)*size;
         if (getsockopt(endpoint->socket, level, socket_opt, value, &optlen) != 0) {
             return POLYCALL_CORE_ERROR_OPERATION_FAILED;
         }
         *size = optlen;
 #endif
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_set_user_data(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     void* user_data
 ) {
     if (!ctx || !endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     endpoint->user_data = user_data;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_get_user_data(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     void** user_data
 ) {
     if (!ctx || !endpoint || !user_data) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *user_data = endpoint->user_data;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_set_event_callback(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_network_event_t event_type,
     void (*callback)(polycall_endpoint_t* endpoint, void* event_data, void* user_data),
     void* user_data
 ) {
     if (!ctx || !endpoint || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we already have a callback for this event type
     for (size_t i = 0; i < endpoint->callback_count; i++) {
         if (endpoint->callbacks[i].event_type == event_type) {
             // Update existing callback
             endpoint->callbacks[i].callback = callback;
             endpoint->callbacks[i].user_data = user_data;
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Add new callback if we have space
     if (endpoint->callback_count < MAX_CALLBACKS) {
         endpoint->callbacks[endpoint->callback_count].event_type = event_type;
         endpoint->callbacks[endpoint->callback_count].callback = callback;
         endpoint->callbacks[endpoint->callback_count].user_data = user_data;
         endpoint->callback_count++;
         return POLYCALL_CORE_SUCCESS;
     }
     
     return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
 }
 
 polycall_core_error_t polycall_endpoint_close(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint
 ) {
     if (!ctx || !endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Trigger disconnect event before closing
     if (endpoint->state == POLYCALL_ENDPOINT_STATE_CONNECTED) {
         endpoint->state = POLYCALL_ENDPOINT_STATE_DISCONNECTING;
         trigger_event(endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
     }
     
     // Close socket
     if (endpoint->socket) {
 #ifdef _WIN32
         closesocket(endpoint->socket);
 #else
         close(endpoint->socket);
 #endif
         endpoint->socket = 0;
     }
     
     // Clean up TLS context if any
     if (endpoint->tls_context) {
         // In a real implementation, we would call the appropriate TLS cleanup function
         // For now, we'll just set it to NULL
         endpoint->tls_context = NULL;
     }
     
     // Update endpoint state
     endpoint->state = POLYCALL_ENDPOINT_STATE_DISCONNECTED;
     
     // Free endpoint structure
     polycall_core_free(ctx, endpoint);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_create_from_socket(
     polycall_core_context_t* ctx,
     void* socket_handle,
     polycall_endpoint_type_t type,
     polycall_endpoint_t** endpoint
 ) {
     if (!ctx || !socket_handle || !endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate endpoint structure
     polycall_endpoint_t* new_endpoint = polycall_core_malloc(ctx, sizeof(polycall_endpoint_t));
     if (!new_endpoint) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize endpoint structure
     memset(new_endpoint, 0, sizeof(polycall_endpoint_t));
     new_endpoint->type = type;
     new_endpoint->state = POLYCALL_ENDPOINT_STATE_CONNECTED;
     new_endpoint->socket = (socket_handle_t)(uintptr_t)socket_handle;
     new_endpoint->connected_time = time(NULL);
     
     // Get peer address
     struct sockaddr_in peer_addr;
     socklen_t addr_len = sizeof(peer_addr);
     
 #ifdef _WIN32
     if (getpeername(new_endpoint->socket, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
         inet_ntop(AF_INET, &peer_addr.sin_addr, new_endpoint->address, sizeof(new_endpoint->address));
         new_endpoint->port = ntohs(peer_addr.sin_port);
     }
     
     // Get local address
     struct sockaddr_in local_addr;
     addr_len = sizeof(local_addr);
     if (getsockname(new_endpoint->socket, (struct sockaddr*)&local_addr, &addr_len) == 0) {
         inet_ntop(AF_INET, &local_addr.sin_addr, new_endpoint->local_address, sizeof(new_endpoint->local_address));
         new_endpoint->local_port = ntohs(local_addr.sin_port);
     }
 #else
     if (getpeername(new_endpoint->socket, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
         inet_ntop(AF_INET, &peer_addr.sin_addr, new_endpoint->address, sizeof(new_endpoint->address));
         new_endpoint->port = ntohs(peer_addr.sin_port);
     }
     
     // Get local address
     struct sockaddr_in local_addr;
     addr_len = sizeof(local_addr);
     if (getsockname(new_endpoint->socket, (struct sockaddr*)&local_addr, &addr_len) == 0) {
         inet_ntop(AF_INET, &local_addr.sin_addr, new_endpoint->local_address, sizeof(new_endpoint->local_address));
         new_endpoint->local_port = ntohs(local_addr.sin_port);
     }
 #endif
     
     // Set default timeout
     new_endpoint->timeout_ms = 30000; // 30 seconds
     
     // Generate peer ID (could be more sophisticated in real implementation)
     snprintf(new_endpoint->peer_id, sizeof(new_endpoint->peer_id), "%s:%d", 
              new_endpoint->address, new_endpoint->port);
     
     // Initialize statistics
     memset(&new_endpoint->stats, 0, sizeof(polycall_network_stats_t));
     new_endpoint->stats.start_time = time(NULL);
     
     *endpoint = new_endpoint;
     
     // Trigger connect event
     trigger_event(new_endpoint, POLYCALL_NETWORK_EVENT_CONNECT, NULL);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_endpoint_get_stats(
     polycall_core_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_network_stats_t* stats
 ) {
     if (!ctx || !endpoint || !stats) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Update some dynamic stats first
     endpoint->stats.uptime_seconds = time(NULL) - endpoint->stats.start_time;
     endpoint->stats.bytes_sent = endpoint->bytes_sent;
     endpoint->stats.bytes_received = endpoint->bytes_received;
     
     // Copy stats to output
     memcpy(stats, &endpoint->stats, sizeof(polycall_network_stats_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
