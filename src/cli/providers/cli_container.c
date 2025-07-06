/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

// cli_container.c
#include "polycall/cli/common/command_registry.h"
#include "polycall/cli/providers/cli_container.h"
#include <string.h>

// Forward declarations
static void *cli_resolve_service(polycall_cli_container_t *container,
                                 const char *service_name);
static int cli_register_service(polycall_cli_container_t *container,
                                const char *service_name, void *service);
static void *cli_create_command_context(polycall_cli_container_t *container);
static int cli_execute_command(polycall_cli_container_t *container,
                               const char *module, const char *command,
                               int argc, char **argv);

polycall_cli_container_t *polycall_cli_container_init() {
  polycall_cli_container_t *container =
      calloc(1, sizeof(polycall_cli_container_t));
  if (!container) {
    return NULL;
  }

  // Initialize base container
  container->base_container = polycall_container_init();
  if (!container->base_container) {
    free(container);
    return NULL;
  }

  // Initialize command registry
  container->command_registry = polycall_command_registry_create();
  if (!container->command_registry) {
    polycall_container_destroy(container->base_container);
    free(container);
    return NULL;
  }

  // Set function pointers
  container->resolve_service = cli_resolve_service;
  container->register_service = cli_register_service;
  container->create_command_context = cli_create_command_context;
  container->execute_command = cli_execute_command;

  // Register self as service
  cli_register_service(container, "cli_container", container);

  return container;
}

void polycall_cli_container_destroy(polycall_cli_container_t *container) {
  if (!container) {
    return;
  }

  // Destroy command registry
  if (container->command_registry) {
    polycall_command_registry_destroy(container->command_registry);
  }

  // Destroy base container
  if (container->base_container) {
    polycall_container_destroy(container->base_container);
  }

  // Free container
  free(container);
}

static void *cli_resolve_service(polycall_cli_container_t *container,
                                 const char *service_name) {
  if (!container || !service_name) {
    return NULL;
  }

  // Check if the service is the CLI container itself
  if (strcmp(service_name, "cli_container") == 0) {
    return container;
  }

  // Check if the service is the command registry
  if (strcmp(service_name, "command_registry") == 0) {
    return container->command_registry;
  }

  // Check if the service is the command context
  if (strcmp(service_name, "command_context") == 0) {
    return container->command_context;
  }

  // Delegate to base container
  return polycall_container_get_service(container->base_container,
                                        service_name);
}

static int cli_register_service(polycall_cli_container_t *container,
                                const char *service_name, void *service) {
  if (!container || !service_name || !service) {
    return -1;
  }

  // Register with base container
  return polycall_container_register_service(container->base_container,
                                             service_name, service);
}

static void *cli_create_command_context(polycall_cli_container_t *container) {
  // Implementation of command context creation
  // For now, just a simple context with a reference to the container
  if (!container) {
    return NULL;
  }

  container->command_context = calloc(1, sizeof(void *));
  if (!container->command_context) {
    return NULL;
  }

  // Store container reference in context
  *((void **)container->command_context) = container;

  return container->command_context;
}

static int cli_execute_command(polycall_cli_container_t *container,
                               const char *module, const char *command,
                               int argc, char **argv) {
  if (!container || !module || !command) {
    return -1;
  }

  // Execute command through registry
  return polycall_command_registry_execute(container->command_registry,
                                           container, module, command, argc,
                                           argv, container->command_context);
}
