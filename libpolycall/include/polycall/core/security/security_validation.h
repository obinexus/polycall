/**
 * @file security_validation.h
 * @brief Zero-Trust Security Model interfaces for LibPolyCall configuration validation
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the interfaces for the zero-trust security validation
 * framework, ensuring all components adhere to security requirements.
 */

 #ifndef POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/micro/micro_config.h"
 #include "polycall/core/edge/edge_config.h"
 #include "polycall/core/network/network_config.h"
 #include "polycall/core/protocol/protocol_config.h"
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Zero-Trust security policy enforcement flags
  */
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 #define POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H
 
 /**
  * @brief Security validation context
  */
 typedef struct polycall_security_validation_context polycall_security_validation_context_t;
 
 /**
  * @brief Create a security validation context
  * 
  * @param ctx Core context
  * @param security_ctx Pointer to receive the created security context
  * @param enforcement_flags Which security policies to enforce
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validation_create(
     polycall_core_context_t* ctx,
     polycall_security_validation_context_t** security_ctx,
     uint32_t enforcement_flags
 );
 
 /**
  * @brief Destroy a security validation context
  * 
  * @param ctx Core context
  * @param security_ctx Security context to destroy
  */
 void polycall_security_validation_destroy(
     polycall_core_context_t* ctx,
     polycall_security_validation_context_t* security_ctx
 );
 
 /**
  * @brief Cleanup global security validation resources
  * 
  * Call this function at program shutdown to release global resources
  */
 void polycall_security_validation_cleanup_registry(void);
 
 /**
  * @brief Set the privileged context flag
  * 
  * @param security_ctx Security context
  * @param is_privileged Whether the context is privileged (can use privileged ports)
  */
 void polycall_security_validation_set_privileged(
     polycall_security_validation_context_t* security_ctx,
     bool is_privileged
 );
 
 /**
  * @brief Get the last error message
  * 
  * @param security_ctx Security context
  * @return const char* Error message
  */
 const char* polycall_security_validation_get_error(
     polycall_security_validation_context_t* security_ctx
 );
 
 /**
  * @brief Validate a network port with zero-trust principles
  * 
  * @param security_ctx Security context
  * @param component_type Type of component using the port
  * @param component_name Name of component using the port
  * @param port Port number to validate
  * @param purpose Purpose of the port (e.g., "discovery", "command")
  * @param register_if_valid Whether to register port usage if validation passes
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_port(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     uint16_t port,
     const char* purpose,
     bool register_if_valid
 );
 
 /**
  * @brief Validate authentication requirements for a component
  * 
  * @param security_ctx Security context
  * @param component_type Type of component
  * @param component_name Name of component
  * @param requires_authentication Whether the component requires authentication
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_authentication(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     bool requires_authentication
 );
 
 /**
  * @brief Validate encryption requirements for a component
  * 
  * @param security_ctx Security context
  * @param component_type Type of component
  * @param component_name Name of component
  * @param enables_encryption Whether the component enables encryption
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_encryption(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     bool enables_encryption
 );
 
 /**
  * @brief Validate isolation level for a component
  * 
  * @param security_ctx Security context
  * @param component_type Type of component
  * @param component_name Name of component
  * @param isolation_level Current isolation level
  * @param minimum_required_level Minimum required isolation level
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_isolation(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     int isolation_level,
     int minimum_required_level
 );
 
 /**
  * @brief Validate audit requirements for a component
  * 
  * @param security_ctx Security context
  * @param component_type Type of component
  * @param component_name Name of component
  * @param enables_audit Whether the component enables audit logging
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_audit(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     bool enables_audit
 );
 
 /**
  * @brief Comprehensive security validation for micro component configuration
  * 
  * @param security_ctx Security context
  * @param config Micro component configuration
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_micro_component(
     polycall_security_validation_context_t* security_ctx,
     const micro_component_config_t* config
 );
 
 /**
  * @brief Comprehensive security validation for edge component configuration
  * 
  * @param security_ctx Security context
  * @param config Edge component configuration
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_edge_component(
     polycall_security_validation_context_t* security_ctx,
     const polycall_edge_component_config_t* config
 );
 
 /**
  * @brief Comprehensive security validation for network configuration
  * 
  * @param security_ctx Security context
  * @param config Network configuration
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_network_config(
     polycall_security_validation_context_t* security_ctx,
     const polycall_network_config_t* config
 );
 
 /**
  * @brief Comprehensive security validation for protocol configuration
  * 
  * @param security_ctx Security context
  * @param config Protocol configuration
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_protocol_config(
     polycall_security_validation_context_t* security_ctx,
     const protocol_config_t* config
 );
 
 /**
  * @brief Validate overall component security configuration
  * 
  * @param security_ctx Security context
  * @param component_type Type of component
  * @param component_config Component configuration (type-specific)
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycall_security_validate_component(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const void* component_config
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_SECURITY_SECURITY_VALIDATION_H_H */