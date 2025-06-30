/**
 * @file polycall_registry.h
 * @brief Service registry for polycall module
 */

#ifndef POLYCALL_POLYCALL_REGISTRY_H
#define POLYCALL_POLYCALL_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct polycall_service {
    char* name;
    void* service;
} polycall_service_t;

/**
 * Registry for polycall services
 */
typedef struct polycall_registry {
    polycall_service_t* services;
    int count;
    int capacity;
} polycall_registry_t;

/**
 * Create a new registry
 */
polycall_registry_t* polycall_registry_create();

/**
 * Destroy a registry
 */
void polycall_registry_destroy(polycall_registry_t* registry);

/**
 * Register a service with the registry
 */
int polycall_registry_register(polycall_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* polycall_registry_get(polycall_registry_t* registry, const char* name);

/**
 * Register default services
 */
int polycall_registry_register_defaults(polycall_registry_t* registry);

#endif /* POLYCALL_POLYCALL_REGISTRY_H */
