/**
 * @file polycall_core.h
 * @brief Core module for LibPolyCall implementing the Program-First approach
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#ifndef POLYCALL_POLYCALL_POLYCALL_CORE_H_H
#define POLYCALL_POLYCALL_POLYCALL_CORE_H_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Forward declarations */
struct polycall_core_context;
typedef struct polycall_core_context polycall_core_context_t;
typedef enum polycall_core_error polycall_core_error_t;
typedef enum polycall_log_level polycall_log_level_t;

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_logger.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Configuration flags for the core module
  */
 typedef enum {
     POLYCALL_CORE_FLAG_NONE = 0,
     POLYCALL_CORE_FLAG_STRICT_MODE = (1 << 0),
     POLYCALL_CORE_FLAG_DEBUG_MODE = (1 << 1),
     POLYCALL_CORE_FLAG_SECURE_MODE = (1 << 2),
     POLYCALL_CORE_FLAG_ASYNC_OPERATIONS = (1 << 3)
 } polycall_core_flags_t;
 
 /**
  * @brief Core configuration structure
  */
 typedef struct {
     polycall_core_flags_t flags;        /**< Configuration flags */
     size_t memory_pool_size;            /**< Size of memory pool in bytes */
     void* user_data;                    /**< User-defined data */
     void (*error_callback)(             /**< Error handling callback */
         polycall_core_error_t error,
         const char* message,
         void* user_data);
 } polycall_core_config_t;
 
 /**
  * @brief Memory allocation function type
  */
 typedef void* (*polycall_core_malloc_fn)(size_t size, void* user_data);
 
 /**
  * @brief Memory free function type
  */
 typedef void (*polycall_core_free_fn)(void* ptr, void* user_data);
 
 /**
  * @brief Initialize the core module
  *
  * @param ctx Pointer to receive the created context
  * @param config Pointer to configuration structure
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_core_init(
     polycall_core_context_t** ctx,
     const polycall_core_config_t* config);
 
 /**
  * @brief Clean up and release resources associated with a core context
  *
  * @param ctx Context to clean up
  */
 void polycall_core_cleanup(polycall_core_context_t* ctx);
 
 /**
  * @brief Set custom memory allocation functions
  *
  * @param ctx Core context
  * @param malloc_fn Custom malloc function
  * @param free_fn Custom free function
  * @param user_data User data to pass to allocation functions
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_core_set_memory_functions(
     polycall_core_context_t* ctx,
     polycall_core_malloc_fn malloc_fn,
     polycall_core_free_fn free_fn,
     void* user_data);
 
 /**
  * @brief Allocate memory from the core context
  *
  * @param ctx Core context
  * @param size Size of memory to allocate
  * @return Pointer to allocated memory, or NULL on failure
  */
void* polycall_core_malloc(polycall_core_context_t* ctx, size_t size);
 
 /**
  * @brief Free memory allocated by polycall_core_malloc
  *
  * @param ctx Core context
  * @param ptr Pointer to memory to free
  */
 void polycall_core_free(polycall_core_context_t* ctx, void* ptr);
 
 /**
  * @brief Set an error in the core context
  *
  * @param ctx Core context
  * @param error Error code
  * @param message Error message
  * @return Error code passed in
  */
 polycall_core_error_t polycall_core_set_error(
     struct polycall_core_context* ctx,
     polycall_core_error_t error,
     const char* message);
 
 /**
  * @brief Get the last error from the core context
  *
  * @param ctx Core context
  * @param message Pointer to receive error message (can be NULL)
  * @return Last error code
  */
 polycall_core_error_t polycall_core_get_last_error(
     struct polycall_core_context* ctx,
     const char** message);
 
 /**
  * @brief Get the core context version
  *
  * @return Version string
  */
 const char* polycall_core_get_version(void);
 
 /**
  * @brief Get user data from core context
  *
  * @param ctx Core context
  * @return User data pointer
  */
 void* polycall_core_get_user_data(polycall_core_context_t* ctx);
 
 /**
  * @brief Set user data in core context
  *
  * @param ctx Core context
  * @param user_data User data pointer
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_core_set_user_data(struct polycall_core_context* ctx,
     void* user_data);
 
 /**
  * @brief Log a message using the core logging system
  *
  * @param ctx Core context
  * @param level Log level
  * @param format Format string
  * @param ... Additional arguments for the format string
  */
 void polycall_core_log(
     polycall_core_context_t* ctx,
     polycall_log_level_t level,
     const char* format,
     ...);
 
 /**
  * @brief Convenience macro for logging
  */
 #define POLYCALL_POLYCALL_POLYCALL_CORE_H_H
     polycall_core_log(ctx, level, format, ##__VA_ARGS__)
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_POLYCALL_POLYCALL_CORE_H_H */
