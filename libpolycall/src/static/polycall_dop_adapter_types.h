/**
 * @file polycall_dop_adapter_types.h
 * @brief DOP (Data-Oriented Programming) Adapter Type Definitions
 * 
 * LibPolyCall DOP Adapter Framework - Core Type System
 * OBINexus Computing - Aegis Project Technical Infrastructure
 * 
 * Defines canonical type system for universal cross-language micro-component
 * adapter framework with Zero Trust security enforcement.
 * 
 * @version 1.0.0
 * @date 2025-06-09
 */

#ifndef POLYCALL_DOP_ADAPTER_TYPES_H
#define POLYCALL_DOP_ADAPTER_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "polycall/core/polycall_core_types.h"
#include "polycall/core/ffi/polycall_ffi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * Forward Declarations
 * ==================================================================== */

typedef struct polycall_dop_adapter_context polycall_dop_adapter_context_t;
typedef struct polycall_dop_component polycall_dop_component_t;
typedef struct polycall_dop_bridge polycall_dop_bridge_t;
typedef struct polycall_dop_security_context polycall_dop_security_context_t;
typedef struct polycall_dop_memory_manager polycall_dop_memory_manager_t;

/* ====================================================================
 * Core Error Types
 * ==================================================================== */

/**
 * @brief DOP Adapter error codes following polycall_core_error_t pattern
 */
typedef enum {
    POLYCALL_DOP_SUCCESS = 0,                    ///< Operation succeeded
    POLYCALL_DOP_ERROR_INVALID_PARAMETER,       ///< Invalid input parameter
    POLYCALL_DOP_ERROR_INVALID_STATE,           ///< Invalid adapter state
    POLYCALL_DOP_ERROR_MEMORY_ALLOCATION,       ///< Memory allocation failed
    POLYCALL_DOP_ERROR_SECURITY_VIOLATION,      ///< Zero Trust security violation
    POLYCALL_DOP_ERROR_PERMISSION_DENIED,       ///< Insufficient permissions
    POLYCALL_DOP_ERROR_COMPONENT_NOT_FOUND,     ///< Component not registered
    POLYCALL_DOP_ERROR_BRIDGE_UNAVAILABLE,      ///< Language bridge not available
    POLYCALL_DOP_ERROR_ISOLATION_BREACH,        ///< Memory isolation violated
    POLYCALL_DOP_ERROR_INVOKE_FAILED,           ///< Component invocation failed
    POLYCALL_DOP_ERROR_LIFECYCLE_VIOLATION,     ///< Invalid lifecycle transition
    POLYCALL_DOP_ERROR_CONFIGURATION_INVALID,   ///< Invalid configuration
    POLYCALL_DOP_ERROR_TIMEOUT,                 ///< Operation timeout
    POLYCALL_DOP_ERROR_NOT_IMPLEMENTED,         ///< Feature not implemented
    POLYCALL_DOP_ERROR_UNKNOWN = 255            ///< Unknown error
} polycall_dop_error_t;

/* ====================================================================
 * Component Lifecycle States
 * ==================================================================== */

/**
 * @brief Component lifecycle states enforcing strict state transitions
 */
typedef enum {
    POLYCALL_DOP_COMPONENT_UNINITIALIZED = 0,   ///< Initial state
    POLYCALL_DOP_COMPONENT_INITIALIZING,        ///< Initialization in progress
    POLYCALL_DOP_COMPONENT_READY,               ///< Ready for invocation
    POLYCALL_DOP_COMPONENT_EXECUTING,           ///< Currently executing
    POLYCALL_DOP_COMPONENT_SUSPENDED,           ///< Temporarily suspended
    POLYCALL_DOP_COMPONENT_ERROR,               ///< Error state
    POLYCALL_DOP_COMPONENT_CLEANUP,             ///< Cleanup in progress
    POLYCALL_DOP_COMPONENT_DESTROYED            ///< Destroyed, no longer usable
} polycall_dop_component_state_t;

/* ====================================================================
 * Security and Isolation Types
 * ==================================================================== */

/**
 * @brief Component isolation levels for Zero Trust enforcement
 */
