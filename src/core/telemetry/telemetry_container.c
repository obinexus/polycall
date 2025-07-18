/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file telemetry_container.c
 * @brief Container for telemetry module
 */

#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/telemetry/telemetry_container.h"
#include <string.h>

/**
 * Initialize telemetry container
 */
int telemetry_container_init(polycall_core_context_t *core_ctx,
                             telemetry_container_t **container) {
  if (!core_ctx || !container) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  telemetry_container_t *c =
      (telemetry_container_t *)malloc(sizeof(telemetry_container_t));
  if (!c) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  memset(c, 0, sizeof(telemetry_container_t));
  c->core_ctx = core_ctx;

  // Initialize module-specific data

  *container = c;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * Register telemetry services
 */
int telemetry_register_services(telemetry_container_t *container) {
  if (!container) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  polycall_core_context_t *ctx = container->core_ctx;

  // Register services with core context
  polycall_register_service(ctx, "telemetry_container", container);

  // Register additional module-specific services

  return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup telemetry container
 */
void telemetry_container_cleanup(telemetry_container_t *container) {
  if (!container) {
    return;
  }

  // Free module-specific resources

  free(container);
}
