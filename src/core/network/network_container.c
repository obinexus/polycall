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
 * @file network_container.c
 * @brief Container for network module
 */

#include "polycall/core/network/network_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <string.h>

/**
 * Initialize network container
 */
int network_container_init(polycall_core_context_t *core_ctx,
                           network_container_t **container) {
  if (!core_ctx || !container) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  network_container_t *c =
      (network_container_t *)malloc(sizeof(network_container_t));
  if (!c) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  memset(c, 0, sizeof(network_container_t));
  c->core_ctx = core_ctx;

  // Initialize module-specific data

  *container = c;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * Register network services
 */
int network_register_services(network_container_t *container) {
  if (!container) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  polycall_core_context_t *ctx = container->core_ctx;

  // Register services with core context
  polycall_register_service(ctx, "network_container", container);

  // Register additional module-specific services

  return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup network container
 */
void network_container_cleanup(network_container_t *container) {
  if (!container) {
    return;
  }

  // Free module-specific resources

  free(container);
}
