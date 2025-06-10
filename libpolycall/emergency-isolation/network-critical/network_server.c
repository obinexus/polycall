/**
#include "polycall/core/network/network_server.h"
#include "polycall/core/network/network_server.h"

 * @file network_server.c
 * @brief Network server implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the server-side networking interface for LibPolyCall,
 * enabling listening for and accepting connections from remote clients
 * with protocol-aware communication.
 */
/**
 * @brief Create a network server
 */
#include "polycall/core/network/network_server.h"
polycall_core_error_t polycall_network_server_create(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    polycall_network_server_t** server,
    const polycall_network_server_config_t* config
) {
    if (!ctx || !proto_ctx || !server) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Use default config if not provided
    polycall_network_server_config_t default_config;
    if (!config) {
        default_config = polycall_network_server_create_default_config();
        config = &default_config;
    }
    
    // Allocate server structure
    polycall_network_server_t* new_server = polycall_core_malloc(ctx, sizeof(polycall_network_server_t));
    if (!new_server) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize server structure
    memset(new_server, 0, sizeof(polycall_network_server_t));
    new_server->core_ctx = ctx;
    new_server->proto_ctx = proto_ctx;
    new_server->config = *config;
    new_server->listen_socket = SOCKET_ERROR_VALUE;
    new_server->next_endpoint_id = 1;
    new_server->user_data = config->user_data;
    new_server->connection_callback = config->connection_callback;
    new_server->error_callback = config->error_callback;
    
    // Initialize mutexes and condition variables
    if (pthread_mutex_init(&new_server->server_mutex, NULL) != 0) {
        polycall_core_free(ctx, new_server);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    if (pthread_mutex_init(&new_server->endpoint_mutex, NULL) != 0) {
        pthread_mutex_destroy(&new_server->server_mutex);
        polycall_core_free(ctx, new_server);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    if (pthread_cond_init(&new_server->server_cond, NULL) != 0) {
        pthread_mutex_destroy(&new_server->endpoint_mutex);
        pthread_mutex_destroy(&new_server->server_mutex);
        polycall_core_free(ctx, new_server);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Determine worker thread count
    uint32_t worker_count = config->worker_thread_count;
    if (worker_count == 0) {
        // Auto-detect (use number of CPU cores, but cap at a reasonable value)
        #ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        worker_count = sysinfo.dwNumberOfProcessors;
        #else
        worker_count = sysconf(_SC_NPROCESSORS_ONLN);
        #endif
        
        if (worker_count < 1) worker_count = 1;
        if (worker_count > MAX_WORKER_THREADS) worker_count = MAX_WORKER_THREADS;
    } else if (worker_count > MAX_WORKER_THREADS) {
        worker_count = MAX_WORKER_THREADS;
    }
    
    // Allocate worker thread array
    new_server->worker_threads = polycall_core_malloc(ctx, worker_count * sizeof(worker_thread_t));
    if (!new_server->worker_threads) {
        pthread_cond_destroy(&new_server->server_cond);
        pthread_mutex_destroy(&new_server->endpoint_mutex);
        pthread_mutex_destroy(&new_server->server_mutex);
        polycall_core_free(ctx, new_server);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize worker thread array
    memset(new_server->worker_threads, 0, worker_count * sizeof(worker_thread_t));
    new_server->worker_thread_count = worker_count;
    
    // Initialize statistics
    memset(&new_server->stats, 0, sizeof(polycall_network_stats_t));
    new_server->stats.start_time = time(NULL);
    
    // Initialize TLS if enabled
    if (config->enable_tls) {
        if (!initialize_tls(ctx, new_server)) {
            polycall_core_free(ctx, new_server->worker_threads);
            pthread_cond_destroy(&new_server->server_cond);
            pthread_mutex_destroy(&new_server->endpoint_mutex);
            pthread_mutex_destroy(&new_server->server_mutex);
            polycall_core_free(ctx, new_server);
            return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
        }
    }
    
    new_server->initialized = true;
    *server = new_server;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Start the server
 */
polycall_core_error_t polycall_network_server_start(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server
) {
    if (!ctx || !server) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Check if server is already running
    if (server->running) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_SUCCESS; // Already running, not an error
    }
    
    // Initialize socket
    if (!initialize_socket(ctx, server)) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Start listening
    if (listen(server->listen_socket, server->config.backlog > 0 ? server->config.backlog : DEFAULT_BACKLOG) != 0) {
        // Clean up socket
        CLOSE_SOCKET(server->listen_socket);
        server->listen_socket = SOCKET_ERROR_VALUE;
        
        pthread_mutex_unlock(&server->server_mutex);
        
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to listen on socket", server->user_data);
        }
        
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    
    // Initialize worker threads
    if (!initialize_worker_threads(ctx, server)) {
        // Clean up socket
        CLOSE_SOCKET(server->listen_socket);
        server->listen_socket = SOCKET_ERROR_VALUE;
        
        pthread_mutex_unlock(&server->server_mutex);
        
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_INITIALIZATION_FAILED, "Failed to start worker threads", server->user_data);
        }
        
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Start I/O thread
    server->io_thread_active = true;
    if (pthread_create(&server->io_thread, NULL, io_thread_func, server) != 0) {
        // Clean up socket and worker threads
        CLOSE_SOCKET(server->listen_socket);
        server->listen_socket = SOCKET_ERROR_VALUE;
        server->io_thread_active = false;
        
        // Signal worker threads to exit
        for (uint32_t i = 0; i < server->worker_thread_count; i++) {
            server->worker_threads[i].active = false;
        }
        
        pthread_cond_broadcast(&server->server_cond);
        pthread_mutex_unlock(&server->server_mutex);
        
        // Wait for worker threads to exit
        for (uint32_t i = 0; i < server->worker_thread_count; i++) {
            if (server->worker_threads[i].thread_id != 0) {
                pthread_join(server->worker_threads[i].thread_id, NULL);
                server->worker_threads[i].thread_id = 0;
            }
        }
        
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_INITIALIZATION_FAILED, "Failed to start I/O thread", server->user_data);
        }
        
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Server is now running
    server->running = true;
    
    pthread_mutex_unlock(&server->server_mutex);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Stop the server
 */
polycall_core_error_t polycall_network_server_stop(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server
) {
    if (!ctx || !server) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Check if server is already stopped
    if (!server->running) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_SUCCESS; // Already stopped, not an error
    }
    
    // Set running flag to false
    server->running = false;
    
    // Signal I/O thread to exit
    server->io_thread_active = false;
    pthread_cond_broadcast(&server->server_cond);
    
    // Close listen socket to unblock accept call
    if (server->listen_socket != SOCKET_ERROR_VALUE) {
        CLOSE_SOCKET(server->listen_socket);
        server->listen_socket = SOCKET_ERROR_VALUE;
    }
    
    // Signal worker threads to exit
    for (uint32_t i = 0; i < server->worker_thread_count; i++) {
        server->worker_threads[i].active = false;
    }
    
    pthread_cond_broadcast(&server->server_cond);
    pthread_mutex_unlock(&server->server_mutex);
    
    // Wait for I/O thread to exit
    if (server->io_thread != 0) {
        pthread_join(server->io_thread, NULL);
        server->io_thread = 0;
    }
    
    // Wait for worker threads to exit
    for (uint32_t i = 0; i < server->worker_thread_count; i++) {
        if (server->worker_threads[i].thread_id != 0) {
            pthread_join(server->worker_threads[i].thread_id, NULL);
            server->worker_threads[i].thread_id = 0;
        }
    }
    
    // Disconnect all endpoints
    pthread_mutex_lock(&server->endpoint_mutex);
    
    server_endpoint_t* current = server->endpoints;
    while (current) {
        server_endpoint_t* next = current->next;
        
        // Notify connection callback
        if (server->connection_callback) {
            server->connection_callback(server, current->endpoint, false, server->user_data);
        }
        
        // Close endpoint
        polycall_endpoint_close(ctx, current->endpoint);
        
        // Free endpoint structure
        polycall_core_free(ctx, current);
        
        current = next;
    }
    
    server->endpoints = NULL;
    server->endpoint_count = 0;
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Accept a new connection
 */
polycall_core_error_t polycall_network_server_accept(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_endpoint_t** endpoint,
    uint32_t timeout_ms
) {
    if (!ctx || !server || !endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Check if server is running
    if (!server->running) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Check if listen socket is valid
    if (server->listen_socket == SOCKET_ERROR_VALUE) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    pthread_mutex_unlock(&server->server_mutex);
    
    // Set up select with timeout
    fd_set read_set;
    struct timeval tv;
    
    FD_ZERO(&read_set);
    FD_SET(server->listen_socket, &read_set);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int select_result = select(server->listen_socket + 1, &read_set, NULL, NULL, &tv);
    
    if (select_result <= 0) {
        // Timeout or error
        if (select_result < 0) {
            if (server->error_callback) {
                server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Select error during accept", server->user_data);
            }
            return POLYCALL_CORE_ERROR_OPERATION_FAILED;
        }
        return POLYCALL_CORE_ERROR_TIMEOUT;
    }
    
    // Accept connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    socket_handle_t client_socket = accept(server->listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    
    if (client_socket == SOCKET_ERROR_VALUE) {
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Accept error", server->user_data);
        }
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    
    // Update statistics
    pthread_mutex_lock(&server->server_mutex);
    server->stats.connection_attempts++;
    server->stats.successful_connections++;
    server->stats.active_connections++;
    pthread_mutex_unlock(&server->server_mutex);
    
    // Create endpoint from socket
    polycall_endpoint_t* new_endpoint;
    polycall_core_error_t result = polycall_endpoint_create_from_socket(
        ctx, (void*)(uintptr_t)client_socket, POLYCALL_ENDPOINT_TYPE_TCP, &new_endpoint);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        CLOSE_SOCKET(client_socket);
        
        if (server->error_callback) {
            server->error_callback(server, result, "Failed to create endpoint", server->user_data);
        }
        
        return result;
    }
    
    // Set up TLS if enabled
    if (server->config.enable_tls && server->tls_context) {
        // In a real implementation, we would perform TLS handshake here
        void* tls_context = server->tls_context;
        polycall_endpoint_set_option(
            ctx, new_endpoint, POLYCALL_NETWORK_OPTION_TLS_CONTEXT, 
            &tls_context, sizeof(tls_context));
    }
    
    // Register event callbacks
    polycall_endpoint_set_event_callback(
        ctx, new_endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, 
        handle_endpoint_event, server);
    
    polycall_endpoint_set_event_callback(
        ctx, new_endpoint, POLYCALL_NETWORK_EVENT_ERROR, 
        handle_endpoint_event, server);
    
    polycall_endpoint_set_event_callback(
        ctx, new_endpoint, POLYCALL_NETWORK_EVENT_DATA_RECEIVED, 
        handle_endpoint_event, server);
    
    // Create server endpoint structure
    server_endpoint_t* server_endpoint = polycall_core_malloc(ctx, sizeof(server_endpoint_t));
    if (!server_endpoint) {
        polycall_endpoint_close(ctx, new_endpoint);
        
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OUT_OF_MEMORY, "Failed to allocate server endpoint", server->user_data);
        }
        
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize server endpoint
    memset(server_endpoint, 0, sizeof(server_endpoint_t));
    server_endpoint->endpoint = new_endpoint;
    server_endpoint->connected = true;
    server_endpoint->connect_time = time(NULL);
    server_endpoint->last_activity = time(NULL);
    
    // Assign endpoint ID
    pthread_mutex_lock(&server->endpoint_mutex);
    server_endpoint->endpoint_id = server->next_endpoint_id++;
    
    // Add to endpoint list
    server_endpoint->next = server->endpoints;
    server->endpoints = server_endpoint;
    server->endpoint_count++;
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Notify connection callback
    if (server->connection_callback) {
        server->connection_callback(server, new_endpoint, true, server->user_data);
    }
    
    // Trigger connect event
    trigger_server_event(server, new_endpoint, POLYCALL_NETWORK_EVENT_CONNECT, NULL);
    
    // Return the new endpoint
    *endpoint = new_endpoint;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Disconnect a client
 */
polycall_core_error_t polycall_network_server_disconnect(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_endpoint_t* endpoint
) {
    if (!ctx || !server || !endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find server endpoint
    pthread_mutex_lock(&server->endpoint_mutex);
    
    server_endpoint_t* prev = NULL;
    server_endpoint_t* current = server->endpoints;
    
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
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Remove from linked list
    if (prev) {
        prev->next = current->next;
    } else {
        server->endpoints = current->next;
    }
    
    server->endpoint_count--;
    
    // Update statistics
    pthread_mutex_lock(&server->server_mutex);
    server->stats.disconnections++;
    server->stats.active_connections--;
    pthread_mutex_unlock(&server->server_mutex);
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Notify connection callback
    if (server->connection_callback) {
        server->connection_callback(server, endpoint, false, server->user_data);
    }
    
    // Trigger disconnect event
    trigger_server_event(server, endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
    
    // Close endpoint
    polycall_core_error_t result = polycall_endpoint_close(ctx, endpoint);
    
    // Free server endpoint
    polycall_core_free(ctx, current);
    
    return result;
}

/**
 * @brief Send a packet to a client
 */
polycall_core_error_t polycall_network_server_send(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_endpoint_t* endpoint,
    polycall_network_packet_t* packet,
    uint32_t timeout_ms
) {
    if (!ctx || !server || !endpoint || !packet) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find server endpoint
    pthread_mutex_lock(&server->endpoint_mutex);
    
    server_endpoint_t* server_endpoint = NULL;
    server_endpoint_t* current = server->endpoints;
    
    while (current) {
        if (current->endpoint == endpoint) {
            server_endpoint = current;
            break;
        }
        
        current = current->next;
    }
    
    if (!server_endpoint || !server_endpoint->connected) {
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Use default timeout if not specified
    if (timeout_ms == 0) {
        timeout_ms = server->config.operation_timeout_ms;
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
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set send timeout", server->user_data);
        }
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    #else
    struct timeval send_timeout;
    send_timeout.tv_sec = timeout_ms / 1000;
    send_timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout)) != 0) {
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set send timeout", server->user_data);
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
            if (server->error_callback) {
                server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Send error", server->user_data);
            }
            
            // Mark endpoint as disconnected
            pthread_mutex_lock(&server->endpoint_mutex);
            server_endpoint->connected = false;
            pthread_mutex_unlock(&server->endpoint_mutex);
            
            return POLYCALL_CORE_ERROR_OPERATION_FAILED;
        }
        
        sent += n;
    }
    
    // Update statistics
    pthread_mutex_lock(&server->server_mutex);
    server->stats.bytes_sent += data_size;
    server->stats.packets_sent++;
    pthread_mutex_unlock(&server->server_mutex);
    
    // Update last activity time
    pthread_mutex_lock(&server->endpoint_mutex);
    server_endpoint->last_activity = time(NULL);
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Trigger data sent event
    trigger_server_event(server, endpoint, POLYCALL_NETWORK_EVENT_DATA_SENT, NULL);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Receive a packet from a client
 */
polycall_core_error_t polycall_network_server_receive(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_endpoint_t* endpoint,
    polycall_network_packet_t** packet,
    uint32_t timeout_ms
) {
    if (!ctx || !server || !endpoint || !packet) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find server endpoint
    pthread_mutex_lock(&server->endpoint_mutex);
    
    server_endpoint_t* server_endpoint = NULL;
    server_endpoint_t* current = server->endpoints;
    
    while (current) {
        if (current->endpoint == endpoint) {
            server_endpoint = current;
            break;
        }
        
        current = current->next;
    }
    
    if (!server_endpoint || !server_endpoint->connected) {
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Use default timeout if not specified
    if (timeout_ms == 0) {
        timeout_ms = server->config.operation_timeout_ms;
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
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set receive timeout", server->user_data);
        }
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    #else
    struct timeval recv_timeout;
    recv_timeout.tv_sec = timeout_ms / 1000;
    recv_timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) != 0) {
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Failed to set receive timeout", server->user_data);
        }
        return POLYCALL_CORE_ERROR_OPERATION_FAILED;
    }
    #endif
    
    // Allocate receive buffer
    size_t buffer_size = server->config.max_message_size > 0 ? 
                        server->config.max_message_size : DEFAULT_BUFFER_SIZE;
    
    void* buffer = polycall_core_malloc(ctx, buffer_size);
    if (!buffer) {
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_OUT_OF_MEMORY, "Failed to allocate receive buffer", server->user_data);
        }
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Receive data
    ssize_t received = recv(sock, buffer, buffer_size, 0);
    
    if (received <= 0) {
        polycall_core_free(ctx, buffer);
        
        if (received == 0) {
            // Connection closed
            pthread_mutex_lock(&server->endpoint_mutex);
            server_endpoint->connected = false;
            pthread_mutex_unlock(&server->endpoint_mutex);
            
            return POLYCALL_CORE_ERROR_CONNECTION_CLOSED;
        } else {
            // Receive error or timeout
            #ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT) {
                return POLYCALL_CORE_ERROR_TIMEOUT;
            }
            #else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return POLYCALL_CORE_ERROR_TIMEOUT;
            }
            #endif
            
            if (server->error_callback) {
                server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, "Receive error", server->user_data);
            }
            
            return POLYCALL_CORE_ERROR_OPERATION_FAILED;
        }
    }
    
    // Create packet from received data
    result = polycall_network_packet_create_from_data(
        ctx, packet, buffer, received, true); // Take ownership of buffer
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_core_free(ctx, buffer);
        
        if (server->error_callback) {
            server->error_callback(server, result, "Failed to create packet from received data", server->user_data);
        }
        
        return result;
    }
    
    // Update statistics
    pthread_mutex_lock(&server->server_mutex);
    server->stats.bytes_received += received;
    server->stats.packets_received++;
    pthread_mutex_unlock(&server->server_mutex);
    
    // Update last activity time
    pthread_mutex_lock(&server->endpoint_mutex);
    server_endpoint->last_activity = time(NULL);
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Trigger data received event
    trigger_server_event(server, endpoint, POLYCALL_NETWORK_EVENT_DATA_RECEIVED, *packet);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Send a protocol message to a client
 */
