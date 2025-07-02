/**
 * @file telemetry_registry.h
 * @brief Service registry for telemetry module
 */

#ifndef POLYCALL_TELEMETRY_REGISTRY_H
#define POLYCALL_TELEMETRY_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct telemetry_service {
    char* name;
    void* service;
} telemetry_service_t;

/**
 * Registry for telemetry services
 */
typedef struct telemetry_registry {
    telemetry_service_t* services;
    int count;
    int capacity;
} telemetry_registry_t;

/**
 * Create a new registry
 */
telemetry_registry_t* telemetry_registry_create();

/**
 * Destroy a registry
 */
void telemetry_registry_destroy(telemetry_registry_t* registry);

/**
 * Register a service with the registry
 */
int telemetry_registry_register(telemetry_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* telemetry_registry_get(telemetry_registry_t* registry, const char* name);

/**
 * Register default services
 */
int telemetry_registry_register_defaults(telemetry_registry_t* registry);

#endif /* POLYCALL_TELEMETRY_REGISTRY_H */
