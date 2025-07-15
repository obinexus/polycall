// cli_container.h
#ifndef POLYCALL_CLI_CONTAINER_H
#define POLYCALL_CLI_CONTAINER_H

#include "polycall/core/polycall/polycall_container.h"
#include "polycall/core/polycall/polycall_context.h"
#include <stddef.h>

typedef struct polycall_cli_container {
  // Base container for core services
  polycall_container_t *base_container;

  // Command-specific services
  void *command_registry;
  void *command_context;

  // Service resolution function
  void *(*resolve_service)(struct polycall_cli_container *container,
                           const char *service_name);

  // Service registration function
  int (*register_service)(struct polycall_cli_container *container,
                          const char *service_name, void *service);

  // Context creation
  void *(*create_command_context)(struct polycall_cli_container *container);

  // Command execution
  int (*execute_command)(struct polycall_cli_container *container,
                         const char *module, const char *command, int argc,
                         char **argv);
} polycall_cli_container_t;

// Initialize CLI container
polycall_cli_container_t *polycall_cli_container_init();

// Destroy CLI container
void polycall_cli_container_destroy(polycall_cli_container_t *container);

#endif /* POLYCALL_CLI_CONTAINER_H */