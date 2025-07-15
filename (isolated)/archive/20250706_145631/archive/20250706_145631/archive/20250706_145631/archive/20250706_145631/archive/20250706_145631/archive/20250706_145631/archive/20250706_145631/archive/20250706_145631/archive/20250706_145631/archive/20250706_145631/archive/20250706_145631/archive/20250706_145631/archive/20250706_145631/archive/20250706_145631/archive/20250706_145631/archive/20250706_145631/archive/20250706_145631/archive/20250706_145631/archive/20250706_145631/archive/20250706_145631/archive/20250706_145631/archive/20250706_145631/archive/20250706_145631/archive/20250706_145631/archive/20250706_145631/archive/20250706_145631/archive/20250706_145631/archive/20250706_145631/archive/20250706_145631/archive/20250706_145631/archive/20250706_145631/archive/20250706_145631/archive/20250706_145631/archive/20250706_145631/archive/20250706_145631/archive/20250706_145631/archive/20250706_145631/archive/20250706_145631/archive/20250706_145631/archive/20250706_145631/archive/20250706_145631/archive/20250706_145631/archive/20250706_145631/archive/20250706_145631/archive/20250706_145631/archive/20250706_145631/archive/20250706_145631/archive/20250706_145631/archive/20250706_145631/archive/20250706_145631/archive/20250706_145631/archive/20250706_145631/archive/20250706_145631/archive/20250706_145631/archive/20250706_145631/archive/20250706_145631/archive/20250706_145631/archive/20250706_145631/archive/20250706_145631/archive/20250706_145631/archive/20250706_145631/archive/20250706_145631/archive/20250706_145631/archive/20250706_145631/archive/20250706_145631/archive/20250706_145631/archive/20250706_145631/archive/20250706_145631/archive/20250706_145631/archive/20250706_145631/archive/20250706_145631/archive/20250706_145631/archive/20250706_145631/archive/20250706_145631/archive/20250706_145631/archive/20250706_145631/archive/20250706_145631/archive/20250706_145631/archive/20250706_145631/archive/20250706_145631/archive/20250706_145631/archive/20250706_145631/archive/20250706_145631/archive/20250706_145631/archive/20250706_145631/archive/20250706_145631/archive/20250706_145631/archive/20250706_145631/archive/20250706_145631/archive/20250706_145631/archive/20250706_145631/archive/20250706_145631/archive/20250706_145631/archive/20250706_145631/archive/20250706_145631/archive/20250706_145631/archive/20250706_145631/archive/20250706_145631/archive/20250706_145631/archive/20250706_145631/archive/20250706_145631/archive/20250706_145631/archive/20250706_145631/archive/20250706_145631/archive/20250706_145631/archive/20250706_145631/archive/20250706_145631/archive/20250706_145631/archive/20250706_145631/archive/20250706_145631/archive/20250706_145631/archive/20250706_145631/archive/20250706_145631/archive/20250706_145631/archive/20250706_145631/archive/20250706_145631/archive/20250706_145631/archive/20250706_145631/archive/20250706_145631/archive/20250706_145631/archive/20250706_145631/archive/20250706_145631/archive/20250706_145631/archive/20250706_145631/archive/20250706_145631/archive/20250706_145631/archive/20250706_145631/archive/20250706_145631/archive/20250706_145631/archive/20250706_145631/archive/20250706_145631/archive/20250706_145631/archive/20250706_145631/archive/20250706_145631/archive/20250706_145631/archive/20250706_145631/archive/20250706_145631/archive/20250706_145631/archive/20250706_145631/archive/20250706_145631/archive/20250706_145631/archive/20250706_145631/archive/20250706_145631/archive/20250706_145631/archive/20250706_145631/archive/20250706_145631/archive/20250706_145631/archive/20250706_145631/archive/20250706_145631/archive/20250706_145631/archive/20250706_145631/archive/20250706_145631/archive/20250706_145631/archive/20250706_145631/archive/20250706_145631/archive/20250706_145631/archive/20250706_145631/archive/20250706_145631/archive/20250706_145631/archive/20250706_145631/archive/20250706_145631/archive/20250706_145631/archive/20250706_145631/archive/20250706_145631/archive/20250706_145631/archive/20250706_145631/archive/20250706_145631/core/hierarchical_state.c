/**
#include "polycall/core/protocol/enhancements/hierarchical_state.h"

 * @file hierarchical_state.c
 * @brief Hierarchical State Machine Implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements advanced state management with inheritance, composition,
 * and permission propagation for complex protocol state machines.
 */

 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
    #include "polycall/core/polycall/polycall_core.h"
    #include "polycall/core/polycall/polycall_context.h"
    #include "polycall/core/polycall/polycall_state_machine.h"
    #include "polycall/core/polycall/hierarchical_state.h"

 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 #define POLYCALL_HIERARCHICAL_STATE_MAGIC 0xA7E13C45
 
 /**
  * @brief Hierarchical state node structure
  */
 typedef struct {
     char name[POLYCALL_SM_MAX_NAME_LENGTH];       /**< State name */
     polycall_state_relationship_t relationship;   /**< Relationship to parent */
     char parent_state[POLYCALL_SM_MAX_NAME_LENGTH]; /**< Parent state name */
     polycall_permission_inheritance_t inheritance_model; /**< Permission inheritance model */
     uint32_t permissions[POLYCALL_MAX_STATE_PERMISSIONS]; /**< State permissions */
     uint32_t permission_count;                   /**< Number of permissions */
     
     // Children tracking
     char children[POLYCALL_MAX_CHILD_STATES][POLYCALL_SM_MAX_NAME_LENGTH]; /**< Child states */
     uint32_t child_count;                        /**< Number of children */
     
     // State index in the underlying state machine
     int sm_state_index;                          /**< State index */
 } polycall_hierarchical_state_node_t;
 
 /**
  * @brief Hierarchical transition structure
  */
 typedef struct {
     char name[POLYCALL_SM_MAX_NAME_LENGTH];       /**< Transition name */
     char from_state[POLYCALL_SM_MAX_NAME_LENGTH]; /**< Source state */
     char to_state[POLYCALL_SM_MAX_NAME_LENGTH];   /**< Target state */
     polycall_hierarchical_transition_type_t type; /**< Transition type */
     
     // Transition index in the underlying state machine
     int sm_transition_index;                     /**< Transition index */
 } polycall_hierarchical_transition_t;
 
 /**
  * @brief Hierarchical state machine context structure
  */
 struct polycall_hierarchical_state_context {
     uint32_t magic;                             /**< Magic number for validation */
     polycall_state_machine_t* state_machine;    /**< Underlying state machine */
     
     // Hierarchical state information
     polycall_hierarchical_state_node_t states[POLYCALL_SM_MAX_STATES]; /**< State nodes */
     uint32_t state_count;                      /**< Number of states */
     
     // Hierarchical transition information
     polycall_hierarchical_transition_t transitions[POLYCALL_SM_MAX_TRANSITIONS]; /**< Transitions */
     uint32_t transition_count;                 /**< Number of transitions */
     
     // Core context reference
     polycall_core_context_t* core_ctx;         /**< Core context reference */
 };
 
 /**
  * @brief Find a state node by name
  */
 static int find_state_node(
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name
 ) {
     if (!hsm_ctx || !state_name) {
         return -1;
     }
     
     for (uint32_t i = 0; i < hsm_ctx->state_count; i++) {
         if (strcmp(hsm_ctx->states[i].name, state_name) == 0) {
             return i;
         }
     }
     
     return -1;
 }
 
 /**
  * @brief Find a transition by name
  */
 static int find_transition(
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* transition_name
 ) {
     if (!hsm_ctx || !transition_name) {
         return -1;
     }
     
     for (uint32_t i = 0; i < hsm_ctx->transition_count; i++) {
         if (strcmp(hsm_ctx->transitions[i].name, transition_name) == 0) {
             return i;
         }
     }
     
     return -1;
 }
 
 /**
  * @brief Get the state path from root to the specified state
  */
 static void get_state_path(
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     char** path,
     uint32_t* path_length
 ) {
     *path_length = 0;
     
     // Find the state node
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return;
     }
     
     // Allocate path array (maximum depth is POLYCALL_MAX_STATE_HIERARCHY_DEPTH)
     *path = polycall_core_malloc(hsm_ctx->core_ctx, 
                                POLYCALL_MAX_STATE_HIERARCHY_DEPTH * 
                                POLYCALL_SM_MAX_NAME_LENGTH);
     
     if (!(*path)) {
         return;
     }
     
     // Start with the current state
     char current_state[POLYCALL_SM_MAX_NAME_LENGTH];
     strcpy(current_state, state_name);
     
     // Traverse up to the root
     while (*path_length < POLYCALL_MAX_STATE_HIERARCHY_DEPTH) {
         // Add current state to path
         strcpy((*path) + (*path_length) * POLYCALL_SM_MAX_NAME_LENGTH, current_state);
         (*path_length)++;
         
         // Find current state node
         int curr_idx = find_state_node(hsm_ctx, current_state);
         if (curr_idx < 0) {
             break;
         }
         
         // Check if there's a parent
         if (hsm_ctx->states[curr_idx].parent_state[0] == '\0') {
             break;
         }
         
         // Move to parent
         strcpy(current_state, hsm_ctx->states[curr_idx].parent_state);
     }
 }
 
 /**
  * @brief Calculate effective permissions for a state based on inheritance
  */
 static void calculate_effective_permissions(
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t* permissions,
     uint32_t* permission_count
 ) {
     *permission_count = 0;
     
     // Find the state node
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return;
     }
     
     // Get the state path
     char* path = NULL;
     uint32_t path_length = 0;
     get_state_path(hsm_ctx, state_name, &path, &path_length);
     
     if (!path || path_length == 0) {
         if (path) {
             polycall_core_free(hsm_ctx->core_ctx, path);
         }
         return;
     }
     
     // Start with the most nested state's permissions
     int curr_idx = find_state_node(hsm_ctx, path);
     if (curr_idx >= 0) {
         memcpy(permissions, hsm_ctx->states[curr_idx].permissions, 
                hsm_ctx->states[curr_idx].permission_count * sizeof(uint32_t));
         *permission_count = hsm_ctx->states[curr_idx].permission_count;
     }
     
     // Process parent states in reverse order (from root to current)
     for (int i = path_length - 2; i >= 0; i--) {
         char* parent_state = path + i * POLYCALL_SM_MAX_NAME_LENGTH;
         int parent_idx = find_state_node(hsm_ctx, parent_state);
         
         if (parent_idx < 0) {
             continue;
         }
         
         // Find the child state
         char* child_state = path + (i + 1) * POLYCALL_SM_MAX_NAME_LENGTH;
         int child_idx = find_state_node(hsm_ctx, child_state);
         
         if (child_idx < 0) {
             continue;
         }
         
         // Apply inheritance model
         switch (hsm_ctx->states[child_idx].inheritance_model) {
             case POLYCALL_PERMISSION_INHERIT_NONE:
                 // Keep child permissions, ignore parent
                 break;
                 
             case POLYCALL_PERMISSION_INHERIT_ADDITIVE:
                 // Add parent permissions to child permissions
                 for (uint32_t j = 0; j < hsm_ctx->states[parent_idx].permission_count; j++) {
                     bool already_exists = false;
                     
                     // Check if permission already exists
                     for (uint32_t k = 0; k < *permission_count; k++) {
                         if (permissions[k] == hsm_ctx->states[parent_idx].permissions[j]) {
                             already_exists = true;
                             break;
                         }
                     }
                     
                     // Add permission if it doesn't exist and there's space
                     if (!already_exists && *permission_count < POLYCALL_MAX_STATE_PERMISSIONS) {
                         permissions[*permission_count] = hsm_ctx->states[parent_idx].permissions[j];
                         (*permission_count)++;
                     }
                 }
                 break;
                 
             case POLYCALL_PERMISSION_INHERIT_SUBTRACTIVE:
                 // Remove parent permissions from child permissions
                 for (uint32_t j = 0; j < hsm_ctx->states[parent_idx].permission_count; j++) {
                     for (uint32_t k = 0; k < *permission_count; k++) {
                         if (permissions[k] == hsm_ctx->states[parent_idx].permissions[j]) {
                             // Remove permission by shifting remaining permissions
                             for (uint32_t l = k; l < *permission_count - 1; l++) {
                                 permissions[l] = permissions[l + 1];
                             }
                             (*permission_count)--;
                             k--;  // Adjust index after removal
                             break;
                         }
                     }
                 }
                 break;
                 
             case POLYCALL_PERMISSION_INHERIT_REPLACE:
                 // Replace child permissions with parent permissions
                 memcpy(permissions, hsm_ctx->states[parent_idx].permissions, 
                        hsm_ctx->states[parent_idx].permission_count * sizeof(uint32_t));
                 *permission_count = hsm_ctx->states[parent_idx].permission_count;
                 break;
         }
     }
     
     // Free path array
     polycall_core_free(hsm_ctx->core_ctx, path);
 }
 
 /**
  * @brief Check if the HSM context is valid
  */
 static bool is_valid_hsm_context(
     polycall_hierarchical_state_context_t* hsm_ctx
 ) {
     return hsm_ctx && hsm_ctx->magic == POLYCALL_HIERARCHICAL_STATE_MAGIC;
 }
 
 /**
  * @brief Initialize hierarchical state machine
  */
 polycall_core_error_t polycall_hierarchical_state_init(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t** hsm_ctx,
     polycall_state_machine_t* sm
 ) {
     if (!core_ctx || !hsm_ctx || !sm) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_hierarchical_state_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_hierarchical_state_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_hierarchical_state_context_t));
     new_ctx->magic = POLYCALL_HIERARCHICAL_STATE_MAGIC;
     new_ctx->state_machine = sm;
     new_ctx->core_ctx = core_ctx;
     
     *hsm_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Remove permission from a state
  */
 polycall_core_error_t polycall_hierarchical_state_remove_permission(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t permission
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find permission
     int perm_idx = -1;
     for (uint32_t i = 0; i < hsm_ctx->states[state_idx].permission_count; i++) {
         if (hsm_ctx->states[state_idx].permissions[i] == permission) {
             perm_idx = i;
             break;
         }
     }
     
     // Permission not found
     if (perm_idx < 0) {
         return POLYCALL_CORE_SUCCESS;  // Already doesn't have the permission
     }
     
     // Remove permission by shifting remaining permissions
     for (uint32_t i = perm_idx; i < hsm_ctx->states[state_idx].permission_count - 1; i++) {
         hsm_ctx->states[state_idx].permissions[i] = hsm_ctx->states[state_idx].permissions[i + 1];
     }
     
     // Decrease permission count
     hsm_ctx->states[state_idx].permission_count--;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set permission inheritance model for a state
  */
 polycall_core_error_t polycall_hierarchical_state_set_inheritance(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     polycall_permission_inheritance_t inheritance_model
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Update inheritance model
     hsm_ctx->states[state_idx].inheritance_model = inheritance_model;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get parent state
  */
 polycall_core_error_t polycall_hierarchical_state_get_parent(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     char* parent_buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name || 
         !parent_buffer || buffer_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check if state has a parent
     if (hsm_ctx->states[state_idx].parent_state[0] == '\0') {
         parent_buffer[0] = '\0';  // No parent
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Copy parent name
     if (strlen(hsm_ctx->states[state_idx].parent_state) >= buffer_size) {
         return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
     }
     
     strcpy(parent_buffer, hsm_ctx->states[state_idx].parent_state);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get child states
  */
 polycall_core_error_t polycall_hierarchical_state_get_children(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     char children[][POLYCALL_SM_MAX_NAME_LENGTH],
     uint32_t max_children,
     uint32_t* child_count
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name || 
         !children || !child_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get child count
     uint32_t count = hsm_ctx->states[state_idx].child_count;
     if (count > max_children) {
         count = max_children;
     }
     
     // Copy child names
     for (uint32_t i = 0; i < count; i++) {
         strcpy(children[i], hsm_ctx->states[state_idx].children[i]);
     }
     
     *child_count = count;
     
     return POLYCALL_CORE_SUCCESS;
 };
 }
 
 /**
  * @brief Clean up hierarchical state machine
  */
 void polycall_hierarchical_state_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx)) {
         return;
     }
     
     // Clear magic number
     hsm_ctx->magic = 0;
     
     // Free context
     polycall_core_free(core_ctx, hsm_ctx);
 }
 
 /**
  * @brief Add a hierarchical state
  */
 polycall_core_error_t polycall_hierarchical_state_add(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const polycall_hierarchical_state_config_t* config
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if state limit is reached
     if (hsm_ctx->state_count >= POLYCALL_SM_MAX_STATES) {
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Check if state already exists
     if (find_state_node(hsm_ctx, config->name) >= 0) {
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check parent state if specified
     if (config->parent_state[0] != '\0') {
         int parent_idx = find_state_node(hsm_ctx, config->parent_state);
         if (parent_idx < 0) {
             return POLYCALL_CORE_ERROR_NOT_FOUND;
         }
         
         // Check if parent has space for more children
         if (hsm_ctx->states[parent_idx].child_count >= POLYCALL_MAX_CHILD_STATES) {
             return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
         }
     }
     
     // Add state to underlying state machine
     polycall_sm_status_t sm_status = polycall_sm_add_state(
         hsm_ctx->state_machine,
         config->name,
         config->on_enter,
         config->on_exit,
         false  // Not locked initially
     );
     
     if (sm_status != POLYCALL_SM_SUCCESS) {
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Get state index in underlying state machine
     int sm_state_idx = polycall_sm_find_state(hsm_ctx->state_machine, config->name);
     if (sm_state_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Initialize state node
     polycall_hierarchical_state_node_t* new_state = &hsm_ctx->states[hsm_ctx->state_count];
     strcpy(new_state->name, config->name);
     new_state->relationship = config->relationship;
     strcpy(new_state->parent_state, config->parent_state);
     new_state->inheritance_model = config->inheritance_model;
     new_state->sm_state_index = sm_state_idx;
     new_state->child_count = 0;
     
     // Copy permissions
     uint32_t permission_count = config->permission_count;
     if (permission_count > POLYCALL_MAX_STATE_PERMISSIONS) {
         permission_count = POLYCALL_MAX_STATE_PERMISSIONS;
     }
     
     memcpy(new_state->permissions, config->permissions, 
            permission_count * sizeof(uint32_t));
     new_state->permission_count = permission_count;
     
     // Add as child to parent
     if (config->parent_state[0] != '\0') {
         int parent_idx = find_state_node(hsm_ctx, config->parent_state);
         if (parent_idx >= 0) {
             strcpy(hsm_ctx->states[parent_idx].children[hsm_ctx->states[parent_idx].child_count],
                    config->name);
             hsm_ctx->states[parent_idx].child_count++;
         }
     }
     
     // Increment state count
     hsm_ctx->state_count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Add a hierarchical transition
  */
 polycall_core_error_t polycall_hierarchical_state_add_transition(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const polycall_hierarchical_transition_config_t* config
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if transition limit is reached
     if (hsm_ctx->transition_count >= POLYCALL_SM_MAX_TRANSITIONS) {
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Check if transition already exists
     if (find_transition(hsm_ctx, config->name) >= 0) {
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check source and target states
     int from_idx = find_state_node(hsm_ctx, config->from_state);
     int to_idx = find_state_node(hsm_ctx, config->to_state);
     
     if (from_idx < 0 || to_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Add transition to underlying state machine
     polycall_sm_status_t sm_status = polycall_sm_add_transition(
         hsm_ctx->state_machine,
         config->name,
         config->from_state,
         config->to_state,
         config->guard,
         NULL  // No user data for now
     );
     
     if (sm_status != POLYCALL_SM_SUCCESS) {
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Get transition index in underlying state machine
     int sm_transition_idx = polycall_sm_find_transition(hsm_ctx->state_machine, config->name);
     if (sm_transition_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Initialize transition
     polycall_hierarchical_transition_t* new_transition = 
         &hsm_ctx->transitions[hsm_ctx->transition_count];
     strcpy(new_transition->name, config->name);
     strcpy(new_transition->from_state, config->from_state);
     strcpy(new_transition->to_state, config->to_state);
     new_transition->type = config->type;
     new_transition->sm_transition_index = sm_transition_idx;
     
     // Increment transition count
     hsm_ctx->transition_count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Execute a hierarchical transition
  */
 polycall_core_error_t polycall_hierarchical_state_execute_transition(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* transition_name
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !transition_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find transition
     int transition_idx = find_transition(hsm_ctx, transition_name);
     if (transition_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Find source and target states
     int from_idx = find_state_node(hsm_ctx, hsm_ctx->transitions[transition_idx].from_state);
     int to_idx = find_state_node(hsm_ctx, hsm_ctx->transitions[transition_idx].to_state);
     
     if (from_idx < 0 || to_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // For internal transitions, we don't execute state machine transition
     if (hsm_ctx->transitions[transition_idx].type == POLYCALL_HTRANSITION_INTERNAL) {
         // Get current state
         char current_state[POLYCALL_SM_MAX_NAME_LENGTH];
         polycall_sm_status_t sm_status = polycall_sm_get_current_state(
             hsm_ctx->state_machine,
             current_state,
             POLYCALL_SM_MAX_NAME_LENGTH
         );
         
         if (sm_status != POLYCALL_SM_SUCCESS) {
             return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
         }
         
         // Check if we're in the correct state
         if (strcmp(current_state, hsm_ctx->transitions[transition_idx].from_state) != 0) {
             return POLYCALL_CORE_ERROR_INVALID_STATE;
         }
         
         // Internal transitions don't change state, so we're done
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Execute transition in underlying state machine
     polycall_sm_status_t sm_status = polycall_sm_execute_transition(
         hsm_ctx->state_machine,
         transition_name
     );
     
     if (sm_status != POLYCALL_SM_SUCCESS) {
         return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Check if state has a specific permission
  */
 bool polycall_hierarchical_state_has_permission(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t permission
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name) {
         return false;
     }
     
     // Calculate effective permissions
     uint32_t permissions[POLYCALL_MAX_STATE_PERMISSIONS];
     uint32_t permission_count = 0;
     
     calculate_effective_permissions(hsm_ctx, state_name, permissions, &permission_count);
     
     // Check if permission exists
     for (uint32_t i = 0; i < permission_count; i++) {
         if (permissions[i] == permission) {
             return true;
         }
     }
     
     return false;
 }
 
 /**
  * @brief Get effective permissions for a state
  */
 polycall_core_error_t polycall_hierarchical_state_get_permissions(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t* permissions,
     uint32_t max_permissions,
     uint32_t* permission_count
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name || 
         !permissions || !permission_count) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Calculate effective permissions
     uint32_t effective_permissions[POLYCALL_MAX_STATE_PERMISSIONS];
     uint32_t effective_count = 0;
     
     calculate_effective_permissions(hsm_ctx, state_name, effective_permissions, &effective_count);
     
     // Copy permissions to output buffer
     uint32_t copy_count = effective_count;
     if (copy_count > max_permissions) {
         copy_count = max_permissions;
     }
     
     memcpy(permissions, effective_permissions, copy_count * sizeof(uint32_t));
     *permission_count = copy_count;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get the current hierarchical state path
  */
 polycall_core_error_t polycall_hierarchical_state_get_path(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     char* path_buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !path_buffer || buffer_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get current state
     char current_state[POLYCALL_SM_MAX_NAME_LENGTH];
     polycall_sm_status_t sm_status = polycall_sm_get_current_state(
         hsm_ctx->state_machine,
         current_state,
         POLYCALL_SM_MAX_NAME_LENGTH
     );
     
     if (sm_status != POLYCALL_SM_SUCCESS) {
         return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
     }
     
     // Get state path
     char* path = NULL;
     uint32_t path_length = 0;
     get_state_path(hsm_ctx, current_state, &path, &path_length);
     
     if (!path || path_length == 0) {
         if (path) {
             polycall_core_free(hsm_ctx->core_ctx, path);
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Format path as string (states separated by '.')
     size_t pos = 0;
     path_buffer[0] = '\0';
     
     for (int i = path_length - 1; i >= 0; i--) {
         char* state = path + i * POLYCALL_SM_MAX_NAME_LENGTH;
         size_t state_len = strlen(state);
         
         // Check if there's enough space
         if (pos + state_len + 1 >= buffer_size) {
             break;
         }
         
         // Add separator if not the first state
         if (i < path_length - 1) {
             path_buffer[pos++] = '.';
         }
         
         // Copy state name
         strcpy(path_buffer + pos, state);
         pos += state_len;
     }
     
     // Free path array
     polycall_core_free(hsm_ctx->core_ctx, path);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Check if a state is active (directly or as part of hierarchy)
  */
 bool polycall_hierarchical_state_is_active(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name) {
         return false;
     }
     
     // Get current state
     char current_state[POLYCALL_SM_MAX_NAME_LENGTH];
     polycall_sm_status_t sm_status = polycall_sm_get_current_state(
         hsm_ctx->state_machine,
         current_state,
         POLYCALL_SM_MAX_NAME_LENGTH
     );
     
     if (sm_status != POLYCALL_SM_SUCCESS) {
         return false;
     }
     
     // Check if current state matches
     if (strcmp(current_state, state_name) == 0) {
         return true;
     }
     
     // Check if state is a parent of current state
     char* path = NULL;
     uint32_t path_length = 0;
     get_state_path(hsm_ctx, current_state, &path, &path_length);
     
     if (!path || path_length <= 1) {
         if (path) {
             polycall_core_free(hsm_ctx->core_ctx, path);
         }
         return false;
     }
     
     // Check if state is in the path
     bool is_active = false;
     for (uint32_t i = 1; i < path_length; i++) {  // Skip current state
         char* parent = path + i * POLYCALL_SM_MAX_NAME_LENGTH;
         if (strcmp(parent, state_name) == 0) {
             is_active = true;
             break;
         }
     }
     
     // Free path array
     polycall_core_free(hsm_ctx->core_ctx, path);
     
     return is_active;
 }
 
 /**
  * @brief Add permission to a state
  */
 polycall_core_error_t polycall_hierarchical_state_add_permission(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t permission
 ) {
     if (!core_ctx || !is_valid_hsm_context(hsm_ctx) || !state_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find state
     int state_idx = find_state_node(hsm_ctx, state_name);
     if (state_idx < 0) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Check if permission already exists
     for (uint32_t i = 0; i < hsm_ctx->states[state_idx].permission_count; i++) {
         if (hsm_ctx->states[state_idx].permissions[i] == permission) {
             return POLYCALL_CORE_SUCCESS;  // Already has the permission
         }
     }
     
     // Check if permission limit is reached
     if (hsm_ctx->states[state_idx].permission_count >= POLYCALL_MAX_STATE_PERMISSIONS) {
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Add permission
     hsm_ctx->states[state_idx].permissions[hsm_ctx->states[state_idx].permission_count++] = 
         permission;
     
     return POLYCALL_CORE_SUCCESS;

    }
    