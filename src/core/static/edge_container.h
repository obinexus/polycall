/**
 * @file edge_container.h
 * @brief Container for edge module
 */

#ifndef POLYCALL_EDGE_CONTAINER_H
#define POLYCALL_EDGE_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * edge container structure
 */
typedef struct edge_container {
  polycall_core_context_t *core_ctx;
  void *module_data;
  // Add component-specific fields here
} edge_container_t;

/**
 * Initialize edge container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int edge_container_init(polycall_core_context_t *core_ctx,
                        edge_container_t **container);

/**
 * Register edge services
 *
 * @param container edge container
 * @return int 0 on success, error code otherwise
 */
int edge_register_services(edge_container_t *container);

/**
 * Cleanup edge container
 *
 * @param container edge container
 */
void edge_container_cleanup(edge_container_t *container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_EDGE_CONTAINER_H */
