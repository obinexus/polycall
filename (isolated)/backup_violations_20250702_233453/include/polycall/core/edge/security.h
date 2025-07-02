/**
 * @file security.h
 * @brief Edge Computing Security Module for LibPolyCall
 * @author Nnamdi Michael Okpala
 *
 * Provides comprehensive security mechanisms for distributed edge computing
 */

 #ifndef POLYCALL_EDGE_SECURITY_H_H
 #define POLYCALL_EDGE_SECURITY_H_H
 

 #include "polycall/core/polycall/polycall_edge.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_config.h"
 #include "polycall/core/polycall/polycall_memory.h"



 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Security threat levels for edge nodes
  */
 typedef enum {
     EDGE_SECURITY_THREAT_NONE = 0,        // No detected threats
     EDGE_SECURITY_THREAT_LOW = 1,         // Minor potential vulnerabilities
     EDGE_SECURITY_THREAT_MEDIUM = 2,      // Significant potential risks
     EDGE_SECURITY_THREAT_HIGH = 3,        // Critical security concerns
     EDGE_SECURITY_THREAT_CRITICAL = 4     // Immediate security breach
 } polycall_edge_threat_level_t;
 
 /**
  * @brief Node authentication types
  */
 typedef enum {
     NODE_AUTH_NONE = 0,           // No authentication
     NODE_AUTH_CERTIFICATE = 1,    // X.509 certificate-based
     NODE_AUTH_TOKEN = 2,          // JWT or custom token-based
     NODE_AUTH_MUTUAL_TLS = 3,     // Mutual TLS authentication
     NODE_AUTH_BIOMETRIC = 4       // Advanced biometric authentication
 } polycall_node_auth_type_t;
 
 /**
  * @brief Security context for edge nodes
  */
 typedef struct {
     const char* node_id;                      // Unique node identifier
     polycall_node_auth_type_t auth_type;      // Authentication method
     bool is_authenticated;                    // Current authentication status
     uint64_t auth_timestamp;                  // Authentication timestamp
     polycall_edge_threat_level_t threat_level; // Current threat assessment
     void* security_token;                     // Opaque security token
     size_t token_size;                        // Security token size
 } polycall_edge_security_context_t;
 
 /**
  * @brief Security policy configuration for edge computing
  */
 typedef struct {
     bool enforce_node_authentication;         // Mandatory node authentication
     bool enable_end_to_end_encryption;        // Data encryption between nodes
     bool validate_node_integrity;             // Verify node system integrity
     uint32_t token_lifetime_ms;               // Security token validity duration
     uint32_t max_failed_auth_attempts;        // Maximum authentication failures
     polycall_edge_threat_level_t min_trust_level; // Minimum acceptable trust level
 } polycall_edge_security_policy_t;
 
 /**
  * @brief Initialize edge security context
  *
  * @param core_ctx Core context
  * @param security_ctx Pointer to receive security context
  * @param policy Security policy configuration
  * @return Error code
  */
 polycall_core_error_t polycall_edge_security_init(
     polycall_core_context_t* core_ctx,
     polycall_edge_security_context_t** security_ctx,
     const polycall_edge_security_policy_t* policy
 );
 
 /**
  * @brief Authenticate an edge node
  *
  * @param core_ctx Core context
  * @param security_ctx Security context
  * @param auth_token Authentication token
  * @param token_size Token size
  * @return Error code
  */
 polycall_core_error_t polycall_edge_security_authenticate(
     polycall_core_context_t* core_ctx,
     polycall_edge_security_context_t* security_ctx,
     const void* auth_token,
     size_t token_size
 );
 
 /**
  * @brief Perform node integrity check
  *
  * @param core_ctx Core context
  * @param security_ctx Security context
  * @return Error code
  */
 polycall_core_error_t polycall_edge_security_check_integrity(
     polycall_core_context_t* core_ctx,
     polycall_edge_security_context_t* security_ctx
 );
 
 /**
  * @brief Assess security threat level for a node
  *
  * @param core_ctx Core context
  * @param security_ctx Security context
  * @param threat_level Pointer to receive threat level
  * @return Error code
  */
 polycall_core_error_t polycall_edge_security_assess_threat(
     polycall_core_context_t* core_ctx,
     polycall_edge_security_context_t* security_ctx,
     polycall_edge_threat_level_t* threat_level
 );
 
 /**
  * @brief Revoke node authentication
  *
  * @param core_ctx Core context
  * @param security_ctx Security context
  * @return Error code
  */
 polycall_core_error_t polycall_edge_security_revoke(
     polycall_core_context_t* core_ctx,
     polycall_edge_security_context_t* security_ctx
 );
 
 /**
  * @brief Clean up edge security context
  *
  * @param core_ctx Core context
  * @param security_ctx Security context to clean up
  */
 void polycall_edge_security_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_edge_security_context_t* security_ctx
 );
 
 /**
  * @brief Create default edge security policy
  *
  * @return Default security policy configuration
  */
 polycall_edge_security_policy_t polycall_edge_security_default_policy(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_EDGE_SECURITY_H_H */