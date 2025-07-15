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

/**
 * @file network_commands.c
 * @brief Command handlers for network module
 */

#include "polycall/cli/command.h"
#include "polycall/cli/commands/network_commands.h"
#include "polycall/core/network/network_container.h"
#include "polycall/core/polycall/polycall.h"
#include <string.h>

// Define subcommands
static command_result_t handle_network_help(int argc, char **argv,
                                            void *context);
static command_result_t handle_network_status(int argc, char **argv,
                                              void *context);
static command_result_t handle_network_configure(int argc, char **argv,
                                                 void *context);

// network subcommands
static subcommand_t network_subcommands[] = {
    {.name = "help",
     .description = "Show help for network commands",
     .usage = "polycall network help",
     .handler = handle_network_help,
     .requires_context = false},
    {.name = "status",
     .description = "Show network module status",
     .usage = "polycall network status",
     .handler = handle_network_status,
     .requires_context = true},
    {.name = "configure",
     .description = "Configure network module",
     .usage = "polycall network configure [options]",
     .handler = handle_network_configure,
     .requires_context = true}};

// network command
static command_t network_command = {.name = "network",
                                    .description = "network module commands",
                                    .usage = "polycall network <subcommand>",
                                    .handler = NULL,
                                    .subcommands = network_subcommands,
                                    .subcommand_count =
                                        sizeof(network_subcommands) /
                                        sizeof(network_subcommands[0]),
                                    .requires_context = true};

/**
 * Handle network help subcommand
 */
static command_result_t handle_network_help(int argc, char **argv,
                                            void *context) {
  printf("%s - %s\n", network_command.name, network_command.description);
  printf("Usage: %s\n\n", network_command.usage);

  printf("Available subcommands:\n");
  for (int i = 0; i < network_command.subcommand_count; i++) {
    printf("  %-15s %s\n", network_command.subcommands[i].name,
           network_command.subcommands[i].description);
  }

  return COMMAND_SUCCESS;
}

/**
 * Handle network status subcommand
 */
static command_result_t handle_network_status(int argc, char **argv,
                                              void *context) {
  polycall_core_context_t *core_ctx = (polycall_core_context_t *)context;

  network_container_t *container =
      polycall_get_service(core_ctx, "network_container");
  if (!container) {
    fprintf(stderr, "Error: network module not initialized\n");
    return COMMAND_ERROR_EXECUTION_FAILED;
  }

  printf("network module status: Active\n");
  // Add module-specific status information here

  return COMMAND_SUCCESS;
}

/**
 * Handle network configure subcommand
 */
static command_result_t handle_network_configure(int argc, char **argv,
                                                 void *context) {
  polycall_core_context_t *core_ctx = (polycall_core_context_t *)context;

  // Define flags
  command_flag_t flags[] = {{.name = "enable",
                             .short_name = "e",
                             .description = "Enable network module",
                             .requires_value = false,
                             .is_present = false},
                            {.name = "disable",
                             .short_name = "d",
                             .description = "Disable network module",
                             .requires_value = false,
                             .is_present = false},
                            {.name = "config",
                             .short_name = "c",
                             .description = "Set configuration file",
                             .requires_value = true,
                             .is_present = false}};
  int flag_count = sizeof(flags) / sizeof(flags[0]);

  // Parse flags
  char *remaining_args[16];
  int remaining_count = 16;

  if (!parse_flags(argc - 1, &argv[1], flags, flag_count, remaining_args,
                   &remaining_count)) {
    fprintf(stderr, "Error parsing flags\n");
    return COMMAND_ERROR_INVALID_ARGUMENTS;
  }

  // Handle mutually exclusive flags
  if (flags[0].is_present && flags[1].is_present) {
    fprintf(stderr,
            "Error: --enable and --disable flags are mutually exclusive\n");
    return COMMAND_ERROR_INVALID_ARGUMENTS;
  }

  network_container_t *container =
      polycall_get_service(core_ctx, "network_container");
  if (!container) {
    fprintf(stderr, "Error: network module not initialized\n");
    return COMMAND_ERROR_EXECUTION_FAILED;
  }

  // Process flags
  if (flags[0].is_present) {
    printf("Enabling network module\n");
    // Enable module
  }

  if (flags[1].is_present) {
    printf("Disabling network module\n");
    // Disable module
  }

  if (flags[2].is_present) {
    printf("Setting network configuration file: %s\n", flags[2].value);
    // Set configuration file
  }

  return COMMAND_SUCCESS;
}

/**
 * Handle network command
 */
int network_command_handler(int argc, char **argv, void *context) {
  if (argc < 1) {
    // No subcommand specified, show help
    return handle_network_help(0, NULL, context);
  }

  const char *subcommand = argv[0];

  // Find and execute subcommand
  for (int i = 0; i < network_command.subcommand_count; i++) {
    if (strcmp(network_command.subcommands[i].name, subcommand) == 0) {
      return network_command.subcommands[i].handler(argc, argv, context);
    }
  }

  fprintf(stderr, "Unknown network subcommand: %s\n", subcommand);
  return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Register network commands
 */
int register_network_commands(void) {
  return cli_register_command(&network_command);
}
