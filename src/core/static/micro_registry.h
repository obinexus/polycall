/**
 * @file micro_registry.h
 * @brief Service registry for micro module
 */

#ifndef POLYCALL_MICRO_REGISTRY_H
#define POLYCALL_MICRO_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct micro_service {
  char *name;
  void *service;
} micro_service_t;

/**
 * Registry for micro services
 */
typedef struct micro_registry {
  micro_service_t *services;
  int count;
  int capacity;
} micro_registry_t;

/**
 * Create a new registry
 */
micro_registry_t *micro_registry_create();

/**
 * Destroy a registry
 */
void micro_registry_destroy(micro_registry_t *registry);

/**
 * Register a service with the registry
 */
int micro_registry_register(micro_registry_t *registry, const char *name,
                            void *service);

/**
 * Get a service from the registry
 */
void *micro_registry_get(micro_registry_t *registry, const char *name);

/**
 * Register default services
 */
int micro_registry_register_defaults(micro_registry_t *registry);

#endif /* POLYCALL_MICRO_REGISTRY_H */
