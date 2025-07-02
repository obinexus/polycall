/**
 * @file config_binding.h
 * @brief OBINexus LibPolyCall v2 Hotwiring Configuration Binding Interface
 * @version 2.0.0
 * @author OBINexus Computing - OpenACE Division
 * 
 * Constitutional configuration bindings for hot-wiring architecture
 * Ensures compliance with OBIAxis governance and v1 compatibility
 */

#ifndef HOTWIRE_CONFIG_BINDING_H
#define HOTWIRE_CONFIG_BINDING_H

#include "core/polycall/polycall_core.h"
#include "core/protocol/protocol_interface.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* Constitutional Constants */
/*---------------------------------------------------------------------------*/

#define HOTWIRE_CONFIG_VERSION_MAJOR    2
#define HOTWIRE_CONFIG_VERSION_MINOR    0
#define HOTWIRE_CONFIG_VERSION_PATCH    0

#define HOTWIRE_CONFIG_MAX_PROTOCOLS    64
#define HOTWIRE_CONFIG_MAX_PATH_LENGTH  512
#define HOTWIRE_CONFIG_MAX_NAME_LENGTH  128

/*---------------------------------------------------------------------------*/
/* Configuration Types and Enums */
/*---------------------------------------------------------------------------*/

/**
 * @brief Hotwiring audit levels per constitutional requirements
 */
typedef enum {
    HOTWIRE_AUDIT_DISABLED = 0,
    HOTWIRE_AUDIT_BASIC,
    HOTWIRE_AUDIT_DETAILED,
    HOTWIRE_AUDIT_CONSTITUTIONAL  // Full OBIAxis compliance audit
} hotwire_audit_level_t;

/**
 * @brief Route execution modes
 */
typedef enum {
    HOTWIRE_MODE_STATELESS = 0,     // Constitutional requirement
    HOTWIRE_MODE_STATEFUL,          // Requires OBIAxis approval
    HOTWIRE_MODE_HYBRID
} hotwire_execution_mode_t;

/**
 * @brief Protocol compatibility levels
 */
typedef enum {
    HOTWIRE_COMPAT_V1_STRICT = 0,   // Full v1 backward compatibility
    HOTWIRE_COMPAT_V1_RELAXED,      // Best-effort v1 compatibility
    HOTWIRE_COMPAT_V2_NATIVE        // v2 native mode only
} hotwire_compatibility_mode_t;

/*---------------------------------------------------------------------------*/
/* Core Configuration Structures */
/*---------------------------------------------------------------------------*/

/**
 * @brief Protocol route configuration
 */
typedef struct {
    char source_protocol[HOTWIRE_CONFIG_MAX_NAME_LENGTH];
    char target_protocol[HOTWIRE_CONFIG_MAX_NAME_LENGTH];
    char config_file_path[HOTWIRE_CONFIG_MAX_PATH_LENGTH];
    hotwire_execution_mode_t execution_mode;
    hotwire_compatibility_mode_t compatibility_mode;
    uint32_t priority;
    uint32_t timeout_ms;
    bool enable_fallback;
    bool enable_caching;
    void* private_config;
} hotwire_route_config_t;

/**
 * @brief Security configuration per Node-Zero requirements
 */
typedef struct {
    bool enable_zero_trust;
    bool enable_audit_trail;
    bool enable_integrity_checks;
    char cert_path[HOTWIRE_CONFIG_MAX_PATH_LENGTH];
    char key_path[HOTWIRE_CONFIG_MAX_PATH_LENGTH];
    uint32_t auth_timeout_ms;
    hotwire_audit_level_t audit_level;
} hotwire_security_config_t;

/**
 * @brief Telemetry configuration for constitutional compliance
 */
typedef struct {
    bool enable_telemetry;
    bool enable_performance_metrics;
    bool enable_constitutional_audit;
    char telemetry_endpoint[HOTWIRE_CONFIG_MAX_PATH_LENGTH];
    uint32_t flush_interval_ms;
    uint32_t max_buffer_size;
} hotwire_telemetry_config_t;

/**
 * @brief Master hotwiring configuration structure
 */
typedef struct {
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    
    bool enable_hotwiring;
    bool enable_audit;
    bool enable_v1_compatibility;
    bool enable_constitutional_mode;
    
    hotwire_execution_mode_t default_execution_mode;
    hotwire_compatibility_mode_t default_compatibility_mode;
    
    size_t route_count;
    hotwire_route_config_t routes[HOTWIRE_CONFIG_MAX_PROTOCOLS];
    
    hotwire_security_config_t security;
    hotwire_telemetry_config_t telemetry;
    
    char polycallrc_path[HOTWIRE_CONFIG_MAX_PATH_LENGTH];
    char config_schema_version[32];
} hotwire_config_t;

/*---------------------------------------------------------------------------*/
/* Router Statistics and Monitoring */
/*---------------------------------------------------------------------------*/

/**
 * @brief Runtime statistics for hotwiring operations
 */
typedef struct {
    uint64_t total_routes;
    uint64_t total_executions;
    uint64_t successful_executions;
    uint64_t failed_executions;
    uint64_t v1_fallback_count;
    uint64_t audit_violations;
    uint32_t flags;
    uint32_t version;
    uint64_t uptime_ms;
} hotwire_router_stats_t;

