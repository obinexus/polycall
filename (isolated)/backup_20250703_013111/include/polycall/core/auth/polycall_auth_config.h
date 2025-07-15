/**
 * @file polycall_auth_config.h
 * @brief Authentication configuration for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the configuration interfaces for LibPolyCall authentication,
 * providing functions to load, validate, and manage authentication settings.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 #include <string.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Load authentication configuration from file
  *
  * @param core_ctx Core context
  * @param config_file Configuration file path
  * @param config Pointer to receive the configuration
  * @return Error code
  */
 polycall_core_error_t polycall_auth_load_config(
     polycall_core_context_t* core_ctx,
     const char* config_file,
     polycall_auth_config_t* config
 );
 
 /**
  * @brief Free resources associated with authentication configuration
  *
  * @param core_ctx Core context
  * @param config Configuration to clean up
  */
 void polycall_auth_cleanup_config(
     polycall_core_context_t* core_ctx,
     polycall_auth_config_t* config
 );
 
 /**
  * @brief Merge two authentication configurations
  *
  * Values from the override configuration take precedence over the base configuration.
  *
  * @param core_ctx Core context
  * @param base Base configuration
  * @param override Override configuration
  * @param result Pointer to receive the merged configuration
  * @return Error code
  */
 polycall_core_error_t polycall_auth_merge_configs(
     polycall_core_context_t* core_ctx,
     const polycall_auth_config_t* base,
     const polycall_auth_config_t* override,
     polycall_auth_config_t* result
 );
 
 /**
  * @brief Apply zero-trust security constraints to authentication configuration
  *
  * This function ensures that authentication configuration meets zero-trust
  * security requirements, overriding unsafe settings if necessary.
  *
  * @param core_ctx Core context
  * @param config Configuration to secure
  * @return Error code
  */
 polycall_core_error_t polycall_auth_apply_zero_trust_constraints(
     polycall_core_context_t* core_ctx,
     polycall_auth_config_t* config
 );
 
 // Configuration constants
 
 /**
  * @brief Minimum allowed token validity period in seconds (5 minutes)
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Maximum allowed token validity period in seconds (24 hours)
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Minimum allowed refresh token validity period in seconds (1 hour)
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Maximum allowed refresh token validity period in seconds (365 days)
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Minimum length of token signing secret
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Recommended length of token signing secret for zero-trust security
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Default token validity period in seconds (1 hour)
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 /**
  * @brief Default refresh token validity period in seconds (30 days)
  */
 #define POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_CONFIG_H_H */