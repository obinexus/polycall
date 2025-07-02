/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/config/security/security_validation.h"

 * @file security_validation.c
 * @brief Zero-Trust Security Model implementation for LibPolyCall configuration validation
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements a comprehensive zero-trust security validation framework for
 * LibPolyCall configuration, ensuring all components adhere to security
 * requirements before accepting connections.
 */

 #include "polycall/core/config/security/security_validation.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 
 // Zero-Trust security policy enforcement flags
 #define ZERO_TRUST_ENFORCE_AUTH           0x0001
 #define ZERO_TRUST_ENFORCE_ENCRYPTION     0x0002
 #define ZERO_TRUST_ENFORCE_INTEGRITY      0x0004
 #define ZERO_TRUST_ENFORCE_ISOLATION      0x0008
 #define ZERO_TRUST_ENFORCE_AUDIT          0x0010
 #define ZERO_TRUST_ENFORCE_LEAST_PRIV     0x0020
 #define ZERO_TRUST_ENFORCE_ALL            0xFFFF
 
 /**
  * @brief Security validation context structure
  */
 struct polycall_security_validation_context {
     polycall_core_context_t* core_ctx;        // Core context
     uint32_t enforcement_flags;               // Which policies to enforce
     bool is_privileged_context;               // Whether the context is privileged
     char error_buffer[512];                   // Error message buffer
 };
 
 /**
  * @brief Network port info structure used to detect port conflicts
  */
 typedef struct {
     uint16_t port;                 // Port number
     polycall_component_type_t type; // Component type using this port
     char component_name[64];       // Component name using this port
     char purpose[32];              // Purpose of the port (e.g., "discovery", "command")
 } port_usage_info_t;
 
 /**
  * @brief Expanded list of commonly reserved service ports
  * 
  * This list includes ports that should be avoided in a zero-trust environment
  * unless specifically required for the service.
  */
 static const uint16_t reserved_ports[] = {
     // Well-known service ports
     20, 21,    // FTP
     22,        // SSH
     23,        // Telnet
     25, 587,   // SMTP
     53,        // DNS
     67, 68,    // DHCP
     80, 443,   // HTTP/HTTPS
     110,       // POP3
     123,       // NTP
     143,       // IMAP
     161, 162,  // SNMP
     389,       // LDAP
     636,       // LDAPS
     
     // Database ports
     1433, 1434, // SQL Server
     3306,       // MySQL
     5432,       // PostgreSQL
     6379,       // Redis
     27017, 27018, 27019, // MongoDB
     
     // Other common service ports
     3389,       // RDP
     5900,       // VNC
     8080, 8443, // Alternative HTTP/HTTPS
     9000,       // Often used for development servers
     9090, 9091  // Often used for management interfaces
 };
 
 /**
  * @brief Port usage tracking to detect conflicts
  */
 static port_usage_info_t* port_usage_registry = NULL;
 static size_t port_usage_count = 0;
 static size_t port_usage_capacity = 0;
 
 /**
  * @brief Create a security validation context
  */
 polycall_core_error_t polycall_security_validation_create(
     polycall_core_context_t* ctx,
     polycall_security_validation_context_t** security_ctx,
     uint32_t enforcement_flags
 ) {
     if (!ctx || !security_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate security validation context
     polycall_security_validation_context_t* new_ctx = 
         polycall_core_malloc(ctx, sizeof(polycall_security_validation_context_t));
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_security_validation_context_t));
     new_ctx->core_ctx = ctx;
     new_ctx->enforcement_flags = enforcement_flags;
     new_ctx->is_privileged_context = false; // Default to non-privileged
 
     // Initialize port usage registry if not already initialized
     if (!port_usage_registry) {
         port_usage_capacity = 32; // Initial capacity
         port_usage_registry = malloc(port_usage_capacity * sizeof(port_usage_info_t));
         if (!port_usage_registry) {
             polycall_core_free(ctx, new_ctx);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         port_usage_count = 0;
     }
     
     *security_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Destroy a security validation context
  */
 void polycall_security_validation_destroy(
     polycall_core_context_t* ctx,
     polycall_security_validation_context_t* security_ctx
 ) {
     if (!ctx || !security_ctx) {
         return;
     }
     
     // Free context
     polycall_core_free(ctx, security_ctx);
 }
 
 /**
  * @brief Cleanup global port registry (call at shutdown)
  */
 void polycall_security_validation_cleanup_registry(void) {
     if (port_usage_registry) {
         free(port_usage_registry);
         port_usage_registry = NULL;
         port_usage_count = 0;
         port_usage_capacity = 0;
     }
 }
 
 /**
  * @brief Set the privileged context flag
  */
 void polycall_security_validation_set_privileged(
     polycall_security_validation_context_t* security_ctx,
     bool is_privileged
 ) {
     if (security_ctx) {
         security_ctx->is_privileged_context = is_privileged;
     }
 }
 
 /**
  * @brief Get the last error message
  */
 const char* polycall_security_validation_get_error(
     polycall_security_validation_context_t* security_ctx
 ) {
     if (!security_ctx) {
         return "Invalid security context";
     }
     
     return security_ctx->error_buffer;
 }
 
 /**
  * @brief Register port usage to detect conflicts
  */
 static polycall_core_error_t register_port_usage(
     polycall_component_type_t component_type,
     const char* component_name,
     uint16_t port,
     const char* purpose
 ) {
     if (!component_name || !purpose) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check for conflicts
     for (size_t i = 0; i < port_usage_count; i++) {
         if (port_usage_registry[i].port == port) {
             // Port conflict detected
             return POLYCALL_CORE_ERROR_RESOURCE_CONFLICT;
         }
     }
     
     // Ensure capacity
     if (port_usage_count >= port_usage_capacity) {
         size_t new_capacity = port_usage_capacity * 2;
         port_usage_info_t* new_registry = realloc(
             port_usage_registry, 
             new_capacity * sizeof(port_usage_info_t)
         );
         
         if (!new_registry) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         port_usage_registry = new_registry;
         port_usage_capacity = new_capacity;
     }
     
     // Register port usage
     port_usage_info_t* info = &port_usage_registry[port_usage_count++];
     info->port = port;
     info->type = component_type;
     strncpy(info->component_name, component_name, sizeof(info->component_name) - 1);
     info->component_name[sizeof(info->component_name) - 1] = '\0';
     strncpy(info->purpose, purpose, sizeof(info->purpose) - 1);
     info->purpose[sizeof(info->purpose) - 1] = '\0';
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Check if a port number is within a valid ephemeral port range
  * 
  * Different operating systems use different ranges for ephemeral ports:
  * - Linux: 32768 to 60999
  * - Windows: 49152 to 65535
  * - FreeBSD: 10000 to 65535
  * 
  * This function checks if a port is within the typical recommended
  * ephemeral port range for servers (49152-65535)
  */
 static bool is_in_ephemeral_range(uint16_t port) {
     return port >= 49152 && port <= 65535;
 }
 
 /**
  * @brief Enhanced port validation with zero-trust principles
  * 
  * Implements a comprehensive approach to port validation:
  * 1. Checks that the port is in a valid range (0-65535)
  * 2. Enforces privileged port restrictions (0-1023)
  * 3. Checks against an expanded list of commonly reserved service ports
  * 4. Validates against ephemeral port ranges
  * 5. Detects port usage conflicts with other components
  */
 polycall_core_error_t polycall_security_validate_port(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     uint16_t port,
     const char* purpose,
     bool register_if_valid
 ) {
     if (!security_ctx || !component_name || !purpose) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check port is in valid range
     if (port > 65535) { // unsigned, so no need to check < 0
         snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                 "Invalid port number: %u (must be 0-65535)", port);
         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
     }
     
     // Check for privileged ports
     if (port < 1024 && !security_ctx->is_privileged_context) {
         snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                 "Port %u requires privileged access (0-1023)", port);
         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
     }
     
     // Check for common reserved ports
     const size_t reserved_port_count = sizeof(reserved_ports) / sizeof(reserved_ports[0]);
     
     for (size_t i = 0; i < reserved_port_count; i++) {
         if (port == reserved_ports[i]) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Port %u is commonly reserved for other services", port);
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     // Check if port is in ephemeral range - warn but don't fail
     if (is_in_ephemeral_range(port)) {
         // Just a warning - log it but don't fail validation
         POLYCALL_LOG(security_ctx->core_ctx, POLYCALL_LOG_WARNING,
                     "Component '%s' is using port %u which is in the ephemeral port range",
                     component_name, port);
     }
     
     // Check for port conflicts with other components
     for (size_t i = 0; i < port_usage_count; i++) {
         if (port_usage_registry[i].port == port) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Port %u is already in use by component '%s' for %s",
                     port, port_usage_registry[i].component_name, 
                     port_usage_registry[i].purpose);
             return POLYCALL_CORE_ERROR_RESOURCE_CONFLICT;
         }
     }
     
     // Register port usage if requested and validation passed
     if (register_if_valid) {
         polycall_core_error_t result = register_port_usage(
             component_type, 
             component_name, 
             port, 
             purpose
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Failed to register port usage: error code %d", result);
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate the authentication requirements for a component
  * 
  * In a zero-trust model, all components must enforce authentication
  * unless explicitly exempted (which should be rare)
  */
 polycall_core_error_t polycall_security_validate_authentication(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     bool requires_authentication
 ) {
     if (!security_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If authentication enforcement is enabled
     if (security_ctx->enforcement_flags & ZERO_TRUST_ENFORCE_AUTH) {
         if (!requires_authentication) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Zero-Trust violation: Component '%s' does not require authentication",
                     component_name);
             return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate the encryption requirements for a component
  * 
  * In a zero-trust model, sensitive network communication should be
  * encrypted unless explicitly exempted
  */
 polycall_core_error_t polycall_security_validate_encryption(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     bool enables_encryption
 ) {
     if (!security_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If encryption enforcement is enabled
     if (security_ctx->enforcement_flags & ZERO_TRUST_ENFORCE_ENCRYPTION) {
         if (!enables_encryption) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Zero-Trust violation: Component '%s' does not enable encryption",
                     component_name);
             return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate the isolation level for a component
  * 
  * In a zero-trust model, components should have appropriate
  * isolation levels based on their purpose
  */
 polycall_core_error_t polycall_security_validate_isolation(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     int isolation_level,
     int minimum_required_level
 ) {
     if (!security_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If isolation enforcement is enabled
     if (security_ctx->enforcement_flags & ZERO_TRUST_ENFORCE_ISOLATION) {
         if (isolation_level < minimum_required_level) {
             const char* level_names[] = {
                 "None", "Memory", "Resources", "Security", "Strict"
             };
             
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Zero-Trust violation: Component '%s' has insufficient isolation level '%s', requires at least '%s'",
                     component_name, 
                     isolation_level >= 0 && isolation_level < 5 ? level_names[isolation_level] : "Unknown",
                     minimum_required_level >= 0 && minimum_required_level < 5 ? level_names[minimum_required_level] : "Unknown");
             return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate the audit requirements for a component
  * 
  * In a zero-trust model, all access should be audited
  */
 polycall_core_error_t polycall_security_validate_audit(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const char* component_name,
     bool enables_audit
 ) {
     if (!security_ctx || !component_name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If audit enforcement is enabled
     if (security_ctx->enforcement_flags & ZERO_TRUST_ENFORCE_AUDIT) {
         if (!enables_audit) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Zero-Trust violation: Component '%s' does not enable audit logging",
                     component_name);
             return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Comprehensive security validation for micro component configuration
  */
 polycall_core_error_t polycall_security_validate_micro_component(
     polycall_security_validation_context_t* security_ctx,
     const micro_component_config_t* config
 ) {
     if (!security_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_core_error_t result;
     
     // Validate authentication requirements
     result = polycall_security_validate_authentication(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_MICRO,
         config->name,
         config->require_authentication
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Validate audit requirements
     result = polycall_security_validate_audit(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_MICRO,
         config->name,
         config->audit_access
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Validate isolation level
     result = polycall_security_validate_isolation(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_MICRO,
         config->name,
         config->isolation_level,
         POLYCALL_ISOLATION_MEMORY // Minimum required level
     );
     
     return result;
 }
 
 /**
  * @brief Comprehensive security validation for edge component configuration
  */
 polycall_core_error_t polycall_security_validate_edge_component(
     polycall_security_validation_context_t* security_ctx,
     const polycall_edge_component_config_t* config
 ) {
     if (!security_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_core_error_t result;
     
     // Validate ports
     result = polycall_security_validate_port(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_EDGE,
         config->component_name,
         config->discovery_port,
         "discovery",
         true
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     result = polycall_security_validate_port(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_EDGE,
         config->component_name,
         config->command_port,
         "command",
         true
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     result = polycall_security_validate_port(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_EDGE,
         config->component_name,
         config->data_port,
         "data",
         true
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Validate security settings
     result = polycall_security_validate_authentication(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_EDGE,
         config->component_name,
         config->security_config.enforce_node_authentication
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     result = polycall_security_validate_encryption(
         security_ctx,
         POLYCALL_COMPONENT_TYPE_EDGE,
         config->component_name,
         config->security_config.enable_end_to_end_encryption
     );
     
     return result;
 }
 
 /**
  * @brief Comprehensive security validation for network configuration
  */
 polycall_core_error_t polycall_security_validate_network_config(
     polycall_security_validation_context_t* security_ctx,
     const polycall_network_config_t* config
 ) {
     if (!security_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // For network config, we need to extract necessary settings based on
     // the configuration entries
     
     // Example of validating encryption
     bool enable_encryption = false;
     bool enable_tls = false;
     
     // Get config values using the network config API
     config_entry_t* entry = find_config_entry(config, "security", "enable_encryption");
     if (entry && entry->type == CONFIG_TYPE_BOOL) {
         enable_encryption = entry->value.bool_value;
     }
     
     entry = find_config_entry(config, "security", "enable_tls");
     if (entry && entry->type == CONFIG_TYPE_BOOL) {
         enable_tls = entry->value.bool_value;
     }
     
     // Validate encryption
     if (security_ctx->enforcement_flags & ZERO_TRUST_ENFORCE_ENCRYPTION) {
         if (!enable_encryption && !enable_tls) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Zero-Trust violation: Network configuration does not enable encryption or TLS");
             return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Comprehensive security validation for protocol configuration
  */
 polycall_core_error_t polycall_security_validate_protocol_config(
     polycall_security_validation_context_t* security_ctx,
     const protocol_config_t* config
 ) {
     if (!security_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Validate TLS/encryption requirements
     if (security_ctx->enforcement_flags & ZERO_TRUST_ENFORCE_ENCRYPTION) {
         if (!config->core.enable_tls) {
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Zero-Trust violation: Protocol configuration does not enable TLS");
             return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate overall component security configuration
  */
 polycall_core_error_t polycall_security_validate_component(
     polycall_security_validation_context_t* security_ctx,
     polycall_component_type_t component_type,
     const void* component_config
 ) {
     if (!security_ctx || !component_config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     switch (component_type) {
         case POLYCALL_COMPONENT_TYPE_MICRO:
             return polycall_security_validate_micro_component(
                 security_ctx,
                 (const micro_component_config_t*)component_config
             );
             
         case POLYCALL_COMPONENT_TYPE_EDGE:
             return polycall_security_validate_edge_component(
                 security_ctx,
                 (const polycall_edge_component_config_t*)component_config
             );
             
         case POLYCALL_COMPONENT_TYPE_NETWORK:
             return polycall_security_validate_network_config(
                 security_ctx,
                 (const polycall_network_config_t*)component_config
             );
             
         case POLYCALL_COMPONENT_TYPE_PROTOCOL:
             return polycall_security_validate_protocol_config(
                 security_ctx,
                 (const protocol_config_t*)component_config
             );
             
         // Add other component types as needed
             
         default:
             snprintf(security_ctx->error_buffer, sizeof(security_ctx->error_buffer),
                     "Unknown component type: %d", component_type);
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 }
