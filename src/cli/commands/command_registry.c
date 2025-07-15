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

// command_registry.c
#include "polycall/cli/common/command_registry.h"
#include <string.h>

#define MAX_MODULES 16
#define MAX_COMMANDS_PER_MODULE 32

typedef struct {
  char name[64];                                        // Module name
  polycall_command_t commands[MAX_COMMANDS_PER_MODULE]; // Commands
  size_t command_count;                                 // Number of commands
} module_entry_t;

typedef struct {
  module_entry_t modules[MAX_MODULES]; // Modules
  size_t module_count;                 // Number of modules
} command_registry_t;

// Find or create module
static module_entry_t *find_or_create_module(command_registry_t *registry,
                                             const char *module) {
  if (!registry || !module) {
    return NULL;
  }

  // Search for existing module
  for (size_t i = 0; i < registry->module_count; i++) {
    if (strcmp(registry->modules[i].name, module) == 0) {
      return &registry->modules[i];
    }
  }

  // Create new module if space available
  if (registry->module_count < MAX_MODULES) {
    module_entry_t *new_module = &registry->modules[registry->module_count++];
    strncpy(new_module->name, module, sizeof(new_module->name) - 1);
    new_module->name[sizeof(new_module->name) - 1] = '\0';
    new_module->command_count = 0;
    return new_module;
  }

  return NULL;
}

// Find module
static module_entry_t *find_module(command_registry_t *registry,
                                   const char *module) {
  if (!registry || !module) {
    return NULL;
  }

  for (size_t i = 0; i < registry->module_count; i++) {
    if (strcmp(registry->modules[i].name, module) == 0) {
      return &registry->modules[i];
    }
  }

  return NULL;
}

// Find command in module
static polycall_command_t *find_command(module_entry_t *module,
                                        const char *command) {
  if (!module || !command) {
    return NULL;
  }

  for (size_t i = 0; i < module->command_count; i++) {
    if (strcmp(module->commands[i].name, command) == 0) {
      return &module->commands[i];
    }
  }

  return NULL;
}

// Create command registry
void *polycall_command_registry_create() {
  command_registry_t *registry = calloc(1, sizeof(command_registry_t));
  return registry;
}

// Destroy command registry
void polycall_command_registry_destroy(void *registry) { free(registry); }

// Register command
int polycall_command_registry_register(void *registry, const char *module,
                                       const polycall_command_t *command) {

  if (!registry || !module || !command || !command->name || !command->execute) {
    return -1;
  }

  command_registry_t *reg = (command_registry_t *)registry;

  // Find or create module
  module_entry_t *mod = find_or_create_module(reg, module);
  if (!mod) {
    return -1;
  }

  // Check for duplicate command
  if (find_command(mod, command->name)) {
    return -1;
  }

  // Add command if space available
  if (mod->command_count < MAX_COMMANDS_PER_MODULE) {
    memcpy(&mod->commands[mod->command_count++], command,
           sizeof(polycall_command_t));
    return 0;
  }

  return -1;
}

// Execute command
int polycall_command_registry_execute(void *registry, void *container,
                                      const char *module, const char *command,
                                      int argc, char **argv, void *context) {

  if (!registry || !container || !module || !command) {
    return -1;
  }

  command_registry_t *reg = (command_registry_t *)registry;

  // Find module
  module_entry_t *mod = find_module(reg, module);
  if (!mod) {
    return -1;
  }

  // Find command
  polycall_command_t *cmd = find_command(mod, command);
  if (!cmd) {
    return -1;
  }

  // Execute command
  return cmd->execute(container, argc, argv, context);
}

// List available commands for module
int polycall_command_registry_list(void *registry, const char *module,
                                   polycall_command_t **commands,
                                   size_t *command_count) {

  if (!registry || !module || !commands || !command_count) {
    return -1;
  }

  command_registry_t *reg = (command_registry_t *)registry;

  // Find module
  module_entry_t *mod = find_module(reg, module);
  if (!mod) {
    *command_count = 0;
    return -1;
  }

  // Set command list
  *commands = mod->commands;
  *command_count = mod->command_count;

  return 0;
}

// List available modules
int polycall_command_registry_list_modules(void *registry, char ***modules,
                                           size_t *module_count) {

  if (!registry || !modules || !module_count) {
    return -1;
  }

  command_registry_t *reg = (command_registry_t *)registry;

  // Allocate module list
  *module_count = reg->module_count;
  *modules = calloc(*module_count, sizeof(char *));
  if (!*modules) {
    return -1;
  }

  // Copy module names
  for (size_t i = 0; i < *module_count; i++) {
    (*modules)[i] = reg->modules[i].name;
  }

  return 0;
}
