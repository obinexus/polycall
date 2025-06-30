/**
 * @file polycall_protocol_context.h
 * @brief Protocol context structure for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the protocol context structure that integrates
 * with the core LibPolyCall system, following the Program-First design
 * approach and providing a messaging layer between applications and
 * network functionality.
 */

 #ifndef POLYCALL_PROTOCOL_POLYCALL_PROTOCOL_CONTEXT_H_H
 #define POLYCALL_PROTOCOL_POLYCALL_PROTOCOL_CONTEXT_H_H

 #include "polycall/core/polycall/polycall_config.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/protocol/enhancements/polycall_state_machine.h"
    #include "polycall/core/polycall/polycall_types.h"
    #include "polycall/core/polycall/polycall_context.h"
    #include "polycall/core/polycall/polycall_error.h"
    #include "polycall/core/polycall/polycall_memory.h"
      
 #include <string.h>
 #include <stdio.h>
 #include <time.h> 
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Protocol state enumeration
  */
 typedef enum {
     POLYCALL_PROTOCOL_STATE_INIT = 0,
     POLYCALL_PROTOCOL_STATE_HANDSHAKE,
     POLYCALL_PROTOCOL_STATE_AUTH,
     POLYCALL_PROTOCOL_STATE_READY,
     POLYCALL_PROTOCOL_STATE_ERROR,
     POLYCALL_PROTOCOL_STATE_CLOSED
 } polycall_protocol_state_t;
 
 /**
  * @brief Protocol message types
  */
 typedef enum {
     POLYCALL_PROTOCOL_MSG_HANDSHAKE = 0,
     POLYCALL_PROTOCOL_MSG_AUTH,
     POLYCALL_PROTOCOL_MSG_COMMAND,
     POLYCALL_PROTOCOL_MSG_RESPONSE,
     POLYCALL_PROTOCOL_MSG_ERROR,
     POLYCALL_PROTOCOL_MSG_HEARTBEAT
 } polycall_protocol_msg_type_t;
 
 /**
  * @brief Protocol message flags
  */
 typedef enum {
     POLYCALL_PROTOCOL_FLAG_NONE = 0,
     POLYCALL_PROTOCOL_FLAG_SECURE = (1 << 0),
     POLYCALL_PROTOCOL_FLAG_RELIABLE = (1 << 1),
     POLYCALL_PROTOCOL_FLAG_ENCRYPTED = (1 << 2),
     POLYCALL_PROTOCOL_FLAG_COMPRESSED = (1 << 3),
     POLYCALL_PROTOCOL_FLAG_FRAGMENTED = (1 << 4),
     POLYCALL_PROTOCOL_FLAG_LAST_FRAGMENT = (1 << 5)
 } polycall_protocol_flags_t;
 
 /**
  * @brief Protocol message header
  */
 typedef struct {
     uint8_t version;                      /**< Protocol version */
     polycall_protocol_msg_type_t type;    /**< Message type */
     polycall_protocol_flags_t flags;      /**< Message flags */
     uint32_t sequence;                    /**< Sequence number */
     uint32_t payload_length;              /**< Payload length */
     uint32_t checksum;                    /**< Message checksum */
 } polycall_protocol_msg_header_t;
 
 /**
  * @brief Protocol configuration
  */
 typedef struct {
     uint32_t max_message_size;            /**< Maximum message size */
     uint32_t timeout_ms;                  /**< Operation timeout in ms */
     polycall_protocol_flags_t flags;      /**< Configuration flags */
     void* user_data;                      /**< User data */
     struct {
         void (*on_handshake)(struct polycall_protocol_context* ctx);
         void (*on_auth_request)(struct polycall_protocol_context* ctx, const void* credentials);
         void (*on_command)(struct polycall_protocol_context* ctx, const void* data, size_t length);
         void (*on_error)(struct polycall_protocol_context* ctx, const void* error_data);
         void (*on_state_change)(struct polycall_protocol_context* ctx, 
                                 polycall_protocol_state_t old_state, 
                                 polycall_protocol_state_t new_state);
     } callbacks;                          /**< Protocol callbacks */
 } polycall_protocol_config_t;
 
 /**
  * @brief Forward declaration for state machine type
  */
 typedef struct polycall_state_machine polycall_state_machine_t;
 
 /**
  * @brief Protocol context structure
  */
 typedef struct polycall_protocol_context {
     polycall_core_context_t* core_ctx;    /**< Core context reference */
     polycall_protocol_state_t state;      /**< Current protocol state */
     NetworkEndpoint* endpoint;            /**< Network endpoint */
     polycall_state_machine_t* state_machine; /**< Protocol state machine */
     uint32_t next_sequence;               /**< Next message sequence number */
     struct {
         uint8_t* buffer;                  /**< Message buffer */
         size_t size;                      /**< Buffer size */
         size_t used;                      /**< Used buffer size */
     } message_cache;                     /**< Message cache */
     void* crypto_context;                 /**< Cryptographic context */
     void* user_data;                      /**< User data */
 } polycall_protocol_context_t;
 
 /**
  * @brief Initialize protocol context
  *
  * @param ctx Protocol context to initialize
  * @param core_ctx Core context
  * @param endpoint Network endpoint
  * @param config Protocol configuration
  * @return true if successful, false otherwise
  */
 bool polycall_protocol_init(
     polycall_protocol_context_t* ctx,
     polycall_core_context_t* core_ctx,
     NetworkEndpoint* endpoint,
     const polycall_protocol_config_t* config
 );
 
 /**
  * @brief Clean up protocol context
  *
  * @param ctx Protocol context to clean up
  */
 void polycall_protocol_cleanup(polycall_protocol_context_t* ctx);
 
 /**
  * @brief Send a protocol message
  *
  * @param ctx Protocol context
  * @param type Message type
  * @param payload Message payload
  * @param payload_length Payload length
  * @param flags Message flags
  * @return true if successful, false otherwise
  */
 bool polycall_protocol_send(
     polycall_protocol_context_t* ctx,
     polycall_protocol_msg_type_t type,
     const void* payload,
     size_t payload_length,
     polycall_protocol_flags_t flags
 );
 
 /**
  * @brief Process received data
  *
  * @param ctx Protocol context
  * @param data Received data
  * @param length Data length
  * @return true if successful, false otherwise
  */
 bool polycall_protocol_process(
     polycall_protocol_context_t* ctx,
     const void* data,
     size_t length
 );
 
 /**
  * @brief Update protocol state
  *
  * @param ctx Protocol context
  */
 void polycall_protocol_update(polycall_protocol_context_t* ctx);
 
 /**
  * @brief Get protocol state
  *
  * @param ctx Protocol context
  * @return Current protocol state
  */
 polycall_protocol_state_t polycall_protocol_get_state(
     const polycall_protocol_context_t* ctx
 );
 
 /**
  * @brief Check if protocol can transition to a state
  *
  * @param ctx Protocol context
  * @param target_state Target state
  * @return true if transition is valid, false otherwise
  */
 bool polycall_protocol_can_transition(
     const polycall_protocol_context_t* ctx,
     polycall_protocol_state_t target_state
 );
 
 /**
  * @brief Start protocol handshake
  *
  * @param ctx Protocol context
  * @return true if successful, false otherwise
  */
 bool polycall_protocol_start_handshake(polycall_protocol_context_t* ctx);
 
 /**
  * @brief Complete protocol handshake
  *
  * @param ctx Protocol context
  * @return true if successful, false otherwise
  */
 bool polycall_protocol_complete_handshake(polycall_protocol_context_t* ctx);
 
 /**
  * @brief Authenticate with the protocol
  *
  * @param ctx Protocol context
  * @param credentials Authentication credentials
  * @param credentials_length Credentials length
  * @return true if successful, false otherwise
  */
 bool polycall_protocol_authenticate(
     polycall_protocol_context_t* ctx,
     const char* credentials,
     size_t credentials_length
 );
 
 /**
  * @brief Calculate message checksum
  *
  * @param data Data to checksum
  * @param length Data length
  * @return Checksum value
  */
 uint32_t polycall_protocol_calculate_checksum(
     const void* data,
     size_t length
 );
 
 /**
  * @brief Verify message checksum
  *
  * @param header Message header
  * @param payload Message payload
  * @param payload_length Payload length
  * @return true if checksum is valid, false otherwise
  */
 bool polycall_protocol_verify_checksum(
     const polycall_protocol_msg_header_t* header,
     const void* payload,
     size_t payload_length
 );
 
 /**
  * @brief Set protocol error
  *
  * @param ctx Protocol context
  * @param error Error message
  */
 void polycall_protocol_set_error(
     polycall_protocol_context_t* ctx,
     const char* error
 );
 
 /**
  * @brief Check protocol version compatibility
  *
  * @param remote_version Remote protocol version
  * @return true if compatible, false otherwise
  */
 bool polycall_protocol_version_compatible(uint8_t remote_version);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_PROTOCOL_POLYCALL_PROTOCOL_CONTEXT_H_H */