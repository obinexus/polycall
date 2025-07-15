/**
 * @file polycall_core_types.h
 * @brief Core Type Definitions - Forward Declaration Header
 * @author OBINexus Computing - LibPolyCall Framework
 *
 * This header provides forward declarations and fundamental types to break
 * circular dependencies in the libpolycall header architecture.
 *
 * ARCHITECTURAL PRINCIPLE:
 * - This header is included FIRST by all other headers
 * - Contains only forward declarations and primitive types
 * - NO function prototypes or complex macros
 * - Establishes type hierarchy foundation
 */

#ifndef POLYCALL_CORE_TYPES_H
#define POLYCALL_CORE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =================================================================
 * Forward Declarations - Core Types (Order Matters)
 * ================================================================= */

/* Core Context Forward Declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
typedef struct polycall_context_ref polycall_context_ref_t;

/* Error Handling Forward Declarations */
typedef struct polycall_error_record polycall_error_record_t;

/* Memory Management Forward Declarations */
typedef struct polycall_memory_pool polycall_memory_pool_t;
typedef struct polycall_memory_region polycall_memory_region_t;

/* =================================================================
 * Enumeration Definitions - Fundamental Types
 * ================================================================= */

/**
 * @brief Core Error Codes
 *
 * Primary error enumeration used throughout the system.
 * This replaces conflicting definitions in polycall_error.h
 */
typedef enum polycall_core_error {
  POLYCALL_CORE_SUCCESS = 0,
  POLYCALL_CORE_ERROR_INVALID_PARAMETER,
  POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
  POLYCALL_CORE_ERROR_NOT_INITIALIZED,
  POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
  POLYCALL_CORE_ERROR_RESOURCE_EXHAUSTED,
  POLYCALL_CORE_ERROR_PERMISSION_DENIED,
  POLYCALL_CORE_ERROR_TIMEOUT,
  POLYCALL_CORE_ERROR_UNKNOWN = 999
} polycall_core_error_t;

/**
 * @brief Log Level Enumeration
 *
 * Unified logging levels to resolve conflicts between polycall_logger.h
 * and polycall_core.h definitions.
 */
typedef enum polycall_log_level {
  POLYCALL_LOG_LEVEL_TRACE = 0,
  POLYCALL_LOG_LEVEL_DEBUG,
  POLYCALL_LOG_LEVEL_INFO,
  POLYCALL_LOG_LEVEL_WARNING,
  POLYCALL_LOG_LEVEL_ERROR,
  POLYCALL_LOG_LEVEL_FATAL,
  POLYCALL_LOG_LEVEL_OFF
} polycall_log_level_t;

/**
 * @brief Error Severity Levels
 */
typedef enum polycall_error_severity {
  POLYCALL_ERROR_SEVERITY_INFO = 0,
  POLYCALL_ERROR_SEVERITY_WARNING,
  POLYCALL_ERROR_SEVERITY_ERROR,
  POLYCALL_ERROR_SEVERITY_CRITICAL,
  POLYCALL_ERROR_SEVERITY_FATAL
} polycall_error_severity_t;

/**
 * @brief Context Type Identifiers
 */
typedef enum polycall_context_type {
  POLYCALL_CONTEXT_TYPE_CORE = 0,
  POLYCALL_CONTEXT_TYPE_CONFIG,
  POLYCALL_CONTEXT_TYPE_ACCESSIBILITY,
  POLYCALL_CONTEXT_TYPE_NETWORK,
  POLYCALL_CONTEXT_TYPE_SECURITY,
  POLYCALL_CONTEXT_TYPE_TELEMETRY,
  POLYCALL_CONTEXT_TYPE_PROTOCOL,
  POLYCALL_CONTEXT_TYPE_CUSTOM = 1000
} polycall_context_type_t;

/**
 * @brief Context Flags
 */
