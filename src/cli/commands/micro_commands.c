/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file micro_commands.c
 * @brief Command handlers for micro module
 */

#include <string.h>
#include "polycall/cli/commands/micro_commands.h"
#include "polycall/cli/command.h"
#include "polycall/core/micro/micro_container.h"
#include "polycall/core/polycall/polycall.h"

// Define subcommands
static command_result_t handle_micro_help(int argc, char** argv, void* context);
static command_result_t handle_micro_status(int argc, char** argv, void* context);
static command_result_t handle_micro_configure(int argc, char** argv, void* context);

// micro subcommands
static subcommand_t micro_subcommands[] = {
    {
        .name = "help",
        .description = "Show help for micro commands",
        .usage = "polycall micro help",
        .handler = handle_micro_help,
        .requires_context = false
    },
    {
        .name = "status",
        .description = "Show micro module status",
        .usage = "polycall micro status",
        .handler = handle_micro_status,
        .requires_context = true
    },
    {
        .name = "configure",
        .description = "Configure micro module",
        .usage = "polycall micro configure [options]",
        .handler = handle_micro_configure,
        .requires_context = true
    }
};

// micro command
static command_t micro_command = {
    .name = "micro",
    .description = "micro module commands",
    .usage = "polycall micro <subcommand>",
    .handler = NULL,
    .subcommands = micro_subcommands,
    .subcommand_count = sizeof(micro_subcommands) / sizeof(micro_subcommands[0]),
    .requires_context = true
};

/**
 * Handle micro help subcommand
 */
static command_result_t handle_micro_help(int argc, char** argv, void* context) {
    printf("%s - %s\n", micro_command.name, micro_command.description);
    printf("Usage: %s\n\n", micro_command.usage);
    
    printf("Available subcommands:\n");
    for (int i = 0; i < micro_command.subcommand_count; i++) {
        printf("  %-15s %s\n", 
               micro_command.subcommands[i].name, 
               micro_command.subcommands[i].description);
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle micro status subcommand
 */
static command_result_t handle_micro_status(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    
    micro_container_t* container = polycall_get_service(core_ctx, "micro_container");
    if (!container) {
        fprintf(stderr, "Error: micro module not initialized\n");
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    printf("micro module status: Active\n");
    // Add module-specific status information here
    
    return COMMAND_SUCCESS;
}

/**
 * Handle micro configure subcommand
 */
static command_result_t handle_micro_configure(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    
    // Define flags
    command_flag_t flags[] = {
        {
            .name = "enable",
            .short_name = "e",
            .description = "Enable micro module",
            .requires_value = false,
            .is_present = false
        },
        {
            .name = "disable",
            .short_name = "d",
            .description = "Disable micro module",
            .requires_value = false,
            .is_present = false
        },
        {
            .name = "config",
            .short_name = "c",
            .description = "Set configuration file",
            .requires_value = true,
            .is_present = false
        }
    };
    int flag_count = sizeof(flags) / sizeof(flags[0]);
    
    // Parse flags
    char* remaining_args[16];
    int remaining_count = 16;
    
    if (!parse_flags(argc - 1, &argv[1], flags, flag_count, remaining_args, &remaining_count)) {
        fprintf(stderr, "Error parsing flags\n");
        return COMMAND_ERROR_INVALID_ARGUMENTS;
    }
    
    // Handle mutually exclusive flags
    if (flags[0].is_present && flags[1].is_present) {
        fprintf(stderr, "Error: --enable and --disable flags are mutually exclusive\n");
        return COMMAND_ERROR_INVALID_ARGUMENTS;
    }
    
    micro_container_t* container = polycall_get_service(core_ctx, "micro_container");
    if (!container) {
        fprintf(stderr, "Error: micro module not initialized\n");
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Process flags
    if (flags[0].is_present) {
        printf("Enabling micro module\n");
        // Enable module
    }
    
    if (flags[1].is_present) {
        printf("Disabling micro module\n");
        // Disable module
    }
    
    if (flags[2].is_present) {
        printf("Setting micro configuration file: %s\n", flags[2].value);
        // Set configuration file
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle micro command
 */
int micro_command_handler(int argc, char** argv, void* context) {
    if (argc < 1) {
        // No subcommand specified, show help
        return handle_micro_help(0, NULL, context);
    }
    
    const char* subcommand = argv[0];
    
    // Find and execute subcommand
    for (int i = 0; i < micro_command.subcommand_count; i++) {
        if (strcmp(micro_command.subcommands[i].name, subcommand) == 0) {
            return micro_command.subcommands[i].handler(argc, argv, context);
        }
    }
    
    fprintf(stderr, "Unknown micro subcommand: %s\n", subcommand);
    return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Register micro commands
 */
int register_micro_commands(void) {
    return cli_register_command(&micro_command);
}
