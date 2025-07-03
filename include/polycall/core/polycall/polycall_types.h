/**
 * @file polycall_types.h
 * @brief Basic type definitions for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines fundamental types used throughout the LibPolyCall
 * system, establishing a foundation for type safety and preventing
 * circular dependencies between modules.
 */

 #ifndef POLYCALL_TYPES_H
 #define POLYCALL_TYPES_H
 
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 #include <limits.h>
#include <stdio.h>
 #include <stdlib.h>
    #include <string.h>
#include "polycall_config.h"
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* Forward declarations of core structures */
 typedef struct polycall_core_context polycall_core_context_t;
 typedef struct polycall_context polycall_context_t;
 typedef struct polycall_config_context polycall_config_context_t;
 typedef struct polycall_protocol_context polycall_protocol_context_t;
 typedef struct polycall_program_graph polycall_program_graph_t;
 typedef struct polycall_program_node polycall_program_node_t;
 typedef struct polycall_state_machine polycall_state_machine_t;
typedef struct NetworkEndpoint NetworkEndpoint;
typedef struct polycall_audit_event polycall_audit_event_t;

typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
/* Logging levels */
typedef enum {
    POLYCALL_LOG_DEBUG = 0,
    POLYCALL_LOG_INFO,
    POLYCALL_LOG_WARNING,
    POLYCALL_LOG_ERROR,
    POLYCALL_LOG_FATAL
} polycall_log_level_t;

 
 /**
  * @brief Core error codes
  */
 typedef enum {
     /* Success code */
     POLYCALL_CORE_SUCCESS = 0,
     
     /* Generic errors */
     POLYCALL_CORE_ERROR_NONE = 0,
     POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
     POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
     POLYCALL_CORE_ERROR_INVALID_OPERATION,
     POLYCALL_CORE_ERROR_INVALID_STATE,
     POLYCALL_CORE_ERROR_INVALID_HANDLE,
     POLYCALL_CORE_ERROR_INVALID_TYPE,
     POLYCALL_CORE_ERROR_INVALID_TOKEN,
     POLYCALL_CORE_ERROR_INVALID_CONTEXT,
     POLYCALL_CORE_ERROR_ACCESS_DENIED,
     
     /* Resource errors */
     POLYCALL_CORE_ERROR_NOT_FOUND,
     POLYCALL_CORE_ERROR_UNAVAILABLE,
     POLYCALL_CORE_ERROR_UNAUTHORIZED,
     POLYCALL_CORE_ERROR_TIMEOUT,
     
     /* Operational errors */
     POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
     POLYCALL_CORE_ERROR_NOT_INITIALIZED,
     POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
     POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
     POLYCALL_CORE_ERROR_NOT_SUPPORTED,
     POLYCALL_CORE_ERROR_CANCELED,
     
     /* I/O errors */
     POLYCALL_CORE_ERROR_IO_ERROR,
     POLYCALL_CORE_ERROR_NETWORK,
     POLYCALL_CORE_ERROR_PROTOCOL,
     POLYCALL_CORE_ERROR_FILE_NOT_FOUND,
     POLYCALL_CORE_ERROR_FILE_OPERATION_FAILED,
     
     /* Data errors */
     POLYCALL_CORE_ERROR_BUFFER_UNDERFLOW,
     POLYCALL_CORE_ERROR_BUFFER_OVERFLOW,
     POLYCALL_CORE_ERROR_TYPE_MISMATCH,
     POLYCALL_CORE_ERROR_VALIDATION_FAILED,
     
     /* System errors */
     POLYCALL_CORE_ERROR_SECURITY,
     POLYCALL_CORE_ERROR_INTERNAL
 } polycall_core_error_t;
 
 /**
  * @brief Public API error codes
  */
 typedef enum {
     POLYCALL_OK = 0,
     POLYCALL_ERROR_INVALID_PARAMETERS,
     POLYCALL_ERROR_INITIALIZATION,
     POLYCALL_ERROR_OUT_OF_MEMORY,
     POLYCALL_ERROR_UNSUPPORTED,
     POLYCALL_ERROR_INVALID_STATE,
     POLYCALL_ERROR_NOT_INITIALIZED,
     POLYCALL_ERROR_ALREADY_INITIALIZED,
     POLYCALL_ERROR_INTERNAL
 } polycall_error_t;
 
 /**
  * @brief Context types
  */
 typedef enum {
     POLYCALL_CONTEXT_TYPE_CORE = 0,
     POLYCALL_CONTEXT_TYPE_PROTOCOL,
     POLYCALL_CONTEXT_TYPE_NETWORK,
     POLYCALL_CONTEXT_TYPE_MICRO,
     POLYCALL_CONTEXT_TYPE_EDGE,
     POLYCALL_CONTEXT_TYPE_PARSER,
     POLYCALL_CONTEXT_TYPE_USER = 0x1000   /**< Start of user-defined context types */
 } polycall_context_type_t;
 
 /**
  * @brief Context flags
  */
 typedef enum {
     POLYCALL_CONTEXT_FLAG_NONE = 0,
     POLYCALL_CONTEXT_FLAG_INITIALIZED = (1 << 0),
     POLYCALL_CONTEXT_FLAG_LOCKED = (1 << 1),
     POLYCALL_CONTEXT_FLAG_SHARED = (1 << 2),
     POLYCALL_CONTEXT_FLAG_RESTRICTED = (1 << 3),
     POLYCALL_CONTEXT_FLAG_ISOLATED = (1 << 4)
 } polycall_context_flags_t;
 
 /**
  * @brief Configuration section types
  */
 typedef enum {
     POLYCALL_CONFIG_SECTION_CORE = 0,    /**< Core configuration */
     POLYCALL_CONFIG_SECTION_SECURITY,    /**< Security configuration */
     POLYCALL_CONFIG_SECTION_MEMORY,      /**< Memory management configuration */
     POLYCALL_CONFIG_SECTION_TYPE,        /**< Type system configuration */
     POLYCALL_CONFIG_SECTION_PERFORMANCE, /**< Performance configuration */
     POLYCALL_CONFIG_SECTION_PROTOCOL,    /**< Protocol bridge configuration */
     POLYCALL_CONFIG_SECTION_C,           /**< C bridge configuration */
     POLYCALL_CONFIG_SECTION_JVM,         /**< JVM bridge configuration */
     POLYCALL_CONFIG_SECTION_JS,          /**< JavaScript bridge configuration */
     POLYCALL_CONFIG_SECTION_PYTHON,      /**< Python bridge configuration */
     POLYCALL_CONFIG_SECTION_USER = 0x1000 /**< Start of user-defined sections */
 } polycall_config_section_t;
 
 /**
  * @brief Configuration value types
  */
 typedef enum {
     POLYCALL_CONFIG_VALUE_BOOLEAN = 0, /**< Boolean value */
     POLYCALL_CONFIG_VALUE_INTEGER,     /**< Integer value */
     POLYCALL_CONFIG_VALUE_FLOAT,       /**< Floating-point value */
     POLYCALL_CONFIG_VALUE_STRING,      /**< String value */
     POLYCALL_CONFIG_VALUE_OBJECT       /**< Complex object value */
 } polycall_config_value_type_t;
 
 /**
  * @brief Configuration change handler function type
  */
 typedef void (*polycall_config_change_handler_t)(
     polycall_core_context_t* ctx,
     polycall_config_section_t section_id,
     const char* key,
     const struct polycall_config_value* old_value,
     const struct polycall_config_value* new_value,
     void* user_data
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_TYPES_H */