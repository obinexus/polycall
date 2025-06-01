/**
 * @file polycall_container.h
 * @brief IoC Container for polycall module
 */

#ifndef POLYCALL_POLYCALL_CONTAINER_H
#define POLYCALL_POLYCALL_CONTAINER_H

#include "polycall/core/polycall/polycall_registry.h"

/**
 * Container for polycall services
 */
typedef struct polycall_container {
    polycall_registry_t* registry;
} polycall_container_t;

/**
 * Initialize a new container
 */
polycall_container_t* polycall_container_init();

/**
 * Destroy a container
 */
void polycall_container_destroy(polycall_container_t* container);

/**
 * Get a service from the container
 */
void* polycall_container_get_service(polycall_container_t* container, const char* service_name);

/**
 * Register a service with the container
 */
int polycall_container_register_service(polycall_container_t* container, const char* service_name, void* service);

#endif /* POLYCALL_POLYCALL_CONTAINER_H */
