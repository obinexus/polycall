/**
 * @file config_registry.h
 * @brief Service registry for config module
 */

#ifndef POLYCALL_CONFIG_REGISTRY_H
#define POLYCALL_CONFIG_REGISTRY_H

#include <stdlib.h>

/**
 * Service entry in the registry
 */
typedef struct config_service {
  char *name;
  void *service;
} config_service_t;

/**
 * Registry for config services
 */
typedef struct config_registry {
  config_service_t *services;
  int count;
  int capacity;
} config_registry_t;

/**
 * Create a new registry
 */
config_registry_t *config_registry_create();

/**
 * Destroy a registry
 */
void config_registry_destroy(config_registry_t *registry);

/**
 * Register a service with the registry
 */
int config_registry_register(config_registry_t *registry, const char *name,
                             void *service);

/**
 * Get a service from the registry
 */
void *config_registry_get(config_registry_t *registry, const char *name);

/**
 * Register default services
 */
int config_registry_register_defaults(config_registry_t *registry);

#endif /* POLYCALL_CONFIG_REGISTRY_H */
