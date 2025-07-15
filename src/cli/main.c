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

// main.c (main CLI application)
#include "polycall/cli/common/command_registry.h"
#include "polycall/cli/providers/cli_container.h"
#include <string.h>

// Forward declarations for module command registration functions
extern void polycall_register_core_commands(void *registry);
extern void polycall_register_protocol_commands(void *registry);
extern void polycall_register_network_commands(void *registry);
extern void polycall_register_config_commands(void *registry);
extern void polycall_register_telemetry_commands(void *registry);
extern void polycall_register_auth_commands(void *registry);
extern void polycall_register_ffi_commands(void *registry);
extern void polycall_register_micro_commands(void *registry);
extern void polycall_register_edge_commands(void *registry);
extern void polycall_register_repl_commands(void *registry);
extern void polycall_register_accessibility_commands(void *registry);

// Print command usage
static void print_usage(const char *command_name) {
  fprintf(stderr, "Usage: %s <module> <command> [arguments...]\n",
          command_name);
  fprintf(stderr, "       %s --help            Show available modules\n",
          command_name);
  fprintf(stderr, "       %s <module> --help   Show commands for module\n",
          command_name);
}

// Print available modules
static void print_modules(polycall_cli_container_t *container) {
  void *registry = container->command_registry;

  char **modules = NULL;
  size_t module_count = 0;

  if (polycall_command_registry_list_modules(registry, &modules,
                                             &module_count) == 0) {
    printf("Available modules:\n");
    for (size_t i = 0; i < module_count; i++) {
      printf("  %s\n", modules[i]);
    }

    free(modules);
  } else {
    printf("No modules available\n");
  }
}

// Print available commands for a module
static void print_module_commands(polycall_cli_container_t *container,
                                  const char *module) {
  void *registry = container->command_registry;

  polycall_command_t *commands = NULL;
  size_t command_count = 0;

  if (polycall_command_registry_list(registry, module, &commands,
                                     &command_count) == 0) {
    printf("Commands for module '%s':\n", module);
    for (size_t i = 0; i < command_count; i++) {
      printf("  %-15s - %s\n", commands[i].name, commands[i].description);
      printf("     Usage: %s\n", commands[i].usage);
    }
  } else {
    printf("Module '%s' not found or has no commands\n", module);
  }
}

int main(int argc, char **argv) {
  // Initialize CLI container
  polycall_cli_container_t *container = polycall_cli_container_init();
  if (!container) {
    fprintf(stderr, "Failed to initialize CLI container\n");
    return 1;
  }

  // Register all module commands
  void *registry = container->command_registry;

  polycall_register_core_commands(registry);
  polycall_register_protocol_commands(registry);
  polycall_register_network_commands(registry);
  polycall_register_config_commands(registry);
  polycall_register_telemetry_commands(registry);
  polycall_register_auth_commands(registry);
  polycall_register_ffi_commands(registry);
  polycall_register_micro_commands(registry);
  polycall_register_edge_commands(registry);
  polycall_register_repl_commands(registry);
  polycall_register_accessibility_commands(registry);

  // Process command line arguments
  if (argc < 2) {
    print_usage(argv[0]);
    polycall_cli_container_destroy(container);
    return 1;
  }

  // Check for help
  if (strcmp(argv[1], "--help") == 0) {
    print_modules(container);
    polycall_cli_container_destroy(container);
    return 0;
  }

  // Get module name
  const char *module = argv[1];

  // Check for module help
  if (argc == 3 && strcmp(argv[2], "--help") == 0) {
    print_module_commands(container, module);
    polycall_cli_container_destroy(container);
    return 0;
  }

  // Execute command
  if (argc >= 3) {
    const char *command = argv[2];
    void *context = container->create_command_context(container);

    int result = container->execute_command(container, module, command,
                                            argc - 3, &argv[3]);

    // Destroy command context if needed
    // ...

    polycall_cli_container_destroy(container);
    return result;
  } else {
    fprintf(stderr, "Error: No command specified\n");
    print_usage(argv[0]);
    polycall_cli_container_destroy(container);
    return 1;
  }
}
