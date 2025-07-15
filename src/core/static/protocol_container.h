/**
 * @file protocol_container.h
 * @brief Container for protocol module
 */

#ifndef POLYCALL_PROTOCOL_CONTAINER_H
#define POLYCALL_PROTOCOL_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * protocol container structure
 */
typedef struct protocol_container {
  polycall_core_context_t *core_ctx;
  void *module_data;
  // Add component-specific fields here
} protocol_container_t;

/**
 * Initialize protocol container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int protocol_container_init(polycall_core_context_t *core_ctx,
                            protocol_container_t **container);

/**
 * Register protocol services
 *
 * @param container protocol container
 * @return int 0 on success, error code otherwise
 */
int protocol_register_services(protocol_container_t *container);

/**
 * Cleanup protocol container
 *
 * @param container protocol container
 */
void protocol_container_cleanup(protocol_container_t *container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_PROTOCOL_CONTAINER_H */
