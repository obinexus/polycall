/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file mock_protocol_context.c
 * @brief Mock implementation of protocol context for testing
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file provides mock implementations of the protocol context functionality
 * for use in unit tests of protocol enhancements.
 */

#include "mock_core_context.h"
#include "mock_protocol_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum number of message handlers */
#define MAX_MESSAGE_HANDLERS 32

/* Message handler entry */
typedef struct {
  char command_name[64];
  void *handler;
  void *user_data;
} message_handler_t;

/* Mock protocol context structure */
struct mock_protocol_context {
  uint32_t magic;                          /* Magic number for validation */
  polycall_core_context_t *core_ctx;       /* Core context */
  NetworkEndpoint *endpoint;               /* Network endpoint */
  polycall_state_machine_t *state_machine; /* State machine */
  polycall_protocol_state_t state;         /* Current protocol state */

  /* Message handlers */
  message_handler_t handlers[MAX_MESSAGE_HANDLERS];
  uint32_t handler_count;

  /* Statistics */
  struct {
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t errors;
  } stats;
};

#define MOCK_PROTOCOL_CONTEXT_MAGIC 0x4D505243 /* "MPRC" */

/**
 * @brief Create a mock protocol context for testing
 */
polycall_protocol_context_t *
mock_protocol_context_create(polycall_core_context_t *core_ctx,
                             NetworkEndpoint *endpoint) {
  if (!core_ctx || !endpoint) {
    return NULL;
  }

  /* Allocate context */
  struct mock_protocol_context *ctx =
      mock_core_malloc(core_ctx, sizeof(struct mock_protocol_context));
  if (!ctx) {
    return NULL;
  }

  /* Initialize context */
  memset(ctx, 0, sizeof(struct mock_protocol_context));
  ctx->magic = MOCK_PROTOCOL_CONTEXT_MAGIC;
  ctx->core_ctx = core_ctx;
  ctx->endpoint = endpoint;
  ctx->state = POLYCALL_PROTOCOL_STATE_INIT;

  /* Create state machine */
  polycall_sm_status_t sm_status =
      polycall_sm_create(core_ctx, &ctx->state_machine);
  if (sm_status != POLYCALL_SM_SUCCESS) {
    mock_core_free(core_ctx, ctx);
    return NULL;
  }

  /* Add protocol states */
  polycall_sm_add_state(ctx->state_machine, "init", NULL, NULL, false);
  polycall_sm_add_state(ctx->state_machine, "handshake", NULL, NULL, false);
  polycall_sm_add_state(ctx->state_machine, "auth", NULL, NULL, false);
  polycall_sm_add_state(ctx->state_machine, "ready", NULL, NULL, false);
  polycall_sm_add_state(ctx->state_machine, "error", NULL, NULL, false);
  polycall_sm_add_state(ctx->state_machine, "closed", NULL, NULL, false);

  /* Add transitions */
  polycall_sm_add_transition(ctx->state_machine, "to_handshake", "init",
                             "handshake", NULL, NULL);
  polycall_sm_add_transition(ctx->state_machine, "to_auth", "handshake", "auth",
                             NULL, NULL);
  polycall_sm_add_transition(ctx->state_machine, "to_ready", "auth", "ready",
                             NULL, NULL);
  polycall_sm_add_transition(ctx->state_machine, "to_error", "ready", "error",
                             NULL, NULL);
  polycall_sm_add_transition(ctx->state_machine, "to_closed", "error", "closed",
                             NULL, NULL);

  return (polycall_protocol_context_t *)ctx;
}

/**
 * @brief Destroy a mock protocol context
 */
void mock_protocol_context_destroy(polycall_protocol_context_t *proto_ctx) {
  struct mock_protocol_context *ctx = (struct mock_protocol_context *)proto_ctx;
  if (!ctx || ctx->magic != MOCK_PROTOCOL_CONTEXT_MAGIC) {
    return;
  }

  /* Destroy state machine */
  if (ctx->state_machine) {
    polycall_sm_destroy(ctx->state_machine);
  }

  /* Clear magic and free */
  ctx->magic = 0;
  mock_core_free(ctx->core_ctx, ctx);
}