typedef enum {
    POLYCALL_DOP_ISOLATION_NONE = 0,            ///< No isolation (testing only)
    POLYCALL_DOP_ISOLATION_BASIC,               ///< Basic memory boundaries
    POLYCALL_DOP_ISOLATION_STANDARD,            ///< Standard security isolation
    POLYCALL_DOP_ISOLATION_STRICT,              ///< Strict Zero Trust isolation
    POLYCALL_DOP_ISOLATION_PARANOID             ///< Maximum security isolation
} polycall_dop_isolation_level_t;

/**
 * @brief Permission flags for component access control
 */
typedef enum {
    POLYCALL_DOP_PERMISSION_NONE        = 0x00,  ///< No permissions
    POLYCALL_DOP_PERMISSION_MEMORY_READ = 0x01,  ///< Read shared memory
    POLYCALL_DOP_PERMISSION_MEMORY_WRITE = 0x02, ///< Write shared memory
    POLYCALL_DOP_PERMISSION_INVOKE_LOCAL = 0x04, ///< Invoke local components
    POLYCALL_DOP_PERMISSION_INVOKE_REMOTE = 0x08,///< Invoke remote components
    POLYCALL_DOP_PERMISSION_FILE_ACCESS = 0x10,  ///< File system access
    POLYCALL_DOP_PERMISSION_NETWORK = 0x20,      ///< Network access
    POLYCALL_DOP_PERMISSION_PRIVILEGED = 0x40,   ///< Privileged operations
    POLYCALL_DOP_PERMISSION_ALL = 0xFF           ///< All permissions (dangerous)
} polycall_dop_permission_flags_t;

/**
 * @brief Security policy configuration
 */
typedef struct {
    polycall_dop_isolation_level_t isolation_level;
    polycall_dop_permission_flags_t allowed_permissions;
    polycall_dop_permission_flags_t denied_permissions;
    uint32_t max_memory_usage;                   ///< Maximum memory in bytes
    uint32_t max_execution_time_ms;              ///< Maximum execution time
    bool audit_enabled;                          ///< Enable audit logging
    bool stack_protection_enabled;               ///< Enable stack protection
    bool heap_protection_enabled;                ///< Enable heap protection
} polycall_dop_security_policy_t;

/* ====================================================================
 * Memory Management Types
 * ==================================================================== */

/**
 * @brief Memory region descriptor for boundary enforcement
 */
typedef struct {
    void* base_address;                          ///< Base memory address
    size_t size;                                 ///< Region size in bytes
    polycall_dop_permission_flags_t permissions; ///< Access permissions
    const char* owner_component_id;              ///< Owning component
    uint32_t reference_count;                    ///< Reference counter
    bool is_shared;                              ///< Shared across components
} polycall_dop_memory_region_t;

/**
 * @brief Memory allocation strategy
 */
typedef enum {
    POLYCALL_DOP_MEMORY_POOL,                   ///< Pool-based allocation
    POLYCALL_DOP_MEMORY_REGION,                 ///< Region-based allocation
    POLYCALL_DOP_MEMORY_GUARD,                  ///< Guarded allocation
    POLYCALL_DOP_MEMORY_ISOLATED                ///< Fully isolated allocation
} polycall_dop_memory_strategy_t;

/* ====================================================================
 * Language Bridge Types
 * ==================================================================== */

/**
 * @brief Supported language runtimes for bridge connections
 */
typedef enum {
    POLYCALL_DOP_LANGUAGE_C = 0,                ///< C/C++ native
    POLYCALL_DOP_LANGUAGE_JAVASCRIPT,           ///< JavaScript/Node.js
    POLYCALL_DOP_LANGUAGE_PYTHON,               ///< Python runtime
    POLYCALL_DOP_LANGUAGE_JVM,                  ///< JVM (Java, Kotlin, Scala)
    POLYCALL_DOP_LANGUAGE_WASM,                 ///< WebAssembly
    POLYCALL_DOP_LANGUAGE_UNKNOWN = 255         ///< Unknown/unsupported
} polycall_dop_language_t;

