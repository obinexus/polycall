/**
 * @file accessibility_registry.h
 * @brief Service registry for accessibility module
 */

#ifndef POLYCALL_ACCESSIBILITY_REGISTRY_H
#define POLYCALL_ACCESSIBILITY_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct accessibility_service {
    char* name;
    void* service;
} accessibility_service_t;

/**
 * Registry for accessibility services
 */
typedef struct accessibility_registry {
    accessibility_service_t* services;
    int count;
    int capacity;
} accessibility_registry_t;

/**
 * Create a new registry
 */
accessibility_registry_t* accessibility_registry_create();

/**
 * Destroy a registry
 */
void accessibility_registry_destroy(accessibility_registry_t* registry);

/**
 * Register a service with the registry
 */
int accessibility_registry_register(accessibility_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* accessibility_registry_get(accessibility_registry_t* registry, const char* name);

/**
 * Register default services
 */
int accessibility_registry_register_defaults(accessibility_registry_t* registry);

#endif /* POLYCALL_ACCESSIBILITY_REGISTRY_H */
