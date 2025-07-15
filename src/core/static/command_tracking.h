// command_tracking.h - Header for command tracking system
#ifndef POLYCALL_COMMAND_TRACKING_H
#define POLYCALL_COMMAND_TRACKING_H

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/protocol/route_mapping.h"

/**
 * @brief Command tracking entry structure
 */
typedef struct {
  uint64_t correlation_id;         // Correlation ID for tracking
  uint32_t command_id;             // Command ID
  uint64_t timestamp;              // Execution timestamp
  polycall_protocol_state_t state; // Protocol state
  bool completed;                  // Whether command completed
  uint32_t result_code;            // Result code
} polycall_command_tracking_entry_t;

/**
 * @brief Command tracking context
 */
typedef struct {
  polycall_command_tracking_entry_t *entries;  // Tracking entries
  size_t entry_count;                          // Number of entries
  size_t capacity;                             // Maximum entries
  polycall_telemetry_context_t *telemetry_ctx; // Telemetry context
} polycall_command_tracking_t;

/**
 * @brief Initialize command tracking
 */
polycall_core_error_t polycall_command_tracking_init(
    polycall_core_context_t *ctx, polycall_command_tracking_t **tracking,
    polycall_telemetry_context_t *telemetry_ctx, size_t capacity);

/**
 * @brief Begin tracking command execution
 */
uint64_t polycall_command_tracking_begin(polycall_core_context_t *ctx,
                                         polycall_command_tracking_t *tracking,
                                         uint32_t command_id,
                                         polycall_protocol_state_t state);

/**
 * @brief End tracking command execution
 */
polycall_core_error_t
polycall_command_tracking_end(polycall_core_context_t *ctx,
                              polycall_command_tracking_t *tracking,
                              uint64_t correlation_id, uint32_t result_code);

/**
 * @brief Get tracking entry by correlation ID
 */
polycall_command_tracking_entry_t *
polycall_command_tracking_get(polycall_core_context_t *ctx,
                              polycall_command_tracking_t *tracking,
                              uint64_t correlation_id);

/**
 * @brief Cleanup command tracking
 */
void polycall_command_tracking_cleanup(polycall_core_context_t *ctx,
                                       polycall_command_tracking_t *tracking);

#endif /* POLYCALL_COMMAND_TRACKING_H */