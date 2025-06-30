/**
 * @file auth_registry.h
 * @brief Service registry for auth module
 */

#ifndef POLYCALL_AUTH_REGISTRY_H
#define POLYCALL_AUTH_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct auth_service {
    char* name;
    void* service;
} auth_service_t;

/**
 * Registry for auth services
 */
typedef struct auth_registry {
    auth_service_t* services;
    int count;
    int capacity;
} auth_registry_t;

/**
 * Create a new registry
 */
auth_registry_t* auth_registry_create();

/**
 * Destroy a registry
 */
void auth_registry_destroy(auth_registry_t* registry);

/**
 * Register a service with the registry
 */
int auth_registry_register(auth_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* auth_registry_get(auth_registry_t* registry, const char* name);

/**
 * Register default services
 */
int auth_registry_register_defaults(auth_registry_t* registry);

#endif /* POLYCALL_AUTH_REGISTRY_H */
