/**
 * @file polycall_context.h
 * @brief Context management module for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This header defines the context management system for LibPolyCall,
 * providing unified state tracking and resource management for the
 * Program-First design approach. The context system serves as a central
 * repository for program state that can be referenced by any module.
 */
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef POLYCALL_POLYCALL_POLYCALL_CONTEXT_H_H
#define POLYCALL_POLYCALL_POLYCALL_CONTEXT_H_H

#ifdef __cplusplus
extern "C" {
#endif
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
typedef struct polycall_core_error polycall_core_error_t;
/**
 * @brief Context initialization function type
 */
typedef polycall_core_error_t (*polycall_context_init_fn)(
    polycall_core_context_t *core_ctx, void *ctx_data, void *init_data);
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
  POLYCALL_CONTEXT_TYPE_USER =
      0x1000 /**< Start of user-defined context types */
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

// Forward declarations
struct polycall_context_ref;
typedef struct polycall_context_ref polycall_context_ref_t;

// Maximum number of contexts and listeners per context
#define POLYCALL_POLYCALL_POLYCALL_CONTEXT_H_H
#define POLYCALL_POLYCALL_POLYCALL_CONTEXT_H_H
#define POLYCALL_POLYCALL_POLYCALL_CONTEXT_H_H
/**
 * @brief Context initialization function type
 */
typedef polycall_core_error_t (*polycall_context_init_fn)(
    polycall_core_context_t *core_ctx, void *ctx_data, void *init_data);

/**
 * @brief Context cleanup function type
 */
typedef void (*polycall_context_cleanup_fn)(polycall_core_context_t *core_ctx,
                                            void *ctx_data);

/**
 * @brief Context listener structure
 */
typedef struct {
  void (*listener)(polycall_context_ref_t *ctx_ref, void *user_data);
  void *user_data;
} context_listener_t;

/**
 * @brief Context registry structure
 */
typedef struct {
  polycall_context_ref_t *contexts[MAX_CONTEXTS];
  size_t context_count;
  pthread_mutex_t registry_lock;
} context_registry_t;

/**
 * @brief Context reference structure
 */
struct polycall_context_ref {
  polycall_context_type_t type;
  const char *name;
  polycall_context_flags_t flags;
  void *data;
  size_t data_size;
  polycall_context_init_fn init_fn;
  polycall_context_cleanup_fn cleanup_fn;
  pthread_mutex_t lock;
  context_listener_t listeners[MAX_LISTENERS];
  size_t listener_count;
};

/**
 * @brief Context initialization structure
 */
typedef struct {
  polycall_context_type_t type;           /**< Context type */
  size_t data_size;                       /**< Size of context data structure */
  polycall_context_flags_t flags;         /**< Context flags */
  const char *name;                       /**< Context name */
  polycall_context_init_fn init_fn;       /**< Initialization function */
  polycall_context_cleanup_fn cleanup_fn; /**< Cleanup function */
  void *init_data;                        /**< Initialization data */
} polycall_context_init_t;

/**
 * @brief Initialize a context
 *
 * @param core_ctx Core context
 * @param ctx_ref Pointer to receive context reference
 * @param init Initialization structure
 * @return Error code indicating success or failure
 */
polycall_core_error_t
polycall_context_init(polycall_core_context_t *core_ctx,
                      polycall_context_ref_t **ctx_ref,
                      const polycall_context_init_t *init);

/**
 * @brief Clean up and release resources associated with a context
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference to clean up
 */
void polycall_context_cleanup(polycall_core_context_t *core_ctx,
                              polycall_context_ref_t *ctx_ref);

/**
 * @brief Get context data
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Pointer to context data, or NULL on failure
 */
void *polycall_context_get_data(polycall_core_context_t *core_ctx,
                                polycall_context_ref_t *ctx_ref);

/**
 * @brief Find a context by type
 *
 * @param core_ctx Core context
 * @param type Context type to find
 * @return Context reference, or NULL if not found
 */
polycall_context_ref_t *
polycall_context_find_by_type(polycall_core_context_t *core_ctx,
                              polycall_context_type_t type);

/**
 * @brief Find a context by name
 *
 * @param core_ctx Core context
 * @param name Context name to find
 * @return Context reference, or NULL if not found
 */
polycall_context_ref_t *
polycall_context_find_by_name(polycall_core_context_t *core_ctx,
                              const char *name);

/**
 * @brief Get context type
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Context type
 */
polycall_context_type_t
polycall_context_get_type(polycall_core_context_t *core_ctx,
                          polycall_context_ref_t *ctx_ref);

/**
 * @brief Get context name
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Context name
 */
const char *polycall_context_get_name(polycall_core_context_t *core_ctx,
                                      polycall_context_ref_t *ctx_ref);

/**
 * @brief Get context flags
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Context flags
 */
polycall_context_flags_t
polycall_context_get_flags(polycall_core_context_t *core_ctx,
                           polycall_context_ref_t *ctx_ref);

/**
 * @brief Set context flags
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @param flags Flags to set
 * @return Error code indicating success or failure
 */
polycall_core_error_t
polycall_context_set_flags(polycall_core_context_t *core_ctx,
                           polycall_context_ref_t *ctx_ref,
                           polycall_context_flags_t flags);

/**
 * @brief Check if a context is initialized
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return true if initialized, false otherwise
 */
bool polycall_context_is_initialized(polycall_core_context_t *core_ctx,
                                     polycall_context_ref_t *ctx_ref);

/**
 * @brief Lock a context
 *
 * This prevents the context from being modified until unlocked.
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_context_lock(polycall_core_context_t *core_ctx,
                                            polycall_context_ref_t *ctx_ref);

/**
 * @brief Unlock a context
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_context_unlock(polycall_core_context_t *core_ctx,
                                              polycall_context_ref_t *ctx_ref);

/**
 * @brief Share a context with another component
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @param component Component to share with
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_context_share(polycall_core_context_t *core_ctx,
                                             polycall_context_ref_t *ctx_ref,
                                             const char *component);

/**
 * @brief Unshare a context
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Error code indicating success or failure
 */
polycall_core_error_t
polycall_context_unshare(polycall_core_context_t *core_ctx,
                         polycall_context_ref_t *ctx_ref);

/**
 * @brief Isolate a context
 *
 * This prevents the context from being shared or accessed by other components.
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @return Error code indicating success or failure
 */
polycall_core_error_t
polycall_context_isolate(polycall_core_context_t *core_ctx,
                         polycall_context_ref_t *ctx_ref);

/**
 * @brief Register a context listener
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @param listener Listener function
 * @param user_data User data to pass to listener
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_context_register_listener(
    polycall_core_context_t *core_ctx, polycall_context_ref_t *ctx_ref,
    void (*listener)(polycall_context_ref_t *ctx_ref, void *user_data),
    void *user_data);

/**
 * @brief Unregister a context listener
 *
 * @param core_ctx Core context
 * @param ctx_ref Context reference
 * @param listener Listener function
 * @param user_data User data passed to listener
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_context_unregister_listener(
    polycall_core_context_t *core_ctx, polycall_context_ref_t *ctx_ref,
    void (*listener)(polycall_context_ref_t *ctx_ref, void *user_data),
    void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_CONTEXT_H_H */