/**
 * @brief Component execution model
 */
typedef enum {
    POLYCALL_DOP_EXEC_SYNCHRONOUS = 0,          ///< Synchronous execution
    POLYCALL_DOP_EXEC_ASYNCHRONOUS,             ///< Asynchronous execution
    POLYCALL_DOP_EXEC_STREAMING,                ///< Streaming execution
    POLYCALL_DOP_EXEC_BATCH                     ///< Batch execution
} polycall_dop_execution_model_t;

/* ====================================================================
 * Component Value System
 * ==================================================================== */

/**
 * @brief Universal value types for cross-language data exchange
 */
typedef enum {
    POLYCALL_DOP_VALUE_NULL = 0,                ///< Null/undefined value
    POLYCALL_DOP_VALUE_BOOL,                    ///< Boolean value
    POLYCALL_DOP_VALUE_INT32,                   ///< 32-bit signed integer
    POLYCALL_DOP_VALUE_INT64,                   ///< 64-bit signed integer
    POLYCALL_DOP_VALUE_UINT32,                  ///< 32-bit unsigned integer
    POLYCALL_DOP_VALUE_UINT64,                  ///< 64-bit unsigned integer
    POLYCALL_DOP_VALUE_FLOAT32,                 ///< 32-bit floating point
    POLYCALL_DOP_VALUE_FLOAT64,                 ///< 64-bit floating point
    POLYCALL_DOP_VALUE_STRING,                  ///< UTF-8 string
    POLYCALL_DOP_VALUE_BYTES,                   ///< Binary data
    POLYCALL_DOP_VALUE_ARRAY,                   ///< Array of values
    POLYCALL_DOP_VALUE_OBJECT,                  ///< Key-value object
    POLYCALL_DOP_VALUE_FUNCTION,                ///< Function reference
    POLYCALL_DOP_VALUE_COMPONENT_REF             ///< Component reference
} polycall_dop_value_type_t;

/**
 * @brief Universal value container for cross-language data exchange
 */
typedef struct {
    polycall_dop_value_type_t type;
    union {
        bool bool_val;
        int32_t int32_val;
        int64_t int64_val;
        uint32_t uint32_val;
        uint64_t uint64_val;
        float float32_val;
        double float64_val;
        struct {
            char* data;
            size_t length;
        } string_val;
        struct {
            void* data;
            size_t size;
        } bytes_val;
        struct {
            struct polycall_dop_value* elements;
            size_t count;
        } array_val;
        struct {
            const char* component_id;
            void* handle;
        } component_ref_val;
    } value;
} polycall_dop_value_t;

/* ====================================================================
 * Component Interface Types
 * ==================================================================== */

/**
 * @brief Component method signature
 */
typedef struct {
    const char* method_name;                     ///< Method identifier
    polycall_dop_value_type_t* parameter_types;  ///< Parameter type array
    size_t parameter_count;                      ///< Number of parameters
    polycall_dop_value_type_t return_type;       ///< Return value type
    polycall_dop_permission_flags_t required_permissions; ///< Required permissions
    uint32_t max_execution_time_ms;              ///< Execution timeout
} polycall_dop_method_signature_t;

/**
 * @brief Component invocation parameters
 */
typedef struct {
    const char* method_name;                     ///< Method to invoke
    polycall_dop_value_t* parameters;            ///< Parameter values
    size_t parameter_count;                      ///< Number of parameters
    polycall_dop_execution_model_t execution_model; ///< Execution model
    uint32_t timeout_ms;                         ///< Execution timeout
    void* user_context;                          ///< User-defined context
} polycall_dop_invocation_t;

/**
 * @brief Component invocation result
 */
typedef struct {
    polycall_dop_error_t error_code;             ///< Operation result
    polycall_dop_value_t return_value;           ///< Return value
    uint32_t execution_time_ms;                  ///< Actual execution time
    size_t memory_used;                          ///< Memory consumed
    const char* error_message;                   ///< Error description
} polycall_dop_result_t;

/* ====================================================================
 * Component Configuration Types
 * ==================================================================== */

/**
 * @brief Component initialization configuration
 */
