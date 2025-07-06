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
 * @file config_commands.c
 * @brief Command handlers for config module
 */

#include "polycall/cli/command.h"
#include "polycall/cli/commands/config_commands.h"
#include "polycall/core/config/config_container.h"
#include "polycall/core/polycall/polycall.h"
#include <string.h>

// Define subcommands
static command_result_t handle_config_help(int argc, char **argv,
                                           void *context);
static command_result_t handle_config_status(int argc, char **argv,
                                             void *context);
static command_result_t handle_config_configure(int argc, char **argv,
                                                void *context);

// config subcommands
static subcommand_t config_subcommands[] = {
    {.name = "help",
     .description = "Show help for config commands",
     .usage = "polycall config help",
     .handler = handle_config_help,
     .requires_context = false},
    {.name = "status",
     .description = "Show config module status",
     .usage = "polycall config status",
     .handler = handle_config_status,
     .requires_context = true},
    {.name = "configure",
     .description = "Configure config module",
     .usage = "polycall config configure [options]",
     .handler = handle_config_configure,
     .requires_context = true}};

// config command
static command_t config_command = {.name = "config",
                                   .description = "config module commands",
                                   .usage = "polycall config <subcommand>",
                                   .handler = NULL,
                                   .subcommands = config_subcommands,
                                   .subcommand_count =
                                       sizeof(config_subcommands) /
                                       sizeof(config_subcommands[0]),
                                   .requires_context = true};

/**
 * Handle config help subcommand
 */
static command_result_t handle_config_help(int argc, char **argv,
                                           void *context) {
  printf("%s - %s\n", config_command.name, config_command.description);
  printf("Usage: %s\n\n", config_command.usage);

  printf("Available subcommands:\n");
  for (int i = 0; i < config_command.subcommand_count; i++) {
    printf("  %-15s %s\n", config_command.subcommands[i].name,
           config_command.subcommands[i].description);
  }

  return COMMAND_SUCCESS;
}

/**
 * Handle config status subcommand
 */
static command_result_t handle_config_status(int argc, char **argv,
                                             void *context) {
  polycall_core_context_t *core_ctx = (polycall_core_context_t *)context;

  config_container_t *container =
      polycall_get_service(core_ctx, "config_container");
  if (!container) {
    fprintf(stderr, "Error: config module not initialized\n");
    return COMMAND_ERROR_EXECUTION_FAILED;
  }

  printf("config module status: Active\n");
  // Add module-specific status information here

  return COMMAND_SUCCESS;
}

/**
 * Handle config configure subcommand
 */
static command_result_t handle_config_configure(int argc, char **argv,
                                                void *context) {
  polycall_core_context_t *core_ctx = (polycall_core_context_t *)context;

  // Define flags
  command_flag_t flags[] = {{.name = "enable",
                             .short_name = "e",
                             .description = "Enable config module",
                             .requires_value = false,
                             .is_present = false},
                            {.name = "disable",
                             .short_name = "d",
                             .description = "Disable config module",
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

  config_container_t *container =
      polycall_get_service(core_ctx, "config_container");
  if (!container) {
    fprintf(stderr, "Error: config module not initialized\n");
    return COMMAND_ERROR_EXECUTION_FAILED;
  }

  // Process flags
  if (flags[0].is_present) {
    printf("Enabling config module\n");
    // Enable module
  }

  if (flags[1].is_present) {
    printf("Disabling config module\n");
    // Disable module
  }

  if (flags[2].is_present) {
    printf("Setting config configuration file: %s\n", flags[2].value);
    // Set configuration file
  }

  return COMMAND_SUCCESS;
}

/**
 * Handle config command
 */
int config_command_handler(int argc, char **argv, void *context) {
  if (argc < 1) {
    // No subcommand specified, show help
    return handle_config_help(0, NULL, context);
  }

  const char *subcommand = argv[0];

  // Find and execute subcommand
  for (int i = 0; i < config_command.subcommand_count; i++) {
    if (strcmp(config_command.subcommands[i].name, subcommand) == 0) {
      return config_command.subcommands[i].handler(argc, argv, context);
    }
  }

  fprintf(stderr, "Unknown config subcommand: %s\n", subcommand);
  return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Register config commands
 */
int register_config_commands(void) {
  return cli_register_command(&config_command);
}