polycall_core_error_t polycall_network_server_send_message(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_protocol_context_t* proto_ctx,
    polycall_endpoint_t* endpoint,
    polycall_message_t* message,
    uint32_t timeout_ms,
    polycall_message_t** response
) {
    if (!ctx || !server || !proto_ctx || !endpoint || !message) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Use default timeout if not specified
    if (timeout_ms == 0) {
        timeout_ms = server->config.operation_timeout_ms;
    }
    
    // Serialize protocol message to a packet
    void* message_data = NULL;
    size_t message_size = 0;
    
    polycall_core_error_t result = polycall_protocol_serialize_message(
        ctx, proto_ctx, message, &message_data, &message_size);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        if (server->error_callback) {
            server->error_callback(server, result, "Failed to serialize protocol message", server->user_data);
        }
        
        return result;
    }
    
    // Create packet
    polycall_network_packet_t* packet = NULL;
    result = polycall_network_packet_create_from_data(
        ctx, &packet, message_data, message_size, true); // Take ownership of message_data
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_core_free(ctx, message_data);
        
        if (server->error_callback) {
            server->error_callback(server, result, "Failed to create packet from message data", server->user_data);
        }
        
        return result;
    }
    
    // Set packet type to protocol message
    polycall_network_packet_set_type(ctx, packet, 1); // 1 = protocol message
    
    // Update statistics before sending
    pthread_mutex_lock(&server->server_mutex);
    server->stats.messages_sent++;
    pthread_mutex_unlock(&server->server_mutex);
    
    // Send packet
    result = polycall_network_server_send(
        ctx, server, endpoint, packet, timeout_ms);
    
    // Clean up packet
    polycall_network_packet_destroy(ctx, packet);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Wait for response if requested
    if (response) {
        polycall_network_packet_t* response_packet = NULL;
        
        // Receive response packet
        result = polycall_network_server_receive(
            ctx, server, endpoint, &response_packet, timeout_ms);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            return result;
        }
        
        // Get packet data
        void* response_data = NULL;
        size_t response_size = 0;
        
        result = polycall_network_packet_get_data(
            ctx, response_packet, &response_data, &response_size);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_network_packet_destroy(ctx, response_packet);
            return result;
        }
        
        // Deserialize response
        result = polycall_protocol_deserialize_message(
            ctx, proto_ctx, response_data, response_size, response);
        
        // Clean up response packet
        polycall_network_packet_destroy(ctx, response_packet);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            if (server->error_callback) {
                server->error_callback(server, result, "Failed to deserialize response message", server->user_data);
            }
            
            return result;
        }
        
        // Update statistics
        pthread_mutex_lock(&server->server_mutex);
        server->stats.messages_received++;
        pthread_mutex_unlock(&server->server_mutex);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Broadcast a packet to all connected clients
 */
