/**
#include "polycall/core/protocol/protocol_state_machine.h"

 * @file polycall_state_machine.c
 * @brief State machine implementation for LibPolyCall protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the state machine functionality that manages
 * state transitions and enforces state-based security constraints
 * for the protocol layer.
 */

 #include "polycall/core/polycall/polycall_state_machine.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <string.h>
 #include <stdio.h>
 #include <time.h>
 
 // Jenkins hash function for integrity checking
 static uint32_t calculate_hash(const void* data, size_t length) {
     const uint8_t* bytes = (const uint8_t*)data;
     uint32_t hash = 0;
     
     for (size_t i = 0; i < length; i++) {
         hash += bytes[i];
         hash += (hash << 10);
         hash ^= (hash >> 6);
     }
     
     hash += (hash << 3);
     hash ^= (hash >> 11);
     hash += (hash << 15);
     
     return hash;
 }
 
 // Calculate state machine checksum for integrity checking
 static uint32_t calculate_state_machine_checksum(const polycall_state_machine_t* state_machine) {
     if (!state_machine) return 0;
     
     // Hash states
     uint32_t states_hash = calculate_hash(state_machine->states, 
                                           sizeof(polycall_sm_state_t) * state_machine->num_states);
     
     // Hash transitions
     uint32_t transitions_hash = calculate_hash(state_machine->transitions,
                                              sizeof(polycall_sm_transition_t) * state_machine->num_transitions);
     
     // Combine hashes
     return states_hash ^ transitions_hash ^ state_machine->current_state;
 }
 
 // Default integrity check function (always passes)
 static bool default_integrity_check(polycall_core_context_t* ctx, void* integrity_data) {
     return true;
 }
 
 polycall_sm_status_t polycall_sm_create(
     polycall_core_context_t* ctx,
     polycall_state_machine_t** state_machine
 ) {
     return polycall_sm_create_with_integrity(ctx, state_machine, NULL);
 }
 
 polycall_sm_status_t polycall_sm_create_with_integrity(
     polycall_core_context_t* ctx,
     polycall_state_machine_t** state_machine,
     void* integrity_data
 ) {
     if (!ctx || !state_machine) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate state machine
     polycall_state_machine_t* new_sm = polycall_core_malloc(ctx, sizeof(polycall_state_machine_t));
     if (!new_sm) {
         return POLYCALL_SM_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize state machine
     memset(new_sm, 0, sizeof(polycall_state_machine_t));
     new_sm->core_ctx = ctx;
     new_sm->integrity_check = default_integrity_check;  // Default integrity check
     new_sm->integrity_data = integrity_data;
     
     *state_machine = new_sm;
     return POLYCALL_SM_SUCCESS;
 }
 
 void polycall_sm_destroy(polycall_state_machine_t* state_machine) {
     if (!state_machine || !state_machine->core_ctx) return;
     
     // Free state machine
     polycall_core_free(state_machine->core_ctx, state_machine);
 }
 
 polycall_sm_status_t polycall_sm_add_state(
     polycall_state_machine_t* state_machine,
     const char* name,
     polycall_sm_state_callback_t on_enter,
     polycall_sm_state_callback_t on_exit,
     bool is_locked
 ) {
     if (!state_machine || !name) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we have space for another state
     if (state_machine->num_states >= POLYCALL_SM_MAX_STATES) {
         return POLYCALL_SM_ERROR_OUT_OF_MEMORY;
     }
     
     // Check if state already exists
     for (unsigned int i = 0; i < state_machine->num_states; i++) {
         if (strcmp(state_machine->states[i].name, name) == 0) {
             // State already exists
             return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
         }
     }
     
     // Add state
     polycall_sm_state_t* state = &state_machine->states[state_machine->num_states];
     strncpy(state->name, name, POLYCALL_SM_MAX_NAME_LENGTH - 1);
     state->name[POLYCALL_SM_MAX_NAME_LENGTH - 1] = '\0';
     state->on_enter = on_enter;
     state->on_exit = on_exit;
     state->user_data = NULL;
     state->is_locked = is_locked;
     
     state_machine->num_states++;
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_add_transition(
     polycall_state_machine_t* state_machine,
     const char* name,
     const char* from_state,
     const char* to_state,
     polycall_sm_guard_fn guard,
     void* user_data
 ) {
     if (!state_machine || !name || !from_state || !to_state) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we have space for another transition
     if (state_machine->num_transitions >= POLYCALL_SM_MAX_TRANSITIONS) {
         return POLYCALL_SM_ERROR_OUT_OF_MEMORY;
     }
     
     // Find source and target states
     int from_index = polycall_sm_find_state(state_machine, from_state);
     int to_index = polycall_sm_find_state(state_machine, to_state);
     
     if (from_index < 0 || to_index < 0) {
         return POLYCALL_SM_ERROR_STATE_NOT_FOUND;
     }
     
     // Check if transition already exists
     for (unsigned int i = 0; i < state_machine->num_transitions; i++) {
         if (strcmp(state_machine->transitions[i].name, name) == 0) {
             // Transition already exists
             return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
         }
     }
     
     // Add transition
     polycall_sm_transition_t* transition = &state_machine->transitions[state_machine->num_transitions];
     strncpy(transition->name, name, POLYCALL_SM_MAX_NAME_LENGTH - 1);
     transition->name[POLYCALL_SM_MAX_NAME_LENGTH - 1] = '\0';
     transition->from_state = from_index;
     transition->to_state = to_index;
     transition->guard = guard;
     transition->user_data = user_data;
     
     state_machine->num_transitions++;
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_execute_transition(
     polycall_state_machine_t* state_machine,
     const char* transition_name
 ) {
     if (!state_machine || !transition_name) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Find transition
     int transition_index = polycall_sm_find_transition(state_machine, transition_name);
     if (transition_index < 0) {
         return POLYCALL_SM_ERROR_TRANSITION_NOT_FOUND;
     }
     
     polycall_sm_transition_t* transition = &state_machine->transitions[transition_index];
     
     // Check if current state matches source state
     if (state_machine->current_state != transition->from_state) {
         return POLYCALL_SM_ERROR_INVALID_TRANSITION;
     }
     
     // Check if target state is locked
     if (state_machine->states[transition->to_state].is_locked) {
         return POLYCALL_SM_ERROR_STATE_LOCKED;
     }
     
     // Check guard if provided
     if (transition->guard && !transition->guard(state_machine->core_ctx, transition->user_data)) {
         return POLYCALL_SM_ERROR_INVALID_TRANSITION;
     }
     
     // Execute exit callback if provided
     polycall_sm_state_t* current_state = &state_machine->states[state_machine->current_state];
     if (current_state->on_exit) {
         current_state->on_exit(state_machine->core_ctx, current_state->user_data);
     }
     
     // Update current state
     state_machine->current_state = transition->to_state;
     
     // Execute enter callback if provided
     polycall_sm_state_t* new_state = &state_machine->states[state_machine->current_state];
     if (new_state->on_enter) {
         new_state->on_enter(state_machine->core_ctx, new_state->user_data);
     }
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_get_current_state(
     const polycall_state_machine_t* state_machine,
     char* state_name,
     size_t buffer_size
 ) {
     if (!state_machine || !state_name || buffer_size == 0) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if current state is valid
     if (state_machine->current_state >= state_machine->num_states) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy state name
     strncpy(state_name, state_machine->states[state_machine->current_state].name, buffer_size - 1);
     state_name[buffer_size - 1] = '\0';
     
     return POLYCALL_SM_SUCCESS;
 }
 
 int polycall_sm_get_current_state_index(
     const polycall_state_machine_t* state_machine
 ) {
     if (!state_machine) return -1;
     
     return state_machine->current_state;
 }
 
 bool polycall_sm_is_transition_valid(
     const polycall_state_machine_t* state_machine,
     const char* transition_name
 ) {
     if (!state_machine || !transition_name) {
         return false;
     }
     
     // Find transition
     int transition_index = polycall_sm_find_transition(state_machine, transition_name);
     if (transition_index < 0) {
         return false;
     }
     
     polycall_sm_transition_t* transition = &state_machine->transitions[transition_index];
     
     // Check if current state matches source state
     if (state_machine->current_state != transition->from_state) {
         return false;
     }
     
     // Check if target state is locked
     if (state_machine->states[transition->to_state].is_locked) {
         return false;
     }
     
     // Check guard if provided
     if (transition->guard && !transition->guard(state_machine->core_ctx, transition->user_data)) {
         return false;
     }
     
     return true;
 }
 
 polycall_sm_status_t polycall_sm_lock_state(
     polycall_state_machine_t* state_machine,
     const char* state_name
 ) {
     if (!state_machine || !state_name) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_index = polycall_sm_find_state(state_machine, state_name);
     if (state_index < 0) {
         return POLYCALL_SM_ERROR_STATE_NOT_FOUND;
     }
     
     // Lock state
     state_machine->states[state_index].is_locked = true;
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_unlock_state(
     polycall_state_machine_t* state_machine,
     const char* state_name
 ) {
     if (!state_machine || !state_name) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_index = polycall_sm_find_state(state_machine, state_name);
     if (state_index < 0) {
         return POLYCALL_SM_ERROR_STATE_NOT_FOUND;
     }
     
     // Unlock state
     state_machine->states[state_index].is_locked = false;
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_create_snapshot(
     const polycall_state_machine_t* state_machine,
     polycall_sm_snapshot_t* snapshot
 ) {
     if (!state_machine || !snapshot) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Create snapshot
     snapshot->state_index = state_machine->current_state;
     snapshot->timestamp = (uint64_t)time(NULL);
     snapshot->checksum = calculate_state_machine_checksum(state_machine);
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_restore_snapshot(
     polycall_state_machine_t* state_machine,
     const polycall_sm_snapshot_t* snapshot
 ) {
     if (!state_machine || !snapshot) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Validate snapshot checksum
     uint32_t current_checksum = calculate_state_machine_checksum(state_machine);
     if (current_checksum != snapshot->checksum) {
         return POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED;
     }
     
     // Check if snapshot state is valid
     if (snapshot->state_index >= state_machine->num_states) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Execute exit callback if provided
     polycall_sm_state_t* current_state = &state_machine->states[state_machine->current_state];
     if (current_state->on_exit) {
         current_state->on_exit(state_machine->core_ctx, current_state->user_data);
     }
     
     // Restore state
     state_machine->current_state = snapshot->state_index;
     
     // Execute enter callback if provided
     polycall_sm_state_t* new_state = &state_machine->states[state_machine->current_state];
     if (new_state->on_enter) {
         new_state->on_enter(state_machine->core_ctx, new_state->user_data);
     }
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_validate_integrity(
     const polycall_state_machine_t* state_machine
 ) {
     if (!state_machine) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if integrity check function is provided
     if (state_machine->integrity_check &&
         !state_machine->integrity_check(state_machine->core_ctx, state_machine->integrity_data)) {
         return POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED;
     }
     
     return POLYCALL_SM_SUCCESS;
 }
 
 polycall_sm_status_t polycall_sm_get_transition(
     const polycall_state_machine_t* state_machine,
     const char* from_state,
     const char* to_state,
     char* transition_name,
     size_t buffer_size
 ) {
     if (!state_machine || !from_state || !to_state || !transition_name || buffer_size == 0) {
         return POLYCALL_SM_ERROR_INVALID_PARAMETERS;
     }
     
     // Find source and target states
     int from_index = polycall_sm_find_state(state_machine, from_state);
     int to_index = polycall_sm_find_state(state_machine, to_state);
     
     if (from_index < 0 || to_index < 0) {
         return POLYCALL_SM_ERROR_STATE_NOT_FOUND;
     }
     
     // Find transition
     for (unsigned int i = 0; i < state_machine->num_transitions; i++) {
         if (state_machine->transitions[i].from_state == from_index &&
             state_machine->transitions[i].to_state == to_index) {
             // Found transition
             strncpy(transition_name, state_machine->transitions[i].name, buffer_size - 1);
             transition_name[buffer_size - 1] = '\0';
             return POLYCALL_SM_SUCCESS;
         }
     }
     
     return POLYCALL_SM_ERROR_TRANSITION_NOT_FOUND;
 }
 
 int polycall_sm_find_state(
     const polycall_state_machine_t* state_machine,
     const char* state_name
 ) {
     if (!state_machine || !state_name) {
         return -1;
     }
     
     // Find state by name
     for (unsigned int i = 0; i < state_machine->num_states; i++) {
         if (strcmp(state_machine->states[i].name, state_name) == 0) {
             return i;
         }
     }
     
     return -1;
 }
 
 int polycall_sm_find_transition(
     const polycall_state_machine_t* state_machine,
     const char* transition_name
 ) {
     if (!state_machine || !transition_name) {
         return -1;
     }
     
     // Find transition by name
     for (unsigned int i = 0; i < state_machine->num_transitions; i++) {
         if (strcmp(state_machine->transitions[i].name, transition_name) == 0) {
             return i;
         }
     }
     
     return -1;
 }
 
 const char* polycall_sm_status_to_string(
     polycall_sm_status_t status
 ) {
     switch (status) {
         case POLYCALL_SM_SUCCESS:
             return "Success";
         case POLYCALL_SM_ERROR_INVALID_PARAMETERS:
             return "Invalid parameters";
         case POLYCALL_SM_ERROR_OUT_OF_MEMORY:
             return "Out of memory";
         case POLYCALL_SM_ERROR_STATE_NOT_FOUND:
             return "State not found";
         case POLYCALL_SM_ERROR_TRANSITION_NOT_FOUND:
             return "Transition not found";
         case POLYCALL_SM_ERROR_INVALID_TRANSITION:
             return "Invalid transition";
         case POLYCALL_SM_ERROR_STATE_LOCKED:
             return "State locked";
         case POLYCALL_SM_ERROR_INTEGRITY_CHECK_FAILED:
             return "Integrity check failed";
         case POLYCALL_SM_ERROR_ALREADY_INITIALIZED:
             return "Already initialized";
         default:
             return "Unknown error";
     }
 }