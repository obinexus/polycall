/**
 * @file polycall_state_machine.h
 * @brief State machine for LibPolyCall protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the state machine used by the protocol layer
 * to manage state transitions and enforce state-based security constraints.
 */

#ifndef POLYCALL_POLYCALL_POLYCALL_STATE_MACHINE_H_H
#define POLYCALL_POLYCALL_POLYCALL_STATE_MACHINE_H_H

#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of states in the state machine
 */
#define POLYCALL_POLYCALL_POLYCALL_STATE_MACHINE_H_H

/**
 * @brief Maximum number of transitions in the state machine
 */
#define POLYCALL_POLYCALL_POLYCALL_STATE_MACHINE_H_H

/**
 * @brief Maximum length of state and transition names
 */
#define POLYCALL_POLYCALL_POLYCALL_STATE_MACHINE_H_H

/**
 * @brief State machine status codes
 */
typedef enum {
  POLYCALL_SM_SUCCESS = 0,
  POLYCALL_SM_ERROR_INVALID_PARAMETERS,
  POLYCALL_SM_ERROR_OUT_OF_MEMORY,
  POLYCALL_SM_ERROR_STATE_NOT_FOUND,
  POLYCALL_SM_ERROR_TRANSITION_NOT_FOUND,
  POLYCALL_SM_ERROR_INVALID_TRANSITION,
  POLYCALL_SM_ERROR_STATE_LOCKED,
  POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED,
  POLYCALL_SM_ERROR_ALREADY_INITIALIZED
} polycall_sm_status_t;

/**
 * @brief State callback function type
 */
typedef void (*polycall_sm_state_callback_t)(polycall_core_context_t *ctx,
                                             void *user_data);

/**
 * @brief Transition guard function type
 * Returns true if transition is allowed, false otherwise
 */
typedef bool (*polycall_sm_guard_fn)(polycall_core_context_t *ctx,
                                     void *user_data);

/**
 * @brief Integrity check function type
 * Used to validate state machine integrity
 */
typedef bool (*polycall_sm_integrity_check_fn)(polycall_core_context_t *ctx,
                                               void *integrity_data);

/**
 * @brief State structure
 */
typedef struct {
  char name[POLYCALL_SM_MAX_NAME_LENGTH]; /**< State name */
  polycall_sm_state_callback_t on_enter;  /**< Callback when entering state */
  polycall_sm_state_callback_t on_exit;   /**< Callback when exiting state */
  void *user_data;                        /**< User data for callbacks */
  bool is_locked;                         /**< Whether state is locked */
} polycall_sm_state_t;

/**
 * @brief Transition structure
 */
typedef struct {
  char name[POLYCALL_SM_MAX_NAME_LENGTH]; /**< Transition name */
  unsigned int from_state;                /**< Source state index */
  unsigned int to_state;                  /**< Target state index */
  polycall_sm_guard_fn guard;             /**< Guard function */
  void *user_data;                        /**< User data for guard */
} polycall_sm_transition_t;

/**
 * @brief State machine structure (opaque)
 */
typedef struct polycall_state_machine {
  polycall_sm_state_t states[POLYCALL_SM_MAX_STATES]; /**< States */
  polycall_sm_transition_t
      transitions[POLYCALL_SM_MAX_TRANSITIONS]; /**< Transitions */
  unsigned int num_states;                      /**< Number of states */
  unsigned int num_transitions;                 /**< Number of transitions */
  unsigned int current_state;                   /**< Current state index */
  polycall_sm_integrity_check_fn
      integrity_check;               /**< Integrity check function */
  void *integrity_data;              /**< Data for integrity check */
  polycall_core_context_t *core_ctx; /**< Core context reference */
} polycall_state_machine_t;

/**
 * @brief State snapshot structure
 * Used to save and restore state machine state
 */
typedef struct {
  unsigned int state_index; /**< State index */
  uint64_t timestamp;       /**< Snapshot timestamp */
  uint32_t checksum;        /**< State machine checksum */
} polycall_sm_snapshot_t;

/**
 * @brief Create a new state machine
 *
 * @param ctx Core context
 * @param state_machine Pointer to receive created state machine
 * @param integrity_check Optional integrity check function
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_create(polycall_core_context_t *ctx,
                   polycall_state_machine_t **state_machine);

/**
 * @brief Create a new state machine with integrity checking
 *
 * @param ctx Core context
 * @param state_machine Pointer to receive created state machine
 * @param integrity_data Optional integrity data
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_create_with_integrity(polycall_core_context_t *ctx,
                                  polycall_state_machine_t **state_machine,
                                  void *integrity_data);

/**
 * @brief Destroy a state machine
 *
 * @param state_machine State machine to destroy
 */