typedef enum polycall_context_flags {
  POLYCALL_CONTEXT_FLAG_NONE = 0x00,
  POLYCALL_CONTEXT_FLAG_THREAD_SAFE = 0x01,
  POLYCALL_CONTEXT_FLAG_PERSISTENT = 0x02,
  POLYCALL_CONTEXT_FLAG_SHARED = 0x04,
  POLYCALL_CONTEXT_FLAG_ISOLATED = 0x08,
  POLYCALL_CONTEXT_FLAG_DEBUG = 0x10
} polycall_context_flags_t;

/**
 * @brief Memory Allocation Flags
 */
typedef enum polycall_memory_flags {
  POLYCALL_MEMORY_FLAG_NONE = 0x00,
  POLYCALL_MEMORY_FLAG_ZERO_INIT = 0x01,
  POLYCALL_MEMORY_FLAG_ALIGNED = 0x02,
  POLYCALL_MEMORY_FLAG_PINNED = 0x04,
  POLYCALL_MEMORY_FLAG_SECURE = 0x08,
  POLYCALL_MEMORY_FLAG_TEMP = 0x10
} polycall_memory_flags_t;

/* =================================================================
 * Core Configuration Constants
 * ================================================================= */

#define POLYCALL_MAX_CONTEXTS 256
#define POLYCALL_MAX_MESSAGE_SIZE (16 * 1024 * 1024) /* 16MB */
#define POLYCALL_MAX_CONNECTIONS 1000
#define POLYCALL_DEFAULT_TIMEOUT_MS 5000
#define POLYCALL_MAX_ERROR_MESSAGE 1024
#define POLYCALL_MAX_CONTEXT_NAME 128

/* =================================================================
 * Function Pointer Types
 * ================================================================= */

/**
 * @brief Generic callback function type
 */
typedef void (*polycall_callback_fn)(void *user_data);

/**
 * @brief Error callback function type
 */
typedef void (*polycall_error_callback_fn)(polycall_core_error_t error_code,
                                           const char *error_message,
                                           void *user_data);

/**
 * @brief Context initialization function type
 */
typedef polycall_core_error_t (*polycall_context_init_fn)(
    polycall_core_context_t *core_ctx, void *config_data, void **context_out);

/**
 * @brief Context cleanup function type
 */
typedef void (*polycall_context_cleanup_fn)(void *context);

/* =================================================================
 * Macro Utilities - Safe Macro Definitions
 * ================================================================= */

/**
 * @brief Safe variadic macro for logging
 *
 * This replaces the problematic ## __VA_ARGS__ construct that was
 * causing compilation errors in polycall_logger.h
 */
#if defined(__GNUC__) && __GNUC__ >= 4
#define POLYCALL_VARIADIC_MACRO_SUPPORT 1
#define POLYCALL_LOG_IMPL(logger, level, file, line, format, ...)              \
  polycall_logger_log_impl(logger, level, file, line, format, ##__VA_ARGS__)
#else
#define POLYCALL_VARIADIC_MACRO_SUPPORT 0
#define POLYCALL_LOG_IMPL(logger, level, file, line, format, ...)              \
  polycall_logger_log_impl(logger, level, file, line, format, __VA_ARGS__)
#endif

/**
 * @brief Compiler attribute macros
 */
#ifdef __GNUC__
#define POLYCALL_ATTR_UNUSED __attribute__((unused))
#define POLYCALL_ATTR_PURE __attribute__((pure))
#define POLYCALL_ATTR_CONST __attribute__((const))
#define POLYCALL_ATTR_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
#define POLYCALL_ATTR_UNUSED
#define POLYCALL_ATTR_PURE
#define POLYCALL_ATTR_CONST
#define POLYCALL_ATTR_NONNULL(...)
#endif

/**
 * @brief Thread safety macros
 */
#ifdef _WIN32
#define POLYCALL_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
#define POLYCALL_THREAD_LOCAL __thread
#else
#define POLYCALL_THREAD_LOCAL
#endif

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CORE_TYPES_H */