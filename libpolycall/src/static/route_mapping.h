// route_mapping.h - Header file for route mapping implementation
#ifndef POLYCALL_ROUTE_MAPPING_H
#define POLYCALL_ROUTE_MAPPING_H

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/protocol/command.h"
#include "polycall/core/protocol/protocol_state_machine.h"
#include "polycall/core/telemetry/polycall_telemetry.h"

/**
 * @brief Route descriptor structure defining command routing rules
 */
typedef struct {
    uint32_t command_id;                // Command ID to route
    polycall_protocol_state_t state;    // Valid protocol state
    uint32_t flags;                     // Routing flags
    polycall_command_handler_t handler; // Command handler function
    void* user_data;                    // User data for handler
} polycall_route_descriptor_t;

/**
 * @brief Route entry structure for internal route tracking
 */
typedef struct polycall_route_entry {
    polycall_route_descriptor_t descriptor;  // Route descriptor
    struct polycall_route_entry* next_state; // Next state (for DFA/NFA)
    uint32_t transition_mask;                // State transition mask
} polycall_route_entry_t;

/**
 * @brief Route mapping context
 */
typedef struct {
    polycall_route_entry_t* routes;    // Route entries
    size_t route_count;                // Number of routes
    polycall_state_machine_t* sm;      // State machine reference
    polycall_telemetry_context_t* telemetry_ctx; // Telemetry context
} polycall_route_mapping_t;

/**
 * @brief Initialize route mapping
 */
polycall_core_error_t polycall_route_mapping_init(
    polycall_core_context_t* ctx,
    polycall_route_mapping_t** mapping,
    polycall_state_machine_t* sm,
    polycall_telemetry_context_t* telemetry_ctx
);

/**
 * @brief Add route to mapping
 */
polycall_core_error_t polycall_route_mapping_add_route(
    polycall_core_context_t* ctx,
    polycall_route_mapping_t* mapping,
    const polycall_route_descriptor_t* descriptor
);

/**
 * @brief Process command through route mapping
 */
polycall_core_error_t polycall_route_mapping_process_command(
    polycall_core_context_t* ctx,
    polycall_route_mapping_t* mapping,
    const polycall_command_message_t* message,
    polycall_command_response_t** response,
    uint64_t correlation_id
);

/**
 * @brief Connect state transitions to routes
 */
polycall_core_error_t polycall_route_mapping_connect_states(
    polycall_core_context_t* ctx,
    polycall_route_mapping_t* mapping,
    uint32_t from_command_id,
    uint32_t to_command_id,
    uint32_t transition_mask
);

/**
 * @brief Cleanup route mapping
 */
void polycall_route_mapping_cleanup(
    polycall_core_context_t* ctx,
    polycall_route_mapping_t* mapping
);

#endif /* POLYCALL_ROUTE_MAPPING_H */