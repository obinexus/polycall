/**
 * @file ffi_registry.h
 * @brief Service registry for ffi module
 */

#ifndef POLYCALL_FFI_REGISTRY_H
#define POLYCALL_FFI_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct ffi_service {
    char* name;
    void* service;
} ffi_service_t;

/**
 * Registry for ffi services
 */
typedef struct ffi_registry {
    ffi_service_t* services;
    int count;
    int capacity;
} ffi_registry_t;

/**
 * Create a new registry
 */
ffi_registry_t* ffi_registry_create();

/**
 * Destroy a registry
 */
void ffi_registry_destroy(ffi_registry_t* registry);

/**
 * Register a service with the registry
 */
int ffi_registry_register(ffi_registry_t* registry, const char* name, void* service);

/**
 * Get a service from the registry
 */
void* ffi_registry_get(ffi_registry_t* registry, const char* name);

/**
 * Register default services
 */
int ffi_registry_register_defaults(ffi_registry_t* registry);

#endif /* POLYCALL_FFI_REGISTRY_H */
