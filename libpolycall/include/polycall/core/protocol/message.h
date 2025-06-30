/**
#include <stdbool.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stddef.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdint.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

 * @file message.h
 * @brief Protocol message handling for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the protocol message handling API for LibPolyCall,
 * supporting serialization, deserialization, validation, and routing of messages.
 */

#ifndef POLYCALL_PROTOCOL_MESSAGE_H_H
#define POLYCALL_PROTOCOL_MESSAGE_H_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Message type enumeration
 */
typedef enum {
    POLYCALL_MESSAGE_TYPE_INVALID = 0,
    POLYCALL_MESSAGE_TYPE_HANDSHAKE,
    POLYCALL_MESSAGE_TYPE_COMMAND,
    POLYCALL_MESSAGE_TYPE_RESPONSE,
    POLYCALL_MESSAGE_TYPE_ERROR,
    POLYCALL_MESSAGE_TYPE_PING,
    POLYCALL_MESSAGE_TYPE_PONG,
    POLYCALL_MESSAGE_TYPE_USER = 0x1000
} polycall_message_type_t;

/**
 * @brief Message flags
 */
typedef enum {
    POLYCALL_MESSAGE_FLAG_NONE = 0,
    POLYCALL_MESSAGE_FLAG_ENCRYPTED = (1 << 0),
    POLYCALL_MESSAGE_FLAG_COMPRESSED = (1 << 1),
    POLYCALL_MESSAGE_FLAG_FRAGMENTED = (1 << 2),
    POLYCALL_MESSAGE_FLAG_CONTINUATION = (1 << 3),
    POLYCALL_MESSAGE_FLAG_LAST_FRAGMENT = (1 << 4),
    POLYCALL_MESSAGE_FLAG_REQUIRES_ACK = (1 << 5),
    POLYCALL_MESSAGE_FLAG_IS_ACK = (1 << 6),
    POLYCALL_MESSAGE_FLAG_USER = (1 << 16)
} polycall_message_flags_t;

/**
 * @brief Message header structure
 */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint32_t flags;
    uint32_t sequence;
    uint32_t payload_size;
    uint32_t metadata_size;
    uint32_t checksum;
} polycall_message_header_t;

/**
 * @brief Message structure
 */
typedef struct {
    polycall_message_header_t header;
    void* payload;
    size_t payload_size;
    void* metadata;
    size_t metadata_size;
} polycall_message_t;

/**
 * @brief Message handler function type
 */
typedef polycall_core_error_t (*polycall_message_handler_t)(
    polycall_core_context_t* ctx,
    polycall_message_t* message,
    void* user_data
);

/* Message creation and destruction */
polycall_core_error_t polycall_message_create(
    polycall_core_context_t* ctx,
    polycall_message_t** message,
    polycall_message_type_t type
);

void polycall_message_destroy(
    polycall_core_context_t* ctx,
    polycall_message_t* message
);

/* Payload and metadata management */
polycall_core_error_t polycall_message_set_payload(
    polycall_core_context_t* ctx,
    polycall_message_t* message,
    const void* payload,
    size_t size
);

polycall_core_error_t polycall_message_set_metadata(
    polycall_core_context_t* ctx,
    polycall_message_t* message,
    const void* metadata,
    size_t size
);

polycall_core_error_t polycall_message_set_flags(
    polycall_core_context_t* ctx,
    polycall_message_t* message,
    polycall_message_flags_t flags
);

/* Serialization and deserialization */
polycall_core_error_t polycall_message_serialize(
    polycall_core_context_t* ctx,
    const polycall_message_t* message,
    void** buffer,
    size_t* size
);

polycall_core_error_t polycall_message_deserialize(
    polycall_core_context_t* ctx,
    const void* buffer,
    size_t buffer_size,
    polycall_message_t** message
);

/* Message routing and handling */
polycall_core_error_t polycall_message_register_handler(
    polycall_core_context_t* ctx,
    polycall_message_type_t type,
    polycall_message_handler_t handler,
    void* user_data
);

polycall_core_error_t polycall_message_dispatch(
    polycall_core_context_t* ctx,
    polycall_message_t* message
);

/* Message state observers */
const void* polycall_message_get_payload(
    const polycall_message_t* message,
    size_t* size
);

const void* polycall_message_get_metadata(
    const polycall_message_t* message,
    size_t* size
);

polycall_message_type_t polycall_message_get_type(
    const polycall_message_t* message
);

polycall_message_flags_t polycall_message_get_flags(
    const polycall_message_t* message
);

uint32_t polycall_message_get_sequence(
    const polycall_message_t* message
);

/* String message helpers */
polycall_core_error_t polycall_message_set_string_payload(
    polycall_core_context_t* ctx,
    polycall_message_t* message,
    const char* str
);

const char* polycall_message_get_string_payload(
    const polycall_message_t* message
);

/* Message pool management */
polycall_core_error_t polycall_message_cleanup_pool(
    polycall_core_context_t* ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_PROTOCOL_MESSAGE_H_H */