/**
 * @brief Route performance metrics
 */
typedef struct {
    char route_name[HOTWIRE_CONFIG_MAX_NAME_LENGTH];
    uint64_t execution_count;
    uint64_t total_time_ms;
    uint64_t avg_time_ms;
    uint64_t min_time_ms;
    uint64_t max_time_ms;
    uint64_t error_count;
    uint64_t last_execution_timestamp;
} hotwire_route_metrics_t;

/*---------------------------------------------------------------------------*/
/* Configuration Management API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Load hotwiring configuration from YAML file
 * @param config_path Path to polycall.config.hotwire.yaml
 * @param config Output configuration structure
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_config_load_from_file(
    const char* config_path,
    hotwire_config_t* config
);

/**
 * @brief Load hotwiring configuration from polycallrc
 * @param polycallrc_path Path to .polycallrc file
 * @param config Output configuration structure
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_config_load_from_polycallrc(
    const char* polycallrc_path,
    hotwire_config_t* config
);

/**
 * @brief Validate configuration against constitutional constraints
 * @param config Configuration to validate
 * @return POLYCALL_CORE_SUCCESS if valid, error code otherwise
 */
polycall_core_error_t hotwire_config_validate(
    const hotwire_config_t* config
);

/**
 * @brief Apply configuration to active hotwiring router
 * @param config Configuration to apply
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_config_apply(
    const hotwire_config_t* config
);

/**
 * @brief Get default hotwiring configuration
 * @param config Output configuration with safe defaults
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_config_get_defaults(
    hotwire_config_t* config
);

/*---------------------------------------------------------------------------*/
/* Protocol Descriptor Management */
/*---------------------------------------------------------------------------*/

/**
 * @brief Protocol descriptor for constitutional compliance
 */
typedef struct {
    char protocol_name[HOTWIRE_CONFIG_MAX_NAME_LENGTH];
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    bool v1_compatible;
    bool requires_authentication;
    bool supports_caching;
    bool supports_fallback;
    char descriptor_checksum[64];
} polycall_protocol_descriptor_t;

/**
 * @brief Generate protocol descriptor for hotwiring route
 * @param route_config Route configuration
 * @param descriptor Output protocol descriptor
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_generate_protocol_descriptor(
    const hotwire_route_config_t* route_config,
    polycall_protocol_descriptor_t* descriptor
);

/**
 * @brief Validate protocol descriptor against known protocols
 * @param descriptor Protocol descriptor to validate
 * @return POLYCALL_CORE_SUCCESS if valid, error code otherwise
 */
polycall_core_error_t hotwire_validate_protocol_descriptor(
    const polycall_protocol_descriptor_t* descriptor
);

/*---------------------------------------------------------------------------*/
/* Constitutional Compliance Functions */
/*---------------------------------------------------------------------------*/

/**
 * @brief Verify OBIAxis governance compliance
 * @param config Configuration to check
 * @return POLYCALL_CORE_SUCCESS if compliant, error code otherwise
 */
polycall_core_error_t hotwire_verify_obiaxis_compliance(
    const hotwire_config_t* config
);

/**
 * @brief Check for constitutional violations in route configuration
 * @param route_config Route configuration to check
 * @return POLYCALL_CORE_SUCCESS if no violations, error code otherwise
 */
polycall_core_error_t hotwire_check_constitutional_violations(
    const hotwire_route_config_t* route_config
);

/**
 * @brief Trigger telemetry audit for constitutional compliance
 * @param audit_data Data to include in audit
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_trigger_constitutional_audit(
    const char* audit_data
);

/*---------------------------------------------------------------------------*/
/* Error Handling and Diagnostics */
/*---------------------------------------------------------------------------*/

/**
 * @brief Get human-readable error description
 * @param error_code Error code from hotwiring operation
 * @return Pointer to error description string
 */
const char* hotwire_get_error_description(
    polycall_core_error_t error_code
);

/**
 * @brief Check if hotwiring subsystem is healthy
 * @return true if healthy, false otherwise
 */
bool hotwire_is_healthy(void);

/**
 * @brief Get detailed diagnostic information
 * @param buffer Buffer to write diagnostic info
 * @param buffer_size Size of buffer
 * @return Number of bytes written to buffer
 */
size_t hotwire_get_diagnostics(
    char* buffer,
    size_t buffer_size
);

/*---------------------------------------------------------------------------*/
/* Configuration Schema Information */
/*---------------------------------------------------------------------------*/

/**
 * @brief Configuration schema version information
 */
#define HOTWIRE_CONFIG_SCHEMA_VERSION "2.0.0"
#define HOTWIRE_CONFIG_SCHEMA_MIN_VERSION "1.0.0"

/**
 * @brief Check if configuration schema version is supported
 * @param schema_version Schema version string
 * @return true if supported, false otherwise
 */
bool hotwire_is_schema_version_supported(
    const char* schema_version
);

/**
 * @brief Migrate configuration from older schema version
 * @param old_config Configuration in old format
 * @param old_version Old schema version
 * @param new_config Output configuration in current format
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_migrate_config_schema(
    const void* old_config,
    const char* old_version,
    hotwire_config_t* new_config
);

#ifdef __cplusplus
}
#endif

#endif /* HOTWIRE_CONFIG_BINDING_H */