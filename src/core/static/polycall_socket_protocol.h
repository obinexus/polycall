/**
 * @file polycall_socket_protocol.h
 * @brief WebSocket Protocol Interface for LibPolyCall
 * @author Implementation based on existing LibPolyCall architecture
 */

#ifndef POLYCALL_SOCKET_PROTOCOL_H
#define POLYCALL_SOCKET_PROTOCOL_H

#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of internal frame structure
typedef struct ws_frame ws_frame_t;

/**
 * @brief Create WebSocket handshake request (client)
 */
polycall_core_error_t polycall_socket_create_handshake_request(
    polycall_core_context_t *core_ctx, const char *host, const char *path,
    const char *protocols, uint8_t **buffer, size_t *buffer_size);

/**
 * @brief Parse WebSocket handshake response (client)
 */
polycall_core_error_t polycall_socket_parse_handshake_response(
    polycall_core_context_t *core_ctx, const uint8_t *buffer,
    size_t buffer_size, bool *success, char **protocol);

/**
 * @brief Create WebSocket handshake response (server)
 */
polycall_core_error_t polycall_socket_create_handshake_response(
    polycall_core_context_t *core_ctx, const char *websocket_key,
    const char *protocol, uint8_t **buffer, size_t *buffer_size);

/**
 * @brief Parse WebSocket handshake request (server)
 */
polycall_core_error_t polycall_socket_parse_handshake_request(
    polycall_core_context_t *core_ctx, const uint8_t *buffer,
    size_t buffer_size, char **key, char **protocols);

/**
 * @brief Create WebSocket frame
 */
polycall_core_error_t
polycall_socket_create_frame(polycall_core_context_t *core_ctx, uint8_t opcode,
                             const uint8_t *payload, size_t payload_size,
                             bool mask_payload, uint8_t **frame_buffer,
                             size_t *frame_size);

/**
 * @brief Parse WebSocket frame
 */
polycall_core_error_t
polycall_socket_parse_frame(polycall_core_context_t *core_ctx,
                            const uint8_t *frame_buffer, size_t buffer_size,
                            ws_frame_t **frame);

/**
 * @brief Free WebSocket frame
 */
void polycall_socket_free_frame(polycall_core_context_t *core_ctx,
                                ws_frame_t *frame);

/**
 * @brief Create a WebSocket close frame
 */
polycall_core_error_t polycall_socket_create_close_frame(
    polycall_core_context_t *core_ctx, uint16_t close_code, const char *reason,
    bool mask_payload, uint8_t **frame_buffer, size_t *frame_size);

/**
 * @brief Create a WebSocket ping frame
 */
polycall_core_error_t
polycall_socket_create_ping_frame(polycall_core_context_t *core_ctx,
                                  const uint8_t *payload, size_t payload_size,
                                  bool mask_payload, uint8_t **frame_buffer,
                                  size_t *frame_size);

/**
 * @brief Create a WebSocket pong frame
 */
polycall_core_error_t
polycall_socket_create_pong_frame(polycall_core_context_t *core_ctx,
                                  const uint8_t *payload, size_t payload_size,
                                  bool mask_payload, uint8_t **frame_buffer,
                                  size_t *frame_size);

/**
 * @brief Create a text WebSocket frame
 */
polycall_core_error_t
polycall_socket_create_text_frame(polycall_core_context_t *core_ctx,
                                  const char *text, bool mask_payload,
                                  uint8_t **frame_buffer, size_t *frame_size);

/**
 * @brief Create a binary WebSocket frame
 */
polycall_core_error_t polycall_socket_create_binary_frame(
    polycall_core_context_t *core_ctx, const uint8_t *data, size_t data_size,
    bool mask_payload, uint8_t **frame_buffer, size_t *frame_size);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_SOCKET_PROTOCOL_H */
