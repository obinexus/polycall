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

// route_mapping.c - Implementation of route mapping
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/protocol/route_mapping.h"
#include <string.h>

#define MAX_ROUTES 256
#define ROUTE_GUID_PREFIX "route:"

/**
 * @brief Initialize route mapping
 */
polycall_core_error_t polycall_route_mapping_init(
    polycall_core_context_t *ctx, polycall_route_mapping_t **mapping,
    polycall_state_machine_t *sm, polycall_telemetry_context_t *telemetry_ctx) {
  if (!ctx || !mapping || !sm) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Allocate route mapping context
  polycall_route_mapping_t *new_mapping =
      polycall_core_malloc(ctx, sizeof(polycall_route_mapping_t));
  if (!new_mapping) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize context
  memset(new_mapping, 0, sizeof(polycall_route_mapping_t));
  new_mapping->sm = sm;
  new_mapping->telemetry_ctx = telemetry_ctx;

  // Allocate routes array
  new_mapping->routes =
      polycall_core_malloc(ctx, MAX_ROUTES * sizeof(polycall_route_entry_t));
  if (!new_mapping->routes) {
    polycall_core_free(ctx, new_mapping);
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize routes
  memset(new_mapping->routes, 0, MAX_ROUTES * sizeof(polycall_route_entry_t));
  new_mapping->route_count = 0;

  *mapping = new_mapping;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Add route to mapping
 */
polycall_core_error_t polycall_route_mapping_add_route(
    polycall_core_context_t *ctx, polycall_route_mapping_t *mapping,
    const polycall_route_descriptor_t *descriptor) {
  if (!ctx || !mapping || !descriptor || !descriptor->handler) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Check if we have space for another route
  if (mapping->route_count >= MAX_ROUTES) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Add route
  polycall_route_entry_t *entry = &mapping->routes[mapping->route_count++];
  memcpy(&entry->descriptor, descriptor, sizeof(polycall_route_descriptor_t));
  entry->next_state = NULL;
  entry->transition_mask = 0;

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Generate telemetry event for command execution
 */
static void generate_command_telemetry(polycall_core_context_t *ctx,
                                       polycall_route_mapping_t *mapping,
                                       uint32_t command_id,
                                       uint64_t correlation_id, bool success,
                                       const char *error_message) {
  if (!mapping->telemetry_ctx) {
    return;
  }

  // Create telemetry event
  polycall_telemetry_event_t event = {
      .timestamp = 0, // Will be set by telemetry system
      .severity = success ? POLYCALL_TELEMETRY_INFO : POLYCALL_TELEMETRY_ERROR,
      .category = TELEMETRY_CATEGORY_PROTOCOL,
      .source_module = "command_router",
      .event_id =
          success ? "command_execution_success" : "command_execution_failure",
      .description = success ? "Command executed successfully" : error_message,
      .additional_data = NULL,
      .additional_data_size = 0};

  // Record event
  polycall_telemetry_record_event(mapping->telemetry_ctx, &event);
}

/**
 * @brief Find route for command in current state
 */
static polycall_route_entry_t *
find_route_for_command(polycall_route_mapping_t *mapping, uint32_t command_id,
                       polycall_protocol_state_t state) {
  for (size_t i = 0; i < mapping->route_count; i++) {
    polycall_route_entry_t *entry = &mapping->routes[i];
    if (entry->descriptor.command_id == command_id &&
        (entry->descriptor.state == state ||
         entry->descriptor.state == POLYCALL_PROTOCOL_STATE_ANY)) {
      return entry;
    }
  }

  return NULL;
}

/**
 * @brief Process command through route mapping
 */
polycall_core_error_t polycall_route_mapping_process_command(
    polycall_core_context_t *ctx, polycall_route_mapping_t *mapping,
    const polycall_command_message_t *message,
    polycall_command_response_t **response, uint64_t correlation_id) {
  if (!ctx || !mapping || !message || !response) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get current state from state machine
  polycall_protocol_state_t current_state =
      polycall_sm_get_current_state_index(mapping->sm);

  // Find route for command
  polycall_route_entry_t *route = find_route_for_command(
      mapping, message->header.command_id, current_state);

  if (!route) {
    generate_command_telemetry(ctx, mapping, message->header.command_id,
                               correlation_id, false,
                               "No route found for command");

    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Execute command handler
  *response = route->descriptor.handler(
      ctx,
      NULL, // Protocol context would be provided in a full implementation
      message, route->descriptor.user_data);

  // Check if handler execution was successful
  if (!*response) {
    generate_command_telemetry(ctx, mapping, message->header.command_id,
                               correlation_id, false, "Command handler failed");

    return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
  }

  // Generate success telemetry
  generate_command_telemetry(ctx, mapping, message->header.command_id,
                             correlation_id, true, NULL);

  // Handle state transitions if next state is defined
  if (route->next_state) {
    // Transition to next state would be handled here
    // This would integrate with the state machine
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Connect state transitions to routes
 */
polycall_core_error_t polycall_route_mapping_connect_states(
    polycall_core_context_t *ctx, polycall_route_mapping_t *mapping,
    uint32_t from_command_id, uint32_t to_command_id,
    uint32_t transition_mask) {
  if (!ctx || !mapping) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Find source and target routes
  polycall_route_entry_t *from_route = NULL;
  polycall_route_entry_t *to_route = NULL;

  for (size_t i = 0; i < mapping->route_count; i++) {
    if (mapping->routes[i].descriptor.command_id == from_command_id) {
      from_route = &mapping->routes[i];
    }
    if (mapping->routes[i].descriptor.command_id == to_command_id) {
      to_route = &mapping->routes[i];
    }
  }

  if (!from_route || !to_route) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Connect routes
  from_route->next_state = to_route;
  from_route->transition_mask = transition_mask;

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup route mapping
 */
void polycall_route_mapping_cleanup(polycall_core_context_t *ctx,
                                    polycall_route_mapping_t *mapping) {
  if (!ctx || !mapping) {
    return;
  }

  // Free routes array
  if (mapping->routes) {
    polycall_core_free(ctx, mapping->routes);
  }

  // Free mapping context
  polycall_core_free(ctx, mapping);
}
