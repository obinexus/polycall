/**
 * @file edge_registry.h
 * @brief Service registry for edge module
 */

#ifndef POLYCALL_EDGE_REGISTRY_H
#define POLYCALL_EDGE_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct edge_service {
    char* name;
    void* service;
} edge_service_t;

/**
 * Registry for edge services
 */
typedef struct edge_registry {
    edge_service_t* services;
    int count;
    int capacity;
} edge_registry_t;

/**
 * Create a new registry
 */
edge_registry_t* edge_registry_create();

/**
 * Destroy a registry
 */
void edge_registry_destroy(edge_registry_t* registry);

/**
 * Register a service with the registry
 */
int edge_registry_register(edge_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* edge_registry_get(edge_registry_t* registry, const char* name);

/**
 * Register default services
 */
int edge_registry_register_defaults(edge_registry_t* registry);

#endif /* POLYCALL_EDGE_REGISTRY_H */