polycall_core_error_t polycall_network_server_broadcast(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_network_packet_t* packet,
    uint32_t timeout_ms
) {
    if (!ctx || !server || !packet) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Use default timeout if not specified
    if (timeout_ms == 0) {
        timeout_ms = server->config.operation_timeout_ms;
    }
    
    // Get a copy of all connected endpoints
    pthread_mutex_lock(&server->endpoint_mutex);
    
    // Count connected endpoints
    uint32_t connected_count = 0;
    server_endpoint_t* current = server->endpoints;
    while (current) {
        if (current->connected) {
            connected_count++;
        }
        current = current->next;
    }
    
    if (connected_count == 0) {
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_SUCCESS; // No connected endpoints, not an error
    }
    
    // Allocate array for connected endpoints
    polycall_endpoint_t** endpoints = polycall_core_malloc(
        ctx, connected_count * sizeof(polycall_endpoint_t*));
    
    if (!endpoints) {
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Fill array with connected endpoints
    uint32_t index = 0;
    current = server->endpoints;
    while (current && index < connected_count) {
        if (current->connected) {
            endpoints[index++] = current->endpoint;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Track success of broadcast
    bool all_success = true;
    
    // Send to each endpoint
    for (uint32_t i = 0; i < connected_count; i++) {
        polycall_core_error_t result = polycall_network_server_send(
            ctx, server, endpoints[i], packet, timeout_ms);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            all_success = false;
            // Continue with other endpoints even if one fails
        }
    }
    
    // Free endpoint array
    polycall_core_free(ctx, endpoints);
    
    return all_success ? POLYCALL_CORE_SUCCESS : POLYCALL_CORE_ERROR_PARTIAL_FAILURE;
}

/**
 * @brief Register a message handler for specific message types
 */
polycall_core_error_t polycall_network_server_register_handler(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    uint32_t message_type,
    polycall_message_handler_t handler,
    void* user_data
) {
    if (!ctx || !server || !handler) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Check if we have reached the maximum number of handlers
    if (server->message_handler_count >= MAX_MESSAGE_HANDLERS) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Check if this message type already has a handler
    for (uint32_t i = 0; i < server->message_handler_count; i++) {
        if (server->message_handlers[i].message_type == message_type) {
            // Update existing handler
            server->message_handlers[i].handler = handler;
            server->message_handlers[i].user_data = user_data;
            
            pthread_mutex_unlock(&server->server_mutex);
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    // Add new handler
    server->message_handlers[server->message_handler_count].message_type = message_type;
    server->message_handlers[server->message_handler_count].handler = handler;
    server->message_handlers[server->message_handler_count].user_data = user_data;
    server->message_handler_count++;
    
    pthread_mutex_unlock(&server->server_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get all connected endpoints
 */
polycall_core_error_t polycall_network_server_get_endpoints(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_endpoint_t** endpoints,
    size_t* count
) {
    if (!ctx || !server || !count) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->endpoint_mutex);
    
    // Count connected endpoints
    uint32_t connected_count = 0;
    server_endpoint_t* current = server->endpoints;
    while (current) {
        if (current->connected) {
            connected_count++;
        }
        current = current->next;
    }
    
    // If caller just wants the count
    if (!endpoints) {
        *count = connected_count;
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Check if buffer is large enough
    if (*count < connected_count) {
        *count = connected_count;
        pthread_mutex_unlock(&server->endpoint_mutex);
        return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Fill array with connected endpoints
    uint32_t index = 0;
    current = server->endpoints;
    while (current && index < connected_count) {
        if (current->connected) {
            endpoints[index++] = current->endpoint;
        }
        current = current->next;
    }
    
    *count = connected_count;
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get server statistics
 */
polycall_core_error_t polycall_network_server_get_stats(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_network_stats_t* stats
) {
    if (!ctx || !server || !stats) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Update dynamic statistics
    server->stats.uptime_seconds = time(NULL) - server->stats.start_time;
    server->stats.active_connections = server->endpoint_count;
    
    // Copy statistics
    memcpy(stats, &server->stats, sizeof(polycall_network_stats_t));
    
    pthread_mutex_unlock(&server->server_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Set server option
 */
polycall_core_error_t polycall_network_server_set_option(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_network_option_t option,
    const void* value,
    size_t size
) {
    if (!ctx || !server || !value) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
    
    switch (option) {
        case POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE:
            if (size != sizeof(int)) {
                result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
                break;
            }
            
            // Apply to listen socket if available
            if (server->listen_socket != SOCKET_ERROR_VALUE) {
                int buffer_size = *(int*)value;
                
                if (setsockopt(server->listen_socket, SOL_SOCKET, SO_RCVBUF, 
                              (const char*)&buffer_size, sizeof(buffer_size)) != 0 ||
                    setsockopt(server->listen_socket, SOL_SOCKET, SO_SNDBUF, 
                              (const char*)&buffer_size, sizeof(buffer_size)) != 0) {
                    result = POLYCALL_CORE_ERROR_OPERATION_FAILED;
                }
            }
            break;
            
        case POLYCALL_NETWORK_OPTION_SOCKET_TIMEOUT:
            if (size != sizeof(int)) {
                result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
                break;
            }
            
            // Update config timeout
            server->config.operation_timeout_ms = *(int*)value;
            break;
            
        case POLYCALL_NETWORK_OPTION_KEEP_ALIVE:
            if (size != sizeof(int)) {
                result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
                break;
            }
            
            // Apply to listen socket if available
            if (server->listen_socket != SOCKET_ERROR_VALUE) {
                int keep_alive = *(int*)value;
                
                if (setsockopt(server->listen_socket, SOL_SOCKET, SO_KEEPALIVE, 
                              (const char*)&keep_alive, sizeof(keep_alive)) != 0) {
                    result = POLYCALL_CORE_ERROR_OPERATION_FAILED;
                }
            }
            break;
            
        case POLYCALL_NETWORK_OPTION_REUSE_ADDRESS:
            if (size != sizeof(int)) {
                result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
                break;
            }
            
            // Apply to listen socket if available (and not already bound)
            if (server->listen_socket != SOCKET_ERROR_VALUE && !server->running) {
                int reuse = *(int*)value;
                
                if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, 
                              (const char*)&reuse, sizeof(reuse)) != 0) {
                    result = POLYCALL_CORE_ERROR_OPERATION_FAILED;
                }
            }
            break;
            
        case POLYCALL_NETWORK_OPTION_TLS_CONTEXT:
            if (size != sizeof(void*)) {
                result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
                break;
            }
            
            // Update TLS context
            server->tls_context = *(void**)value;
            server->config.enable_tls = (server->tls_context != NULL);
            break;
            
        default:
            result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
            break;
    }
    
    pthread_mutex_unlock(&server->server_mutex);
    
    return result;
}

/**
 * @brief Get server option
 */
polycall_core_error_t polycall_network_server_get_option(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_network_option_t option,
    void* value,
    size_t* size
) {
    if (!ctx || !server || !value || !size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
    
    switch (option) {
        case POLYCALL_NETWORK_OPTION_SOCKET_BUFFER_SIZE:
            if (*size < sizeof(int)) {
                result = POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                *size = sizeof(int);
                break;
            }
            
            // Get from listen socket if available
            if (server->listen_socket != SOCKET_ERROR_VALUE) {
                int buffer_size = 0;
                socklen_t optlen = sizeof(buffer_size);
                
                if (getsockopt(server->listen_socket, SOL_SOCKET, SO_RCVBUF, 
                              (char*)&buffer_size, &optlen) != 0) {
                    result = POLYCALL_CORE_ERROR_OPERATION_FAILED;
                    break;
                }
                
                *(int*)value = buffer_size;
                *size = sizeof(int);
            } else {
                // Default value
                *(int*)value = DEFAULT_BUFFER_SIZE;
                *size = sizeof(int);
            }
            break;
            
        case POLYCALL_NETWORK_OPTION_SOCKET_TIMEOUT:
            if (*size < sizeof(int)) {
                result = POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                *size = sizeof(int);
                break;
            }
            
            // Return config timeout
            *(int*)value = server->config.operation_timeout_ms;
            *size = sizeof(int);
            break;
            
        case POLYCALL_NETWORK_OPTION_KEEP_ALIVE:
            if (*size < sizeof(int)) {
                result = POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                *size = sizeof(int);
                break;
            }
            
            // Get from listen socket if available
            if (server->listen_socket != SOCKET_ERROR_VALUE) {
                int keep_alive = 0;
                socklen_t optlen = sizeof(keep_alive);
                
                if (getsockopt(server->listen_socket, SOL_SOCKET, SO_KEEPALIVE, 
                              (char*)&keep_alive, &optlen) != 0) {
                    result = POLYCALL_CORE_ERROR_OPERATION_FAILED;
                    break;
                }
                
                *(int*)value = keep_alive;
                *size = sizeof(int);
            } else {
                // Default value
                *(int*)value = 0;
                *size = sizeof(int);
            }
            break;
            
        case POLYCALL_NETWORK_OPTION_REUSE_ADDRESS:
            if (*size < sizeof(int)) {
                result = POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                *size = sizeof(int);
                break;
            }
            
            // Get from listen socket if available
            if (server->listen_socket != SOCKET_ERROR_VALUE) {
                int reuse = 0;
                socklen_t optlen = sizeof(reuse);
                
                if (getsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, 
                              (char*)&reuse, &optlen) != 0) {
                    result = POLYCALL_CORE_ERROR_OPERATION_FAILED;
                    break;
                }
                
                *(int*)value = reuse;
                *size = sizeof(int);
            } else {
                // Default value
                *(int*)value = 0;
                *size = sizeof(int);
            }
            break;
            
        case POLYCALL_NETWORK_OPTION_TLS_CONTEXT:
            if (*size < sizeof(void*)) {
                result = POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                *size = sizeof(void*);
                break;
            }
            
            // Return TLS context
            *(void**)value = server->tls_context;
            *size = sizeof(void*);
            break;
            
        default:
            result = POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
            break;
    }
    
    pthread_mutex_unlock(&server->server_mutex);
    
    return result;
}

/**
 * @brief Set server event callback
 */
polycall_core_error_t polycall_network_server_set_event_callback(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    polycall_network_event_t event_type,
    void (*callback)(polycall_network_server_t* server, polycall_endpoint_t* endpoint, void* event_data, void* user_data),
    void* user_data
) {
    if (!ctx || !server || !callback) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Check if we have reached the maximum number of handlers
    if (server->event_handler_count >= sizeof(server->event_handlers) / sizeof(server->event_handlers[0])) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Check if this event type already has a handler
    for (uint32_t i = 0; i < server->event_handler_count; i++) {
        if (server->event_handlers[i].event_type == event_type) {
            // Update existing handler
            server->event_handlers[i].handler = callback;
            server->event_handlers[i].user_data = user_data;
            
            pthread_mutex_unlock(&server->server_mutex);
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    // Add new handler
    server->event_handlers[server->event_handler_count].event_type = event_type;
    server->event_handlers[server->event_handler_count].handler = callback;
    server->event_handlers[server->event_handler_count].user_data = user_data;
    server->event_handler_count++;
    
    pthread_mutex_unlock(&server->server_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Process pending events
 */
polycall_core_error_t polycall_network_server_process_events(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    uint32_t timeout_ms
) {
    if (!ctx || !server) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // The I/O thread automatically processes events in the background
    // This function provides an additional opportunity to process events
    // and check for idle timeouts
    
    pthread_mutex_lock(&server->server_mutex);
    
    // Check if server is running
    if (!server->running) {
        pthread_mutex_unlock(&server->server_mutex);
        return POLYCALL_CORE_SUCCESS; // Not running, nothing to do
    }
    
    pthread_mutex_unlock(&server->server_mutex);
    
    // Check for idle timeouts
    if (server->config.idle_timeout_ms > 0) {
        time_t now = time(NULL);
        time_t idle_timeout_seconds = server->config.idle_timeout_ms / 1000;
        
        pthread_mutex_lock(&server->endpoint_mutex);
        
        server_endpoint_t* current = server->endpoints;
        server_endpoint_t* prev = NULL;
        
        while (current) {
            if (current->connected && (now - current->last_activity) > idle_timeout_seconds) {
                // Endpoint has timed out
                server_endpoint_t* to_disconnect = current;
                
                // Update linked list
                if (prev) {
                    prev->next = current->next;
                    current = current->next;
                } else {
                    server->endpoints = current->next;
                    current = server->endpoints;
                }
                
                server->endpoint_count--;
                
                // Update statistics
                pthread_mutex_lock(&server->server_mutex);
                server->stats.disconnections++;
                server->stats.active_connections--;
                pthread_mutex_unlock(&server->server_mutex);
                
                // Trigger disconnect event
                trigger_server_event(server, to_disconnect->endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
                polycall_network_server_disconnect(ctx, server, to_disconnect->endpoint);
                // endpoint structure is already freed in polycall_network_server_disconnect
            } else {
                prev = current;
                current = current->next;
            }
        }
        pthread_mutex_unlock(&server->endpoint_mutex);
    }
    // Process other events
    // This could include checking for new connections, data received, etc.
    // For now, we'll just return success
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up server resources
 */
void polycall_network_server_cleanup(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server
) {
    if (!ctx || !server) {
        return;
    }
    
    // Stop server if running
    if (server->running) {
        polycall_network_server_stop(ctx, server);
    }
    
    // Free resources
    if (server->worker_threads) {
        polycall_core_free(ctx, server->worker_threads);
    }
    
    // Clean up mutexes and condition variables
    pthread_cond_destroy(&server->server_cond);
    pthread_mutex_destroy(&server->endpoint_mutex);
    pthread_mutex_destroy(&server->server_mutex);
    
    // Clean up TLS context if any
    if (server->tls_context) {
        // In a real implementation, we would call the appropriate TLS cleanup function
        // For now, we'll just set it to NULL
        server->tls_context = NULL;
    }
    
    // Free server structure
    polycall_core_free(ctx, server);
}

/**
 * @brief Create default server configuration
 */
polycall_network_server_config_t polycall_network_server_create_default_config(void) {
    polycall_network_server_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.port = 8080;                   // Default port
    config.bind_address = NULL;           // Any address
    config.backlog = DEFAULT_BACKLOG;     // Default backlog
    config.max_connections = 100;         // Default max connections
    config.connection_timeout_ms = 5000;  // 5 seconds
    config.operation_timeout_ms = 30000;  // 30 seconds
    config.idle_timeout_ms = 300000;      // 5 minutes
    config.enable_tls = false;            // Disable TLS by default
    config.max_message_size = 1048576;    // 1MB default
    config.io_thread_count = 1;           // Single I/O thread by default
    config.worker_thread_count = 0;       // Auto-detect worker threads
    config.enable_protocol_dispatch = true; // Enable protocol dispatch
    
    return config;
}

/**
 * @brief I/O thread function for accepting connections and handling I/O
 */
static void* io_thread_func(void* arg) {
    polycall_network_server_t* server = (polycall_network_server_t*)arg;
    polycall_core_context_t* ctx = server->core_ctx;
    
    fd_set read_set, write_set, error_set;
    struct timeval tv;
    socket_handle_t max_fd;
    
    while (server->io_thread_active) {
        // Initialize fd sets
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&error_set);
        
        // Add listen socket to read set if running
        pthread_mutex_lock(&server->server_mutex);
        bool is_running = server->running;
        socket_handle_t listen_socket = server->listen_socket;
        pthread_mutex_unlock(&server->server_mutex);
        
        if (is_running && listen_socket != SOCKET_ERROR_VALUE) {
            FD_SET(listen_socket, &read_set);
            max_fd = listen_socket;
        } else {
            max_fd = 0;
        }
        
        // Add client sockets to fd sets
        pthread_mutex_lock(&server->endpoint_mutex);
        
        server_endpoint_t* current = server->endpoints;
        while (current) {
            if (current->connected) {
                polycall_endpoint_info_t info;
                if (polycall_endpoint_get_info(ctx, current->endpoint, &info) == POLYCALL_CORE_SUCCESS) {
                    socket_handle_t sock = (socket_handle_t)(uintptr_t)info.socket_handle;
                    
                    if (sock != SOCKET_ERROR_VALUE) {
                        FD_SET(sock, &read_set);
                        FD_SET(sock, &error_set);
                        
                        if (sock > max_fd) {
                            max_fd = sock;
                        }
                    }
                }
            }
            
            current = current->next;
        }
        
        pthread_mutex_unlock(&server->endpoint_mutex);
        
        // Set timeout
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms
        
        // Wait for activity
        if (max_fd > 0) {
            int select_result = select(max_fd + 1, &read_set, &write_set, &error_set, &tv);
            
            if (select_result > 0) {
                // Check listen socket
                if (is_running && listen_socket != SOCKET_ERROR_VALUE && 
                    FD_ISSET(listen_socket, &read_set)) {
                    // Accept new connection
                    polycall_endpoint_t* new_endpoint;
                    if (polycall_network_server_accept(ctx, server, &new_endpoint, 0) != POLYCALL_CORE_SUCCESS) {
                        // Accept failed, but continue processing
                    }
                }
                
                // Check client sockets
                pthread_mutex_lock(&server->endpoint_mutex);
                
                current = server->endpoints;
                while (current) {
                    if (current->connected) {
                        polycall_endpoint_info_t info;
                        if (polycall_endpoint_get_info(ctx, current->endpoint, &info) == POLYCALL_CORE_SUCCESS) {
                            socket_handle_t sock = (socket_handle_t)(uintptr_t)info.socket_handle;
                            
                            if (sock != SOCKET_ERROR_VALUE) {
                                if (FD_ISSET(sock, &read_set)) {
                                    // Data available to read
                                    server_endpoint_t* to_process = current;
                                    
                                    // Move to next before processing
                                    current = current->next;
                                    
                                    // Release lock during processing
                                    pthread_mutex_unlock(&server->endpoint_mutex);
                                    
                                    // Process incoming data
                                    if (process_incoming_data(ctx, server, to_process) != POLYCALL_CORE_SUCCESS) {
                                        // Processing failed, disconnect endpoint
                                        polycall_network_server_disconnect(ctx, server, to_process->endpoint);
                                    }
                                    
                                    // Reacquire lock
                                    pthread_mutex_lock(&server->endpoint_mutex);
                                    continue;
                                }
                                
                                if (FD_ISSET(sock, &error_set)) {
                                    // Socket error
                                    server_endpoint_t* to_disconnect = current;
                                    
                                    // Move to next before disconnecting
                                    current = current->next;
                                    
                                    // Release lock during disconnect
                                    pthread_mutex_unlock(&server->endpoint_mutex);
                                    
                                    // Disconnect endpoint
                                    polycall_network_server_disconnect(ctx, server, to_disconnect->endpoint);
                                    
                                    // Reacquire lock
                                    pthread_mutex_lock(&server->endpoint_mutex);
                                    continue;
                                }
                            }
                        }
                    }
                    
                    current = current->next;
                }
                
                pthread_mutex_unlock(&server->endpoint_mutex);
            } else if (select_result < 0) {
                // Select error
                if (server->error_callback) {
                    server->error_callback(server, POLYCALL_CORE_ERROR_OPERATION_FAILED, 
                                          "Select error in I/O thread", server->user_data);
                }
                
                // Sleep briefly to avoid busy loop on error
                usleep(10000);
            }
        } else {
            // No sockets to process, sleep briefly
            usleep(10000);
        }
        
        // Check for idle timeouts
        if (server->config.idle_timeout_ms > 0) {
            time_t now = time(NULL);
            time_t idle_timeout_seconds = server->config.idle_timeout_ms / 1000;
            
            pthread_mutex_lock(&server->endpoint_mutex);
            
            server_endpoint_t* current = server->endpoints;
            server_endpoint_t* prev = NULL;
            
            while (current) {
                if (current->connected && (now - current->last_activity) > idle_timeout_seconds) {
                    // Endpoint has timed out
                    server_endpoint_t* to_disconnect = current;
                    
                    // Update linked list
                    if (prev) {
                        prev->next = current->next;
                        current = current->next;
                    } else {
                        server->endpoints = current->next;
                        current = server->endpoints;
                    }
                    
                    server->endpoint_count--;
                    
                    // Update statistics
                    pthread_mutex_lock(&server->server_mutex);
                    server->stats.disconnections++;
                    server->stats.active_connections--;
                    pthread_mutex_unlock(&server->server_mutex);
                    
                    // Release endpoint mutex during disconnect
                    pthread_mutex_unlock(&server->endpoint_mutex);
                    
                    // Notify connection callback
                    if (server->connection_callback) {
                        server->connection_callback(server, to_disconnect->endpoint, false, server->user_data);
                    }
                    
                    // Trigger disconnect event
                    trigger_server_event(server, to_disconnect->endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
                    
                    // Close endpoint
                    polycall_endpoint_close(ctx, to_disconnect->endpoint);
                    
                    // Free endpoint structure
                    polycall_core_free(ctx, to_disconnect);
                    
                    // Reacquire endpoint mutex
                    pthread_mutex_lock(&server->endpoint_mutex);
                    continue;
                }
                
                prev = current;
                current = current->next;
            }
            
            pthread_mutex_unlock(&server->endpoint_mutex);
        }
    }
    
    return NULL;
}

/**
 * @brief Worker thread function for processing requests
 */
static void* worker_thread_func(void* arg) {
    worker_thread_t* worker = (worker_thread_t*)arg;
    polycall_network_server_t* server = (polycall_network_server_t*)worker->thread_data;
    polycall_core_context_t* ctx = server->core_ctx;
    
    while (worker->active) {
        // Wait for work items or shutdown signal
        pthread_mutex_lock(&server->server_mutex);
        
        // Wait for condition variable signal
        pthread_cond_wait(&server->server_cond, &server->server_mutex);
        
        // Check if thread should exit
        if (!worker->active) {
            pthread_mutex_unlock(&server->server_mutex);
            break;
        }
        
        pthread_mutex_unlock(&server->server_mutex);
        
        // Actual work would be done here
        // For now, workers are only used for protocol message processing
        // which is triggered from endpoint events
    }
    
    return NULL;
}

/**
 * @brief Handle endpoint events
 */
static void handle_endpoint_event(
    polycall_endpoint_t* endpoint,
    void* event_data,
    void* user_data
) {
    polycall_network_server_t* server = (polycall_network_server_t*)user_data;
    polycall_core_context_t* ctx = server->core_ctx;
    
    // Find server endpoint for this endpoint
    pthread_mutex_lock(&server->endpoint_mutex);
    
    server_endpoint_t* server_endpoint = NULL;
    server_endpoint_t* current = server->endpoints;
    
    while (current) {
        if (current->endpoint == endpoint) {
            server_endpoint = current;
            break;
        }
        
        current = current->next;
    }
    
    if (!server_endpoint) {
        pthread_mutex_unlock(&server->endpoint_mutex);
        return;
    }
    
    // Update last activity time
    server_endpoint->last_activity = time(NULL);
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    
    // Get event type
    polycall_network_event_t event_type = POLYCALL_NETWORK_EVENT_NONE;
    polycall_endpoint_info_t info;
    
    if (polycall_endpoint_get_info(ctx, endpoint, &info) == POLYCALL_CORE_SUCCESS) {
        // Determine event type based on endpoint state
        if (info.state == POLYCALL_ENDPOINT_STATE_DISCONNECTED) {
            event_type = POLYCALL_NETWORK_EVENT_DISCONNECT;
        } else if (event_data != NULL) {
            event_type = POLYCALL_NETWORK_EVENT_DATA_RECEIVED;
        } else {
            event_type = POLYCALL_NETWORK_EVENT_ERROR;
        }
    }
    
    // Trigger server event
    trigger_server_event(server, endpoint, event_type, event_data);
    
    // Handle disconnect event
    if (event_type == POLYCALL_NETWORK_EVENT_DISCONNECT) {
        polycall_network_server_disconnect(ctx, server, endpoint);
    }
    
    // Handle data received event
    if (event_type == POLYCALL_NETWORK_EVENT_DATA_RECEIVED && 
        server->config.enable_protocol_dispatch) {
        // Process data
        process_incoming_data(ctx, server, server_endpoint);
    }
}

/**
 * @brief Trigger server event
 */
static void trigger_server_event(
    polycall_network_server_t* server,
    polycall_endpoint_t* endpoint,
    polycall_network_event_t event_type,
    void* event_data
) {
    // Call registered event handlers
    pthread_mutex_lock(&server->server_mutex);
    
    for (uint32_t i = 0; i < server->event_handler_count; i++) {
        if (server->event_handlers[i].event_type == event_type && 
            server->event_handlers[i].handler != NULL) {
            // Call handler outside of lock
            pthread_mutex_unlock(&server->server_mutex);
            server->event_handlers[i].handler(server, endpoint, event_data, 
                                             server->event_handlers[i].user_data);
            pthread_mutex_lock(&server->server_mutex);
        }
    }
    
    pthread_mutex_unlock(&server->server_mutex);
}

/**
 * @brief Process incoming data
 */
static polycall_core_error_t process_incoming_data(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    server_endpoint_t* server_endpoint
) {
    if (!ctx || !server || !server_endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if protocol dispatch is enabled
    if (!server->config.enable_protocol_dispatch) {
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Receive packet
    polycall_network_packet_t* packet = NULL;
    polycall_core_error_t result = polycall_network_server_receive(
        ctx, server, server_endpoint->endpoint, &packet, 0);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        if (result == POLYCALL_CORE_ERROR_TIMEOUT) {
            // No data available, not an error
            return POLYCALL_CORE_SUCCESS;
        }
        
        return result;
    }
    
    // Get packet type
    uint16_t packet_type = 0;
    result = polycall_network_packet_get_type(ctx, packet, &packet_type);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    // Get packet data
    void* data = NULL;
    size_t data_size = 0;
    
    result = polycall_network_packet_get_data(ctx, packet, &data, &data_size);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_network_packet_destroy(ctx, packet);
        return result;
    }
    
    // Handle protocol message if applicable
    if (packet_type == 1 && server->proto_ctx != NULL) { // 1 = protocol message
        // Deserialize message
        polycall_message_t* message = NULL;
        result = polycall_protocol_deserialize_message(
            ctx, server->proto_ctx, data, data_size, &message);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_network_packet_destroy(ctx, packet);
            return result;
        }
        
        // Find handler for message type
        uint32_t message_type = 0;
        result = polycall_protocol_get_message_type(
            ctx, server->proto_ctx, message, &message_type);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_protocol_destroy_message(ctx, server->proto_ctx, message);
            polycall_network_packet_destroy(ctx, packet);
            return result;
        }
        
        // Look for specific handler
        polycall_message_handler_t handler = NULL;
        void* handler_data = NULL;
        
        pthread_mutex_lock(&server->server_mutex);
        
        for (uint32_t i = 0; i < server->message_handler_count; i++) {
            if (server->message_handlers[i].message_type == message_type) {
                handler = server->message_handlers[i].handler;
                handler_data = server->message_handlers[i].user_data;
                break;
            }
        }
        
        // If no specific handler, try default handler
        if (!handler && server->config.message_handler) {
            handler = server->config.message_handler;
            handler_data = server->user_data;
        }
        
        pthread_mutex_unlock(&server->server_mutex);
        
        // Call handler if found
        if (handler) {
            polycall_message_t* response = NULL;
            
            result = handler(ctx, server->proto_ctx, server_endpoint->endpoint, 
                            message, &response, handler_data);
            
            if (result == POLYCALL_CORE_SUCCESS && response != NULL) {
                // Send response
                polycall_network_server_send_message(
                    ctx, server, server->proto_ctx, server_endpoint->endpoint, 
                    response, 0, NULL);
                
                // Clean up response
                polycall_protocol_destroy_message(ctx, server->proto_ctx, response);
            }
        }
        
        // Clean up message
        polycall_protocol_destroy_message(ctx, server->proto_ctx, message);
    }
    
    // Clean up packet
    polycall_network_packet_destroy(ctx, packet);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up endpoint
 */
static void cleanup_endpoint(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server,
    server_endpoint_t* server_endpoint
) {
    if (!ctx || !server || !server_endpoint) {
        return;
    }
    
    // Notify connection callback
    if (server->connection_callback) {
        server->connection_callback(server, server_endpoint->endpoint, false, server->user_data);
    }
    
    // Close endpoint
    polycall_endpoint_close(ctx, server_endpoint->endpoint);
    
    // Free endpoint structure
    polycall_core_free(ctx, server_endpoint);
}

/**
 * @brief Initialize socket
 */
static bool initialize_socket(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server
) {
    // Create socket
    server->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (server->listen_socket == SOCKET_ERROR_VALUE) {
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_INITIALIZATION_FAILED, 
                                  "Failed to create socket", server->user_data);
        }
        return false;
    }
    
    // Set socket options
    int option_value = 1;
    
    // Set SO_REUSEADDR
    if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, 
                  (const char*)&option_value, sizeof(option_value)) != 0) {
        CLOSE_SOCKET(server->listen_socket);
        server->listen_socket = SOCKET_ERROR_VALUE;
        
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_INITIALIZATION_FAILED, 
                                  "Failed to set socket options", server->user_data);
        }
        return false;
    }
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->config.port);
    
    if (server->config.bind_address) {
        addr.sin_addr.s_addr = inet_addr(server->config.bind_address);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    if (bind(server->listen_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        CLOSE_SOCKET(server->listen_socket);
        server->listen_socket = SOCKET_ERROR_VALUE;
        
        if (server->error_callback) {
            server->error_callback(server, POLYCALL_CORE_ERROR_INITIALIZATION_FAILED, 
                                  "Failed to bind socket", server->user_data);
        }
        return false;
    }
    
    return true;
}

/**
 * @brief Initialize TLS
 */
static bool initialize_tls(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server
) {
    // In a real implementation, we would initialize TLS context here
    // using a library like OpenSSL
    
    // For this implementation, we'll just set a placeholder context
    server->tls_context = (void*)1;
    
    return true;
}

/**
 * @brief Initialize worker threads
 */
static bool initialize_worker_threads(
    polycall_core_context_t* ctx,
    polycall_network_server_t* server
) {
    // Initialize worker threads
    for (uint32_t i = 0; i < server->worker_thread_count; i++) {
        server->worker_threads[i].active = true;
        server->worker_threads[i].thread_data = server;
        
        if (pthread_create(&server->worker_threads[i].thread_id, NULL, 
                         worker_thread_func, &server->worker_threads[i]) != 0) {
            // Failed to create thread
            
            // Mark all threads as inactive
            for (uint32_t j = 0; j <= i; j++) {
                server->worker_threads[j].active = false;
            }
            
            // Signal all threads to exit
            pthread_cond_broadcast(&server->server_cond);
            
            // Wait for threads to exit
            for (uint32_t j = 0; j < i; j++) {
                if (server->worker_threads[j].thread_id != 0) {
                    pthread_join(server->worker_threads[j].thread_id, NULL);
                    server->worker_threads[j].thread_id = 0;
                }
            }
            
            if (server->error_callback) {
                server->error_callback(server, POLYCALL_CORE_ERROR_INITIALIZATION_FAILED, 
                                      "Failed to create worker thread", server->user_data);
            }
            
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Find endpoint by ID
 */
static server_endpoint_t* find_endpoint_by_id(
    polycall_network_server_t* server,
    uint32_t endpoint_id
) {
    if (!server) {
        return NULL;
    }
    
    pthread_mutex_lock(&server->endpoint_mutex);
    
    server_endpoint_t* current = server->endpoints;
    while (current) {
        if (current->endpoint_id == endpoint_id) {
            pthread_mutex_unlock(&server->endpoint_mutex);
            return current;
        }
        
        current = current->next;
    }
    
    pthread_mutex_unlock(&server->endpoint_mutex);
    return NULL;
}