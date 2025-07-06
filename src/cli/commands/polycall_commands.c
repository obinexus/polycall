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
#include "polycall/cli/commands/polycall_commands.h"

 * @file polycall_command.c
 * @brief Main command system implementation for LibPolyCall CLI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the main command system functionality for the
LibPolyCall
 * CLI, including command registration, discovery, and execution.
 */

#include "polycall/cli/commands/protocol_command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of commands that can be registered
#define MAX_COMMANDS 128

// Static array of commands
static command_t s_commands[MAX_COMMANDS];
static int s_command_count = 0;

// Flag to track if command system is initialized
static bool s_initialized = false;

/**
 * @brief Initialize the command system
 *
 * @return int 0 on success, error code otherwise
 */
int cli_command_init(void) {
  if (s_initialized) {
    return 0; // Already initialized
  }

  // Clear commands array
  memset(s_commands, 0, sizeof(s_commands));
  s_command_count = 0;

  s_initialized = true;
  return 0;
}

/**
 * @brief Clean up the command system
 */
void cli_command_cleanup(void) {
  if (!s_initialized) {
    return; // Not initialized
  }

  // Clear commands array
  memset(s_commands, 0, sizeof(s_commands));
  s_command_count = 0;

  s_initialized = false;
}

/**
 * @brief Register a command with the command system
 *
 * @param command Command descriptor to register
 * @return int 0 on success, error code otherwise
 */
int cli_register_command(const command_t *command) {
  if (!s_initialized) {
    // Initialize command system if not already initialized
    int result = cli_command_init();
    if (result != 0) {
      return result;
    }
  }

  if (!command || !command->name || !command->handler) {
    return -1; // Invalid command
  }

  if (s_command_count >= MAX_COMMANDS) {
    return -2; // Command limit reached
  }

  // Check for duplicate command
  for (int i = 0; i < s_command_count; i++) {
    if (strcmp(s_commands[i].name, command->name) == 0) {
      return -3; // Command already registered
    }
  }

  // Register command
  s_commands[s_command_count] = *command;
  s_command_count++;

  return 0;
}

/**
 * @brief Execute a command
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return command_result_t Command result code
 */
command_result_t cli_execute_command(int argc, char **argv, void *context) {
  if (!s_initialized || argc < 1 || !argv || !argv[0]) {
    return COMMAND_ERROR_INVALID_ARGUMENTS;
  }

  const char *command_name = argv[0];

  // Find command
  const command_t *command = NULL;
  for (int i = 0; i < s_command_count; i++) {
    // Check for exact match or prefix match (for commands with subcommands)
    size_t name_len = strlen(s_commands[i].name);
    if (strcmp(s_commands[i].name, command_name) == 0 ||
        (strncmp(s_commands[i].name, command_name, name_len) == 0 &&
         command_name[name_len] == ' ')) {
      command = &s_commands[i];
      break;
    }
  }

  if (!command) {
    // Command not found
    fprintf(stderr, "Unknown command: %s\n", command_name);
    fprintf(stderr, "Type 'help' for a list of available commands.\n");
    return COMMAND_ERROR_NOT_FOUND;
  }

  // Check if command requires context
  if (command->requires_context && !context) {
    fprintf(stderr, "Command '%s' requires an initialized context.\n",
            command_name);
    return COMMAND_ERROR_INVALID_ARGUMENTS;
  }

  // Execute command handler
  return command->handler(argc - 1, &argv[1], context);
}

/**
 * @brief Get help for a command
 *
 * @param command_name Command name
 * @return const command_t* Command descriptor, or NULL if not found
 */
const command_t *cli_get_command_help(const char *command_name) {
  if (!s_initialized || !command_name) {
    return NULL;
  }

  // Find command
  for (int i = 0; i < s_command_count; i++) {
    if (strcmp(s_commands[i].name, command_name) == 0) {
      return &s_commands[i];
    }
  }

  return NULL;
}

/**
 * @brief List all registered commands
 *
 * @param commands Array to receive command descriptors
 * @param max_commands Maximum number of commands to retrieve
 * @return int Number of commands returned
 */
int cli_list_commands(command_t *commands, int max_commands) {
  if (!s_initialized || !commands || max_commands <= 0) {
    return 0;
  }

  // Copy commands to output array
  int count = (s_command_count < max_commands) ? s_command_count : max_commands;
  for (int i = 0; i < count; i++) {
    commands[i] = s_commands[i];
  }

  return count;
}

/**
 * @brief Handle the 'help' command
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return command_result_t Command result code
 */
static command_result_t handle_help_command(int argc, char **argv,
                                            void *context) {
  if (argc >= 1) {
    // Help for specific command
    const char *command_name = argv[0];
    const command_t *command = cli_get_command_help(command_name);

    if (command) {
      printf("%s - %s\n", command->name, command->description);
      if (command->usage) {
        printf("Usage: %s\n", command->usage);
      }
    } else {
      printf("No help available for command '%s'.\n", command_name);
    }
  } else {
    // List all commands
    printf("Available Commands:\n");

    for (int i = 0; i < s_command_count; i++) {
      // Skip subcommands (containing space in name)
      if (strchr(s_commands[i].name, ' ') == NULL) {
        printf("  %-20s %s\n", s_commands[i].name, s_commands[i].description);
      }
    }

    printf("\nUse 'help <command>' for more information about a specific "
           "command.\n");
  }

  return COMMAND_SUCCESS;
}

/**
 * @brief Handle the 'exit' command
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return command_result_t Command result code
 */
static command_result_t handle_exit_command(int argc, char **argv,
                                            void *context) {
  printf("Exiting LibPolyCall CLI...\n");
  exit(0);
  return COMMAND_SUCCESS; // Not reached
}

/**
 * @brief Handle the 'version' command
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return command_result_t Command result code
 */
static command_result_t handle_version_command(int argc, char **argv,
                                               void *context) {
  // Display version information
  printf("LibPolyCall CLI Version 1.0.0\n");
  printf("Build Date: %s %s\n", __DATE__, __TIME__);
  printf("Copyright (c) 2024 OBINexusComputing\n");

  return COMMAND_SUCCESS;
}

/**
 * @brief Register built-in commands
 */
void cli_register_builtin_commands(void) {
  // Help command
  command_t help_command = {.name = "help",
                            .description = "Display help information",
                            .handler = handle_help_command,
                            .usage = "help [command]",
                            .requires_context = false};
  cli_register_command(&help_command);

  // Exit command
  command_t exit_command = {.name = "exit",
                            .description = "Exit the CLI",
                            .handler = handle_exit_command,
                            .usage = "exit",
                            .requires_context = false};
  cli_register_command(&exit_command);

  // Alias 'quit' to 'exit'
  command_t quit_command = {.name = "quit",
                            .description = "Exit the CLI",
                            .handler = handle_exit_command,
                            .usage = "quit",
                            .requires_context = false};
  cli_register_command(&quit_command);

  // Version command
  command_t version_command = {.name = "version",
                               .description = "Display version information",
                               .handler = handle_version_command,
                               .usage = "version",
                               .requires_context = false};
  cli_register_command(&version_command);
}
