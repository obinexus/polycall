/**
#include "polycall/core/protocol/handshake.h"

 * @file handshake.c
 * @brief Protocol handshake implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the protocol handshake functionality for LibPolyCall,
 * enabling secure establishment of connections between endpoints with
 * version negotiation, capability exchange, and protocol initialization.
 * 
 * Version 3 enhancements:
 * - Improved error handling with specific error codes
 * - Timeout management for handshake stages
 * - Robust state machine transition mechanism
 * - Enhanced security parameter negotiation
 * - Detailed logging for diagnostic purposes
 */

 #include "polycall/core/protocol/handshake.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/protocol/crypto.h"
 #include <string.h>
 #include <stdio.h>
 #include <time.h>
 #include <stdarg.h>
 
 // Handshake constants
 #define POLYCALL_HANDSHAKE_MAGIC 0x50434853 /* "PCHS" in ASCII */
 #define POLYCALL_HANDSHAKE_VERSION 1
 #define POLYCALL_HANDSHAKE_TIMEOUT_MS 5000
 #define POLYCALL_HANDSHAKE_MAX_RETRIES 3
 #define POLYCALL_HANDSHAKE_STAGE_TIMEOUT_MS 10000
 #define POLYCALL_HANDSHAKE_RETRY_INTERVAL_MS 1000
 
 // Handshake error codes
 typedef enum {
     HANDSHAKE_ERROR_NONE = 0,
     HANDSHAKE_ERROR_INVALID_STATE,
     HANDSHAKE_ERROR_TIMEOUT,
     HANDSHAKE_ERROR_INVALID_MAGIC,
     HANDSHAKE_ERROR_VERSION_MISMATCH,
     HANDSHAKE_ERROR_SESSION_MISMATCH,
     HANDSHAKE_ERROR_CRYPTO_FAILURE,
     HANDSHAKE_ERROR_PARAMETER_MISMATCH,
     HANDSHAKE_ERROR_MAX_RETRIES_EXCEEDED,
     HANDSHAKE_ERROR_PROTOCOL_VIOLATION
 } handshake_error_t;
 
 // Handshake stages
 typedef enum {
     HANDSHAKE_STAGE_INIT = 0,
     HANDSHAKE_STAGE_HELLO_SENT,
     HANDSHAKE_STAGE_HELLO_RECEIVED,
     HANDSHAKE_STAGE_CAPABILITIES_SENT,
     HANDSHAKE_STAGE_CAPABILITIES_RECEIVED,
     HANDSHAKE_STAGE_PARAMS_SENT,
     HANDSHAKE_STAGE_PARAMS_RECEIVED,
     HANDSHAKE_STAGE_COMPLETE,
     HANDSHAKE_STAGE_FAILED
 } handshake_stage_t;
 
 // Handshake context
 typedef struct {
     handshake_stage_t stage;
     uint32_t retry_count;
     uint64_t last_attempt_time;
     uint64_t stage_start_time;
     handshake_error_t last_error;
     char error_message[256];
     polycall_handshake_capabilities_t local_capabilities;
     polycall_handshake_capabilities_t remote_capabilities;
     polycall_handshake_params_t negotiated_params;
     polycall_crypto_context_t* crypto_ctx;
     uint32_t session_id;
     uint32_t remote_session_id;
     void* user_data;
     
     // Handshake callbacks
     struct {
         void (*on_complete)(polycall_handshake_context_t*, void*);
         void (*on_error)(polycall_handshake_context_t*, const char*, void*);
         void (*on_state_change)(polycall_handshake_context_t*, handshake_stage_t, handshake_stage_t, void*);
     } callbacks;
     
     // Statistics
     struct {
         uint32_t messages_sent;
         uint32_t messages_received;
         uint32_t retries;
         uint64_t start_time;
         uint64_t end_time;
     } stats;
 } handshake_context_t;
 
 // Hello message structure
 typedef struct {
     uint32_t magic;
     uint8_t version;
     uint16_t flags;
     uint32_t session_id;
     uint8_t protocol_options;
     uint8_t reserved[3];
 } handshake_hello_t;
 
 // Capabilities message structure
 typedef struct {
     polycall_handshake_capabilities_t capabilities;
     uint32_t option_flags;
     uint16_t max_message_size;
     uint16_t heartbeat_interval;
     uint8_t supported_features[16]; // Bitmap of supported features
 } handshake_capabilities_t;
 
 // Parameters message structure
 typedef struct {
     polycall_handshake_params_t params;
     uint32_t flags;
     uint16_t selected_features;
     uint16_t reserved;
     uint8_t extended_params[16]; // For future extensions
 } handshake_params_t;
 
 // Forward declarations of internal functions
 static polycall_core_error_t process_hello_message(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     const void* payload,
     size_t payload_size,
     polycall_message_t** response
 );
 
 static polycall_core_error_t process_hello_response(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     const void* payload,
     size_t payload_size
 );
 
 static polycall_core_error_t create_capabilities_message(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     polycall_message_t** message
 );
 
 static polycall_core_error_t process_capabilities_message(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     const void* payload,
     size_t payload_size
 );
 
 static polycall_core_error_t create_params_message(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     polycall_message_t** message
 );
 
 static polycall_core_error_t process_params_message(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     const void* payload,
     size_t payload_size
 );
 
 static void transition_handshake_state(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     handshake_stage_t new_stage
 );
 
 static void set_handshake_error(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     handshake_error_t error,
     const char* format,
     ...
 );
 
 static bool is_stage_timeout(
     handshake_context_t* handshake_ctx,
     uint64_t current_time
 );
 
 // Internal helper functions
 static uint64_t get_current_time_ms() {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
 }
 
 static uint32_t generate_session_id() {
     // Improved implementation with better entropy
     uint32_t time_part = (uint32_t)time(NULL) & 0xFFFF;
     
     // Use higher resolution time for more entropy
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     uint32_t high_res_part = (uint32_t)(ts.tv_nsec & 0xFFFF);
     
     return (time_part << 16) | high_res_part;
 }
 
 static void set_handshake_error(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     handshake_error_t error,
     const char* format,
     ...
 ) {
     if (!handshake_ctx) return;
     
     // Set error code
     handshake_ctx->last_error = error;
     
     // Format error message
     va_list args;
     va_start(args, format);
     vsnprintf(handshake_ctx->error_message, sizeof(handshake_ctx->error_message) - 1, format, args);
     va_end(args);
     handshake_ctx->error_message[sizeof(handshake_ctx->error_message) - 1] = '\0';
     
     // Log error to polycall error system
     if (ctx) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "%s", handshake_ctx->error_message);
     }
     
     // Notify error callback if registered
     if (handshake_ctx->callbacks.on_error) {
         handshake_ctx->callbacks.on_error(
             (polycall_handshake_context_t*)handshake_ctx,
             handshake_ctx->error_message,
             handshake_ctx->user_data
         );
     }
     
     // Transition to failed state
     transition_handshake_state(ctx, handshake_ctx, HANDSHAKE_STAGE_FAILED);
 }
 
 static void transition_handshake_state(
     polycall_core_context_t* ctx,
     handshake_context_t* handshake_ctx,
     handshake_stage_t new_stage
 ) {
     if (!handshake_ctx) return;
     
     handshake_stage_t old_stage = handshake_ctx->stage;
     
     // Don't transition if already in the target state
     if (old_stage == new_stage) {
         return;
     }
     
     // Don't allow transitions out of terminal states
     if (old_stage == HANDSHAKE_STAGE_COMPLETE || old_stage == HANDSHAKE_STAGE_FAILED) {
         if (new_stage != HANDSHAKE_STAGE_INIT) {  // Allow reset to init
             set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_INVALID_STATE,
                                "Cannot transition from terminal state %d to %d", 
                                old_stage, new_stage);
             return;
         }
     }
     
     // Validate state transition
     bool valid_transition = false;
     
     switch (old_stage) {
         case HANDSHAKE_STAGE_INIT:
             valid_transition = (new_stage == HANDSHAKE_STAGE_HELLO_SENT ||
                               new_stage == HANDSHAKE_STAGE_HELLO_RECEIVED ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
         
         case HANDSHAKE_STAGE_HELLO_SENT:
             valid_transition = (new_stage == HANDSHAKE_STAGE_HELLO_RECEIVED ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
             
         case HANDSHAKE_STAGE_HELLO_RECEIVED:
             valid_transition = (new_stage == HANDSHAKE_STAGE_CAPABILITIES_SENT ||
                               new_stage == HANDSHAKE_STAGE_CAPABILITIES_RECEIVED ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
             
         case HANDSHAKE_STAGE_CAPABILITIES_SENT:
             valid_transition = (new_stage == HANDSHAKE_STAGE_CAPABILITIES_RECEIVED ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
             
         case HANDSHAKE_STAGE_CAPABILITIES_RECEIVED:
             valid_transition = (new_stage == HANDSHAKE_STAGE_PARAMS_SENT ||
                               new_stage == HANDSHAKE_STAGE_PARAMS_RECEIVED ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
             
         case HANDSHAKE_STAGE_PARAMS_SENT:
             valid_transition = (new_stage == HANDSHAKE_STAGE_PARAMS_RECEIVED ||
                               new_stage == HANDSHAKE_STAGE_COMPLETE ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
             
         case HANDSHAKE_STAGE_PARAMS_RECEIVED:
             valid_transition = (new_stage == HANDSHAKE_STAGE_PARAMS_SENT ||
                               new_stage == HANDSHAKE_STAGE_COMPLETE ||
                               new_stage == HANDSHAKE_STAGE_FAILED);
             break;
             
         case HANDSHAKE_STAGE_COMPLETE:
         case HANDSHAKE_STAGE_FAILED:
             valid_transition = (new_stage == HANDSHAKE_STAGE_INIT);  // Allow reset to init
             break;
     }
     
     if (!valid_transition) {
         set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_INVALID_STATE,
                            "Invalid state transition from %d to %d", 
                            old_stage, new_stage);
         return;
     }
     
     // Update state
     handshake_ctx->stage = new_stage;
     handshake_ctx->stage_start_time = get_current_time_ms();
     
     // Reset retry count when changing stages
     if (old_stage != new_stage) {
         handshake_ctx->retry_count = 0;
     }
     
     // Handle terminal states
     if (new_stage == HANDSHAKE_STAGE_COMPLETE) {
         handshake_ctx->stats.end_time = get_current_time_ms();
         
         // Notify completion
         if (handshake_ctx->callbacks.on_complete) {
             handshake_ctx->callbacks.on_complete(
                 (polycall_handshake_context_t*)handshake_ctx,
                 handshake_ctx->user_data
             );
         }
     }
     
     // Notify state change
     if (handshake_ctx->callbacks.on_state_change) {
         handshake_ctx->callbacks.on_state_change(
             (polycall_handshake_context_t*)handshake_ctx,
             old_stage,
             new_stage,
             handshake_ctx->user_data
         );
     }
 }
 
 static bool is_stage_timeout(
     handshake_context_t* handshake_ctx,
     uint64_t current_time
 ) {
     if (!handshake_ctx) return true;
     
     // Skip timeout check for terminal states
     if (handshake_ctx->stage == HANDSHAKE_STAGE_COMPLETE ||
         handshake_ctx->stage == HANDSHAKE_STAGE_FAILED ||
         handshake_ctx->stage == HANDSHAKE_STAGE_INIT) {
         return false;
     }
     
     uint64_t elapsed = current_time - handshake_ctx->stage_start_time;
     return elapsed > POLYCALL_HANDSHAKE_STAGE_TIMEOUT_MS;
 }
 
 // Handshake initialization
 polycall_core_error_t polycall_handshake_init(
     polycall_core_context_t* ctx,
     polycall_handshake_context_t** handshake_ctx,
     const polycall_handshake_config_t* config
 ) {
     if (!ctx || !handshake_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate handshake context
     handshake_context_t* new_ctx = polycall_core_malloc(ctx, sizeof(handshake_context_t));
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(handshake_context_t));
     new_ctx->stage = HANDSHAKE_STAGE_INIT;
     new_ctx->retry_count = 0;
     new_ctx->last_attempt_time = 0;
     new_ctx->stage_start_time = get_current_time_ms();
     new_ctx->last_error = HANDSHAKE_ERROR_NONE;
     new_ctx->session_id = generate_session_id();
     new_ctx->remote_session_id = 0;
     new_ctx->user_data = config->user_data;
     
     // Initialize statistics
     new_ctx->stats.messages_sent = 0;
     new_ctx->stats.messages_received = 0;
     new_ctx->stats.retries = 0;
     new_ctx->stats.start_time = get_current_time_ms();
     new_ctx->stats.end_time = 0;
     
     // Copy local capabilities
     memcpy(&new_ctx->local_capabilities, &config->capabilities, 
           sizeof(polycall_handshake_capabilities_t));
     
     // Copy callbacks
     if (config->callbacks.on_complete) {
         new_ctx->callbacks.on_complete = config->callbacks.on_complete;
     }
     
     if (config->callbacks.on_error) {
         new_ctx->callbacks.on_error = config->callbacks.on_error;
     }
     
     if (config->callbacks.on_state_change) {
         new_ctx->callbacks.on_state_change = config->callbacks.on_state_change;
     }
     
     // Initialize crypto context if secure handshake is requested
     if (config->flags & POLYCALL_HANDSHAKE_FLAG_SECURE) {
         polycall_crypto_config_t crypto_config = {
             .key_strength = POLYCALL_CRYPTO_KEY_STRENGTH_HIGH,
             .cipher_mode = POLYCALL_CRYPTO_MODE_AES_GCM,
             .flags = POLYCALL_CRYPTO_FLAG_EPHEMERAL_KEYS,
             .user_data = config->user_data
         };
         
         polycall_core_error_t result = polycall_crypto_init(ctx, &new_ctx->crypto_ctx, &crypto_config);
         if (result != POLYCALL_CORE_SUCCESS) {
             set_handshake_error(ctx, new_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                                "Failed to initialize crypto context (error %d)", result);
             polycall_core_free(ctx, new_ctx);
             return result;
         }
     }
     
     *handshake_ctx = (polycall_handshake_context_t*)new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Handshake cleanup
 void polycall_handshake_cleanup(
     polycall_core_context_t* ctx,
     polycall_handshake_context_t* handshake_ctx
 ) {
     if (!ctx || !handshake_ctx) {
         return;
     }
     
     handshake_context_t* internal_ctx = (handshake_context_t*)handshake_ctx;
     
     // Clean up crypto context if any
     if (internal_ctx->crypto_ctx) {
         polycall_crypto_cleanup(ctx, internal_ctx->crypto_ctx);
         internal_ctx->crypto_ctx = NULL;
     }
     
     // Free context
     polycall_core_free(ctx, internal_ctx);
 }
 
 // Start handshake process
 polycall_core_error_t polycall_handshake_start(
     polycall_core_context_t* ctx,
     polycall_handshake_context_t* handshake_ctx,
     polycall_message_t** message
 ) {
     if (!ctx || !handshake_ctx || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     handshake_context_t* internal_ctx = (handshake_context_t*)handshake_ctx;
     
     // Check handshake state
     if (internal_ctx->stage != HANDSHAKE_STAGE_INIT) {
         set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_INVALID_STATE,
                            "Handshake start called from invalid state %d", 
                            internal_ctx->stage);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Create hello message
     polycall_message_t* hello_message;
     polycall_core_error_t result = polycall_message_create(
         ctx, &hello_message, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                            "Failed to create hello message (error %d)", result);
         return result;
     }
     
     // Prepare hello payload
     handshake_hello_t hello = {
         .magic = POLYCALL_HANDSHAKE_MAGIC,
         .version = POLYCALL_HANDSHAKE_VERSION,
         .flags = 0, // Default flags
         .session_id = internal_ctx->session_id,
         .protocol_options = 0,
         .reserved = {0}
     };
     
     // Set flags based on capabilities
     if (internal_ctx->local_capabilities.security_level >= POLYCALL_SECURITY_LEVEL_HIGH) {
         hello.flags |= POLYCALL_HANDSHAKE_FLAG_SECURE;
     }
     
     if (internal_ctx->local_capabilities.compression_supported) {
         hello.flags |= POLYCALL_HANDSHAKE_FLAG_COMPRESSION;
     }
     
     if (internal_ctx->local_capabilities.streaming_supported) {
         hello.protocol_options |= 0x01;  // Streaming support flag
     }
     
     if (internal_ctx->local_capabilities.fragmentation_supported) {
         hello.protocol_options |= 0x02;  // Fragmentation support flag
     }
     
     // Set hello message payload
     result = polycall_message_set_payload(ctx, hello_message, &hello, sizeof(hello));
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_message_destroy(ctx, hello_message);
         set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                            "Failed to set hello message payload (error %d)", result);
         return result;
     }
     
     // Set message flags
     polycall_message_set_flags(ctx, hello_message, POLYCALL_MESSAGE_FLAG_RELIABLE);
     
     // Update handshake statistics
     internal_ctx->stats.messages_sent++;
     
     // Update handshake state
     transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_HELLO_SENT);
     internal_ctx->last_attempt_time = get_current_time_ms();
     
     *message = hello_message;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Process handshake response
 polycall_core_error_t polycall_handshake_process(
     polycall_core_context_t* ctx,
     polycall_handshake_context_t* handshake_ctx,
     const polycall_message_t* message,
     polycall_message_t** response
 ) {
     if (!ctx || !handshake_ctx || !message) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     handshake_context_t* internal_ctx = (handshake_context_t*)handshake_ctx;
     polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
     
     // Initialize response pointer
     if (response) {
         *response = NULL;
     }
     
     // Check if handshake is in terminal state
     if (internal_ctx->stage == HANDSHAKE_STAGE_COMPLETE) {
         // Handshake already completed, ignore the message
         return POLYCALL_CORE_SUCCESS;
     }
     
     if (internal_ctx->stage == HANDSHAKE_STAGE_FAILED) {
         // Handshake already failed, can't process further messages
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check message type
     polycall_message_type_t msg_type = polycall_message_get_type(message);
     if (msg_type != POLYCALL_MESSAGE_TYPE_HANDSHAKE) {
         set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                            "Invalid message type for handshake: %d", msg_type);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Process message based on current handshake stage
     size_t payload_size = 0;
     const void* payload = polycall_message_get_payload(message, &payload_size);
     
     // Update statistics
     internal_ctx->stats.messages_received++;
     
     switch (internal_ctx->stage) {
         case HANDSHAKE_STAGE_INIT:
             // Received unsolicited hello, process it and send response
             if (payload_size < sizeof(handshake_hello_t)) {
                 set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                    "Invalid hello message size: %zu < %zu", 
                                    payload_size, sizeof(handshake_hello_t));
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             result = process_hello_message(ctx, internal_ctx, payload, payload_size, response);
             if (result == POLYCALL_CORE_SUCCESS) {
                 transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_HELLO_RECEIVED);
             }
             break;
             
         case HANDSHAKE_STAGE_HELLO_SENT:
             // Process hello response and send capabilities
             if (payload_size < sizeof(handshake_hello_t)) {
                 set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                    "Invalid hello response size: %zu < %zu", 
                                    payload_size, sizeof(handshake_hello_t));
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             result = process_hello_response(ctx, internal_ctx, payload, payload_size);
             if (result == POLYCALL_CORE_SUCCESS) {
                 transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_HELLO_RECEIVED);
                 
                 // Create and send capabilities message if response is requested
                 if (response) {
                     result = create_capabilities_message(ctx, internal_ctx, response);
                     if (result == POLYCALL_CORE_SUCCESS) {
                         transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_CAPABILITIES_SENT);
                     }
                 }
             }
             break;
             
         case HANDSHAKE_STAGE_HELLO_RECEIVED:
             // Process capabilities message and send capabilities response
             if (payload_size < sizeof(handshake_capabilities_t)) {
                 set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                    "Invalid capabilities message size: %zu < %zu", 
                                    payload_size, sizeof(handshake_capabilities_t));
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             result = process_capabilities_message(ctx, internal_ctx, payload, payload_size);
             if (result == POLYCALL_CORE_SUCCESS) {
                 transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_CAPABILITIES_RECEIVED);
                 
                 // Create and send capabilities message if response is requested
                 if (response) {
                     result = create_capabilities_message(ctx, internal_ctx, response);
                     if (result == POLYCALL_CORE_SUCCESS) {
                         transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_CAPABILITIES_SENT);
                     }
                 }
             }
             break;
             
         case HANDSHAKE_STAGE_CAPABILITIES_SENT:
             // Process capabilities response and send parameters
             if (payload_size < sizeof(handshake_capabilities_t)) {
                 set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                    "Invalid capabilities response size: %zu < %zu", 
                                    payload_size, sizeof(handshake_capabilities_t));
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             result = process_capabilities_message(ctx, internal_ctx, payload, payload_size);
             if (result == POLYCALL_CORE_SUCCESS) {
                 transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_CAPABILITIES_RECEIVED);
                 
                 // Create and send parameters message if response is requested
                 if (response) {
                     result = create_params_message(ctx, internal_ctx, response);
                     if (result == POLYCALL_CORE_SUCCESS) {
                         transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_PARAMS_SENT);
                     }
                 }
             }
             break;
             
         case HANDSHAKE_STAGE_CAPABILITIES_RECEIVED:
             // Process parameters message and send parameters response
             if (payload_size < sizeof(handshake_params_t)) {
                 set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                    "Invalid parameters message size: %zu < %zu", 
                                    payload_size, sizeof(handshake_params_t));
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             result = process_params_message(ctx, internal_ctx, payload, payload_size);
             if (result == POLYCALL_CORE_SUCCESS) {
                 transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_PARAMS_RECEIVED);
                 
                 // Create and send parameters message if response is requested
                 if (response) {
                     result = create_params_message(ctx, internal_ctx, response);
                     if (result == POLYCALL_CORE_SUCCESS) {
                         transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_PARAMS_SENT);
                         
                         // Handshake is now complete
                         transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_COMPLETE);
                     }
                     }
                 }
                 break;
                 
             case HANDSHAKE_STAGE_PARAMS_SENT:
                 // Process parameters response and complete handshake
                 if (payload_size < sizeof(handshake_params_t)) {
                     set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                        "Invalid parameters response size: %zu < %zu", 
                                        payload_size, sizeof(handshake_params_t));
                     return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
                 }
                 
                 result = process_params_message(ctx, internal_ctx, payload, payload_size);
                 if (result == POLYCALL_CORE_SUCCESS) {
                     // Handshake is now complete
                     transition_handshake_state(ctx, internal_ctx, HANDSHAKE_STAGE_COMPLETE);
                 }
                 break;
                 
             case HANDSHAKE_STAGE_PARAMS_RECEIVED:
                 // Unexpected message in PARAMS_RECEIVED state
                 set_handshake_error(ctx, internal_ctx, HANDSHAKE_ERROR_PROTOCOL_VIOLATION,
                                    "Unexpected message in PARAMS_RECEIVED state");
                 result = POLYCALL_CORE_ERROR_INVALID_STATE;
                 break;
                 
             case HANDSHAKE_STAGE_COMPLETE:
             case HANDSHAKE_STAGE_FAILED:
                 // Already in terminal state
                 break;
         }
         
         return result;
     }
     
    // 1. process_hello_message - Handles incoming hello message in INIT state
    static polycall_core_error_t process_hello_message(
        polycall_core_context_t* ctx,
        handshake_context_t* handshake_ctx,
        const void* payload,
        size_t payload_size,
        polycall_message_t** response
    ) {
        if (!ctx || !handshake_ctx || !payload || !response) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        const handshake_hello_t* hello = (const handshake_hello_t*)payload;
        
        // Validate magic number
        if (hello->magic != POLYCALL_HANDSHAKE_MAGIC) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_INVALID_MAGIC,
                               "Invalid hello magic: 0x%08x", hello->magic);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Validate version
        if (hello->version != POLYCALL_HANDSHAKE_VERSION) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_VERSION_MISMATCH,
                               "Unsupported handshake version: %d", hello->version);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Store remote session ID
        handshake_ctx->remote_session_id = hello->session_id;
        
        // Create hello response message
        polycall_message_t* hello_response;
        polycall_core_error_t result = polycall_message_create(
            ctx, &hello_response, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                               "Failed to create hello response message (error %d)", result);
            return result;
        }
        
        // Prepare hello response payload
        handshake_hello_t response_hello = {
            .magic = POLYCALL_HANDSHAKE_MAGIC,
            .version = POLYCALL_HANDSHAKE_VERSION,
            .flags = 0,
            .session_id = handshake_ctx->session_id,
            .protocol_options = 0,
            .reserved = {0}
        };
        
        // Set flags based on local capabilities
        if (handshake_ctx->local_capabilities.security_level >= POLYCALL_SECURITY_LEVEL_HIGH) {
            response_hello.flags |= POLYCALL_HANDSHAKE_FLAG_SECURE;
        }
        
        if (handshake_ctx->local_capabilities.compression_supported) {
            response_hello.flags |= POLYCALL_HANDSHAKE_FLAG_COMPRESSION;
        }
        
        if (handshake_ctx->local_capabilities.streaming_supported) {
            response_hello.protocol_options |= 0x01;  // Streaming support flag
        }
        
        if (handshake_ctx->local_capabilities.fragmentation_supported) {
            response_hello.protocol_options |= 0x02;  // Fragmentation support flag
        }
        
        // Set hello response message payload
        result = polycall_message_set_payload(ctx, hello_response, &response_hello, sizeof(response_hello));
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_message_destroy(ctx, hello_response);
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                               "Failed to set hello response payload (error %d)", result);
            return result;
        }
        
        // Set message flags
        polycall_message_set_flags(ctx, hello_response, POLYCALL_MESSAGE_FLAG_RELIABLE);
        
        // Update handshake statistics
        handshake_ctx->stats.messages_sent++;
        
        *response = hello_response;
        return POLYCALL_CORE_SUCCESS;
    }

    // 2. process_hello_response - Handles hello response in HELLO_SENT state
    static polycall_core_error_t process_hello_response(
        polycall_core_context_t* ctx,
        handshake_context_t* handshake_ctx,
        const void* payload,
        size_t payload_size
    ) {
        if (!ctx || !handshake_ctx || !payload) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        const handshake_hello_t* hello = (const handshake_hello_t*)payload;
        
        // Validate magic number
        if (hello->magic != POLYCALL_HANDSHAKE_MAGIC) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_INVALID_MAGIC,
                               "Invalid hello magic: 0x%08x", hello->magic);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Validate version
        if (hello->version != POLYCALL_HANDSHAKE_VERSION) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_VERSION_MISMATCH,
                               "Unsupported handshake version: %d", hello->version);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Store remote session ID
        handshake_ctx->remote_session_id = hello->session_id;
        
        return POLYCALL_CORE_SUCCESS;
    }

    // 3. create_capabilities_message - Creates capabilities message for local capabilities
    static polycall_core_error_t create_capabilities_message(
        polycall_core_context_t* ctx,
        handshake_context_t* handshake_ctx,
        polycall_message_t** message
    ) {
        if (!ctx || !handshake_ctx || !message) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Create capabilities message
        polycall_message_t* capabilities_message;
        polycall_core_error_t result = polycall_message_create(
            ctx, &capabilities_message, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                               "Failed to create capabilities message (error %d)", result);
            return result;
        }
        
        // Prepare capabilities payload
        handshake_capabilities_t capabilities = {
            .capabilities = handshake_ctx->local_capabilities,
            .option_flags = 0,
            .max_message_size = 16384,  // Default max message size
            .heartbeat_interval = 30000,  // Default 30s heartbeat interval
            .supported_features = {0}
        };
        
        // Set supported features bits
        if (handshake_ctx->local_capabilities.security_level > POLYCALL_SECURITY_LEVEL_NONE) {
            capabilities.supported_features[0] |= 0x01;  // Security support
        }
        
        if (handshake_ctx->local_capabilities.compression_supported) {
            capabilities.supported_features[0] |= 0x02;  // Compression support
        }
        
        if (handshake_ctx->local_capabilities.encryption_supported) {
            capabilities.supported_features[0] |= 0x04;  // Encryption support
        }
        
        if (handshake_ctx->local_capabilities.streaming_supported) {
            capabilities.supported_features[0] |= 0x08;  // Streaming support
        }
        
        if (handshake_ctx->local_capabilities.fragmentation_supported) {
            capabilities.supported_features[0] |= 0x10;  // Fragmentation support
        }
        
        // Set capabilities message payload
        result = polycall_message_set_payload(ctx, capabilities_message, &capabilities, sizeof(capabilities));
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_message_destroy(ctx, capabilities_message);
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                               "Failed to set capabilities payload (error %d)", result);
            return result;
        }
        
        // Set message flags
        polycall_message_set_flags(ctx, capabilities_message, POLYCALL_MESSAGE_FLAG_RELIABLE);
        
        // Update handshake statistics
        handshake_ctx->stats.messages_sent++;
        
        *message = capabilities_message;
        return POLYCALL_CORE_SUCCESS;
    }

    // 4. process_capabilities_message - Processes remote capabilities message
    static polycall_core_error_t process_capabilities_message(
        polycall_core_context_t* ctx,
        handshake_context_t* handshake_ctx,
        const void* payload,
        size_t payload_size
    ) {
        if (!ctx || !handshake_ctx || !payload) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        const handshake_capabilities_t* capabilities = (const handshake_capabilities_t*)payload;
        
        // Store remote capabilities
        memcpy(&handshake_ctx->remote_capabilities, &capabilities->capabilities,
               sizeof(polycall_handshake_capabilities_t));
        
        // Negotiate parameters here - implement detailed capability negotiation logic
        // 1. Security level should be the minimum of the two
        handshake_ctx->negotiated_params.security_level = 
            (handshake_ctx->local_capabilities.security_level < handshake_ctx->remote_capabilities.security_level) ?
            handshake_ctx->local_capabilities.security_level : handshake_ctx->remote_capabilities.security_level;
        
        // 2. Feature enablement requires both sides to support
        handshake_ctx->negotiated_params.use_compression = 
            handshake_ctx->local_capabilities.compression_supported && 
            handshake_ctx->remote_capabilities.compression_supported;
        
        handshake_ctx->negotiated_params.use_encryption = 
            handshake_ctx->local_capabilities.encryption_supported && 
            handshake_ctx->remote_capabilities.encryption_supported;
        
        handshake_ctx->negotiated_params.use_streaming = 
            handshake_ctx->local_capabilities.streaming_supported && 
            handshake_ctx->remote_capabilities.streaming_supported;
        
        handshake_ctx->negotiated_params.use_fragmentation = 
            handshake_ctx->local_capabilities.fragmentation_supported && 
            handshake_ctx->remote_capabilities.fragmentation_supported;
        
        // 3. Calculate max message size as minimum of both sides
        handshake_ctx->negotiated_params.max_message_size = 
            (handshake_ctx->local_capabilities.max_message_size < handshake_ctx->remote_capabilities.max_message_size) ?
            handshake_ctx->local_capabilities.max_message_size : handshake_ctx->remote_capabilities.max_message_size;
        
        // Ensure min message size is reasonable (1KB)
        if (handshake_ctx->negotiated_params.max_message_size < 1024) {
            handshake_ctx->negotiated_params.max_message_size = 1024;
        }
        
        // 4. Calculate heartbeat interval as maximum of both sides
        handshake_ctx->negotiated_params.heartbeat_interval_ms = 
            (handshake_ctx->local_capabilities.heartbeat_interval_ms > handshake_ctx->remote_capabilities.heartbeat_interval_ms) ?
            handshake_ctx->local_capabilities.heartbeat_interval_ms : handshake_ctx->remote_capabilities.heartbeat_interval_ms;
        
        return POLYCALL_CORE_SUCCESS;
    }

    // 5. create_params_message - Creates message with negotiated parameters
    static polycall_core_error_t create_params_message(
        polycall_core_context_t* ctx,
        handshake_context_t* handshake_ctx,
        polycall_message_t** message
    ) {
        if (!ctx || !handshake_ctx || !message) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Create parameters message
        polycall_message_t* params_message;
        polycall_core_error_t result = polycall_message_create(
            ctx, &params_message, POLYCALL_MESSAGE_TYPE_HANDSHAKE);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                               "Failed to create params message (error %d)", result);
            return result;
        }
        
        // Prepare parameters payload
        handshake_params_t params = {
            .params = handshake_ctx->negotiated_params,
            .flags = 0,
            .selected_features = 0,
            .reserved = 0,
            .extended_params = {0}
        };
        
        // Set selected features bits
        if (handshake_ctx->negotiated_params.use_compression) {
            params.selected_features |= 0x01;
        }
        
        if (handshake_ctx->negotiated_params.use_encryption) {
            params.selected_features |= 0x02;
        }
        
        if (handshake_ctx->negotiated_params.use_streaming) {
            params.selected_features |= 0x04;
        }
        
        if (handshake_ctx->negotiated_params.use_fragmentation) {
            params.selected_features |= 0x08;
        }
        
        // Set flags based on security level
        if (handshake_ctx->negotiated_params.security_level >= POLYCALL_SECURITY_LEVEL_HIGH) {
            params.flags |= 0x01;  // High security
        } else if (handshake_ctx->negotiated_params.security_level >= POLYCALL_SECURITY_LEVEL_MEDIUM) {
            params.flags |= 0x02;  // Medium security
        }
        
        // Set parameters message payload
        result = polycall_message_set_payload(ctx, params_message, &params, sizeof(params));
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_message_destroy(ctx, params_message);
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_CRYPTO_FAILURE,
                               "Failed to set params payload (error %d)", result);
            return result;
        }
        
        // Set message flags
        polycall_message_set_flags(ctx, params_message, POLYCALL_MESSAGE_FLAG_RELIABLE);
        
        // Update handshake statistics
        handshake_ctx->stats.messages_sent++;
        
        *message = params_message;
        return POLYCALL_CORE_SUCCESS;
    }

    // 6. process_params_message - Processes negotiated parameters message
    static polycall_core_error_t process_params_message(
        polycall_core_context_t* ctx,
        handshake_context_t* handshake_ctx,
        const void* payload,
        size_t payload_size
    ) {
        if (!ctx || !handshake_ctx || !payload) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        const handshake_params_t* params = (const handshake_params_t*)payload;
        
        // Verify negotiated parameters against local expectations
        polycall_handshake_params_t remote_params = params->params;
        
        // Validate security level is acceptable
        if (remote_params.security_level < handshake_ctx->local_capabilities.min_security_level) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_PARAMETER_MISMATCH,
                               "Security level too low: %d < %d", 
                               remote_params.security_level,
                               handshake_ctx->local_capabilities.min_security_level);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Validate encryption if required locally
        if (handshake_ctx->local_capabilities.encryption_required && !remote_params.use_encryption) {
            set_handshake_error(ctx, handshake_ctx, HANDSHAKE_ERROR_PARAMETER_MISMATCH,
                               "Encryption required but not negotiated");
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Accept remote parameters if they match our expectations
        memcpy(&handshake_ctx->negotiated_params, &remote_params, sizeof(polycall_handshake_params_t));
        
        // Apply security parameters if needed
        if (handshake_ctx->negotiated_params.use_encryption && handshake_ctx->crypto_ctx) {
            // Configure crypto context with negotiated parameters
            polycall_crypto_update_config(ctx, handshake_ctx->crypto_ctx, 
                                         handshake_ctx->negotiated_params.security_level);
        }
        
        return POLYCALL_CORE_SUCCESS;
    }
                     