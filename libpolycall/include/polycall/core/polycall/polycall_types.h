/**
 * @file polycall_types.h
 * @brief Core Type Definitions for LibPolyCall Framework
 * @author OBINexus Computing - Aegis Project
 * 
 * Centralized type definitions to prevent header conflicts and ensure
 * consistent type usage across the entire polycall framework.
 * 
 * This header serves as the single source of truth for all core types,
 * eliminating duplicate typedefs and circular dependencies.
 */

#ifndef POLYCALL_CORE_TYPES_H
#define POLYCALL_CORE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ==================================================================
 * FORWARD DECLARATIONS - Prevents circular dependencies
 * ================================================================== */

/* Core context structures */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
typedef struct polycall_container_context polycall_container_context_t;

/* Error handling structures */
typedef struct polycall_core_error polycall_core_error_t;
typedef struct polycall_error_record polycall_error_record_t;
typedef struct polycall_error_context polycall_error_context_t;

/* Registry and IoC structures */
typedef struct polycall_registry polycall_registry_t;
typedef struct polycall_component polycall_component_t;
typedef struct polycall_service polycall_service_t;

/* Protocol and communication structures */
typedef struct polycall_protocol_context polycall_protocol_context_t;
typedef struct polycall_message polycall_message_t;
typedef struct polycall_packet polycall_packet_t;

/* Authentication and security structures */
typedef struct polycall_auth_context polycall_auth_context_t;
typedef struct polycall_security_context polycall_security_context_t;
typedef struct polycall_token polycall_token_t;

/* Network and communication structures */
typedef struct polycall_network_context polycall_network_context_t;
typedef struct polycall_endpoint polycall_endpoint_t;
typedef struct polycall_connection polycall_connection_t;

/* FFI and language bridge structures */
typedef struct polycall_ffi_context polycall_ffi_context_t;
typedef struct polycall_bridge polycall_bridge_t;
typedef struct polycall_language_runtime polycall_language_runtime_t;

/* ==================================================================
 * ENUMERATION TYPES - Single source of truth
 * ================================================================== */

/**
 * @brief Logging severity levels for the polycall framework
 * 
 * Centralized definition to prevent conflicts between logger and core modules.
 */
typedef enum polycall_log_level {
    POLYCALL_LOG_TRACE = 0,     /**< Detailed trace information */
    POLYCALL_LOG_DEBUG = 1,     /**< Debug information */
    POLYCALL_LOG_INFO = 2,      /**< General information */
    POLYCALL_LOG_WARNING = 3,   /**< Warning conditions */
    POLYCALL_LOG_ERROR = 4,     /**< Error conditions */
    POLYCALL_LOG_FATAL = 5,     /**< Fatal error conditions */
    POLYCALL_LOG_OFF = 6        /**< Logging disabled */
} polycall_log_level_t;

/**
 * @brief Error severity classifications
 */
typedef enum polycall_error_severity {
    POLYCALL_ERROR_TRACE = 0,
    POLYCALL_ERROR_INFO = 1,
    POLYCALL_ERROR_WARNING = 2,
    POLYCALL_ERROR_ERROR = 3,
    POLYCALL_ERROR_FATAL = 4
} polycall_error_severity_t;

/**
 * @brief Component lifecycle states
 */
typedef enum polycall_component_state {
    POLYCALL_COMPONENT_UNINITIALIZED = 0,
    POLYCALL_COMPONENT_INITIALIZING = 1,
    POLYCALL_COMPONENT_READY = 2,
    POLYCALL_COMPONENT_RUNNING = 3,
    POLYCALL_COMPONENT_STOPPING = 4,
    POLYCALL_COMPONENT_STOPPED = 5,
    POLYCALL_COMPONENT_ERROR = 6
} polycall_component_state_t;

/**
 * @brief Protocol message types
 */
typedef enum polycall_message_type {
    POLYCALL_MSG_REQUEST = 0,
    POLYCALL_MSG_RESPONSE = 1,
    POLYCALL_MSG_NOTIFICATION = 2,
    POLYCALL_MSG_ERROR = 3,
    POLYCALL_MSG_HEARTBEAT = 4
} polycall_message_type_t;

/**
 * @brief Authentication status codes
 */
typedef enum polycall_auth_status {
    POLYCALL_AUTH_UNKNOWN = 0,
    POLYCALL_AUTH_PENDING = 1,
    POLYCALL_AUTH_AUTHENTICATED = 2,
    POLYCALL_AUTH_DENIED = 3,
    POLYCALL_AUTH_EXPIRED = 4,
    POLYCALL_AUTH_REVOKED = 5
} polycall_auth_status_t;

/* ==================================================================
 * PRIMITIVE TYPE ALIASES - Consistent sizing across platforms
 * ================================================================== */

/* Core identifier types */
typedef uint64_t polycall_id_t;
typedef uint32_t polycall_handle_t;
typedef uint16_t polycall_port_t;

/* Memory and size types */
typedef size_t polycall_size_t;
typedef ptrdiff_t polycall_offset_t;

/* Time and duration types */
typedef uint64_t polycall_timestamp_t;
typedef uint32_t polycall_duration_t;

/* Status and result types */
typedef int32_t polycall_status_t;
typedef int32_t polycall_result_t;

/* ==================================================================
 * FUNCTION POINTER TYPES - Callback and handler definitions
 * ================================================================== */

/* Generic callback function types */
typedef void (*polycall_callback_t)(void* context, void* data);
typedef polycall_result_t (*polycall_handler_t)(void* context, void* input, void* output);

/* Error handling callbacks */
typedef void (*polycall_error_callback_t)(polycall_error_context_t* error_ctx, const polycall_error_record_t* error);

/* Logging callbacks */
typedef void (*polycall_log_callback_t)(polycall_log_level_t level, const char* module, const char* message);

/* Memory management callbacks */
typedef void* (*polycall_malloc_t)(size_t size);
typedef void (*polycall_free_t)(void* ptr);
typedef void* (*polycall_realloc_t)(void* ptr, size_t new_size);

/* ==================================================================
 * CONSTANTS AND LIMITS
 * ================================================================== */

/* String length limits */
#define POLYCALL_MAX_MODULE_NAME_LEN    64
#define POLYCALL_MAX_ERROR_MESSAGE_LEN  256
#define POLYCALL_MAX_LOG_MESSAGE_LEN    512
#define POLYCALL_MAX_CONFIG_KEY_LEN     128
#define POLYCALL_MAX_CONFIG_VALUE_LEN   256

/* Component limits */
#define POLYCALL_MAX_COMPONENTS         256
#define POLYCALL_MAX_SERVICES           512
#define POLYCALL_MAX_CONNECTIONS        1024

/* Protocol limits */
#define POLYCALL_MAX_MESSAGE_SIZE       (1024 * 1024)  /* 1MB */
#define POLYCALL_MAX_PACKET_SIZE        (64 * 1024)    /* 64KB */

/* Status codes */
#define POLYCALL_SUCCESS                0
#define POLYCALL_ERROR_GENERIC         -1
#define POLYCALL_ERROR_INVALID_PARAM   -2
#define POLYCALL_ERROR_OUT_OF_MEMORY   -3
#define POLYCALL_ERROR_NOT_FOUND       -4
#define POLYCALL_ERROR_TIMEOUT         -5
#define POLYCALL_ERROR_PERMISSION      -6
#define POLYCALL_ERROR_NETWORK         -7
#define POLYCALL_ERROR_AUTH            -8

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CORE_TYPES_H */