typedef struct {
    const char* component_id;                    ///< Unique component identifier
    const char* component_name;                  ///< Human-readable name
    const char* version;                         ///< Component version
    polycall_dop_language_t language;            ///< Runtime language
    polycall_dop_security_policy_t security_policy; ///< Security configuration
    polycall_dop_memory_strategy_t memory_strategy; ///< Memory allocation strategy
    polycall_dop_method_signature_t* methods;    ///< Available methods
    size_t method_count;                         ///< Number of methods
    void* language_specific_config;              ///< Language-specific config
    size_t config_size;                          ///< Config size in bytes
} polycall_dop_component_config_t;

/* ====================================================================
 * Callback Function Types
 * ==================================================================== */

/**
 * @brief Component initialization callback
 */
typedef polycall_dop_error_t (*polycall_dop_init_callback_t)(
    polycall_dop_component_t* component,
    const polycall_dop_component_config_t* config,
    void* user_data
);

/**
 * @brief Component method invocation callback
 */
typedef polycall_dop_error_t (*polycall_dop_invoke_callback_t)(
    polycall_dop_component_t* component,
    const polycall_dop_invocation_t* invocation,
    polycall_dop_result_t* result,
    void* user_data
);

/**
 * @brief Component cleanup callback
 */
typedef polycall_dop_error_t (*polycall_dop_cleanup_callback_t)(
    polycall_dop_component_t* component,
    void* user_data
);

/**
 * @brief Security validation callback
 */
typedef polycall_dop_error_t (*polycall_dop_security_callback_t)(
    polycall_dop_component_t* component,
    const polycall_dop_invocation_t* invocation,
    void* user_data
);

/* ====================================================================
 * Event and Audit Types
 * ==================================================================== */

/**
 * @brief DOP Adapter audit event types
 */
typedef enum {
    POLYCALL_DOP_AUDIT_COMPONENT_CREATED,       ///< Component created
    POLYCALL_DOP_AUDIT_COMPONENT_DESTROYED,     ///< Component destroyed
    POLYCALL_DOP_AUDIT_METHOD_INVOKED,          ///< Method invoked
    POLYCALL_DOP_AUDIT_SECURITY_VIOLATION,      ///< Security violation detected
    POLYCALL_DOP_AUDIT_MEMORY_ALLOCATED,        ///< Memory allocated
    POLYCALL_DOP_AUDIT_MEMORY_FREED,            ///< Memory freed
    POLYCALL_DOP_AUDIT_ISOLATION_BREACH,        ///< Isolation boundary breach
    POLYCALL_DOP_AUDIT_PERMISSION_DENIED        ///< Permission denied
} polycall_dop_audit_event_type_t;

/**
 * @brief Audit event structure
 */
typedef struct {
    polycall_dop_audit_event_type_t event_type;
    uint64_t timestamp_ns;                       ///< Nanosecond timestamp
    const char* component_id;                    ///< Component identifier
    const char* method_name;                     ///< Method name (if applicable)
    polycall_dop_error_t error_code;             ///< Associated error code
    const char* details;                         ///< Additional details
    void* context_data;                          ///< Context-specific data
    size_t context_size;                         ///< Context data size
} polycall_dop_audit_event_t;

/* ====================================================================
 * Constants and Limits
 * ==================================================================== */

#define POLYCALL_DOP_MAX_COMPONENT_ID_LENGTH    256
#define POLYCALL_DOP_MAX_COMPONENT_NAME_LENGTH  512
#define POLYCALL_DOP_MAX_METHOD_NAME_LENGTH     128
#define POLYCALL_DOP_MAX_VERSION_LENGTH         64
#define POLYCALL_DOP_MAX_ERROR_MESSAGE_LENGTH   1024
#define POLYCALL_DOP_MAX_PARAMETERS             64
#define POLYCALL_DOP_MAX_METHODS_PER_COMPONENT  256
#define POLYCALL_DOP_DEFAULT_TIMEOUT_MS         5000
#define POLYCALL_DOP_MAX_MEMORY_REGIONS         1024

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_DOP_ADAPTER_TYPES_H */