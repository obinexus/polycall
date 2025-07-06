/**
 * @file polycall_guid.h
 * @brief Cryptographic GUID implementation for secure state tracking
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_GUID_H
#define POLYCALL_GUID_H

#include "polycall/core/polycall/polycall_types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief GUID structure for cryptographic state tracking
 */
typedef struct {
  uint8_t bytes[16];         /**< 128-bit GUID value */
  uint32_t current_state;    /**< Current state encoded in GUID */
  uint32_t transition_count; /**< Number of state transitions */
  uint64_t timestamp;        /**< Creation timestamp */
} polycall_guid_t;

/**
 * @brief Generate a cryptographic GUID based on context information
 *
 * @param ctx Protocol context
 * @param command_path Command path for GUID scope
 * @param state_id Initial state identifier
 * @param user_identity Optional user identity (NULL if unauthenticated)
 * @return polycall_guid_t The generated GUID
 */
polycall_guid_t polycall_generate_cryptonomic_guid(polycall_core_context_t *ctx,
                                                   const char *command_path,
                                                   uint32_t state_id,
                                                   const char *user_identity);

/**
 * @brief Update GUID with new state transition information
 *
 * @param ctx Protocol context
 * @param current_guid Current GUID to update
 * @param new_state New state identifier
 * @param transition_name Transition name/identifier
 * @return polycall_guid_t Updated GUID reflecting the state transition
 */
polycall_guid_t polycall_update_guid_state(polycall_core_context_t *ctx,
                                           polycall_guid_t current_guid,
                                           uint32_t new_state,
                                           const char *transition_name);

/**
 * @brief Validate GUID authenticity and integrity
 *
 * @param ctx Protocol context
 * @param guid GUID to validate
 * @return bool True if GUID is valid, false otherwise
 */
bool polycall_guid_validate(polycall_core_context_t *ctx, polycall_guid_t guid);

/**
 * @brief Convert GUID to string representation
 *
 * @param guid GUID to convert
 * @param buffer Buffer to store string representation
 * @param buffer_size Size of the buffer
 * @return int Characters written or -1 on error
 */
int polycall_guid_to_string(polycall_guid_t guid, char *buffer,
                            size_t buffer_size);

#endif /* POLYCALL_GUID_H */