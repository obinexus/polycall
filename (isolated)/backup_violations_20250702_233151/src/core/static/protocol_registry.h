/**
 * @file protocol_registry.h
 * @brief Service registry for protocol module
 */

#ifndef POLYCALL_PROTOCOL_REGISTRY_H
#define POLYCALL_PROTOCOL_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct protocol_service {
    char* name;
    void* service;
} protocol_service_t;

/**
 * Registry for protocol services
 */
typedef struct protocol_registry {
    protocol_service_t* services;
    int count;
    int capacity;
} protocol_registry_t;

/**
 * Create a new registry
 */
protocol_registry_t* protocol_registry_create();

/**
 * Destroy a registry
 */
void protocol_registry_destroy(protocol_registry_t* registry);

/**
 * Register a service with the registry
 */
int protocol_registry_register(protocol_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* protocol_registry_get(protocol_registry_t* registry, const char* name);

/**
 * Register default services
 */
int protocol_registry_register_defaults(protocol_registry_t* registry);

#endif /* POLYCALL_PROTOCOL_REGISTRY_H */
