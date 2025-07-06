/**
 * @file telemetry_container.h
 * @brief Container for telemetry module
 */

#ifndef POLYCALL_TELEMETRY_CONTAINER_H
#define POLYCALL_TELEMETRY_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * telemetry container structure
 */
typedef struct telemetry_container {
  polycall_core_context_t *core_ctx;
  void *module_data;
  // Add component-specific fields here
} telemetry_container_t;

/**
 * Initialize telemetry container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int telemetry_container_init(polycall_core_context_t *core_ctx,
                             telemetry_container_t **container);

/**
 * Register telemetry services
 *
 * @param container telemetry container
 * @return int 0 on success, error code otherwise
 */
int telemetry_register_services(telemetry_container_t *container);

/**
 * Cleanup telemetry container
 *
 * @param container telemetry container
 */
void telemetry_container_cleanup(telemetry_container_t *container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_TELEMETRY_CONTAINER_H */
