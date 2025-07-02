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
#include "polycall/core/polycall/core/polycall.h"

 * @file polycall.c
 * @brief Main public API implementation for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file implements the main public API for the LibPolyCall library,
 * as defined in polycall_public.h. It provides the primary entry point
 * for applications using the library, implementing the Program-First design.
 */


 #include "polycall/core/polycall/polycall.h"



// Define version macros
#define POLYCALL_VERSION_MAJOR 0
#define POLYCALL_VERSION_MINOR 1
#define POLYCALL_VERSION_PATCH 0
#define POLYCALL_VERSION_STRING "0.1.0"

// Define the LibPolyCall version
static const polycall_version_t POLYCALL_VERSION = {
    .major = POLYCALL_VERSION_MAJOR,
    .minor = POLYCALL_VERSION_MINOR,
    .patch = POLYCALL_VERSION_PATCH,
    .string = POLYCALL_VERSION_STRING
};

 /**
  * @brief Map a core error to a public error code
  */
 static polycall_error_t map_core_error(polycall_core_error_t core_error) {
     switch (core_error) {
         case POLYCALL_CORE_SUCCESS:
             return POLYCALL_OK;
         case POLYCALL_CORE_ERROR_INVALID_PARAMETERS:
             return POLYCALL_ERROR_INVALID_PARAMETERS;
         case POLYCALL_CORE_ERROR_INITIALIZATION_FAILED:
             return POLYCALL_ERROR_INITIALIZATION;
         case POLYCALL_CORE_ERROR_OUT_OF_MEMORY:
             return POLYCALL_ERROR_OUT_OF_MEMORY;
         case POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION:
             return POLYCALL_ERROR_UNSUPPORTED;
         case POLYCALL_CORE_ERROR_INVALID_STATE:
             return POLYCALL_ERROR_INVALID_STATE;
         case POLYCALL_CORE_ERROR_NOT_INITIALIZED:
             return POLYCALL_ERROR_NOT_INITIALIZED;
         case POLYCALL_CORE_ERROR_ALREADY_INITIALIZED:
             return POLYCALL_ERROR_ALREADY_INITIALIZED;
         default:
             return POLYCALL_ERROR_INTERNAL;
     }
 }
 
 /**
  * @brief Set error information in the context
  */
 static void set_error(polycall_context_t* ctx, polycall_error_t error, const char* format, ...) {
     if (!ctx) return;
     
     ctx->last_error = error;
     
     if (format) {
         va_list args;
         va_start(args, format);
         vsnprintf(ctx->error_message, sizeof(ctx->error_message) - 1, format, args);
         va_end(args);
         ctx->error_message[sizeof(ctx->error_message) - 1] = '\0';
     } else {
         ctx->error_message[0] = '\0';
     }
     
     // Also log to core error system if available
     if (ctx->core_ctx) {
         polycall_core_set_error(ctx->core_ctx, 
                                (polycall_core_error_t)error, 
                                ctx->error_message);
     }
 }
 
 /**
  * @brief Error callback for the core context
  */
 static void error_callback(
     polycall_core_error_t error,
     const char* message,
     void* user_data
 ) {
     polycall_context_t* ctx = (polycall_context_t*)user_data;
     if (ctx) {
         ctx->last_error = map_core_error(error);
         if (message) {
             strncpy(ctx->error_message, message, sizeof(ctx->error_message) - 1);
             ctx->error_message[sizeof(ctx->error_message) - 1] = '\0';
         }
     }
 }
 
 polycall_error_t polycall_init(
     polycall_context_t** ctx,
     const polycall_config_t* config
 ) {
     if (!ctx) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default config if none provided
     polycall_config_t default_config;
     if (!config) {
         default_config = polycall_create_default_config();
         config = &default_config;
     }
     
     // Allocate context
     polycall_context_t* new_ctx = calloc(1, sizeof(polycall_context_t));
     if (!new_ctx) {
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     // Map configuration to core configuration
     polycall_core_config_t core_config = {
         .flags = 0,
         .memory_pool_size = config->memory_pool_size,
         .user_data = new_ctx,  // Set the context as user data for callbacks
         .error_callback = error_callback
     };
    
     // Map flags
     if (config->flags & POLYCALL_FLAG_SECURE) {
         core_config.flags |= POLYCALL_CORE_FLAG_SECURE_MODE;
     }
     if (config->flags & POLYCALL_FLAG_DEBUG) {
         core_config.flags |= POLYCALL_CORE_FLAG_DEBUG_MODE;
     }
     if (config->flags & POLYCALL_FLAG_ASYNC) {
         core_config.flags |= POLYCALL_CORE_FLAG_ASYNC_OPERATIONS;
     }
     
     // Initialize core context
     polycall_core_error_t core_result = polycall_core_init(&new_ctx->core_ctx, &core_config);
     if (core_result != POLYCALL_CORE_SUCCESS) {
         free(new_ctx);
         return map_core_error(core_result);
     }
     
     // Initialize error subsystem
     if (polycall_error_init(new_ctx->core_ctx) != POLYCALL_CORE_SUCCESS) {
         polycall_core_cleanup(new_ctx->core_ctx);
         free(new_ctx);
         return POLYCALL_ERROR_INITIALIZATION;
     }
     
     // Store user data
     new_ctx->user_data = config->user_data;
     
     // Set up initial error state
     strncpy(new_ctx->error_message, "No error", sizeof(new_ctx->error_message));
     new_ctx->last_error = POLYCALL_OK;
     
     // Return the created context
     *ctx = new_ctx;
     return POLYCALL_OK;
 }
 
 void polycall_cleanup(polycall_context_t* ctx) {
     if (!ctx) return;
     
     // Clean up FFI context if initialized
     if (ctx->ffi.ffi_initialized && ctx->ffi.ffi_context) {
         // TODO: Implement FFI cleanup when the module is ready
         ctx->ffi.ffi_initialized = false;
         ctx->ffi.ffi_context = NULL;
     }
     
     // Clean up protocol context if initialized
     if (ctx->protocol.protocol_initialized && ctx->protocol.protocol_context) {
         // TODO: Implement protocol cleanup when the module is ready
         ctx->protocol.protocol_initialized = false;
         ctx->protocol.protocol_context = NULL;
     }
     
     // Clean up error subsystem
     polycall_error_cleanup(ctx->core_ctx);
     
     // Clean up core context
     polycall_core_cleanup(ctx->core_ctx);
     
     // Clean up the context itself
     free(ctx);
 }
 
 polycall_version_t polycall_get_version(void) {
     return POLYCALL_VERSION;
 }
 
 const char* polycall_get_error_message(polycall_context_t* ctx) {
     return ctx ? ctx->error_message : "Invalid context";
 }
 
 polycall_error_t polycall_get_error_code(polycall_context_t* ctx) {
     return ctx ? ctx->last_error : POLYCALL_ERROR_INVALID_PARAMETERS;
 }
 
 polycall_error_t polycall_connect(
     polycall_context_t* ctx,
     polycall_session_t** session,
     const polycall_connection_info_t* info
 ) {
     if (!ctx || !session || !info) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for connection");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if protocol system is initialized
     if (!ctx->protocol.protocol_initialized) {
         set_error(ctx, POLYCALL_ERROR_NOT_INITIALIZED, "Protocol system not initialized");
         return POLYCALL_ERROR_NOT_INITIALIZED;
     }
     
     // Allocate session
     polycall_session_t* new_session = calloc(1, sizeof(polycall_session_t));
     if (!new_session) {
         set_error(ctx, POLYCALL_ERROR_OUT_OF_MEMORY, "Failed to allocate session");
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize session
     new_session->ctx = ctx;
     strncpy(new_session->address, info->host, INET_ADDRSTRLEN - 1);
     new_session->address[INET_ADDRSTRLEN - 1] = '\0';
     new_session->port = info->port;
     new_session->timeout_ms = info->timeout_ms;
     new_session->connected = false;
     new_session->authenticated = false;
     new_session->sequence_number = 1;
     
     // TODO: Implement actual connection when the network module is ready
     // For now, simulate a successful connection
     new_session->connected = true;
     
     // Return the session
     *session = new_session;
     return POLYCALL_OK;
 }
 
 polycall_error_t polycall_disconnect(
     polycall_context_t* ctx,
     polycall_session_t* session
 ) {
     if (!ctx || !session) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for disconnection");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // TODO: Implement actual disconnection when the network module is ready
     // For now, just clean up the session
     
     // Clean up secure context if any
     if (session->secure_context) {
         // TODO: Implement secure context cleanup
         session->secure_context = NULL;
     }
     
     // Clean up connection if any
     if (session->connection) {
         // TODO: Implement connection cleanup
         session->connection = NULL;
     }
     
     // Reset session state
     session->connected = false;
     session->authenticated = false;
     
     // Free session
     free(session);
     
     return POLYCALL_OK;
 }
 
 polycall_error_t polycall_create_message(
     polycall_context_t* ctx,
     polycall_message_t** message,
     polycall_message_type_t type
 ) {
     if (!ctx || !message) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for message creation");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate message
     polycall_message_t* new_message = calloc(1, sizeof(polycall_message_t));
     if (!new_message) {
         set_error(ctx, POLYCALL_ERROR_OUT_OF_MEMORY, "Failed to allocate message");
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize message
     new_message->type = type;
     new_message->sequence = 0;  // Will be set on send
     
     *message = new_message;
     return POLYCALL_OK;
 }
 
 polycall_error_t polycall_destroy_message(
     polycall_context_t* ctx,
     polycall_message_t* message
 ) {
     if (!ctx || !message) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for message destruction");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Free data
     if (message->data) {
         free(message->data);
     }
     
     // Free JSON data
     if (message->json_data) {
         free(message->json_data);
     }
     
     // Free secure envelope if any
     if (message->secure_envelope) {
         // TODO: Implement secure envelope cleanup
         message->secure_envelope = NULL;
     }
     
     // Free message
     free(message);
     
     return POLYCALL_OK;
 }
 
 polycall_error_t polycall_message_set_path(
     polycall_context_t* ctx,
     polycall_message_t* message,
     const char* path
 ) {
     if (!ctx || !message || !path) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for setting message path");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy path
     strncpy(message->path, path, sizeof(message->path) - 1);
     message->path[sizeof(message->path) - 1] = '\0';
     
     return POLYCALL_OK;
 }
 
 polycall_error_t polycall_message_set_data(
     polycall_context_t* ctx,
     polycall_message_t* message,
     const void* data,
     size_t size
 ) {
     if (!ctx || !message || !data || size == 0) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for setting message data");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Free existing data
     if (message->data) {
         free(message->data);
         message->data = NULL;
         message->data_size = 0;
     }
     
     // Allocate and copy data
     message->data = malloc(size);
     if (!message->data) {
         set_error(ctx, POLYCALL_ERROR_OUT_OF_MEMORY, "Failed to allocate message data");
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(message->data, data, size);
     message->data_size = size;
     
     return POLYCALL_OK;
 }
 
 polycall_error_t polycall_message_set_string(
     polycall_context_t* ctx,
     polycall_message_t* message,
     const char* string
 ) {
     if (!ctx || !message || !string) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for setting message string");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     return polycall_message_set_data(ctx, message, string, strlen(string) + 1);
 }
 
 polycall_error_t polycall_message_set_json(
     polycall_context_t* ctx,
     polycall_message_t* message,
     const char* json
 ) {
     if (!ctx || !message || !json) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for setting message JSON");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Free existing JSON data
     if (message->json_data) {
         free(message->json_data);
         message->json_data = NULL;
     }
     
     // Copy JSON string
     size_t json_len = strlen(json) + 1;
     message->json_data = malloc(json_len);
     if (!message->json_data) {
         set_error(ctx, POLYCALL_ERROR_OUT_OF_MEMORY, "Failed to allocate message JSON data");
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(message->json_data, json, json_len);
     
     // Also set as regular data for compatibility
     return polycall_message_set_data(ctx, message, json, json_len);
 }
 
 polycall_error_t polycall_send_message(
     polycall_context_t* ctx,
     polycall_session_t* session,
     polycall_message_t* message,
     polycall_message_t** response
 ) {
     if (!ctx || !session || !message) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for sending message");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if session is connected
     if (!session->connected) {
         set_error(ctx, POLYCALL_ERROR_INVALID_STATE, "Session not connected");
         return POLYCALL_ERROR_INVALID_STATE;
     }
     
     // Set message sequence number
     message->sequence = session->sequence_number++;
     
     // TODO: Implement actual message sending when protocol module is ready
     // For now, simulate a successful send
     
     // Create response if requested
     if (response) {
         polycall_error_t result = polycall_create_message(ctx, response, POLYCALL_MESSAGE_RESPONSE);
         if (result != POLYCALL_OK) {
             return result;
         }
         
         // For now, just echo the request path
         polycall_message_set_path(ctx, *response, message->path);
         
         // Set a placeholder response
         const char* placeholder = "{\"status\":\"ok\",\"message\":\"Request processed\"}";
         polycall_message_set_json(ctx, *response, placeholder);
     }
     
     return POLYCALL_OK;
 }
 
 const char* polycall_message_get_path(
     polycall_context_t* ctx,
     polycall_message_t* message
 ) {
     if (!ctx || !message) {
         return NULL;
     }
     
     return message->path;
 }
 
 const void* polycall_message_get_data(
     polycall_context_t* ctx,
     polycall_message_t* message,
     size_t* size
 ) {
     if (!ctx || !message) {
         return NULL;
     }
     
     if (size) {
         *size = message->data_size;
     }
     
     return message->data;
 }
 
 const char* polycall_message_get_string(
     polycall_context_t* ctx,
     polycall_message_t* message
 ) {
     if (!ctx || !message || !message->data) {
         return NULL;
     }
     
     return (const char*)message->data;
 }
 
 const char* polycall_message_get_json(
     polycall_context_t* ctx,
     polycall_message_t* message
 ) {
     if (!ctx || !message) {
         return NULL;
     }
     
     // Prefer the dedicated JSON field if available
     if (message->json_data) {
         return (const char*)message->json_data;
     }
     
     // Fall back to regular data field
     return (const char*)message->data;
 }
 
 polycall_config_t polycall_create_default_config(void) {
     polycall_config_t config = {
         .flags = 0,
         .memory_pool_size = 1048576,  // 1MB default pool size
         .config_file = NULL,
         .user_data = NULL,
         .error_callback = NULL,
         .log_callback = NULL
     };
     
     return config;
 }
 
 polycall_error_t polycall_load_config_file(
     polycall_context_t* ctx,
     const char* filename,
     polycall_config_t* config
 ) {
     if (!ctx || !filename || !config) {
         set_error(ctx, POLYCALL_ERROR_INVALID_PARAMETERS, "Invalid parameters for loading config file");
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Start with default config
     *config = polycall_create_default_config();
     
     // TODO: Implement config file loading
     // This will be implemented when we have a configuration module
     
     config->config_file = filename;
     
     // For now, just set some default values
     config->memory_pool_size = 2097152;  // 2MB
     
     return POLYCALL_OK;
 }
 
 polycall_config_t polycall_load_config(const char* filename) {
     // Start with default config
     polycall_config_t config = polycall_create_default_config();
     
     if (!filename) {
         return config;
     }
     
     // Try to open the file
     FILE* file = fopen(filename, "r");
     if (!file) {
         // Could not open the file, return default config
         return config;
     }
     
     // TODO: Implement config file parsing
     // This will be implemented when we have a configuration module
     
     // Close the file
     fclose(file);
     
     config.config_file = filename;
     return config;
 }
 
 /**
  * @brief Initialize FFI subsystem
  * 
  * This function initializes the FFI subsystem for the given context.
  * 
  * @param ctx LibPolyCall context
  * @param ffi_config FFI configuration
  * @return Error code
  */
 polycall_error_t polycall_init_ffi(
     polycall_context_t* ctx,
     const void* ffi_config
 ) {
     if (!ctx) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if already initialized
     if (ctx->ffi.ffi_initialized) {
         set_error(ctx, POLYCALL_ERROR_ALREADY_INITIALIZED, "FFI already initialized");
         return POLYCALL_ERROR_ALREADY_INITIALIZED;
     }
     
     // TODO: Implement FFI initialization when the module is ready
     
     // For now, just set the flag
     ctx->ffi.ffi_initialized = true;
     
     return POLYCALL_OK;
 }
 
 /**
  * @brief Initialize protocol subsystem
  * 
  * This function initializes the protocol subsystem for the given context.
  * 
  * @param ctx LibPolyCall context
  * @param protocol_config Protocol configuration
  * @return Error code
  */
 polycall_error_t polycall_init_protocol(
     polycall_context_t* ctx,
     const void* protocol_config
 ) {
     if (!ctx) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if already initialized
     if (ctx->protocol.protocol_initialized) {
         set_error(ctx, POLYCALL_ERROR_ALREADY_INITIALIZED, "Protocol already initialized");
         return POLYCALL_ERROR_ALREADY_INITIALIZED;
     }
     
     // TODO: Implement protocol initialization when the module is ready
     
     // For now, just set the flag
     ctx->protocol.protocol_initialized = true;
     
     return POLYCALL_OK;
 }
 
 /**
  * @brief Set a user data pointer in the context
  * 
  * @param ctx LibPolyCall context
  * @param user_data User data pointer
  * @return Error code
  */
 polycall_error_t polycall_set_user_data(
     polycall_context_t* ctx,
     void* user_data
 ) {
     if (!ctx) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     ctx->user_data = user_data;
     
     return POLYCALL_OK;
 }
 
 /**
  * @brief Get the user data pointer from the context
  * 
  * @param ctx LibPolyCall context
  * @return User data pointer, or NULL if not set
  */
 void* polycall_get_user_data(polycall_context_t* ctx) {
     return ctx ? ctx->user_data : NULL;
 }
 
 /**
  * @brief Register a callback for a specific event
  * 
  * This function registers a callback function to be called when a specific event occurs.
  * 
  * @param ctx LibPolyCall context
  * @param event_type Event type
  * @param callback Callback function
  * @param user_data User data to pass to the callback
  * @return Error code
  */
 polycall_error_t polycall_register_callback(
     polycall_context_t* ctx,
     uint32_t event_type,
     void (*callback)(polycall_context_t*, void*),
     void* user_data
 ) {
     if (!ctx || !callback) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // TODO: Implement callback registration when the event system is ready
     
     return POLYCALL_ERROR_UNSUPPORTED;
 }
 
 /**
  * @brief Unregister a callback for a specific event
  * 
  * This function unregisters a previously registered callback function.
  * 
  * @param ctx LibPolyCall context
  * @param event_type Event type
  * @param callback Callback function
  * @return Error code
  */
 polycall_error_t polycall_unregister_callback(
     polycall_context_t* ctx,
     uint32_t event_type,
     void (*callback)(polycall_context_t*, void*)
 ) {
     if (!ctx || !callback) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // TODO: Implement callback unregistration when the event system is ready
     
     return POLYCALL_ERROR_UNSUPPORTED;
 }
 
 /**
  * @brief Set log callback
  * 
  * This function sets a callback function to be called for log messages.
  * 
  * @param ctx LibPolyCall context
  * @param callback Log callback function
  * @param user_data User data to pass to the callback
  * @return Error code
  */
 polycall_error_t polycall_set_log_callback(
     polycall_context_t* ctx,
     void (*callback)(int level, const char* message, void* user_data),
     void* user_data
 ) {
     if (!ctx || !callback) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // TODO: Implement log callback setup when the logging system is ready
     
     return POLYCALL_OK;
 }
 
 /**
  * @brief Process incoming messages
  * 
  * This function processes any pending incoming messages.
  * 
  * @param ctx LibPolyCall context
  * @param session Session to process messages for
  * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
  * @return Error code
  */
 polycall_error_t polycall_process_messages(
     polycall_context_t* ctx,
     polycall_session_t* session,
     uint32_t timeout_ms
 ) {
     if (!ctx || !session) {
         return POLYCALL_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if session is connected
     if (!session->connected) {
         set_error(ctx, POLYCALL_ERROR_INVALID_STATE, "Session not connected");
         return POLYCALL_ERROR_INVALID_STATE;
     }
     
     // TODO: Implement message processing when the network/protocol modules are ready
     
     return POLYCALL_OK;
 }
 
 
 /**
  * @brief Initialize all required subsystems
  * 
  * This function initializes all required subsystems based on the provided configuration.
  * 
  * @param ctx LibPolyCall context
  * @param config Configuration
  * @return Error code
  */
 polycall_error_t polycall_init_all(
     polycall_context_t** ctx,
     const polycall_config_t* config
 ) {
     // Initialize core
     polycall_error_t result = polycall_init(ctx, config);
     if (result != POLYCALL_OK) {
         return result;
     }
     
     // Initialize FFI if needed
     if (config && (config->flags & POLYCALL_FLAG_MICRO_ENABLED)) {
         result = polycall_init_ffi(*ctx, NULL);
         if (result != POLYCALL_OK) {
             polycall_cleanup(*ctx);
             *ctx = NULL;
             return result;
         }
     }
     
     // Initialize protocol
     result = polycall_init_protocol(*ctx, NULL);
     if (result != POLYCALL_OK) {
         polycall_cleanup(*ctx);
         *ctx = NULL;
         return result;
     }
     
     return POLYCALL_OK;
 }
