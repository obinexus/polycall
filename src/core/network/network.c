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
#include "polycall/core/network/network.h"

 * @file network.c
 * @brief Main network module implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the main interface for LibPolyCall's network module,
 * integrating all networking components and providing a unified API.
 */

 #include "polycall/core/network/network.h"

 /**
  * @brief Network event handler for global management
  */
 static void network_event_handler(
     polycall_endpoint_t* endpoint,
     void* event_data,
     void* user_data
 ) {
     polycall_network_context_t* network_ctx = (polycall_network_context_t*)user_data;
     
     if (!network_ctx || !endpoint) {
         return;
     }
     
     // Determine event type from endpoint state
     polycall_endpoint_info_t info;
     polycall_endpoint_get_info(network_ctx->core_ctx, endpoint, &info);
     
     // Trigger appropriate event based on endpoint state
     polycall_network_event_t event_type = POLYCALL_NETWORK_EVENT_NONE;
     
     if (info.state == POLYCALL_ENDPOINT_STATE_CONNECTED) {
         event_type = POLYCALL_NETWORK_EVENT_CONNECT;
     } else if (info.state == POLYCALL_ENDPOINT_STATE_DISCONNECTED) {
         event_type = POLYCALL_NETWORK_EVENT_DISCONNECT;
     } else if (event_data != NULL) {
         event_type = POLYCALL_NETWORK_EVENT_DATA_RECEIVED;
     }
     
     if (event_type != POLYCALL_NETWORK_EVENT_NONE) {
         trigger_event(network_ctx, endpoint, event_type, event_data);
         update_statistics(network_ctx, event_type);
     }
 }
 
 /**
  * @brief Initialize network module
  */
 polycall_core_error_t polycall_network_init(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t** network_ctx,
     const polycall_network_config_t* config
 ) {
     if (!core_ctx || !network_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Ensure network subsystem is initialized
     polycall_core_error_t result = polycall_network_subsystem_init();
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Use default config if not provided
     polycall_network_config_t default_config;
     if (!config) {
         default_config = polycall_network_create_default_config();
         config = &default_config;
     }
     
     // Allocate network context
     polycall_network_context_t* ctx = polycall_core_malloc(core_ctx, sizeof(polycall_network_context_t));
     if (!ctx) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to allocate network context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(ctx, 0, sizeof(polycall_network_context_t));
     ctx->core_ctx = core_ctx;
     ctx->config = *config;
     ctx->flags = POLYCALL_NETWORK_FLAG_SUBSYSTEM_INITIALIZED;
     ctx->start_time = time(NULL);
     
     // Initialize mutexes
     if (pthread_mutex_init(&ctx->thread_mutex, NULL) != 0 ||
         pthread_mutex_init(&ctx->client_mutex, NULL) != 0 ||
         pthread_mutex_init(&ctx->server_mutex, NULL) != 0 ||
         pthread_mutex_init(&ctx->endpoint_mutex, NULL) != 0 ||
         pthread_mutex_init(&ctx->event_mutex, NULL) != 0 ||
         pthread_mutex_init(&ctx->stats_mutex, NULL) != 0 ||
         pthread_cond_init(&ctx->thread_cond, NULL) != 0) {
         
         // Clean up on error
         if (pthread_mutex_destroy(&ctx->thread_mutex) == 0) {
             pthread_mutex_destroy(&ctx->client_mutex);
             pthread_mutex_destroy(&ctx->server_mutex);
             pthread_mutex_destroy(&ctx->endpoint_mutex);
             pthread_mutex_destroy(&ctx->event_mutex);
             pthread_mutex_destroy(&ctx->stats_mutex);
             pthread_cond_destroy(&ctx->thread_cond);
         }
         
         polycall_core_free(core_ctx, ctx);
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to initialize network context mutexes");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Initialize thread pool if requested
     if (config->thread_pool_size > 0) {
         result = initialize_thread_pool(core_ctx, ctx, config->thread_pool_size);
         if (result != POLYCALL_CORE_SUCCESS) {
             // Clean up on error
             pthread_mutex_destroy(&ctx->thread_mutex);
             pthread_mutex_destroy(&ctx->client_mutex);
             pthread_mutex_destroy(&ctx->server_mutex);
             pthread_mutex_destroy(&ctx->endpoint_mutex);
             pthread_mutex_destroy(&ctx->event_mutex);
             pthread_mutex_destroy(&ctx->stats_mutex);
             pthread_cond_destroy(&ctx->thread_cond);
             
             polycall_core_free(core_ctx, ctx);
             
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to initialize thread pool");
             return result;
         }
         
         ctx->flags |= POLYCALL_NETWORK_FLAG_THREADED;
     }
     
     // Initialize TLS context if enabled
     if (config->enable_tls) {
         result = initialize_tls_context(core_ctx, ctx);
         if (result != POLYCALL_CORE_SUCCESS) {
             // Clean up on error
             cleanup_thread_pool(core_ctx, ctx);
             
             pthread_mutex_destroy(&ctx->thread_mutex);
             pthread_mutex_destroy(&ctx->client_mutex);
             pthread_mutex_destroy(&ctx->server_mutex);
             pthread_mutex_destroy(&ctx->endpoint_mutex);
             pthread_mutex_destroy(&ctx->event_mutex);
             pthread_mutex_destroy(&ctx->stats_mutex);
             pthread_cond_destroy(&ctx->thread_cond);
             
             polycall_core_free(core_ctx, ctx);
             
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to initialize TLS context");
             return result;
         }
         
         ctx->flags |= POLYCALL_NETWORK_FLAG_TLS_ENABLED;
     }
     
     // Set compression flag if enabled
     if (config->enable_compression) {
         ctx->flags |= POLYCALL_NETWORK_FLAG_COMPRESSED;
     }
     
     // Set encryption flag if enabled
     if (config->enable_encryption) {
         ctx->flags |= POLYCALL_NETWORK_FLAG_ENCRYPTED;
     }
     
     // Mark as initialized
     ctx->flags |= POLYCALL_NETWORK_FLAG_INITIALIZED;
     
     // Return the network context
     *network_ctx = ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up network module
  */
 void polycall_network_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx
 ) {
     if (!core_ctx || !network_ctx) {
         return;
     }
     
     // Clean up all clients
     pthread_mutex_lock(&network_ctx->client_mutex);
     
     client_entry_t* client_entry = network_ctx->clients;
     while (client_entry) {
         client_entry_t* next = client_entry->next;
         
         polycall_network_client_cleanup(core_ctx, client_entry->client);
         polycall_core_free(core_ctx, client_entry);
         
         client_entry = next;
     }
     
     network_ctx->clients = NULL;
     network_ctx->client_count = 0;
     
     pthread_mutex_unlock(&network_ctx->client_mutex);
     
     // Clean up all servers
     pthread_mutex_lock(&network_ctx->server_mutex);
     
     server_entry_t* server_entry = network_ctx->servers;
     while (server_entry) {
         server_entry_t* next = server_entry->next;
         
         polycall_network_server_cleanup(core_ctx, server_entry->server);
         polycall_core_free(core_ctx, server_entry);
         
         server_entry = next;
     }
     
     network_ctx->servers = NULL;
     network_ctx->server_count = 0;
     
     pthread_mutex_unlock(&network_ctx->server_mutex);
     
     // Clean up all endpoints
     pthread_mutex_lock(&network_ctx->endpoint_mutex);
     
     endpoint_entry_t* endpoint_entry = network_ctx->endpoints;
     while (endpoint_entry) {
         endpoint_entry_t* next = endpoint_entry->next;
         
         polycall_endpoint_close(core_ctx, endpoint_entry->endpoint);
         polycall_core_free(core_ctx, endpoint_entry);
         
         endpoint_entry = next;
     }
     
     network_ctx->endpoints = NULL;
     network_ctx->endpoint_count = 0;
     
     pthread_mutex_unlock(&network_ctx->endpoint_mutex);
     
     // Clean up thread pool
     cleanup_thread_pool(core_ctx, network_ctx);
     
     // Clean up TLS context
     cleanup_tls_context(core_ctx, network_ctx);
     
     // Destroy mutexes and condition variables
     pthread_mutex_destroy(&network_ctx->thread_mutex);
     pthread_mutex_destroy(&network_ctx->client_mutex);
     pthread_mutex_destroy(&network_ctx->server_mutex);
     pthread_mutex_destroy(&network_ctx->endpoint_mutex);
     pthread_mutex_destroy(&network_ctx->event_mutex);
     pthread_mutex_destroy(&network_ctx->stats_mutex);
     pthread_cond_destroy(&network_ctx->thread_cond);
     
     // Free context
     polycall_core_free(core_ctx, network_ctx);
 }
 
 /**
  * @brief Create a client for a specific protocol
  */
 polycall_core_error_t polycall_network_create_client(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_client_t** client,
     const polycall_network_client_config_t* config
 ) {
     if (!core_ctx || !network_ctx || !client) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default config if not provided
     polycall_network_client_config_t default_config;
     if (!config) {
         default_config = polycall_network_client_create_default_config();
         config = &default_config;
     }
     
     // Apply network context settings to client config
     polycall_network_client_config_t modified_config = *config;
     
     if (modified_config.connect_timeout_ms == 0) {
         modified_config.connect_timeout_ms = network_ctx->config.connection_timeout_ms;
     }
     
     if (modified_config.operation_timeout_ms == 0) {
         modified_config.operation_timeout_ms = network_ctx->config.operation_timeout_ms;
     }
     
     if (modified_config.max_message_size == 0) {
         modified_config.max_message_size = network_ctx->config.max_message_size;
     }
     
     // Inherit TLS settings if not specified
     if (!modified_config.enable_tls && (network_ctx->flags & POLYCALL_NETWORK_FLAG_TLS_ENABLED)) {
         modified_config.enable_tls = true;
         modified_config.tls_cert_file = network_ctx->config.tls_cert_file;
         modified_config.tls_key_file = network_ctx->config.tls_key_file;
         modified_config.tls_ca_file = network_ctx->config.tls_ca_file;
     }
     
     // Create the client
     polycall_core_error_t result = polycall_network_client_create(
         core_ctx, proto_ctx, client, &modified_config);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create network client");
         return result;
     }
     
     // Register the client
     result = register_client(core_ctx, network_ctx, *client);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_network_client_cleanup(core_ctx, *client);
         *client = NULL;
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to register network client");
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a server for a specific protocol
  */
 polycall_core_error_t polycall_network_create_server(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_network_server_t** server,
     const polycall_network_server_config_t* config
 ) {
     if (!core_ctx || !network_ctx || !server) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default config if not provided
     polycall_network_server_config_t default_config;
     if (!config) {
         default_config = polycall_network_server_create_default_config();
         config = &default_config;
     }
     
     // Apply network context settings to server config
     polycall_network_server_config_t modified_config = *config;
     
     if (modified_config.connection_timeout_ms == 0) {
         modified_config.connection_timeout_ms = network_ctx->config.connection_timeout_ms;
     }
     
     if (modified_config.operation_timeout_ms == 0) {
         modified_config.operation_timeout_ms = network_ctx->config.operation_timeout_ms;
     }
     
     if (modified_config.max_message_size == 0) {
         modified_config.max_message_size = network_ctx->config.max_message_size;
     }
     
     if (modified_config.max_connections == 0) {
         modified_config.max_connections = network_ctx->config.max_connections;
     }
     
     // Inherit TLS settings if not specified
     if (!modified_config.enable_tls && (network_ctx->flags & POLYCALL_NETWORK_FLAG_TLS_ENABLED)) {
         modified_config.enable_tls = true;
         modified_config.tls_cert_file = network_ctx->config.tls_cert_file;
         modified_config.tls_key_file = network_ctx->config.tls_key_file;
         modified_config.tls_ca_file = network_ctx->config.tls_ca_file;
     }
     
     // Use thread pool if available
     if (network_ctx->flags & POLYCALL_NETWORK_FLAG_THREADED) {
         if (modified_config.worker_thread_count == 0) {
             modified_config.worker_thread_count = network_ctx->worker_thread_count;
         }
     }
     
     // Create the server
     polycall_core_error_t result = polycall_network_server_create(
         core_ctx, proto_ctx, server, &modified_config);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create network server");
         return result;
     }
     
     // Register the server
     result = register_server(core_ctx, network_ctx, *server);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_network_server_cleanup(core_ctx, *server);
         *server = NULL;
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to register network server");
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get network statistics
  */
 polycall_core_error_t polycall_network_get_stats(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_stats_t* stats
 ) {
     if (!core_ctx || !network_ctx || !stats) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&network_ctx->stats_mutex);
     
     // Update dynamic statistics
     network_ctx->stats.uptime_seconds = time(NULL) - network_ctx->start_time;
     
     // Calculate bytes sent/received from all endpoints
     uint64_t total_bytes_sent = 0;
     uint64_t total_bytes_received = 0;
     
     pthread_mutex_lock(&network_ctx->endpoint_mutex);
     
     endpoint_entry_t* endpoint_entry = network_ctx->endpoints;
     while (endpoint_entry) {
         polycall_endpoint_info_t info;
         if (polycall_endpoint_get_info(core_ctx, endpoint_entry->endpoint, &info) == POLYCALL_CORE_SUCCESS) {
             total_bytes_sent += info.bytes_sent;
             total_bytes_received += info.bytes_received;
         }
         
         endpoint_entry = endpoint_entry->next;
     }
     
     pthread_mutex_unlock(&network_ctx->endpoint_mutex);
     
     network_ctx->stats.bytes_sent = total_bytes_sent;
     network_ctx->stats.bytes_received = total_bytes_received;
     
     // Copy statistics to output
     memcpy(stats, &network_ctx->stats, sizeof(polycall_network_stats_t));
     
     // Add current connection counts
     pthread_mutex_lock(&network_ctx->client_mutex);
     stats->active_clients = network_ctx->client_count;
     pthread_mutex_unlock(&network_ctx->client_mutex);
     
     pthread_mutex_lock(&network_ctx->server_mutex);
     stats->active_servers = network_ctx->server_count;
     pthread_mutex_unlock(&network_ctx->server_mutex);
     
     pthread_mutex_lock(&network_ctx->endpoint_mutex);
     stats->active_connections = network_ctx->endpoint_count;
     pthread_mutex_unlock(&network_ctx->endpoint_mutex);
     
     pthread_mutex_unlock(&network_ctx->stats_mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Process network events
  */
 polycall_core_error_t polycall_network_process_events(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     uint32_t timeout_ms
 ) {
     if (!core_ctx || !network_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If using threads, no need to manually process events
     if (network_ctx->flags & POLYCALL_NETWORK_FLAG_THREADED) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Process client events
     pthread_mutex_lock(&network_ctx->client_mutex);
     
     client_entry_t* client_entry = network_ctx->clients;
     while (client_entry) {
         polycall_network_client_process_events(
             core_ctx, client_entry->client, 0); // Non-blocking
         
         client_entry = client_entry->next;
     }
     
     pthread_mutex_unlock(&network_ctx->client_mutex);
     
     // Process server events
     pthread_mutex_lock(&network_ctx->server_mutex);
     
     server_entry_t* server_entry = network_ctx->servers;
     while (server_entry) {
         polycall_network_server_process_events(
             core_ctx, server_entry->server, 0); // Non-blocking
         
         server_entry = server_entry->next;
     }
     
     pthread_mutex_unlock(&network_ctx->server_mutex);
     
     // Sleep if timeout specified
     if (timeout_ms > 0) {
         struct timespec wait_time;
         wait_time.tv_sec = timeout_ms / 1000;
         wait_time.tv_nsec = (timeout_ms % 1000) * 1000000;
         nanosleep(&wait_time, NULL);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register a global network event handler
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
 ) {
     if (!core_ctx || !network_ctx || !handler || 
         event_type >= POLYCALL_NETWORK_EVENT_COUNT) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&network_ctx->event_mutex);
     
     // Check if we've reached the maximum number of handlers
     if (network_ctx->event_handler_counts[event_type] >= MAX_EVENT_HANDLERS) {
         pthread_mutex_unlock(&network_ctx->event_mutex);
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Maximum number of event handlers reached for event type %d",
                           event_type);
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Add the handler
     uint32_t index = network_ctx->event_handler_counts[event_type]++;
     network_ctx->event_handlers[event_type][index].event_type = event_type;
     network_ctx->event_handlers[event_type][index].handler = handler;
     network_ctx->event_handlers[event_type][index].user_data = user_data;
     
     pthread_mutex_unlock(&network_ctx->event_mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set network option
  */
 polycall_core_error_t polycall_network_set_option(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_option_t option,
     const void* value,
     size_t size
 ) {
     if (!core_ctx || !network_ctx || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
     
     switch (option) {
         case POLYCALL_NETWORK_OPTION_TLS_ENABLED:
             if (size != sizeof(bool)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             if (*(const bool*)value) {
                 if (!(network_ctx->flags & POLYCALL_NETWORK_FLAG_TLS_ENABLED)) {
                     // Enable TLS if not already enabled
                     result = initialize_tls_context(core_ctx, network_ctx);
                     if (result == POLYCALL_CORE_SUCCESS) {
                         network_ctx->flags |= POLYCALL_NETWORK_FLAG_TLS_ENABLED;
                     }
                 }
             } else {
                 if (network_ctx->flags & POLYCALL_NETWORK_FLAG_TLS_ENABLED) {
                     // Disable TLS if enabled
                     cleanup_tls_context(core_ctx, network_ctx);
                     network_ctx->flags &= ~POLYCALL_NETWORK_FLAG_TLS_ENABLED;
                 }
             }
             break;
             
         case POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED:
             if (size != sizeof(bool)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             if (*(const bool*)value) {
                 network_ctx->flags |= POLYCALL_NETWORK_FLAG_COMPRESSED;
             } else {
                 network_ctx->flags &= ~POLYCALL_NETWORK_FLAG_COMPRESSED;
             }
             break;
             
         case POLYCALL_NETWORK_OPTION_ENCRYPTION_ENABLED:
             if (size != sizeof(bool)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             if (*(const bool*)value) {
                 network_ctx->flags |= POLYCALL_NETWORK_FLAG_ENCRYPTED;
             } else {
                 network_ctx->flags &= ~POLYCALL_NETWORK_FLAG_ENCRYPTED;
             }
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_CERT_FILE:
             if (!value || size == 0) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             network_ctx->config.tls_cert_file = (const char*)value;
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_KEY_FILE:
             if (!value || size == 0) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             network_ctx->config.tls_key_file = (const char*)value;
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_CA_FILE:
             if (!value || size == 0) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             network_ctx->config.tls_ca_file = (const char*)value;
             break;
             
         case POLYCALL_NETWORK_OPTION_CONNECTION_TIMEOUT:
             if (size != sizeof(uint32_t)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             network_ctx->config.connection_timeout_ms = *(const uint32_t*)value;
             break;
             
         case POLYCALL_NETWORK_OPTION_OPERATION_TIMEOUT:
             if (size != sizeof(uint32_t)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             network_ctx->config.operation_timeout_ms = *(const uint32_t*)value;
             break;
             
         case POLYCALL_NETWORK_OPTION_IO_BUFFER_SIZE:
             if (size != sizeof(uint32_t)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             network_ctx->config.io_buffer_size = *(const uint32_t*)value;
             break;
             
         default:
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                             POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                             POLYCALL_ERROR_SEVERITY_ERROR,
                             "Unknown or unsupported network option: %d", option);
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return result;
 }
 
 /**
  * @brief Get network option
  */
 polycall_core_error_t polycall_network_get_option(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_option_t option,
     void* value,
     size_t* size
 ) {
     if (!core_ctx || !network_ctx || !value || !size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     switch (option) {
         case POLYCALL_NETWORK_OPTION_TLS_ENABLED:
             if (*size < sizeof(bool)) {
                 *size = sizeof(bool);
                 return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
             }
             
             *(bool*)value = (network_ctx->flags & POLYCALL_NETWORK_FLAG_TLS_ENABLED) != 0;
             *size = sizeof(bool);
             break;
             
         case POLYCALL_NETWORK_OPTION_COMPRESSION_ENABLED:
             if (*size < sizeof(bool)) {
                 *size = sizeof(bool);
                 return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
             }
             
             *(bool*)value = (network_ctx->flags & POLYCALL_NETWORK_FLAG_COMPRESSED) != 0;
             *size = sizeof(bool);
             break;
             
         case POLYCALL_NETWORK_OPTION_ENCRYPTION_ENABLED:
             if (*size < sizeof(bool)) {
                 *size = sizeof(bool);
                 return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
             }
             
             *(bool*)value = (network_ctx->flags & POLYCALL_NETWORK_FLAG_ENCRYPTED) != 0;
             *size = sizeof(bool);
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_CERT_FILE:
             if (!network_ctx->config.tls_cert_file) {
                 return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
             }
             
             {
                 size_t len = strlen(network_ctx->config.tls_cert_file) + 1;
                 if (*size < len) {
                     *size = len;
                     return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                 }
                 
                 memcpy(value, network_ctx->config.tls_cert_file, len);
                 *size = len;
             }
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_KEY_FILE:
             if (!network_ctx->config.tls_key_file) {
                 return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
             }
             
             {
                 size_t len = strlen(network_ctx->config.tls_key_file) + 1;
                 if (*size < len) {
                     *size = len;
                     return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                 }
                 
                 memcpy(value, network_ctx->config.tls_key_file, len);
                 *size = len;
             }
             break;
             
         case POLYCALL_NETWORK_OPTION_TLS_CA_FILE:
             if (!network_ctx->config.tls_ca_file) {
                 return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
             }
             
             {
                 size_t len = strlen(network_ctx->config.tls_ca_file) + 1;
                 if (*size < len) {
                     *size = len;
                     return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                 }
                 
                 memcpy(value, network_ctx->config.tls_ca_file, len);
                 *size = len;
             }
             break;
             
         case POLYCALL_NETWORK_OPTION_CONNECTION_TIMEOUT:
             if (*size < sizeof(uint32_t)) {
                 *size = sizeof(uint32_t);
                 return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
             }
             
             *(uint32_t*)value = network_ctx->config.connection_timeout_ms;
             *size = sizeof(uint32_t);
             break;
             
         case POLYCALL_NETWORK_OPTION_OPERATION_TIMEOUT:
             if (*size < sizeof(uint32_t)) {
                 *size = sizeof(uint32_t);
                 return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
             }
             
             *(uint32_t*)value = network_ctx->config.operation_timeout_ms;
             *size = sizeof(uint32_t);
             break;
             
         case POLYCALL_NETWORK_OPTION_IO_BUFFER_SIZE:
             if (*size < sizeof(uint32_t)) {
                 *size = sizeof(uint32_t);
                 return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
             }
             
             *(uint32_t*)value = network_ctx->config.io_buffer_size;
             *size = sizeof(uint32_t);
             break;
             
         default:
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                             POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                             POLYCALL_ERROR_SEVERITY_ERROR,
                             "Unknown or unsupported network option: %d", option);
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create network packet
  */
 polycall_core_error_t polycall_network_create_packet(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_network_packet_t** packet,
     size_t initial_capacity
 ) {
     if (!core_ctx || !network_ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If no capacity specified, use the configured buffer size
     if (initial_capacity == 0) {
         initial_capacity = network_ctx->config.io_buffer_size;
     }
     
     // Create the packet
     polycall_core_error_t result = polycall_network_packet_create(
         core_ctx, packet, initial_capacity);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create network packet");
         return result;
     }
     
     // Set compression flag if enabled
     if (network_ctx->flags & POLYCALL_NETWORK_FLAG_COMPRESSED) {
         polycall_packet_flags_t flags = 0;
         polycall_network_packet_get_flags(core_ctx, *packet, &flags);
         flags |= POLYCALL_PACKET_FLAG_COMPRESSED;
         polycall_network_packet_set_flags(core_ctx, *packet, flags);
     }
     
     // Set encryption flag if enabled
     if (network_ctx->flags & POLYCALL_NETWORK_FLAG_ENCRYPTED) {
         polycall_packet_flags_t flags = 0;
         polycall_network_packet_get_flags(core_ctx, *packet, &flags);
         flags |= POLYCALL_PACKET_FLAG_ENCRYPTED;
         polycall_network_packet_set_flags(core_ctx, *packet, flags);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Connect to remote endpoint
  */
 polycall_core_error_t polycall_network_connect(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     const char* address,
     uint16_t port,
     uint32_t timeout_ms,
     polycall_endpoint_t** endpoint
 ) {
     if (!core_ctx || !network_ctx || !address || !endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create a client
     polycall_network_client_t* client = NULL;
     polycall_core_error_t result = polycall_network_create_client(
         core_ctx, network_ctx, proto_ctx, &client, NULL);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create client for connection");
         return result;
     }
     
     // Connect to endpoint
     result = polycall_network_client_connect(
         core_ctx, client, address, port, timeout_ms, endpoint);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         // Don't unregister the client, as that's handled by cleanup
         polycall_network_client_cleanup(core_ctx, client);
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to connect to %s:%d", address, port);
         return result;
     }
     
     // Register endpoint
     result = register_endpoint(core_ctx, network_ctx, *endpoint);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_network_client_disconnect(core_ctx, client, *endpoint);
         *endpoint = NULL;
         
         // Don't unregister the client, as that's handled by cleanup
         polycall_network_client_cleanup(core_ctx, client);
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to register endpoint");
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Start listening for connections
  */
 polycall_core_error_t polycall_network_listen(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     uint16_t port,
     uint32_t backlog,
     polycall_network_server_t** server
 ) {
     if (!core_ctx || !network_ctx || !server) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create server configuration
     polycall_network_server_config_t config = polycall_network_server_create_default_config();
     config.port = port;
     config.backlog = backlog;
     
     // Create a server
     polycall_core_error_t result = polycall_network_create_server(
         core_ctx, network_ctx, proto_ctx, server, &config);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to create server for listening");
         return result;
     }
     
     // Start the server
     result = polycall_network_server_start(core_ctx, *server);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         // Don't unregister the server, as that's handled by cleanup
         polycall_network_server_cleanup(core_ctx, *server);
         *server = NULL;
         
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to start server on port %d", port);
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Send message to endpoint
  */
 polycall_core_error_t polycall_network_send_message(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_endpoint_t* endpoint,
     polycall_message_t* message,
     uint32_t timeout_ms,
     polycall_message_t** response
 ) {
     if (!core_ctx || !network_ctx || !endpoint || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get endpoint info to determine if we're using a client or server
     polycall_endpoint_info_t info;
     polycall_core_error_t result = polycall_endpoint_get_info(
         core_ctx, endpoint, &info);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to get endpoint info");
         return result;
     }
     
     // Use the appropriate function based on endpoint type
     if (info.type == POLYCALL_ENDPOINT_TYPE_CLIENT) {
         // Find the client that owns this endpoint
         pthread_mutex_lock(&network_ctx->client_mutex);
         
         client_entry_t* client_entry = network_ctx->clients;
         polycall_network_client_t* client = NULL;
         
         while (client_entry) {
             // Check if this client owns the endpoint
             // In a real implementation, we'd have a proper way to determine this
             client = client_entry->client;
             client_entry = client_entry->next;
         }
         
         pthread_mutex_unlock(&network_ctx->client_mutex);
         
         if (!client) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                               POLYCALL_CORE_ERROR_INVALID_STATE,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to find client for endpoint");
             return POLYCALL_CORE_ERROR_INVALID_STATE;
         }
         
         // Send the message using the client
         return polycall_network_client_send_message(
             core_ctx, client, proto_ctx, endpoint, message, timeout_ms, response);
     } else if (info.type == POLYCALL_ENDPOINT_TYPE_SERVER) {
         // Find the server that owns this endpoint
         pthread_mutex_lock(&network_ctx->server_mutex);
         
         server_entry_t* server_entry = network_ctx->servers;
         polycall_network_server_t* server = NULL;
         
         while (server_entry) {
             // Check if this server owns the endpoint
             // In a real implementation, we'd have a proper way to determine this
             server = server_entry->server;
             server_entry = server_entry->next;
         }
         
         pthread_mutex_unlock(&network_ctx->server_mutex);
         
         if (!server) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                               POLYCALL_CORE_ERROR_INVALID_STATE,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to find server for endpoint");
             return POLYCALL_CORE_ERROR_INVALID_STATE;
         }
         
         // Send the message using the server
         return polycall_network_server_send_message(
             core_ctx, server, proto_ctx, endpoint, message, timeout_ms, response);
     } else {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Unsupported endpoint type: %d", info.type);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 }
 
 /**
  * @brief Create default network configuration
  */
 polycall_network_config_t polycall_network_create_default_config(void) {
     polycall_network_config_t config;
     
     memset(&config, 0, sizeof(config));
     config.thread_pool_size = 4;             // Default to 4 threads
     config.max_connections = 100;            // 100 simultaneous connections
     config.max_endpoints = 1000;             // 1000 endpoints
     config.connection_timeout_ms = 30000;    // 30 seconds
     config.operation_timeout_ms = 30000;     // 30 seconds
     config.max_message_size = 1048576;       // 1 MB
     config.enable_compression = true;        // Enable compression by default
     config.enable_encryption = false;        // Disable encryption by default
     config.enable_tls = false;               // Disable TLS by default
     config.tls_cert_file = NULL;
     config.tls_key_file = NULL;
     config.tls_ca_file = NULL;
     config.io_buffer_size = 8192;            // 8 KB
     config.flags = 0;
     config.user_data = NULL;
     config.error_callback = NULL;
     
     return config;
 }
 
 /**
  * @brief Initialize network subsystem
  */
 polycall_core_error_t polycall_network_subsystem_init(void) {
     // Initialize platform-specific network subsystem
     INIT_SOCKETS();
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up network subsystem
  */
 void polycall_network_subsystem_cleanup(void) {
     // Clean up platform-specific network subsystem
     CLEANUP_SOCKETS();
 }
 
 /**
  * @brief Get network module version
  */
 const char* polycall_network_get_version(void) {
     return POLYCALL_NETWORK_VERSION;
 }
 
 /**
  * @brief Trigger an event for handlers
  */
 static void trigger_event(
     polycall_network_context_t* ctx,
     polycall_endpoint_t* endpoint,
     polycall_network_event_t event_type,
     void* event_data
 ) {
     if (!ctx || !endpoint || event_type >= POLYCALL_NETWORK_EVENT_COUNT) {
         return;
     }
     
     pthread_mutex_lock(&ctx->event_mutex);
     
     // Call all registered handlers for this event type
     for (uint32_t i = 0; i < ctx->event_handler_counts[event_type]; i++) {
         if (ctx->event_handlers[event_type][i].handler) {
             ctx->event_handlers[event_type][i].handler(
                 ctx, endpoint, event_data, ctx->event_handlers[event_type][i].user_data);
         }
     }
     
     pthread_mutex_unlock(&ctx->event_mutex);
 }
 
 /**
  * @brief Update statistics based on event
  */
 static void update_statistics(
     polycall_network_context_t* network_ctx,
     polycall_network_event_t event_type
 ) {
     if (!network_ctx) {
         return;
     }
     
     pthread_mutex_lock(&network_ctx->stats_mutex);
     
     switch (event_type) {
         case POLYCALL_NETWORK_EVENT_CONNECT:
             network_ctx->stats.connections++;
             break;
             
         case POLYCALL_NETWORK_EVENT_DISCONNECT:
             network_ctx->stats.disconnections++;
             break;
             
         case POLYCALL_NETWORK_EVENT_DATA_RECEIVED:
             network_ctx->stats.received_packets++;
             break;
             
         case POLYCALL_NETWORK_EVENT_DATA_SENT:
             network_ctx->stats.sent_packets++;
             break;
             
         case POLYCALL_NETWORK_EVENT_ERROR:
             network_ctx->stats.errors++;
             break;
             
         default:
             break;
     }
     
     pthread_mutex_unlock(&network_ctx->stats_mutex);
 }
 
 /**
  * @brief Initialize thread pool
  */
 static polycall_core_error_t initialize_thread_pool(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx,
     uint32_t thread_count
 ) {
     if (!core_ctx || !network_ctx || thread_count == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate thread array
     network_ctx->worker_threads = polycall_core_malloc(
         core_ctx, thread_count * sizeof(worker_thread_t));
     
     if (!network_ctx->worker_threads) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR,
                           "Failed to allocate thread pool");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize thread data
     memset(network_ctx->worker_threads, 0, thread_count * sizeof(worker_thread_t));
     network_ctx->worker_thread_count = thread_count;
     
     // Start threads
     for (uint32_t i = 0; i < thread_count; i++) {
         network_ctx->worker_threads[i].active = true;
         network_ctx->worker_threads[i].thread_data = network_ctx;
         
         if (pthread_create(&network_ctx->worker_threads[i].thread_id, NULL,
                          worker_thread_func, &network_ctx->worker_threads[i]) != 0) {
             
             // Failed to create thread, clean up
             for (uint32_t j = 0; j < i; j++) {
                 network_ctx->worker_threads[j].active = false;
             }
             
             // Signal threads to exit
             pthread_cond_broadcast(&network_ctx->thread_cond);
             
             // Wait for threads to exit
             for (uint32_t j = 0; j < i; j++) {
                 if (network_ctx->worker_threads[j].thread_id) {
                     pthread_join(network_ctx->worker_threads[j].thread_id, NULL);
                 }
             }
             
             // Free thread array
             polycall_core_free(core_ctx, network_ctx->worker_threads);
             network_ctx->worker_threads = NULL;
             network_ctx->worker_thread_count = 0;
             
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                               POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                               POLYCALL_ERROR_SEVERITY_ERROR,
                               "Failed to create worker thread");
             return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up thread pool
  */
 static void cleanup_thread_pool(
     polycall_core_context_t* core_ctx,
     polycall_network_context_t* network_ctx
 ) {
     if (!core_ctx || !network_ctx || !network_ctx->worker_threads) {
         return;
     }
     
     // Signal all threads to exit
     pthread_mutex_lock(&network_ctx->thread_mutex);
     
     for (uint32_t i = 0; i < network_ctx->worker_thread_count; i++) {
         network_ctx->worker_threads[i].active = false;
     }
     
     pthread_cond_broadcast(&network_ctx->thread_cond);
     pthread_mutex_unlock(&network_ctx->thread_mutex);
     
     // Wait for threads to exit
     for (uint32_t i = 0; i < network_ctx->worker_thread_count; i++) {
         if (network_ctx->worker_threads[i].thread_id) {
             pthread_join(network_ctx->worker_threads[i].thread_id, NULL);
         }
     }
     
     // Free thread array
     polycall_core_free(core_ctx, network_ctx->worker_threads);
     network_ctx->worker_threads = NULL;
     network_ctx->worker_thread_count = 0;
 }
 
/**
 * @brief Worker thread function
 */
static void* worker_thread_func(void* arg) {
    worker_thread_t* worker = (worker_thread_t*)arg;
    polycall_network_context_t* network_ctx = worker->thread_data;
    
    while (worker->active) {
        // Wait for signal
        pthread_mutex_lock(&network_ctx->thread_mutex);
        pthread_cond_wait(&network_ctx->thread_cond, &network_ctx->thread_mutex);
        
        // Check if we should exit
        if (!worker->active) {
            pthread_mutex_unlock(&network_ctx->thread_mutex);
            break;
        }
        
        // Process client events
        pthread_mutex_lock(&network_ctx->client_mutex);
        
        client_entry_t* client_entry = network_ctx->clients;
        while (client_entry) {
            polycall_network_client_process_events(
                network_ctx->core_ctx, client_entry->client, 0);
            client_entry = client_entry->next;
        }
        
        pthread_mutex_unlock(&network_ctx->client_mutex);
        
        // Process server events
        pthread_mutex_lock(&network_ctx->server_mutex);
        
        server_entry_t* server_entry = network_ctx->servers;
        while (server_entry) {
            polycall_network_server_process_events(
                network_ctx->core_ctx, server_entry->server, 0);
            server_entry = server_entry->next;
        }
        
        pthread_mutex_unlock(&network_ctx->server_mutex);
        
        // Release mutex before processing
        pthread_mutex_unlock(&network_ctx->thread_mutex);
    }
    
    return NULL;
}

/**
 * @brief Initialize TLS context
 */
static polycall_core_error_t initialize_tls_context(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx
) {
    if (!core_ctx || !network_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if TLS files are available
    if (!network_ctx->config.tls_cert_file || !network_ctx->config.tls_key_file) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "TLS certificate or key file not specified");
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // In a real implementation, we would initialize the TLS context using a library like OpenSSL
    // For this implementation, we'll just allocate an empty context
    
    network_ctx->tls_context = polycall_core_malloc(core_ctx, sizeof(void*));
    if (!network_ctx->tls_context) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate TLS context");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up TLS context
 */
static void cleanup_tls_context(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx
) {
    if (!core_ctx || !network_ctx || !network_ctx->tls_context) {
        return;
    }
    
    // In a real implementation, we would clean up the TLS context
    // For this implementation, we'll just free the allocated memory
    
    polycall_core_free(core_ctx, network_ctx->tls_context);
    network_ctx->tls_context = NULL;
}

/**
 * @brief Register endpoint
 */
static polycall_core_error_t register_endpoint(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx,
    polycall_endpoint_t* endpoint
) {
    if (!core_ctx || !network_ctx || !endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&network_ctx->endpoint_mutex);
    
    // Check if we've reached the maximum number of endpoints
    if (network_ctx->endpoint_count >= network_ctx->config.max_endpoints) {
        pthread_mutex_unlock(&network_ctx->endpoint_mutex);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Maximum number of endpoints reached");
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Allocate entry
    endpoint_entry_t* entry = polycall_core_malloc(core_ctx, sizeof(endpoint_entry_t));
    if (!entry) {
        pthread_mutex_unlock(&network_ctx->endpoint_mutex);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate endpoint entry");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize entry
    entry->endpoint = endpoint;
    entry->next = network_ctx->endpoints;
    network_ctx->endpoints = entry;
    network_ctx->endpoint_count++;
    
    pthread_mutex_unlock(&network_ctx->endpoint_mutex);
    
    // Trigger connect event
    trigger_event(network_ctx, endpoint, POLYCALL_NETWORK_EVENT_CONNECT, NULL);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Unregister endpoint
 */
static polycall_core_error_t unregister_endpoint(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx,
    polycall_endpoint_t* endpoint
) {
    if (!core_ctx || !network_ctx || !endpoint) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&network_ctx->endpoint_mutex);
    
    // Find endpoint in registry
    endpoint_entry_t** pp = &network_ctx->endpoints;
    endpoint_entry_t* entry = *pp;
    bool found = false;
    
    while (entry) {
        if (entry->endpoint == endpoint) {
            // Remove from list
            *pp = entry->next;
            
            // Free entry
            polycall_core_free(core_ctx, entry);
            
            network_ctx->endpoint_count--;
            found = true;
            break;
        }
        
        pp = &entry->next;
        entry = *pp;
    }
    
    pthread_mutex_unlock(&network_ctx->endpoint_mutex);
    
    if (!found) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_NOT_FOUND,
                          POLYCALL_ERROR_SEVERITY_WARNING,
                          "Endpoint not found in registry");
        return POLYCALL_CORE_ERROR_NOT_FOUND;
    }
    
    // Trigger disconnect event
    trigger_event(network_ctx, endpoint, POLYCALL_NETWORK_EVENT_DISCONNECT, NULL);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register client
 */
static polycall_core_error_t register_client(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx,
    polycall_network_client_t* client
) {
    if (!core_ctx || !network_ctx || !client) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&network_ctx->client_mutex);
    
    // Allocate entry
    client_entry_t* entry = polycall_core_malloc(core_ctx, sizeof(client_entry_t));
    if (!entry) {
        pthread_mutex_unlock(&network_ctx->client_mutex);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate client entry");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize entry
    entry->client = client;
    entry->next = network_ctx->clients;
    network_ctx->clients = entry;
    network_ctx->client_count++;
    
    pthread_mutex_unlock(&network_ctx->client_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Unregister client
 */
static polycall_core_error_t unregister_client(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx,
    polycall_network_client_t* client
) {
    if (!core_ctx || !network_ctx || !client) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&network_ctx->client_mutex);
    
    // Find client in registry
    client_entry_t** pp = &network_ctx->clients;
    client_entry_t* entry = *pp;
    bool found = false;
    
    while (entry) {
        if (entry->client == client) {
            // Remove from list
            *pp = entry->next;
            
            // Free entry
            polycall_core_free(core_ctx, entry);
            
            network_ctx->client_count--;
            found = true;
            break;
        }
        
        pp = &entry->next;
        entry = *pp;
    }
    
    pthread_mutex_unlock(&network_ctx->client_mutex);
    
    if (!found) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_NOT_FOUND,
                          POLYCALL_ERROR_SEVERITY_WARNING,
                          "Client not found in registry");
        return POLYCALL_CORE_ERROR_NOT_FOUND;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register server
 */
static polycall_core_error_t register_server(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx,
    polycall_network_server_t* server
) {
    if (!core_ctx || !network_ctx || !server) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&network_ctx->server_mutex);
    
    // Allocate entry
    server_entry_t* entry = polycall_core_malloc(core_ctx, sizeof(server_entry_t));
    if (!entry) {
        pthread_mutex_unlock(&network_ctx->server_mutex);
        
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate server entry");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize entry
    entry->server = server;
    entry->next = network_ctx->servers;
    network_ctx->servers = entry;
    network_ctx->server_count++;
    
    pthread_mutex_unlock(&network_ctx->server_mutex);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Unregister server
 */
static polycall_core_error_t unregister_server(
    polycall_core_context_t* core_ctx,
    polycall_network_context_t* network_ctx,
    polycall_network_server_t* server
) {
    if (!core_ctx || !network_ctx || !server) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    pthread_mutex_lock(&network_ctx->server_mutex);
    
    // Find server in registry
    server_entry_t** pp = &network_ctx->servers;
    server_entry_t* entry = *pp;
    bool found = false;
    
    while (entry) {
        if (entry->server == server) {
            // Remove from list
            *pp = entry->next;
            
            // Free entry
            polycall_core_free(core_ctx, entry);
            
            network_ctx->server_count--;
            found = true;
            break;
        }
        
        pp = &entry->next;
        entry = *pp;
    }
    
    pthread_mutex_unlock(&network_ctx->server_mutex);
    
    if (!found) {
        POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_NETWORK,
                          POLYCALL_CORE_ERROR_NOT_FOUND,
                          POLYCALL_ERROR_SEVERITY_WARNING,
                          "Server not found in registry");
        return POLYCALL_CORE_ERROR_NOT_FOUND;
    }
    
    return POLYCALL_CORE_SUCCESS;
}
