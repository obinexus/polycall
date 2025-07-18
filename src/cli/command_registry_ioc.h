/*
 * command_registry_ioc.h
 * IoC Command Registry for PolyCall CLI
 * Auto-generated by OBINexus Migration Enforcer
 */

#ifndef POLYCALL_COMMAND_REGISTRY_IOC_H
#define POLYCALL_COMMAND_REGISTRY_IOC_H

#include <stddef.h>

typedef struct {
  const char *name;
  const char *description;
  int (*handler)(int argc, char **argv);
} polycall_command_t;

typedef struct {
  polycall_command_t *commands;
  size_t count;
  size_t capacity;
} polycall_registry_t;

// IoC Registry API
polycall_registry_t *polycall_registry_create(void);
void polycall_registry_destroy(polycall_registry_t *registry);
int polycall_registry_register(polycall_registry_t *registry, const char *name,
                               const char *description,
                               int (*handler)(int, char **));
int polycall_registry_execute(polycall_registry_t *registry,
                              const char *command, int argc, char **argv);
void polycall_registry_list(polycall_registry_t *registry);

// Command module initializers
void polycall_register_micro_commands(polycall_registry_t *registry);
void polycall_register_telemetry_commands(polycall_registry_t *registry);
void polycall_register_edge_commands(polycall_registry_t *registry);
void polycall_register_auth_commands(polycall_registry_t *registry);
void polycall_register_config_commands(polycall_registry_t *registry);
void polycall_register_network_commands(polycall_registry_t *registry);

#endif /* POLYCALL_COMMAND_REGISTRY_IOC_H */