/**
 * @brief Mock implementation of sending a protocol message
 */
bool mock_protocol_send(polycall_protocol_context_t *proto_ctx,
                        polycall_protocol_msg_type_t type, const void *payload,
                        size_t payload_length,
                        polycall_protocol_flags_t flags) {
  struct mock_protocol_context *ctx = (struct mock_protocol_context *)proto_ctx;
  if (!ctx || ctx->magic != MOCK_PROTOCOL_CONTEXT_MAGIC || !payload) {
    return false;
  }

  /* Create mock protocol message */
  /* In a real implementation, this would construct a proper message */

  /* Create packet with header and payload */
  uint8_t *packet_data = malloc(sizeof(uint32_t) + payload_length);
  if (!packet_data) {
    ctx->stats.errors++;
    return false;
  }

  /* Set message type as first 4 bytes */
  *((uint32_t *)packet_data) = (uint32_t)type;

  /* Copy payload */
  memcpy(packet_data + sizeof(uint32_t), payload, payload_length);

  /* Create network packet */
  NetworkPacket packet = {.data = packet_data,
                          .size = sizeof(uint32_t) + payload_length,
                          .flags = (uint32_t)flags};

  /* Send packet */
  bool result = mock_network_endpoint_send(ctx->endpoint, &packet, 0);

  /* Free packet data */
  free(packet_data);

  /* Update statistics */
  if (result) {
    ctx->stats.messages_sent++;
  } else {
    ctx->stats.errors++;
  }

  return result;
}

/**
 * @brief Mock implementation of processing a received message
 */
bool mock_protocol_process(polycall_protocol_context_t *proto_ctx,
                           const void *data, size_t length) {
  struct mock_protocol_context *ctx = (struct mock_protocol_context *)proto_ctx;
  if (!ctx || ctx->magic != MOCK_PROTOCOL_CONTEXT_MAGIC || !data ||
      length < sizeof(uint32_t)) {
    return false;
  }

  /* Parse message type from first 4 bytes */
  polycall_protocol_msg_type_t type =
      (polycall_protocol_msg_type_t)(*((uint32_t *)data));

  /* Extract payload */
  const void *payload = (const uint8_t *)data + sizeof(uint32_t);
  size_t payload_length = length - sizeof(uint32_t);

  /* Update statistics */
  ctx->stats.messages_received++;

  /* TODO: Call appropriate handler based on message type */

  return true;
}

/**
 * @brief Get the state machine from a mock protocol context
 */
polycall_state_machine_t *
mock_protocol_get_state_machine(polycall_protocol_context_t *proto_ctx) {
  struct mock_protocol_context *ctx = (struct mock_protocol_context *)proto_ctx;
  if (!ctx || ctx->magic != MOCK_PROTOCOL_CONTEXT_MAGIC) {
    return NULL;
  }

  return ctx->state_machine;
}

/**
 * @brief Add a message handler to the mock protocol context
 */
bool mock_protocol_register_message_handler(
    polycall_core_context_t *core_ctx, polycall_protocol_context_t *proto_ctx,
    const char *command_name, void *handler, void *user_data) {
  struct mock_protocol_context *ctx = (struct mock_protocol_context *)proto_ctx;
  if (!ctx || ctx->magic != MOCK_PROTOCOL_CONTEXT_MAGIC || !command_name ||
      !handler) {
    return false;
  }

  /* Check if handler limit reached */
  if (ctx->handler_count >= MAX_MESSAGE_HANDLERS) {
    return false;
  }

  /* Add handler */
  message_handler_t *new_handler = &ctx->handlers[ctx->handler_count++];
  strncpy(new_handler->command_name, command_name,
          sizeof(new_handler->command_name) - 1);
  new_handler->command_name[sizeof(new_handler->command_name) - 1] = '\0';
  new_handler->handler = handler;
  new_handler->user_data = user_data;

  return true;
}
