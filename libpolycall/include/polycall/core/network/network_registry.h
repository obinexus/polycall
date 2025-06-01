/**
 * @file network_registry.h
 * @brief Service registry for network module
 */

#ifndef POLYCALL_NETWORK_REGISTRY_H
#define POLYCALL_NETWORK_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct network_service {
    char* name;
    void* service;
} network_service_t;

/**
 * Registry for network services
 */
typedef struct network_registry {
    network_service_t* services;
    int count;
    int capacity;
} network_registry_t;

/**
 * Create a new registry
 */
network_registry_t* network_registry_create();

/**
 * Destroy a registry
 */
void network_registry_destroy(network_registry_t* registry);

/**
 * Register a service with the registry
 */
int network_registry_register(network_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* network_registry_get(network_registry_t* registry, const char* name);

/**
 * Register default services
 */
int network_registry_register_defaults(network_registry_t* registry);

#endif /* POLYCALL_NETWORK_REGISTRY_H */
