/*
 * command_registry_ioc.c
 * IoC Command Registry Implementation
 * Auto-generated by OBINexus Migration Enforcer
 */

#include "command_registry_ioc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

polycall_registry_t *polycall_registry_create(void) {
  polycall_registry_t *registry = malloc(sizeof(polycall_registry_t));
  if (!registry)
    return NULL;

  registry->commands = malloc(sizeof(polycall_command_t) * INITIAL_CAPACITY);
  if (!registry->commands) {
    free(registry);
    return NULL;
  }

  registry->count = 0;
  registry->capacity = INITIAL_CAPACITY;
  return registry;
}

void polycall_registry_destroy(polycall_registry_t *registry) {
  if (!registry)
    return;

  if (registry->commands) {
    for (size_t i = 0; i < registry->count; i++) {
      free((char *)registry->commands[i].name);
      free((char *)registry->commands[i].description);
    }
    free(registry->commands);
  }
  free(registry);
}

int polycall_registry_register(polycall_registry_t *registry, const char *name,
                               const char *description,
                               int (*handler)(int, char **)) {
  if (!registry || !name || !handler)
    return -1;

  // Expand capacity if needed
  if (registry->count >= registry->capacity) {
    size_t new_capacity = registry->capacity * 2;
    polycall_command_t *new_commands =
        realloc(registry->commands, sizeof(polycall_command_t) * new_capacity);
    if (!new_commands)
      return -1;

    registry->commands = new_commands;
    registry->capacity = new_capacity;
  }

  // Add command
  registry->commands[registry->count].name = strdup(name);
  registry->commands[registry->count].description =
      description ? strdup(description) : strdup("");
  registry->commands[registry->count].handler = handler;
  registry->count++;

  return 0;
}

int polycall_registry_execute(polycall_registry_t *registry,
                              const char *command, int argc, char **argv) {
  if (!registry || !command)
    return -1;

  for (size_t i = 0; i < registry->count; i++) {
    if (strcmp(registry->commands[i].name, command) == 0) {
      return registry->commands[i].handler(argc, argv);
    }
  }

  fprintf(stderr, "Unknown command: %s\n", command);
  return -1;
}

void polycall_registry_list(polycall_registry_t *registry) {
  if (!registry)
    return;

  printf("Available commands:\n");
  for (size_t i = 0; i < registry->count; i++) {
    printf("  %-20s %s\n", registry->commands[i].name,
           registry->commands[i].description);
  }
}
