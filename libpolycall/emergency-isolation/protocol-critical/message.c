/**
#include "polycall/core/protocol/message.h"

 * @file message.c
 * @brief Protocol message handling for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the protocol message handling functionality for LibPolyCall,
 * supporting serialization, deserialization, validation, and routing of messages
 * in alignment with the Program-First architecture.
 */

 #include "polycall/core/protocol/message.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <string.h>
 #include <stdio.h>
 
 // Protocol constants
 #define POLYCALL_PROTOCOL_VERSION 1
 #define POLYCALL_MESSAGE_ALIGNMENT 8
 #define POLYCALL_MAX_MESSAGE_SIZE 16384
 #define POLYCALL_HEADER_MAGIC 0x504C4D /* "PLM" in ASCII */
 
 // Internal message pool structure
 typedef struct {
     polycall_message_t* messages;
     size_t capacity;
     size_t used;
     polycall_memory_pool_t* memory_pool;
 } message_pool_t;
 
 // Global message pool
 static message_pool_t g_message_pool = {0};
 
 // Internal helpers for message management
 static polycall_message_t* allocate_message(polycall_core_context_t* ctx) {
     // Initialize message pool if needed
     if (!g_message_pool.messages) {
         g_message_pool.capacity = 32; // Initial capacity
         
         // Create memory pool
         polycall_memory_create_pool(ctx, &g_message_pool.memory_pool, 
                                    g_message_pool.capacity * sizeof(polycall_message_t));
         
         // Allocate message array
         g_message_pool.messages = polycall_memory_alloc(
             ctx, g_message_pool.memory_pool, 
             g_message_pool.capacity * sizeof(polycall_message_t),
             POLYCALL_MEMORY_FLAG_ZERO_INIT
         );
         
         g_message_pool.used = 0;
     }
     
     // Check if we need to resize the pool
     if (g_message_pool.used >= g_message_pool.capacity) {
         size_t new_capacity = g_message_pool.capacity * 2;
         polycall_message_t* new_messages = polycall_memory_alloc(
             ctx, g_message_pool.memory_pool,
             new_capacity * sizeof(polycall_message_t),
             POLYCALL_MEMORY_FLAG_ZERO_INIT
         );
         
         if (!new_messages) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to resize message pool");
             return NULL;
         }
         
         // Copy existing messages
         memcpy(new_messages, g_message_pool.messages, 
                g_message_pool.used * sizeof(polycall_message_t));
         
         // Free old array
         polycall_memory_free(ctx, g_message_pool.memory_pool, g_message_pool.messages);
         
         g_message_pool.messages = new_messages;
         g_message_pool.capacity = new_capacity;
     }
     
     // Return next available message
     polycall_message_t* message = &g_message_pool.messages[g_message_pool.used++];
     memset(message, 0, sizeof(polycall_message_t));
     
     return message;
 }
 
 static void release_message(polycall_core_context_t* ctx, polycall_message_t* message) {
     if (!ctx || !message || !g_message_pool.messages) {
         return;
     }
     
     // Find message in pool
     size_t index = (message - g_message_pool.messages);
     if (index >= g_message_pool.used) {
         return; // Not in pool
     }
     
     // Free message payload if any
     if (message->payload) {
         polycall_memory_free(ctx, g_message_pool.memory_pool, message->payload);
         message->payload = NULL;
         message->payload_size = 0;
     }
     
     // Free message metadata if any
     if (message->metadata) {
         polycall_memory_free(ctx, g_message_pool.memory_pool, message->metadata);
         message->metadata = NULL;
         message->metadata_size = 0;
     }
     
     // Move last message to this position if not the last one
     if (index < g_message_pool.used - 1) {
         memcpy(&g_message_pool.messages[index], 
                &g_message_pool.messages[g_message_pool.used - 1],
                sizeof(polycall_message_t));
     }
     
     g_message_pool.used--;
 }
 
 // Alignment helpers
 static size_t align_size(size_t size) {
     return (size + (POLYCALL_MESSAGE_ALIGNMENT - 1)) & ~(POLYCALL_MESSAGE_ALIGNMENT - 1);
 }
 
 // Message creation and manipulation
 polycall_core_error_t polycall_message_create(
     polycall_core_context_t* ctx,
     polycall_message_t** message,
     polycall_message_type_t type
 ) {
     if (!ctx || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_message_t* new_message = allocate_message(ctx);
     if (!new_message) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize message
     new_message->header.magic = POLYCALL_HEADER_MAGIC;
     new_message->header.version = POLYCALL_PROTOCOL_VERSION;
     new_message->header.type = type;
     new_message->header.flags = 0;
     new_message->header.sequence = 0; // Will be set when sent
     new_message->header.payload_size = 0;
     new_message->header.metadata_size = 0;
     new_message->header.checksum = 0; // Will be calculated when serialized
     
     *message = new_message;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_message_destroy(
     polycall_core_context_t* ctx,
     polycall_message_t* message
 ) {
     if (!ctx || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     release_message(ctx, message);
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_message_set_payload(
     polycall_core_context_t* ctx,
     polycall_message_t* message,
     const void* payload,
     size_t size
 ) {
     if (!ctx || !message || (!payload && size > 0)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Free existing payload if any
     if (message->payload) {
         polycall_memory_free(ctx, g_message_pool.memory_pool, message->payload);
         message->payload = NULL;
         message->payload_size = 0;
     }
     
     // Skip if no payload
     if (!payload || size == 0) {
         message->header.payload_size = 0;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Check message size limit
     if (size > POLYCALL_MAX_MESSAGE_SIZE) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Message payload exceeds maximum size");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate and copy payload
     message->payload = polycall_memory_alloc(
         ctx, g_message_pool.memory_pool, 
         size,
         POLYCALL_MEMORY_FLAG_NONE
     );
     
     if (!message->payload) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(message->payload, payload, size);
     message->payload_size = size;
     message->header.payload_size = size;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_message_set_metadata(
     polycall_core_context_t* ctx,
     polycall_message_t* message,
     const void* metadata,
     size_t size
 ) {
     if (!ctx || !message || (!metadata && size > 0)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Free existing metadata if any
     if (message->metadata) {
         polycall_memory_free(ctx, g_message_pool.memory_pool, message->metadata);
         message->metadata = NULL;
         message->metadata_size = 0;
     }
     
     // Skip if no metadata
     if (!metadata || size == 0) {
         message->header.metadata_size = 0;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Check metadata size limit (arbitrary limit, adjust as needed)
     if (size > 1024) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Message metadata exceeds maximum size");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate and copy metadata
     message->metadata = polycall_memory_alloc(
         ctx, g_message_pool.memory_pool, 
         size,
         POLYCALL_MEMORY_FLAG_NONE
     );
     
     if (!message->metadata) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(message->metadata, metadata, size);
     message->metadata_size = size;
     message->header.metadata_size = size;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_message_set_flags(
     polycall_core_context_t* ctx,
     polycall_message_t* message,
     polycall_message_flags_t flags
 ) {
     if (!ctx || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     message->header.flags = flags;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Message serialization and deserialization
 polycall_core_error_t polycall_message_serialize(
     polycall_core_context_t* ctx,
     const polycall_message_t* message,
     void** buffer,
     size_t* size
 ) {
     if (!ctx || !message || !buffer || !size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Calculate total size
     size_t header_size = sizeof(polycall_message_header_t);
     size_t payload_size = message->payload_size;
     size_t metadata_size = message->metadata_size;
     size_t total_size = header_size + payload_size + metadata_size;
     
     // Allocate buffer
     *buffer = polycall_memory_alloc(
         ctx, g_message_pool.memory_pool, 
         total_size,
         POLYCALL_MEMORY_FLAG_NONE
     );
     
     if (!*buffer) {
         *size = 0;
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Prepare header with updated fields
     polycall_message_header_t header = message->header;
     header.payload_size = payload_size;
     header.metadata_size = metadata_size;
     
     // Calculate checksum (placeholder implementation - should be replaced with a proper algorithm)
     uint32_t checksum = 0;
     const uint8_t* payload_bytes = (const uint8_t*)message->payload;
     for (size_t i = 0; i < payload_size; i++) {
         checksum = ((checksum << 5) | (checksum >> 27)) + payload_bytes[i];
     }
     header.checksum = checksum;
     
     // Copy data to buffer
     uint8_t* buffer_ptr = (uint8_t*)*buffer;
     memcpy(buffer_ptr, &header, header_size);
     buffer_ptr += header_size;
     
     if (payload_size > 0 && message->payload) {
         memcpy(buffer_ptr, message->payload, payload_size);
         buffer_ptr += payload_size;
     }
     
     if (metadata_size > 0 && message->metadata) {
         memcpy(buffer_ptr, message->metadata, metadata_size);
     }
     
     *size = total_size;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_message_deserialize(
     polycall_core_context_t* ctx,
     const void* buffer,
     size_t buffer_size,
     polycall_message_t** message
 ) {
     if (!ctx || !buffer || buffer_size < sizeof(polycall_message_header_t) || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract header
     const polycall_message_header_t* header = (const polycall_message_header_t*)buffer;
     
     // Validate header
     if (header->magic != POLYCALL_HEADER_MAGIC) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Invalid message magic");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (header->version != POLYCALL_PROTOCOL_VERSION) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Unsupported protocol version");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Validate buffer size
     size_t expected_size = sizeof(polycall_message_header_t) + 
                          header->payload_size + header->metadata_size;
     if (buffer_size < expected_size) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Buffer too small for message");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create new message
     polycall_message_t* new_message = NULL;
     polycall_core_error_t result = polycall_message_create(ctx, &new_message, header->type);
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Copy header fields
     new_message->header = *header;
     
     // Extract payload if any
     if (header->payload_size > 0) {
         const uint8_t* payload_ptr = (const uint8_t*)buffer + sizeof(polycall_message_header_t);
         result = polycall_message_set_payload(ctx, new_message, payload_ptr, header->payload_size);
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_message_destroy(ctx, new_message);
             return result;
         }
     }
     
     // Extract metadata if any
     if (header->metadata_size > 0) {
         const uint8_t* metadata_ptr = (const uint8_t*)buffer + 
                                     sizeof(polycall_message_header_t) + 
                                     header->payload_size;
         result = polycall_message_set_metadata(ctx, new_message, metadata_ptr, header->metadata_size);
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_message_destroy(ctx, new_message);
             return result;
         }
     }
     
     // Verify checksum
     uint32_t calculated_checksum = 0;
     const uint8_t* payload_bytes = (const uint8_t*)new_message->payload;
     for (size_t i = 0; i < new_message->payload_size; i++) {
         calculated_checksum = ((calculated_checksum << 5) | (calculated_checksum >> 27)) + payload_bytes[i];
     }
     
     if (calculated_checksum != header->checksum) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Checksum verification failed");
         polycall_message_destroy(ctx, new_message);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *message = new_message;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Message routing and handling
 polycall_core_error_t polycall_message_register_handler(
     polycall_core_context_t* ctx,
     polycall_message_type_t type,
     polycall_message_handler_t handler,
     void* user_data
 ) {
     // Placeholder implementation - would need to be completed with a registry
     // of message handlers stored in the protocol context
     
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 polycall_core_error_t polycall_message_dispatch(
     polycall_core_context_t* ctx,
     polycall_message_t* message
 ) {
     // Placeholder implementation - would need to be completed with handler lookup
     // and invocation based on message type
     
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 // Message state observers
 const void* polycall_message_get_payload(
     const polycall_message_t* message,
     size_t* size
 ) {
     if (!message) {
         if (size) {
             *size = 0;
         }
         return NULL;
     }
     
     if (size) {
         *size = message->payload_size;
     }
     
     return message->payload;
 }
 
 const void* polycall_message_get_metadata(
     const polycall_message_t* message,
     size_t* size
 ) {
     if (!message) {
         if (size) {
             *size = 0;
         }
         return NULL;
     }
     
     if (size) {
         *size = message->metadata_size;
     }
     
     return message->metadata;
 }
 
 polycall_message_type_t polycall_message_get_type(
     const polycall_message_t* message
 ) {
     return message ? message->header.type : POLYCALL_MESSAGE_TYPE_INVALID;
 }
 
 polycall_message_flags_t polycall_message_get_flags(
     const polycall_message_t* message
 ) {
     return message ? message->header.flags : 0;
 }
 
 uint32_t polycall_message_get_sequence(
     const polycall_message_t* message
 ) {
     return message ? message->header.sequence : 0;
 }
 
 // String message helpers
 polycall_core_error_t polycall_message_set_string_payload(
     polycall_core_context_t* ctx,
     polycall_message_t* message,
     const char* str
 ) {
     if (!ctx || !message || !str) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return polycall_message_set_payload(ctx, message, str, strlen(str) + 1);
 }
 
 const char* polycall_message_get_string_payload(
     const polycall_message_t* message
 ) {
     if (!message || !message->payload) {
         return NULL;
     }
     
     return (const char*)message->payload;
 }
 
 // Message pool management
 polycall_core_error_t polycall_message_cleanup_pool(polycall_core_context_t* ctx) {
     if (!ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (g_message_pool.messages) {
         polycall_memory_free(ctx, g_message_pool.memory_pool, g_message_pool.messages);
         g_message_pool.messages = NULL;
     }
     
     if (g_message_pool.memory_pool) {
         polycall_memory_destroy_pool(ctx, g_message_pool.memory_pool);
         g_message_pool.memory_pool = NULL;
     }
     
     g_message_pool.capacity = 0;
     g_message_pool.used = 0;
     
     return POLYCALL_CORE_SUCCESS;
 }