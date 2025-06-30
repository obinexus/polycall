/**
 * @file hierarchical_state.h
 * @brief Hierarchical State Management for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides advanced state management with inheritance, composition,
 * and permission propagation for complex protocol state machines.
 */

 #ifndef POLYCALL_POLYCALL_HIERARCHICAL_STATE_H_H
 #define POLYCALL_POLYCALL_HIERARCHICAL_STATE_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_state_machine.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Maximum depth of the state hierarchy
  */
 #define POLYCALL_POLYCALL_HIERARCHICAL_STATE_H_H
 
 /**
  * @brief Maximum number of child states per parent
  */
 #define POLYCALL_POLYCALL_HIERARCHICAL_STATE_H_H
 
 /**
  * @brief Maximum number of permissions per state
  */
 #define POLYCALL_POLYCALL_HIERARCHICAL_STATE_H_H
 
 /**
  * @brief Hierarchical state relationship types
  */
 typedef enum {
     POLYCALL_STATE_RELATIONSHIP_PARENT,       /**< Parent-child relationship */
     POLYCALL_STATE_RELATIONSHIP_COMPOSITION,  /**< Composite state relationship */
     POLYCALL_STATE_RELATIONSHIP_PARALLEL      /**< Parallel state relationship */
 } polycall_state_relationship_t;
 
 /**
  * @brief Permission inheritance models
  */
 typedef enum {
     POLYCALL_PERMISSION_INHERIT_NONE,         /**< No permission inheritance */
     POLYCALL_PERMISSION_INHERIT_ADDITIVE,     /**< Add permissions from parent */
     POLYCALL_PERMISSION_INHERIT_SUBTRACTIVE,  /**< Remove parent permissions */
     POLYCALL_PERMISSION_INHERIT_REPLACE       /**< Replace with parent permissions */
 } polycall_permission_inheritance_t;
 
 /**
  * @brief Hierarchical state transition types
  */
 typedef enum {
     POLYCALL_HTRANSITION_LOCAL,               /**< Transition within same parent */
     POLYCALL_HTRANSITION_EXTERNAL,            /**< Transition exiting parent state */
     POLYCALL_HTRANSITION_INTERNAL             /**< Transition without exiting current state */
 } polycall_hierarchical_transition_type_t;
 
 /**
  * @brief State hierarchy node configuration
  */
 typedef struct {
     char name[POLYCALL_SM_MAX_NAME_LENGTH];       /**< State name */
     polycall_state_relationship_t relationship;   /**< Relationship to parent */
     char parent_state[POLYCALL_SM_MAX_NAME_LENGTH]; /**< Parent state name */
     polycall_sm_state_callback_t on_enter;        /**< Enter callback */
     polycall_sm_state_callback_t on_exit;         /**< Exit callback */
     polycall_permission_inheritance_t inheritance_model; /**< Permission inheritance model */
     uint32_t permissions[POLYCALL_MAX_STATE_PERMISSIONS]; /**< State permissions */
     uint32_t permission_count;                   /**< Number of permissions */
 } polycall_hierarchical_state_config_t;
 
 /**
  * @brief Hierarchical transition configuration
  */
 typedef struct {
     char name[POLYCALL_SM_MAX_NAME_LENGTH];       /**< Transition name */
     char from_state[POLYCALL_SM_MAX_NAME_LENGTH]; /**< Source state */
     char to_state[POLYCALL_SM_MAX_NAME_LENGTH];   /**< Target state */
     polycall_hierarchical_transition_type_t type; /**< Transition type */
     polycall_sm_guard_fn guard;                   /**< Guard function */
 } polycall_hierarchical_transition_config_t;
 
 /**
  * @brief Hierarchical state machine context (opaque)
  */
 typedef struct polycall_hierarchical_state_context polycall_hierarchical_state_context_t;
 
 /**
  * @brief Initialize hierarchical state machine
  *
  * @param core_ctx Core context
  * @param hsm_ctx Pointer to receive hierarchical state machine context
  * @param sm State machine to enhance
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_init(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t** hsm_ctx,
     polycall_state_machine_t* sm
 );
 
 /**
  * @brief Clean up hierarchical state machine
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context to clean up
  */
 void polycall_hierarchical_state_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx
 );
 
 /**
  * @brief Add a hierarchical state
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param config State configuration
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_add(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const polycall_hierarchical_state_config_t* config
 );
 
 /**
  * @brief Add a hierarchical transition
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param config Transition configuration
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_add_transition(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const polycall_hierarchical_transition_config_t* config
 );
 
 /**
  * @brief Execute a hierarchical transition
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param transition_name Transition name
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_execute_transition(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* transition_name
 );
 
 /**
  * @brief Check if state has a specific permission
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param permission Permission to check
  * @return true if permission is granted, false otherwise
  */
 bool polycall_hierarchical_state_has_permission(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t permission
 );
 
 /**
  * @brief Get effective permissions for a state
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param permissions Array to receive permissions
  * @param max_permissions Maximum number of permissions to retrieve
  * @param permission_count Pointer to receive number of permissions
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_get_permissions(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t* permissions,
     uint32_t max_permissions,
     uint32_t* permission_count
 );
 
 /**
  * @brief Get the current hierarchical state path
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param path_buffer Buffer to receive state path
  * @param buffer_size Buffer size
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_get_path(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     char* path_buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Check if a state is active (directly or as part of hierarchy)
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @return true if state is active, false otherwise
  */
 bool polycall_hierarchical_state_is_active(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name
 );
 
 /**
  * @brief Add permission to a state
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param permission Permission to add
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_add_permission(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t permission
 );
 
 /**
  * @brief Remove permission from a state
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param permission Permission to remove
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_remove_permission(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     uint32_t permission
 );
 
 /**
  * @brief Set permission inheritance model for a state
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param inheritance_model Inheritance model
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_set_inheritance(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     polycall_permission_inheritance_t inheritance_model
 );
 
 /**
  * @brief Get parent state
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param parent_buffer Buffer to receive parent state name
  * @param buffer_size Buffer size
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_get_parent(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     char* parent_buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Get child states
  *
  * @param core_ctx Core context
  * @param hsm_ctx Hierarchical state machine context
  * @param state_name State name
  * @param children Array to receive child state names
  * @param max_children Maximum number of children to retrieve
  * @param child_count Pointer to receive number of children
  * @return Error code
  */
 polycall_core_error_t polycall_hierarchical_state_get_children(
     polycall_core_context_t* core_ctx,
     polycall_hierarchical_state_context_t* hsm_ctx,
     const char* state_name,
     char children[][POLYCALL_SM_MAX_NAME_LENGTH],
     uint32_t max_children,
     uint32_t* child_count
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_POLYCALL_HIERARCHICAL_STATE_H_H */