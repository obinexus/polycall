/**
 * @file polycall_micro_component.h
 * @brief Component registry and component management for LibPolyCall micro command system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the component registry and component management interfaces
 * for the LibPolyCall micro command system.
 */

 #ifndef POLYCALL_MICRO_POLYCALL_MICRO_COMPONENT_H_H
 #define POLYCALL_MICRO_POLYCALL_MICRO_COMPONENT_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_micro_context.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Forward declarations
 typedef struct component_registry component_registry_t;
 typedef struct component_security_context component_security_context_t;
 
 /**
  * @brief Component registry configuration
  */
 typedef struct {
     size_t initial_capacity;    // Initial component capacity
     bool thread_safe;           // Enable thread safety
 } component_registry_config_t;
 
 /**
  * @brief Component information
  */
 typedef struct {
     const char* name;                      // Component name
     polycall_isolation_level_t isolation;  // Isolation level
     polycall_component_state_t state;      // Current state
     size_t command_count;                  // Number of registered commands
     size_t memory_usage;                   // Current memory usage
     uint32_t cpu_usage;                    // Current CPU usage
     uint32_t io_usage;                     // Current I/O usage
 } polycall_component_info_t;
 
 /**
  * @brief Component event callback function type
  */
 typedef void (*component_event_callback_t)(
     polycall_core_context_t* ctx,
     polycall_micro_context_t* micro_ctx,
     polycall_micro_component_t* component,
     polycall_component_state_t old_state,
     polycall_component_state_t new_state,
     void* user_data
 );
 
 /**
  * @brief Initialize component registry
  * 
  * @param ctx Core context
  * @param registry Pointer to receive registry
  * @param config Registry configuration
  * @return Error code
  */
 polycall_core_error_t component_registry_init(
     polycall_core_context_t* ctx,
     component_registry_t** registry,
     const component_registry_config_t* config
 );
 
 /**
  * @brief Clean up component registry
  * 
  * @param ctx Core context
  * @param registry Registry to clean up
  */
 void component_registry_cleanup(
     polycall_core_context_t* ctx,
     component_registry_t* registry
 );
 
 /**
  * @brief Register a component
  * 
  * @param ctx Core context
  * @param registry Component registry
  * @param component Component to register
  * @return Error code
  */
 polycall_core_error_t component_registry_register(
     polycall_core_context_t* ctx,
     component_registry_t* registry,
     polycall_micro_component_t* component
 );
 
 /**
  * @brief Unregister a component
  * 
  * @param ctx Core context
  * @param registry Component registry
  * @param component Component to unregister
  * @return Error code
  */
 polycall_core_error_t component_registry_unregister(
     polycall_core_context_t* ctx,
     component_registry_t* registry,
     polycall_micro_component_t* component
 );
 
 /**
  * @brief Find a component by name
  * 
  * @param ctx Core context
  * @param registry Component registry
  * @param name Component name
  * @param component Pointer to receive component
  * @return Error code
  */
 polycall_core_error_t component_registry_find(
     polycall_core_context_t* ctx,
     component_registry_t* registry,
     const char* name,
     polycall_micro_component_t** component
 );
 
 /**
  * @brief Get all components
  * 
  * @param ctx Core context
  * @param registry Component registry
  * @param components Array to receive components
  * @param count Pointer to size of array (in/out)
  * @return Error code
  */
 polycall_core_error_t component_registry_get_all(
     polycall_core_context_t* ctx,
     component_registry_t* registry,
     polycall_micro_component_t** components,
     size_t* count
 );
 
 /**
  * @brief Create a component
  * 
  * @param ctx Core context
  * @param component Pointer to receive component
  * @param name Component name
  * @param isolation_level Isolation level
  * @return Error code
  */
 polycall_core_error_t polycall_micro_component_create(
     polycall_core_context_t* ctx,
     polycall_micro_component_t** component,
     const char* name,
     polycall_isolation_level_t isolation_level
 );
 
 /**
  * @brief Destroy a component
  * 
  * @param ctx Core context
  * @param component Component to destroy
  */
 void polycall_micro_component_destroy(
     polycall_core_context_t* ctx,
     polycall_micro_component_t* component
 );
 
 /**
  * @brief Get component info
  * 
  * @param ctx Core context
  * @param component Component to get info for
  * @param info Pointer to receive info
  * @return Error code
  */
 polycall_core_error_t polycall_micro_component_get_info(
     polycall_core_context_t* ctx,
     polycall_micro_component_t* component,
     polycall_component_info_t* info
 );
 
 /**
  * @brief Register component state change callback
  * 
  * @param ctx Core context
  * @param component Component to register callback for
  * @param callback Callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_micro_component_register_callback(
     polycall_core_context_t* ctx,
     polycall_micro_component_t* component,
     component_event_callback_t callback,
     void* user_data
 );
 
 /**
  * @brief Unregister component state change callback
  * 
  * @param ctx Core context
  * @param component Component to unregister callback for
  * @param callback Callback function
  * @return Error code
  */
 polycall_core_error_t polycall_micro_component_unregister_callback(
     polycall_core_context_t* ctx,
     polycall_micro_component_t* component,
     component_event_callback_t callback
 );
 
 /**
  * @brief Initialize component security context
  * 
  * @param ctx Core context
  * @param component Component to initialize security for
  * @param security_ctx Pointer to receive security context
  * @return Error code
  */
 polycall_core_error_t polycall_micro_component_init_security(
     polycall_core_context_t* ctx,
     polycall_micro_component_t* component,
     component_security_context_t** security_ctx
 );
 
 /**
  * @brief Get component security context
  * 
  * @param ctx Core context
  * @param component Component to get security context for
  * @param security_ctx Pointer to receive security context
  * @return Error code
  */
 polycall_core_error_t polycall_micro_component_get_security(
     polycall_core_context_t* ctx,
     polycall_micro_component_t* component,
     component_security_context_t** security_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_MICRO_POLYCALL_MICRO_COMPONENT_H_H */