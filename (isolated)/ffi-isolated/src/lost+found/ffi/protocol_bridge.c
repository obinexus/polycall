/**
 * @file protocol_bridge.c
 * @brief Protocol bridge implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the protocol bridge for LibPolyCall FFI, connecting
 * the FFI layer to the PolyCall Protocol system to enable network-transparent
 * function calls between different language runtimes.
 */


 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/ffi/protocol_bridge.h"

 #include <string.h>
 #include <pthread.h>
    #include <stdio.h>
    #include <stdlib.h>
    
 /**
  * @brief Initialize protocol bridge
  */
 polycall_core_error_t polycall_protocol_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_protocol_context_t* proto_ctx,
     protocol_bridge_t** bridge,
     const protocol_bridge_config_t* config
 ) {
     if (!ctx || !ffi_ctx || !proto_ctx || !bridge || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate bridge structure
     protocol_bridge_t* new_bridge = polycall_core_malloc(ctx, sizeof(protocol_bridge_t));
     if (!new_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate protocol bridge");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize bridge structure
     memset(new_bridge, 0, sizeof(protocol_bridge_t));
     new_bridge->core_ctx = ctx;
     new_bridge->ffi_ctx = ffi_ctx;
     new_bridge->proto_ctx = proto_ctx;
     new_bridge->config = *config;
     
     // Initialize mutex
     if (pthread_mutex_init(&new_bridge->mutex, NULL) != 0) {
         polycall_core_free(ctx, new_bridge);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize protocol bridge mutex");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Initialize routing table
     polycall_core_error_t error = init_routing_table(ctx, &new_bridge->routing_table);
     if (error != POLYCALL_CORE_SUCCESS) {
         pthread_mutex_destroy(&new_bridge->mutex);
         polycall_core_free(ctx, new_bridge);
         return error;
     }
     
     // Register default converters
     // This would register built-in converters for common formats like JSON/Binary
     
     *bridge = new_bridge;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up protocol bridge
  */
 void polycall_protocol_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge
 ) {
     if (!ctx || !bridge) {
         return;
     }
     
     // Clean up converters
     message_converter_t* current_converter = bridge->converters;
     while (current_converter) {
         message_converter_t* next = current_converter->next;
         polycall_core_free(ctx, current_converter);
         current_converter = next;
     }
     
     // Clean up remote functions
     remote_function_t* current_func = bridge->remote_functions;
     while (current_func) {
         remote_function_t* next = current_func->next;
         polycall_core_free(ctx, current_func);
         current_func = next;
     }
     
     // Clean up routing table
     if (bridge->routing_table) {
         cleanup_routing_table(ctx, bridge->routing_table);
     }
     
     // Destroy mutex
     pthread_mutex_destroy(&bridge->mutex);
     
     // Free bridge structure
     polycall_core_free(ctx, bridge);
 }
 
 /**
  * @brief Route protocol message to FFI function
  */
 polycall_core_error_t polycall_protocol_route_to_ffi(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     polycall_message_t* message,
     const char* target_language,
     const char* function_name
 ) {
     if (!ctx || !ffi_ctx || !bridge || !message || !target_language || !function_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract function arguments from the message
     void* data = NULL;
     size_t data_size = 0;
     
     polycall_core_error_t error = polycall_protocol_get_message_data(
         ctx, bridge->proto_ctx, message, &data, &data_size);
         
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to extract message data");
         return error;
     }
     
     // Determine message format and convert if necessary
     uint32_t message_type = 0;
     error = polycall_protocol_get_message_type(ctx, bridge->proto_ctx, message, &message_type);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to determine message type");
         return error;
     }
     
     // Deserialize arguments
     size_t arg_count = 0;
     ffi_value_t* args = NULL;
     
     // This would extract arguments from the message data based on format
     // For simplicity, we're assuming a common binary format here
     error = deserialize_ffi_value(ctx, data, data_size, &args);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to deserialize function arguments");
         return error;
     }
     
     // Determine argument count from the serialized data
     // This would be encoded in the serialized data format
     arg_count = *(size_t*)data; // Simplified - actual implementation would be format-specific
     
     // Prepare result container
     ffi_value_t result;
     memset(&result, 0, sizeof(ffi_value_t));
     
     // Call the FFI function
     error = polycall_ffi_call_function(
         ctx, ffi_ctx, function_name, args, arg_count, &result, target_language);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         // Clean up
         for (size_t i = 0; i < arg_count; i++) {
             // Free any allocated memory in args
         }
         polycall_core_free(ctx, args);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to call FFI function %s in language %s", 
                           function_name, target_language);
         return error;
     }
     
     // Store result in the message
     // This would typically involve updating a response message
     // For now, we'll just clean up
     
     // Clean up
     for (size_t i = 0; i < arg_count; i++) {
         // Free any allocated memory in args
     }
     polycall_core_free(ctx, args);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert FFI result to protocol message
  */
 polycall_core_error_t polycall_protocol_ffi_result_to_message(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     ffi_value_t* result,
     polycall_message_t** message
 ) {
     if (!ctx || !ffi_ctx || !bridge || !result || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Serialize FFI result
     void* serialized_data = NULL;
     size_t serialized_size = 0;
     
     polycall_core_error_t error = serialize_ffi_value(
         ctx, result, &serialized_data, &serialized_size);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to serialize FFI result");
         return error;
     }
     
     // Create a new protocol message
     polycall_message_t* new_message = NULL;
     error = polycall_protocol_create_message(ctx, bridge->proto_ctx, &new_message);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, serialized_data);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create protocol message");
         return error;
     }
     
     // Set message data
     error = polycall_protocol_set_message_data(
         ctx, bridge->proto_ctx, new_message, serialized_data, serialized_size);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, new_message);
         polycall_core_free(ctx, serialized_data);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to set message data");
         return error;
     }
     
     // Set message type to binary format
     error = polycall_protocol_set_message_type(
         ctx, bridge->proto_ctx, new_message, PROTOCOL_MESSAGE_TYPE_BINARY);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, new_message);
         polycall_core_free(ctx, serialized_data);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to set message type");
         return error;
     }
     
     // Free serialized data and return the message
     polycall_core_free(ctx, serialized_data);
     *message = new_message;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register FFI function for remote calls
  */
 polycall_core_error_t polycall_protocol_register_remote_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     const char* function_name,
     const char* language,
     ffi_signature_t* signature
 ) {
     if (!ctx || !ffi_ctx || !bridge || !function_name || !language || !signature) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock the bridge
     pthread_mutex_lock(&bridge->mutex);
     
     // Check if the function is already registered
     remote_function_t* existing = find_remote_function(bridge, function_name);
     if (existing) {
         pthread_mutex_unlock(&bridge->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Remote function %s already registered", function_name);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Register the function internally
     polycall_core_error_t error = register_remote_function_internal(
         ctx, bridge, function_name, language, signature);
     
     pthread_mutex_unlock(&bridge->mutex);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to register remote function %s", function_name);
         return error;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Call a remote FFI function
  */
 polycall_core_error_t polycall_protocol_call_remote_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result,
     const char* target_endpoint
 ) {
     if (!ctx || !ffi_ctx || !bridge || !function_name || 
         (!args && arg_count > 0) || !result || !target_endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the remote function registration
     pthread_mutex_lock(&bridge->mutex);
     remote_function_t* func = find_remote_function(bridge, function_name);
     pthread_mutex_unlock(&bridge->mutex);
     
     if (!func) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Remote function %s not registered", function_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Serialize function arguments
     void* serialized_args = NULL;
     size_t serialized_size = 0;
     
     // This would serialize all arguments into a binary format
     // For simplicity, we'll create a placeholder
     serialized_args = polycall_core_malloc(ctx, sizeof(size_t) + arg_count * sizeof(ffi_value_t));
     if (!serialized_args) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for serialized arguments");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Store argument count in the beginning
     *(size_t*)serialized_args = arg_count;
     
     // Copy arguments (simplified - real implementation would do proper serialization)
     if (arg_count > 0) {
         memcpy((char*)serialized_args + sizeof(size_t), args, arg_count * sizeof(ffi_value_t));
     }
     
     serialized_size = sizeof(size_t) + arg_count * sizeof(ffi_value_t);
     
     // Create protocol message
     polycall_message_t* message = NULL;
     polycall_core_error_t error = polycall_protocol_create_message(
         ctx, bridge->proto_ctx, &message);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, serialized_args);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create protocol message");
         return error;
     }
     
     // Set message properties
     error = polycall_protocol_set_message_type(
         ctx, bridge->proto_ctx, message, PROTOCOL_MESSAGE_TYPE_BINARY);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         polycall_core_free(ctx, serialized_args);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to set message type");
         return error;
     }
     
     // Set message data
     error = polycall_protocol_set_message_data(
         ctx, bridge->proto_ctx, message, serialized_args, serialized_size);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         polycall_core_free(ctx, serialized_args);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to set message data");
         return error;
     }
     
     // Set function path
     char path[MAX_PATH_LENGTH];
     snprintf(path, MAX_PATH_LENGTH, "/function/%s", function_name);
     
     error = polycall_protocol_set_message_path(
         ctx, bridge->proto_ctx, message, path);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         polycall_core_free(ctx, serialized_args);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to set message path");
         return error;
     }
     
     // Set target language metadata
     error = polycall_protocol_set_message_metadata(
         ctx, bridge->proto_ctx, message, "language", func->language);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         polycall_core_free(ctx, serialized_args);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to set language metadata");
         return error;
     }
     
     // Send message to target endpoint and wait for response
     polycall_message_t* response = NULL;
     error = polycall_protocol_send_message(
         ctx, bridge->proto_ctx, message, target_endpoint, bridge->config.timeout_ms, &response);
     
     // Free serialized data now that it's been sent
     polycall_core_free(ctx, serialized_args);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to send message to endpoint %s", target_endpoint);
         return error;
     }
     
     // Extract result from response
     void* response_data = NULL;
     size_t response_size = 0;
     
     error = polycall_protocol_get_message_data(
         ctx, bridge->proto_ctx, response, &response_data, &response_size);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, response);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to extract response data");
         return error;
     }
     
     // Deserialize response into result
     error = deserialize_ffi_value(ctx, response_data, response_size, result);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
         polycall_protocol_destroy_message(ctx, bridge->proto_ctx, response);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to deserialize response");
         return error;
     }
     
     // Clean up messages
     polycall_protocol_destroy_message(ctx, bridge->proto_ctx, message);
     polycall_protocol_destroy_message(ctx, bridge->proto_ctx, response);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register message converter
  */
 polycall_core_error_t polycall_protocol_register_converter(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     uint32_t source_type,
     uint32_t target_type,
     message_conversion_result_t (*converter)(
         polycall_core_context_t* ctx,
         const void* source,
         size_t source_size,
         void* user_data
     ),
     void* user_data
 ) {
     if (!ctx || !ffi_ctx || !bridge || !converter) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&bridge->mutex);
     
     // Check if converter already exists
     message_converter_t* existing = find_converter(bridge, source_type, target_type);
     if (existing) {
         pthread_mutex_unlock(&bridge->mutex);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Converter from type %u to %u already registered", 
                           source_type, target_type);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Register converter
     polycall_core_error_t error = register_converter_internal(
         ctx, bridge, source_type, target_type, converter, user_data);
     
     pthread_mutex_unlock(&bridge->mutex);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to register converter from type %u to %u", 
                           source_type, target_type);
         return error;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert message between formats
  */
 polycall_core_error_t polycall_protocol_convert_message(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     uint32_t source_type,
     const void* source,
     size_t source_size,
     uint32_t target_type,
     message_conversion_result_t* result
 ) {
     if (!ctx || !ffi_ctx || !bridge || !source || source_size == 0 || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find converter
     pthread_mutex_lock(&bridge->mutex);
     message_converter_t* converter = find_converter(bridge, source_type, target_type);
     pthread_mutex_unlock(&bridge->mutex);
     
     if (!converter) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "No converter found from type %u to %u", 
                           source_type, target_type);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Apply conversion
     *result = converter->converter(ctx, source, source_size, converter->user_data);
     
     if (!result->success) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_CONVERSION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Conversion failed: %s", result->error_message);
         return POLYCALL_CORE_ERROR_CONVERSION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Add routing rule
  */
 polycall_core_error_t polycall_protocol_add_routing_rule(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     const char* source_pattern,
     const char* target_endpoint,
     uint32_t priority
 ) {
     if (!ctx || !ffi_ctx || !bridge || !source_pattern || !target_endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return add_routing_rule_internal(ctx, bridge->routing_table, source_pattern, target_endpoint, priority);
 }
 
 /**
  * @brief Remove routing rule
  */
 polycall_core_error_t polycall_protocol_remove_routing_rule(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     const char* source_pattern,
     const char* target_endpoint
 ) {
     if (!ctx || !ffi_ctx || !bridge || !source_pattern || !target_endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return remove_routing_rule_internal(ctx, bridge->routing_table, source_pattern, target_endpoint);
 }
 
 /**
  * @brief Synchronize state between protocol and FFI
  */
 polycall_core_error_t polycall_protocol_sync_state(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     polycall_protocol_context_t* proto_ctx
 ) {
     if (!ctx || !ffi_ctx || !bridge || !proto_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // This would typically involve several steps:
     // 1. Synchronize protocol-level state (connections, sessions)
     // 2. Update remote function registrations
     // 3. Update routing rules based on active connections
     
     // For now, just ensure the protocol context pointer is up-to-date
     bridge->proto_ctx = proto_ctx;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Handle protocol message
  */
 polycall_core_error_t polycall_protocol_handle_message(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     protocol_bridge_t* bridge,
     polycall_message_t* message,
     polycall_message_t** response
 ) {
     if (!ctx || !ffi_ctx || !bridge || !message || !response) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get message path
     char path[MAX_PATH_LENGTH];
     polycall_core_error_t error = polycall_protocol_get_message_path(
         ctx, bridge->proto_ctx, message, path, MAX_PATH_LENGTH);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get message path");
         return error;
     }
     
     // Check if this is a function call
     if (strncmp(path, "/function/", 10) == 0) {
         // Extract function name
         const char* function_name = path + 10;
         
         // Get target language from metadata
         char language[64];
         error = polycall_protocol_get_message_metadata(
             ctx, bridge->proto_ctx, message, "language", language, sizeof(language));
         
         if (error != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               error,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to get language metadata");
             return error;
         }
         
         // Create response message
         polycall_message_t* new_response = NULL;
         error = polycall_protocol_create_message(
             ctx, bridge->proto_ctx, &new_response);
         
         if (error != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               error,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to create response message");
             return error;
         }
         
         // Route message to FFI function
         error = polycall_protocol_route_to_ffi(
             ctx, ffi_ctx, bridge, message, language, function_name);
         
         if (error != POLYCALL_CORE_SUCCESS) {
             // Create error response
             polycall_protocol_set_message_metadata(
                 ctx, bridge->proto_ctx, new_response, "error", "true");
             polycall_protocol_set_message_metadata(
                 ctx, bridge->proto_ctx, new_response, "error_code", "function_call_failed");
             
             *response = new_response;
             return POLYCALL_CORE_SUCCESS; // We return success but with error metadata
         }
         
         // Set successful response
         polycall_protocol_set_message_metadata(
             ctx, bridge->proto_ctx, new_response, "error", "false");
         polycall_protocol_set_message_path(
             ctx, bridge->proto_ctx, new_response, "/function/response");
         
         *response = new_response;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Check if this is a system command
     if (strncmp(path, "/system/", 8) == 0) {
         // Handle system commands (registration, discovery, etc.)
         // This would be implemented based on specific system commands
         
         // Create a default response for now
         polycall_message_t* new_response = NULL;
         error = polycall_protocol_create_message(
             ctx, bridge->proto_ctx, &new_response);
         
         if (error != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               error,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to create response message");
             return error;
         }
         
         polycall_protocol_set_message_metadata(
             ctx, bridge->proto_ctx, new_response, "error", "true");
         polycall_protocol_set_message_metadata(
             ctx, bridge->proto_ctx, new_response, "error_code", "unknown_system_command");
         
         *response = new_response;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Unknown path
     polycall_message_t* new_response = NULL;
     error = polycall_protocol_create_message(
         ctx, bridge->proto_ctx, &new_response);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create response message");
         return error;
     }
     
     polycall_protocol_set_message_metadata(
         ctx, bridge->proto_ctx, new_response, "error", "true");
     polycall_protocol_set_message_metadata(
         ctx, bridge->proto_ctx, new_response, "error_code", "unknown_path");
     
     *response = new_response;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a default protocol bridge configuration
  */
 protocol_bridge_config_t polycall_protocol_bridge_create_default_config(void) {
     protocol_bridge_config_t config;
     
     config.enable_message_compression = true;
     config.enable_streaming = false;
     config.enable_fragmentation = true;
     config.max_message_size = 1024 * 1024; // 1MB default
     config.timeout_ms = 30000; // 30 seconds default
     config.user_data = NULL;
     
     return config;
 }
 
 //-----------------------------------------------------------------------------
 // Internal implementations
 //-----------------------------------------------------------------------------
 
 /**
  * @brief Initialize routing table
  */
 static polycall_core_error_t init_routing_table(
     polycall_core_context_t* ctx,
     routing_table_t** table
 ) {
     if (!ctx || !table) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     routing_table_t* new_table = polycall_core_malloc(ctx, sizeof(routing_table_t));
     if (!new_table) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate routing table");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize table
     new_table->rules = NULL;
     
     // Initialize mutex
     if (pthread_mutex_init(&new_table->mutex, NULL) != 0) {
         polycall_core_free(ctx, new_table);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize routing table mutex");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     *table = new_table;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up routing table
  */
 static void cleanup_routing_table(
     polycall_core_context_t* ctx,
     routing_table_t* table
 ) {
     if (!ctx || !table) {
         return;
     }
     
     pthread_mutex_lock(&table->mutex);
     
     // Free all rules
     routing_rule_t* current_rule = table->rules;
     while (current_rule) {
         routing_rule_t* next = current_rule->next;
         polycall_core_free(ctx, current_rule);
         current_rule = next;
     }
     
     pthread_mutex_unlock(&table->mutex);
     
     // Destroy mutex
     pthread_mutex_destroy(&table->mutex);
     
     // Free table
     polycall_core_free(ctx, table);
 }
 
 /**
  * @brief Add routing rule (internal implementation)
  */
 static polycall_core_error_t add_routing_rule_internal(
     polycall_core_context_t* ctx,
     routing_table_t* table,
     const char* source_pattern,
     const char* target_endpoint,
     uint32_t priority
 ) {
     if (!ctx || !table || !source_pattern || !target_endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check input lengths
     if (strlen(source_pattern) >= MAX_PATH_LENGTH || 
         strlen(target_endpoint) >= MAX_PATH_LENGTH) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Source pattern or target endpoint too long");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate new rule
     routing_rule_t* new_rule = polycall_core_malloc(ctx, sizeof(routing_rule_t));
     if (!new_rule) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate routing rule");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize rule
     strcpy(new_rule->source_pattern, source_pattern);
     strcpy(new_rule->target_endpoint, target_endpoint);
     new_rule->priority = priority;
     new_rule->next = NULL;
     
     // Add to table
     pthread_mutex_lock(&table->mutex);
     
     // Sorted insert based on priority (higher priority first)
     routing_rule_t** insert_pos = &table->rules;
     while (*insert_pos && (*insert_pos)->priority > priority) {
         insert_pos = &(*insert_pos)->next;
     }
     
     new_rule->next = *insert_pos;
     *insert_pos = new_rule;
     
     pthread_mutex_unlock(&table->mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Remove routing rule (internal implementation)
  */
 static polycall_core_error_t remove_routing_rule_internal(
     polycall_core_context_t* ctx,
     routing_table_t* table,
     const char* source_pattern,
     const char* target_endpoint
 ) {
     if (!ctx || !table || !source_pattern || !target_endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&table->mutex);
     
     // Find the rule
     routing_rule_t** pp = &table->rules;
     routing_rule_t* current = *pp;
     
     while (current) {
         if (strcmp(current->source_pattern, source_pattern) == 0 &&
             strcmp(current->target_endpoint, target_endpoint) == 0) {
             // Found the rule, remove it
             *pp = current->next;
             polycall_core_free(ctx, current);
             pthread_mutex_unlock(&table->mutex);
             return POLYCALL_CORE_SUCCESS;
         }
         
         pp = &current->next;
         current = current->next;
     }
     
     pthread_mutex_unlock(&table->mutex);
     
     // Rule not found
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                       POLYCALL_ERROR_SEVERITY_WARNING, 
                       "Routing rule not found");
     return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
 }
 
 /**
  * @brief Find a message converter
  */
 static message_converter_t* find_converter(
     protocol_bridge_t* bridge,
     uint32_t source_type,
     uint32_t target_type
 ) {
     if (!bridge) {
         return NULL;
     }
     
     message_converter_t* current = bridge->converters;
     
     while (current) {
         if (current->source_type == source_type && 
             current->target_type == target_type) {
             return current;
         }
         
         current = current->next;
     }
     
     return NULL;
 }
 
 /**
  * @brief Register message converter (internal implementation)
  */
 static polycall_core_error_t register_converter_internal(
     polycall_core_context_t* ctx,
     protocol_bridge_t* bridge,
     uint32_t source_type,
     uint32_t target_type,
     message_conversion_result_t (*converter)(
         polycall_core_context_t* ctx,
         const void* source,
         size_t source_size,
         void* user_data
     ),
     void* user_data
 ) {
     if (!ctx || !bridge || !converter) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate new converter
     message_converter_t* new_converter = polycall_core_malloc(ctx, sizeof(message_converter_t));
     if (!new_converter) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate message converter");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize converter
     new_converter->source_type = source_type;
     new_converter->target_type = target_type;
     new_converter->converter = converter;
     new_converter->user_data = user_data;
     new_converter->next = NULL;
     
     // Add to list
     if (!bridge->converters) {
         bridge->converters = new_converter;
     } else {
         message_converter_t* current = bridge->converters;
         while (current->next) {
             current = current->next;
         }
         current->next = new_converter;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register remote function (internal implementation)
  */
 static polycall_core_error_t register_remote_function_internal(
     polycall_core_context_t* ctx,
     protocol_bridge_t* bridge,
     const char* function_name,
     const char* language,
     ffi_signature_t* signature
 ) {
     if (!ctx || !bridge || !function_name || !language || !signature) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check function name length
     if (strlen(function_name) >= MAX_PATH_LENGTH) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Function name too long");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check language length
     if (strlen(language) >= 64) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Language name too long");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate new function
     remote_function_t* new_func = polycall_core_malloc(ctx, sizeof(remote_function_t));
     if (!new_func) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate remote function");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize function
     strcpy(new_func->name, function_name);
     strcpy(new_func->language, language);
     new_func->signature = signature;
     new_func->next = NULL;
     
     // Add to list
     if (!bridge->remote_functions) {
         bridge->remote_functions = new_func;
     } else {
         remote_function_t* current = bridge->remote_functions;
         while (current->next) {
             current = current->next;
         }
         current->next = new_func;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Find a remote function
  */
 static remote_function_t* find_remote_function(
     protocol_bridge_t* bridge,
     const char* function_name
 ) {
     if (!bridge || !function_name) {
         return NULL;
     }
     
     remote_function_t* current = bridge->remote_functions;
     
     while (current) {
         if (strcmp(current->name, function_name) == 0) {
             return current;
         }
         
         current = current->next;
     }
     
     return NULL;
 }
 
 /**
  * @brief Serialize FFI value
  */
 static polycall_core_error_t serialize_ffi_value(
     polycall_core_context_t* ctx,
     ffi_value_t* value,
     void** data,
     size_t* data_size
 ) {
     if (!ctx || !value || !data || !data_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // This would be a complex implementation that serializes the FFI value
     // based on its type. For simplicity, we'll just create a placeholder.
     
     // Allocate memory for the serialized data
     *data = polycall_core_malloc(ctx, sizeof(ffi_value_t));
     if (!*data) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for serialized data");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Copy the value (simplified - real implementation would handle all types)
     memcpy(*data, value, sizeof(ffi_value_t));
     *data_size = sizeof(ffi_value_t);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Deserialize FFI value
  */
 static polycall_core_error_t deserialize_ffi_value(
     polycall_core_context_t* ctx,
     const void* data,
     size_t data_size,
     ffi_value_t* value
 ) {
     if (!ctx || !data || data_size < sizeof(ffi_value_t) || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // This would be a complex implementation that deserializes the FFI value
     // based on its type. For simplicity, we'll just create a placeholder.
     
     // Copy the value (simplified - real implementation would handle all types)
     memcpy(value, data, sizeof(ffi_value_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Route a message to the appropriate endpoint
  */
 static polycall_core_error_t route_message(
     polycall_core_context_t* ctx,
     protocol_bridge_t* bridge,
     polycall_message_t* message,
     const char** target_endpoint
 ) {
     if (!ctx || !bridge || !message || !target_endpoint) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get message path
     char path[MAX_PATH_LENGTH];
     polycall_core_error_t error = polycall_protocol_get_message_path(
         ctx, bridge->proto_ctx, message, path, MAX_PATH_LENGTH);
     
     if (error != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           error,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get message path");
         return error;
     }
     
     // Lock routing table
     pthread_mutex_lock(&bridge->routing_table->mutex);
     
     // Find matching rule
     routing_rule_t* current = bridge->routing_table->rules;
     while (current) {
         // Simple prefix matching (could be enhanced with pattern matching)
         if (strncmp(path, current->source_pattern, strlen(current->source_pattern)) == 0) {
             *target_endpoint = current->target_endpoint;
             pthread_mutex_unlock(&bridge->routing_table->mutex);
             return POLYCALL_CORE_SUCCESS;
         }
         
         current = current->next;
     }
     
     pthread_mutex_unlock(&bridge->routing_table->mutex);
     
     // No matching rule
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "No routing rule found for path %s", path);
     return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
 }