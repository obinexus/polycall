/**
 * @file schema_security_integration.h
 * @brief Integration between schema validation and security validation
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the integration between the schema validation system
 * and the zero-trust security validation framework.
 */

 #ifndef POLYCALL_CONFIG_SECURITY_SCHEMA_SECURITY_INTEGRATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SCHEMA_SECURITY_INTEGRATION_H_H
 
 #include "polycall/core/config/schema/config_schema.h"
 #include "polycall/core/config/security/security_validation.h"
 #include "polycall/core/polycall/config/polycall_config.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Validation hook function type
  */
 typedef bool (*polycall_config_validation_hook_t)(
     polycall_config_context_t* config_ctx,
     polycall_component_type_t component_type,
     const void* component_config,
     void* hook_ctx,
     char* error_message,
     size_t error_message_size
 );
 
 /**
  * @brief Integrated schema and security validation for component configuration
  * 
  * Performs both schema validation and security validation on a component
  * configuration, ensuring it meets both structural and security requirements.
  * 
  * @param ctx Core context
  * @param schema_ctx Schema validation context
  * @param security_ctx Security validation context
  * @param component_type Component type
  * @param component_config Component configuration
  * @param error_message Error message buffer
  * @param error_message_size Error message buffer size
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_validate_component_configuration(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const void* component_config,
     char* error_message,
     size_t error_message_size
 );
 
 /**
  * @brief Create integrated validation contexts
  * 
  * Creates both schema and security validation contexts with default settings.
  * 
  * @param ctx Core context
  * @param schema_ctx Pointer to receive the schema context
  * @param security_ctx Pointer to receive the security context
  * @param strict_validation Whether to use strict validation
  * @param security_flags Security policy enforcement flags
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_create_validation_contexts(
     polycall_core_context_t* ctx,
     polycall_schema_context_t** schema_ctx,
     polycall_security_validation_context_t** security_ctx,
     bool strict_validation,
     uint32_t security_flags
 );
 
 /**
  * @brief Destroy integrated validation contexts
  * 
  * Destroys both schema and security validation contexts.
  * 
  * @param ctx Core context
  * @param schema_ctx Schema context to destroy
  * @param security_ctx Security context to destroy
  */
 void polycall_destroy_validation_contexts(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     polycall_security_validation_context_t* security_ctx
 );
 
 /**
  * @brief Register configuration loading hook
  * 
  * Registers a hook that will be called when configuration is being loaded,
  * allowing security validation to be performed automatically.
  * 
  * @param config_ctx Configuration context
  * @param validation_hook Validation hook function
  * @param hook_ctx Hook context
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_register_config_validation_hook(
     polycall_config_context_t* config_ctx,
     polycall_config_validation_hook_t validation_hook,
     void* hook_ctx
 );
 
 /**
  * @brief Default validation hook implementation
  * 
  * @param config_ctx Configuration context
  * @param component_type Component type
  * @param component_config Component configuration
  * @param hook_ctx Hook context (should be a validation_context_pair_t*)
  * @param error_message Error message buffer
  * @param error_message_size Error message buffer size
  * @return bool True if validation passed, false otherwise
  */
 bool default_validation_hook(
     polycall_config_context_t* config_ctx,
     polycall_component_type_t component_type,
     const void* component_config,
     void* hook_ctx,
     char* error_message,
     size_t error_message_size
 );
 
 /**
  * @brief Setup integrated validation for configuration loading
  * 
  * Sets up the integration between the configuration system, schema validation,
  * and security validation, ensuring all loaded configurations meet both
  * structural and security requirements.
  * 
  * @param ctx Core context
  * @param config_ctx Configuration context
  * @param strict_validation Whether to use strict validation
  * @param security_flags Security policy enforcement flags
  * @param validation_contexts Pointer to receive the validation contexts
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_setup_integrated_validation(
     polycall_core_context_t* ctx,
     polycall_config_context_t* config_ctx,
     bool strict_validation,
     uint32_t security_flags,
     void** validation_contexts
 );
 
 /**
  * @brief Cleanup integrated validation
  * 
  * Cleans up the integrated validation setup.
  * 
  * @param ctx Core context
  * @param validation_contexts Validation contexts to clean up
  */
 void polycall_cleanup_integrated_validation(
     polycall_core_context_t* ctx,
     void* validation_contexts
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_SECURITY_SCHEMA_SECURITY_INTEGRATION_H_H */