/**
 * @file auth_container.h
 * @brief Container for auth module
 */

#ifndef POLYCALL_AUTH_CONTAINER_H
#define POLYCALL_AUTH_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * auth container structure
 */
typedef struct auth_container {
  polycall_core_context_t *core_ctx;
  void *module_data;
  // Add component-specific fields here
} auth_container_t;

/**
 * Initialize auth container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int auth_container_init(polycall_core_context_t *core_ctx,
                        auth_container_t **container);

/**
 * Register auth services
 *
 * @param container auth container
 * @return int 0 on success, error code otherwise
 */
int auth_register_services(auth_container_t *container);

/**
 * Cleanup auth container
 *
 * @param container auth container
 */
void auth_container_cleanup(auth_container_t *container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_AUTH_CONTAINER_H */
