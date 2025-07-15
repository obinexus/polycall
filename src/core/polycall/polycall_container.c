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
 * @file polycall_container.c
 * @brief IoC Container implementation for polycall module
 */

#include "polycall/core/polycall/polycall_container.h"
#include "polycall/core/polycall/polycall_registry.h"

/* Container implementation */
polycall_container_t *polycall_container_init() {
  polycall_container_t *container = malloc(sizeof(polycall_container_t));
  if (!container) {
    return NULL;
  }

  container->registry = polycall_registry_create();
  if (!container->registry) {
    free(container);
    return NULL;
  }

  // Initialize default services
  polycall_registry_register_defaults(container->registry);

  return container;
}

void polycall_container_destroy(polycall_container_t *container) {
  if (container) {
    if (container->registry) {
      polycall_registry_destroy(container->registry);
    }
    free(container);
  }
}

void *polycall_container_get_service(polycall_container_t *container,
                                     const char *service_name) {
  if (!container || !container->registry || !service_name) {
    return NULL;
  }

  return polycall_registry_get(container->registry, service_name);
}

int polycall_container_register_service(polycall_container_t *container,
                                        const char *service_name,
                                        void *service) {
  if (!container || !container->registry || !service_name || !service) {
    return -1;
  }

  return polycall_registry_register(container->registry, service_name, service);
}