void polycall_sm_destroy(polycall_state_machine_t *state_machine);

/**
 * @brief Add a state to the state machine
 *
 * @param state_machine State machine
 * @param name State name
 * @param on_enter Optional callback when entering state
 * @param on_exit Optional callback when exiting state
 * @param is_locked Whether state is locked
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_add_state(polycall_state_machine_t *state_machine, const char *name,
                      polycall_sm_state_callback_t on_enter,
                      polycall_sm_state_callback_t on_exit, bool is_locked);

/**
 * @brief Add a transition to the state machine
 *
 * @param state_machine State machine
 * @param name Transition name
 * @param from_state Source state name
 * @param to_state Target state name
 * @param guard Optional guard function
 * @param user_data User data for guard
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_add_transition(polycall_state_machine_t *state_machine,
                           const char *name, const char *from_state,
                           const char *to_state, polycall_sm_guard_fn guard,
                           void *user_data);

/**
 * @brief Execute a transition
 *
 * @param state_machine State machine
 * @param transition_name Transition name
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_execute_transition(polycall_state_machine_t *state_machine,
                               const char *transition_name);

/**
 * @brief Get the current state
 *
 * @param state_machine State machine
 * @param state_name Buffer to receive state name
 * @param buffer_size Buffer size
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_get_current_state(const polycall_state_machine_t *state_machine,
                              char *state_name, size_t buffer_size);

/**
 * @brief Get the current state index
 *
 * @param state_machine State machine
 * @return Current state index, or -1 on error
 */
int polycall_sm_get_current_state_index(
    const polycall_state_machine_t *state_machine);

/**
 * @brief Check if a transition is valid
 *
 * @param state_machine State machine
 * @param transition_name Transition name
 * @return true if transition is valid, false otherwise
 */
bool polycall_sm_is_transition_valid(
    const polycall_state_machine_t *state_machine, const char *transition_name);

/**
 * @brief Lock a state
 *
 * @param state_machine State machine
 * @param state_name State name
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_lock_state(polycall_state_machine_t *state_machine,
                       const char *state_name);

/**
 * @brief Unlock a state
 *
 * @param state_machine State machine
 * @param state_name State name
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_unlock_state(polycall_state_machine_t *state_machine,
                         const char *state_name);

/**
 * @brief Create a snapshot of the current state
 *
 * @param state_machine State machine
 * @param snapshot Pointer to receive snapshot
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_create_snapshot(const polycall_state_machine_t *state_machine,
                            polycall_sm_snapshot_t *snapshot);

/**
 * @brief Restore state from a snapshot
 *
 * @param state_machine State machine
 * @param snapshot Snapshot to restore
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_restore_snapshot(polycall_state_machine_t *state_machine,
                             const polycall_sm_snapshot_t *snapshot);

/**
 * @brief Validate state machine integrity
 *
 * @param state_machine State machine
 * @return Status code (POLYCALL_SM_SUCCESS if valid)
 */
polycall_sm_status_t
polycall_sm_validate_integrity(const polycall_state_machine_t *state_machine);

/**
 * @brief Get transition between states
 *
 * @param state_machine State machine
 * @param from_state Source state name
 * @param to_state Target state name
 * @param transition_name Buffer to receive transition name
 * @param buffer_size Buffer size
 * @return Status code
 */
polycall_sm_status_t
polycall_sm_get_transition(const polycall_state_machine_t *state_machine,
                           const char *from_state, const char *to_state,
                           char *transition_name, size_t buffer_size);

/**
 * @brief Find state index by name
 *
 * @param state_machine State machine
 * @param state_name State name
 * @return State index, or -1 if not found
 */
int polycall_sm_find_state(const polycall_state_machine_t *state_machine,
                           const char *state_name);

/**
 * @brief Find transition index by name
 *
 * @param state_machine State machine
 * @param transition_name Transition name
 * @return Transition index, or -1 if not found
 */
int polycall_sm_find_transition(const polycall_state_machine_t *state_machine,
                                const char *transition_name);

/**
 * @brief Convert state machine status to string
 *
 * @param status Status code
 * @return String representation of status
 */
const char *polycall_sm_status_to_string(polycall_sm_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_STATE_MACHINE_H_H */