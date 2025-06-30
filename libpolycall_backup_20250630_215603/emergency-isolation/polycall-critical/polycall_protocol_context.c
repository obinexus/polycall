/**
#include "polycall/core/protocol/polycall_protocol_context.h"

 * @file polycall_protocol_context.c
 * @brief Protocol context implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the protocol context functionality that bridges
 * the core LibPolyCall system with network operations, following the
 * Program-First design approach.
 */

 #include "polycall/core/protocol/polycall_protocol_context.h"

 
 // Protocol constants
 #define PROTOCOL_VERSION 1
 #define PROTOCOL_MAGIC 0x504C43   // "PLC"
 #define PROTOCOL_BUFFER_SIZE 4096
 #define MAX_ERROR_MESSAGE_LENGTH 256
 
 // Protocol state transition names
 #define PROTOCOL_TRANSITION_TO_HANDSHAKE "to_handshake"
 #define PROTOCOL_TRANSITION_TO_AUTH "to_auth"
 #define PROTOCOL_TRANSITION_TO_READY "to_ready"
 #define PROTOCOL_TRANSITION_TO_ERROR "to_error"
 #define PROTOCOL_TRANSITION_TO_CLOSED "to_closed"
 
 // Protocol error buffer
 static char protocol_error_buffer[MAX_ERROR_MESSAGE_LENGTH] = {0};
 
 // Internal helper functions
 static bool validate_message_header(const polycall_protocol_msg_header_t* header) {
     if (!header) return false;
     
     // Check version compatibility
     if (header->version != PROTOCOL_VERSION) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH,
                 "Protocol version mismatch: expected %d, got %d",
                 PROTOCOL_VERSION, header->version);
         return false;
     }
     
     // Validate message type
     if (header->type > POLYCALL_PROTOCOL_MSG_HEARTBEAT) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH,
                 "Invalid message type: %d", header->type);
         return false;
     }
     
     return true;
 }
 
 static bool transition_protocol_state(
     polycall_protocol_context_t* ctx,
     polycall_protocol_state_t new_state
 ) {
     if (!ctx || !ctx->state_machine) return false;
     
     polycall_protocol_state_t old_state = ctx->state;
     const char* transition_name = NULL;
     
     // Determine appropriate transition name
     switch (new_state) {
         case POLYCALL_PROTOCOL_STATE_HANDSHAKE:
             transition_name = PROTOCOL_TRANSITION_TO_HANDSHAKE;
             break;
         case POLYCALL_PROTOCOL_STATE_AUTH:
             transition_name = PROTOCOL_TRANSITION_TO_AUTH;
             break;
         case POLYCALL_PROTOCOL_STATE_READY:
             transition_name = PROTOCOL_TRANSITION_TO_READY;
             break;
         case POLYCALL_PROTOCOL_STATE_ERROR:
             transition_name = PROTOCOL_TRANSITION_TO_ERROR;
             break;
         case POLYCALL_PROTOCOL_STATE_CLOSED:
             transition_name = PROTOCOL_TRANSITION_TO_CLOSED;
             break;
         default:
             return false;
     }
     
     // Assuming the state_machine interface includes execute_transition
     if (polycall_sm_execute_transition(ctx->state_machine, transition_name) 
         != POLYCALL_SM_SUCCESS) {
         return false;
     }
     
     ctx->state = new_state;
     
     // Notify state change if callback is set
     if (ctx->state != old_state) {
         // We need to cast to access the callbacks from the config
         polycall_protocol_config_t* config = (polycall_protocol_config_t*)ctx->user_data;
         if (config && config->callbacks.on_state_change) {
             config->callbacks.on_state_change(ctx, old_state, new_state);
         }
     }
     
     return true;
 }
 
 bool polycall_protocol_init(
     polycall_protocol_context_t* ctx,
     polycall_core_context_t* core_ctx,
     NetworkEndpoint* endpoint,
     const polycall_protocol_config_t* config
 ) {
     if (!ctx || !core_ctx || !endpoint || !config) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH, "Invalid parameters");
         return false;
     }
     
     // Initialize context fields
     ctx->core_ctx = core_ctx;
     ctx->endpoint = endpoint;
     ctx->state = POLYCALL_PROTOCOL_STATE_INIT;
     ctx->next_sequence = 1;
     ctx->crypto_context = NULL; // Will be initialized when needed
     
     // Store user data (including callbacks)
     ctx->user_data = (void*)config;
     
     // Allocate message buffer
     ctx->message_cache.buffer = polycall_core_malloc(core_ctx, config->max_message_size);
     if (!ctx->message_cache.buffer) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH, "Failed to allocate message buffer");
         return false;
     }
     ctx->message_cache.size = config->max_message_size;
     ctx->message_cache.used = 0;
     
     // Initialize state machine - assumes polycall_sm_create_with_integrity exists
     polycall_sm_status_t sm_status = polycall_sm_create_with_integrity(
         core_ctx,
         &ctx->state_machine,
         NULL  // No integrity check for now
     );
     
     if (sm_status != POLYCALL_SM_SUCCESS) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH,
                 "Failed to create protocol state machine");
         polycall_core_free(core_ctx, ctx->message_cache.buffer);
         return false;
     }
     
     // Register protocol context with core context system
     polycall_context_init_t context_init = {
         .type = POLYCALL_CONTEXT_TYPE_PROTOCOL,
         .data_size = 0,  // We're not storing additional data in the context
         .flags = POLYCALL_CONTEXT_FLAG_NONE,
         .name = "protocol",
         .init_fn = NULL,  // No additional initialization needed
         .cleanup_fn = NULL,  // No additional cleanup needed
         .init_data = NULL
     };
     
     polycall_context_ref_t* ctx_ref;
     if (polycall_context_init(core_ctx, &ctx_ref, &context_init) != POLYCALL_CORE_SUCCESS) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH,
                 "Failed to register protocol context");
         polycall_sm_destroy(ctx->state_machine);
         polycall_core_free(core_ctx, ctx->message_cache.buffer);
         return false;
     }
     
     return true;
 }
 
 void polycall_protocol_cleanup(polycall_protocol_context_t* ctx) {
     if (!ctx) return;
     
     // Clean up state machine
     if (ctx->state_machine) {
         polycall_sm_destroy(ctx->state_machine);
         ctx->state_machine = NULL;
     }
     
     // Free message buffer
     if (ctx->message_cache.buffer) {
         polycall_core_free(ctx->core_ctx, ctx->message_cache.buffer);
         ctx->message_cache.buffer = NULL;
         ctx->message_cache.size = 0;
         ctx->message_cache.used = 0;
     }
     
     // Clean up crypto context if any
     if (ctx->crypto_context) {
         // TODO: Implement crypto context cleanup when crypto module is ready
         ctx->crypto_context = NULL;
     }
     
     // Reset context fields
     ctx->core_ctx = NULL;
     ctx->endpoint = NULL;
     ctx->state = POLYCALL_PROTOCOL_STATE_CLOSED;
     ctx->next_sequence = 0;
     ctx->user_data = NULL;
 }
 
 bool polycall_protocol_send(
     polycall_protocol_context_t* ctx,
     polycall_protocol_msg_type_t type,
     const void* payload,
     size_t payload_length,
     polycall_protocol_flags_t flags
 ) {
     if (!ctx || !ctx->endpoint || !payload || payload_length == 0) {
         return false;
     }
     
     // Create message header
     polycall_protocol_msg_header_t header = {
         .version = PROTOCOL_VERSION,
         .type = type,
         .flags = flags,
         .sequence = ctx->next_sequence++,
         .payload_length = (uint32_t)payload_length,
         .checksum = 0
     };
     
     // Calculate checksum
     header.checksum = polycall_protocol_calculate_checksum(payload, payload_length);
     
     // Prepare network packet
     uint8_t buffer[PROTOCOL_BUFFER_SIZE];
     
     // Check if message is too large
     size_t total_size = sizeof(header) + payload_length;
     if (total_size > PROTOCOL_BUFFER_SIZE) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH,
                 "Message too large: %zu bytes", total_size);
         return false;
     }
     
     // Copy header and payload to buffer
     memcpy(buffer, &header, sizeof(header));
     memcpy(buffer + sizeof(header), payload, payload_length);
     
     // Create network packet
     NetworkPacket packet = {
         .data = buffer,
         .size = total_size,
         .flags = 0
     };
     
     // Send packet
    // Send through network endpoint using the protocol's network layer
    polycall_core_error_t result = polycall_endpoint_send(
        ctx->core_ctx,
        ctx->endpoint, 
        &packet,
        POLYCALL_ENDPOINT_FLAG_NONE
    );
    
    return result == POLYCALL_CORE_SUCCESS;
 }
 
 bool polycall_protocol_process(
     polycall_protocol_context_t* ctx,
     const void* data,
     size_t length
 ) {
     if (!ctx || !data || length < sizeof(polycall_protocol_msg_header_t)) {
         return false;
     }
     
     // Cast config from user_data for callbacks
     polycall_protocol_config_t* config = (polycall_protocol_config_t*)ctx->user_data;
     if (!config) return false;
     
     // Extract header and payload
     const polycall_protocol_msg_header_t* header = (const polycall_protocol_msg_header_t*)data;
     const void* payload = (const uint8_t*)data + sizeof(polycall_protocol_msg_header_t);
     size_t payload_length = length - sizeof(polycall_protocol_msg_header_t);
     
     // Validate message
     if (!validate_message_header(header)) {
         return false;
     }
     
     // Verify checksum
     if (!polycall_protocol_verify_checksum(header, payload, payload_length)) {
         snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH,
                 "Checksum verification failed");
         return false;
     }
     
     // Process message based on type
     switch (header->type) {
         case POLYCALL_PROTOCOL_MSG_HANDSHAKE:
             if (config->callbacks.on_handshake) {
                 config->callbacks.on_handshake(ctx);
             }
             break;
             
         case POLYCALL_PROTOCOL_MSG_AUTH:
             if (config->callbacks.on_auth_request) {
                 config->callbacks.on_auth_request(ctx, payload);
             }
             break;
             
         case POLYCALL_PROTOCOL_MSG_COMMAND:
             if (config->callbacks.on_command) {
                 config->callbacks.on_command(ctx, payload, payload_length);
             }
             break;
             
         case POLYCALL_PROTOCOL_MSG_ERROR:
             if (config->callbacks.on_error) {
                 config->callbacks.on_error(ctx, payload);
             }
             break;
             
         case POLYCALL_PROTOCOL_MSG_HEARTBEAT:
             // Process heartbeat - typically just respond with a heartbeat
             polycall_protocol_send(ctx, POLYCALL_PROTOCOL_MSG_HEARTBEAT, 
                                   "", 1, POLYCALL_PROTOCOL_FLAG_NONE);
             break;
             
         default:
             return false;
     }
     
     return true;
 }
 
 void polycall_protocol_update(polycall_protocol_context_t* ctx) {
     if (!ctx) return;
     
     // Process any pending state transitions
     switch (ctx->state) {
         case POLYCALL_PROTOCOL_STATE_INIT:
             if (polycall_protocol_can_transition(ctx, POLYCALL_PROTOCOL_STATE_HANDSHAKE)) {
                 polycall_protocol_start_handshake(ctx);
             }
             break;
             
         case POLYCALL_PROTOCOL_STATE_HANDSHAKE:
             if (polycall_protocol_can_transition(ctx, POLYCALL_PROTOCOL_STATE_AUTH)) {
                 transition_protocol_state(ctx, POLYCALL_PROTOCOL_STATE_AUTH);
             }
             break;
             
         case POLYCALL_PROTOCOL_STATE_AUTH:
             if (polycall_protocol_can_transition(ctx, POLYCALL_PROTOCOL_STATE_READY)) {
                 transition_protocol_state(ctx, POLYCALL_PROTOCOL_STATE_READY);
             }
             break;
             
         default:
             break;
     }
 }
 
 polycall_protocol_state_t polycall_protocol_get_state(
     const polycall_protocol_context_t* ctx
 ) {
     return ctx ? ctx->state : POLYCALL_PROTOCOL_STATE_ERROR;
 }
 
 bool polycall_protocol_can_transition(
     const polycall_protocol_context_t* ctx,
     polycall_protocol_state_t target_state
 ) {
     if (!ctx || !ctx->state_machine) return false;
     
     // Check if target state is valid based on current state
     switch (ctx->state) {
         case POLYCALL_PROTOCOL_STATE_INIT:
             return target_state == POLYCALL_PROTOCOL_STATE_HANDSHAKE;
             
         case POLYCALL_PROTOCOL_STATE_HANDSHAKE:
             return target_state == POLYCALL_PROTOCOL_STATE_AUTH;
             
         case POLYCALL_PROTOCOL_STATE_AUTH:
             return target_state == POLYCALL_PROTOCOL_STATE_READY;
             
         case POLYCALL_PROTOCOL_STATE_READY:
             return target_state == POLYCALL_PROTOCOL_STATE_ERROR ||
                    target_state == POLYCALL_PROTOCOL_STATE_CLOSED;
             
         case POLYCALL_PROTOCOL_STATE_ERROR:
             return target_state == POLYCALL_PROTOCOL_STATE_CLOSED;
             
         default:
             return false;
     }
 }
 
 bool polycall_protocol_start_handshake(polycall_protocol_context_t* ctx) {
     if (!ctx || ctx->state != POLYCALL_PROTOCOL_STATE_INIT) return false;
     
     // Create handshake payload
     struct {
         uint32_t magic;
         uint8_t version;
         uint16_t flags;
     } handshake = {
         .magic = PROTOCOL_MAGIC,
         .version = PROTOCOL_VERSION,
         .flags = 0
     };
     
     // Send handshake message
     if (!polycall_protocol_send(ctx, POLYCALL_PROTOCOL_MSG_HANDSHAKE,
                               &handshake, sizeof(handshake),
                               POLYCALL_PROTOCOL_FLAG_RELIABLE)) {
         return false;
     }
     
     return transition_protocol_state(ctx, POLYCALL_PROTOCOL_STATE_HANDSHAKE);
 }
 
 bool polycall_protocol_complete_handshake(polycall_protocol_context_t* ctx) {
     if (!ctx || ctx->state != POLYCALL_PROTOCOL_STATE_HANDSHAKE) return false;
     return transition_protocol_state(ctx, POLYCALL_PROTOCOL_STATE_AUTH);
 }
 
 bool polycall_protocol_authenticate(
     polycall_protocol_context_t* ctx,
     const char* credentials,
     size_t credentials_length
 ) {
     if (!ctx || !credentials || credentials_length == 0) return false;
     
     // Send authentication message
     if (!polycall_protocol_send(ctx, POLYCALL_PROTOCOL_MSG_AUTH,
                               credentials, credentials_length,
                               POLYCALL_PROTOCOL_FLAG_ENCRYPTED | POLYCALL_PROTOCOL_FLAG_RELIABLE)) {
         return false;
     }
     
     return transition_protocol_state(ctx, POLYCALL_PROTOCOL_STATE_READY);
 }
 
 uint32_t polycall_protocol_calculate_checksum(const void* data, size_t length) {
     if (!data || length == 0) return 0;
     
     const uint8_t* bytes = (const uint8_t*)data;
     uint32_t checksum = 0;
     
     // Simple Fletcher-like checksum algorithm
     for (size_t i = 0; i < length; i++) {
         checksum = ((checksum << 5) | (checksum >> 27)) + bytes[i];
     }
     
     return checksum;
 }
 
 bool polycall_protocol_verify_checksum(
     const polycall_protocol_msg_header_t* header,
     const void* payload,
     size_t payload_length
 ) {
     if (!header || !payload || payload_length == 0) return false;
     
     uint32_t calculated = polycall_protocol_calculate_checksum(payload, payload_length);
     return calculated == header->checksum;
 }
 
 void polycall_protocol_set_error(
     polycall_protocol_context_t* ctx,
     const char* error
 ) {
     if (!ctx || !error) return;
     
     // Cast config from user_data for error callback
     polycall_protocol_config_t* config = (polycall_protocol_config_t*)ctx->user_data;
     
     // Store error message
     snprintf(protocol_error_buffer, MAX_ERROR_MESSAGE_LENGTH, "%s", error);
     
     // Transition to error state
     transition_protocol_state(ctx, POLYCALL_PROTOCOL_STATE_ERROR);
     
     // Call error callback if available
     if (config && config->callbacks.on_error) {
         config->callbacks.on_error(ctx, protocol_error_buffer);
     }
 }
 bool polycall_protocol_version_compatible(uint8_t remote_version) {
     // For now, only our exact version is compatible
     return remote_version == PROTOCOL_VERSION;
 }