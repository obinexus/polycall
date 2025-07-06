/**
#include <stdbool.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stddef.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdint.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

 * @file advanced_security.h
 * @brief Advanced Security Module for LibPolyCall Protocol
 * @author Nnamdi Okpala (OBINexus Computing)
 *
 * Provides comprehensive security mechanisms with dynamic threat assessment,
 * adaptive authentication, and granular access control.
 */

#ifndef POLYCALL_AUTH_ADVANCED_SECURITY_H_H
#define POLYCALL_AUTH_ADVANCED_SECURITY_H_H

 /**
  * Magic number for security context validation
  */
#define POLYCALL_AUTH_ADVANCED_SECURITY_H_H

/**
 * @brief Opaque handle for the advanced security context
 */
typedef struct polycall_advanced_security_context* polycall_advanced_security_context_t;

 /**
  * @brief Security threat level assessment
  */
 typedef enum {
   POLYCALL_SECURITY_THREAT_NONE = 0, /**< No security threat detected */
   POLYCALL_SECURITY_THREAT_LOW,      /**< Low-level security concern */
   POLYCALL_SECURITY_THREAT_MEDIUM,   /**< Medium-level security threat */
   POLYCALL_SECURITY_THREAT_HIGH,     /**< High-level security threat */
   POLYCALL_SECURITY_THREAT_CRITICAL  /**< Critical security threat */
 } polycall_security_threat_level_t;

 /**
  * @brief Authentication strategy modes
  */
 typedef enum {
   POLYCALL_AUTH_STRATEGY_SINGLE_FACTOR = 0, /**< Simple authentication */
   POLYCALL_AUTH_STRATEGY_MULTI_FACTOR,      /**< Multi-factor authentication */
   POLYCALL_AUTH_STRATEGY_ADAPTIVE /**< Adaptive authentication based on threat
                                      level */
 } polycall_auth_strategy_t;

 /**
  * @brief Authentication methods
  */
 typedef enum {
   POLYCALL_AUTH_METHOD_NONE = 0, /**< No authentication */
   POLYCALL_AUTH_METHOD_PASSWORD, /**< Password-based authentication */
   POLYCALL_AUTH_METHOD_TOKEN,    /**< Token-based authentication */
   POLYCALL_AUTH_METHOD_BIOMETRIC /**< Biometric authentication */
 } polycall_auth_method_t;

 /**
  * @brief Security event callback function type
  */
 typedef void (*polycall_security_event_callback_t)(uint32_t event_id,
                                                    void *event_data,
                                                    void *user_data);

 /**
  * @brief Configuration for advanced security initialization
  */
 typedef struct {
   polycall_auth_strategy_t
       initial_strategy; /**< Initial authentication strategy */
   polycall_auth_method_t
       default_auth_method;  /**< Default authentication method */
   uint32_t max_permissions; /**< Maximum number of permissions */
   polycall_security_event_callback_t
       event_callback; /**< Optional callback for security events */
   void *user_data;    /**< User data passed to callbacks */
 } polycall_advanced_security_config_t;

 /**
  * Initialize advanced security context
  *
  * @param core_ctx The core context
  * @param security_ctx Pointer to receive the new security context
  * @param config Security configuration parameters
  * @return Error code
  */
 polycall_core_error_t polycall_advanced_security_init(
     polycall_core_context_t *core_ctx,
     polycall_advanced_security_context_t *security_ctx,
     const polycall_advanced_security_config_t *config);

 /**
  * Authenticate using the current authentication strategy
  *
  * @param core_ctx The core context
  * @param security_ctx The security context
  * @param credentials Authentication credentials
  * @param credentials_size Size of the credentials data
  * @return Error code
  */
 polycall_core_error_t polycall_advanced_security_authenticate(
     polycall_core_context_t *core_ctx,
     polycall_advanced_security_context_t security_ctx, const void *credentials,
     size_t credentials_size);

 /**
  * Validate access to a specific permission
  *
  * @param security_ctx The security context
  * @param permission_id Permission identifier to check
  * @return True if access is granted, false otherwise
  */
 bool polycall_advanced_security_check_permission(
     polycall_advanced_security_context_t security_ctx, uint32_t permission_id);

 /**
  * Grant a specific permission
  *
  * @param security_ctx The security context
  * @param permission_id Permission identifier to grant
  * @return Error code
  */
 polycall_core_error_t polycall_advanced_security_grant_permission(
     polycall_advanced_security_context_t security_ctx, uint32_t permission_id);

 /**
  * Revoke a specific permission
  *
  * @param security_ctx The security context
  * @param permission_id Permission identifier to revoke
  * @return Error code
  */
 polycall_core_error_t polycall_advanced_security_revoke_permission(
     polycall_advanced_security_context_t security_ctx, uint32_t permission_id);

 /**
  * Perform cryptographic key rotation
  *
  * @param security_ctx The security context
  * @return Error code
  */
 polycall_core_error_t polycall_advanced_security_rotate_keys(
     polycall_advanced_security_context_t security_ctx);

 /**
  * Clean up and free resources associated with a security context
  *
  * @param security_ctx The security context to clean up
  */
 void polycall_advanced_security_cleanup(
     polycall_advanced_security_context_t security_ctx);

#endif /* POLYCALL_AUTH_ADVANCED_SECURITY_H_H */
