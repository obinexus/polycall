/**
#include "polycall/core/config/schema/schema_security_integration.h"

 * @file schema_security_integration.c
 * @brief Integration between schema validation and security validation
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the integration between the schema validation system
 * and the zero-trust security validation framework.
 */

 #include "polycall/core/config/schema/config_schema.h"
 #include "polycall/core/config/security/security_validation.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <string.h>
 
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
 ) {
     if (!ctx || !schema_ctx || !security_ctx || !component_config || !error_message || error_message_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // First, validate the schema
     polycall_core_error_t result = polycall_schema_validate_component(
         ctx,
         schema_ctx,
         component_type,
         component_config,
         error_message,
         error_message_size
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Then, validate security
     result = polycall_security_validate_component(
         security_ctx,
         component_type,
         component_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         // Copy error message from security validation
         const char* security_error = polycall_security_validation_get_error(security_ctx);
         if (security_error) {
             strncpy(error_message, security_error, error_message_size - 1);
             error_message[error_message_size - 1] = '\0';
         }
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
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
 ) {
     if (!ctx || !schema_ctx || !security_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create schema validation context
     polycall_core_error_t result = polycall_schema_context_create(
         ctx,
         schema_ctx,
         strict_validation
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Create security validation context
     result = polycall_security_validation_create(
         ctx,
         security_ctx,
         security_flags
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_schema_context_destroy(ctx, *schema_ctx);
         *schema_ctx = NULL;
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
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
 ) {
     if (!ctx) {
         return;
     }
     
     if (schema_ctx) {
         polycall_schema_context_destroy(ctx, schema_ctx);
     }
     
     if (security_ctx) {
         polycall_security_validation_destroy(ctx, security_ctx);
     }
 }
 
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
 ) {
     if (!config_ctx || !validation_hook) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return polycall_config_register_validation_hook(config_ctx, validation_hook, hook_ctx);
 }
 
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
 ) {
     if (!config_ctx || !component_config || !hook_ctx || !error_message) {
         return false;
     }
     
     // Extract validation contexts from hook context
     typedef struct {
         polycall_core_context_t* core_ctx;
         polycall_schema_context_t* schema_ctx;
         polycall_security_validation_context_t* security_ctx;
     } validation_context_pair_t;
     
     validation_context_pair_t* contexts = (validation_context_pair_t*)hook_ctx;
     
     // Perform integrated validation
     polycall_core_error_t result = polycall_validate_component_configuration(
         contexts->core_ctx,
         contexts->schema_ctx,
         contexts->security_ctx,
         component_type,
         component_config,
         error_message,
         error_message_size
     );
     
     return (result == POLYCALL_CORE_SUCCESS);
 }
 
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
 ) {
     if (!ctx || !config_ctx || !validation_contexts) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate validation contexts pair
     typedef struct {
         polycall_core_context_t* core_ctx;
         polycall_schema_context_t* schema_ctx;
         polycall_security_validation_context_t* security_ctx;
     } validation_context_pair_t;
     
     validation_context_pair_t* contexts = malloc(sizeof(validation_context_pair_t));
     if (!contexts) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     contexts->core_ctx = ctx;
     
     // Create validation contexts
     polycall_core_error_t result = polycall_create_validation_contexts(
         ctx,
         &contexts->schema_ctx,
         &contexts->security_ctx,
         strict_validation,
         security_flags
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         free(contexts);
         return result;
     }
     
     // Register validation hook
     result = polycall_register_config_validation_hook(
         config_ctx,
         default_validation_hook,
         contexts
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_destroy_validation_contexts(
             ctx,
             contexts->schema_ctx,
             contexts->security_ctx
         );
         free(contexts);
         return result;
     }
     
     *validation_contexts = contexts;
     return POLYCALL_CORE_SUCCESS;
 }
 
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
 ) {
     if (!ctx || !validation_contexts) {
         return;
     }
     
     typedef struct {
         polycall_core_context_t* core_ctx;
         polycall_schema_context_t* schema_ctx;
         polycall_security_validation_context_t* security_ctx;
     } validation_context_pair_t;
     
     validation_context_pair_t* contexts = (validation_context_pair_t*)validation_contexts;
     
     // Destroy validation contexts
     polycall_destroy_validation_contexts(
         ctx,
         contexts->schema_ctx,
         contexts->security_ctx
     );
     
     // Free contexts pair
     free(contexts);
     
     // Clean up global resources
     polycall_security_validation_cleanup_registry();
 }