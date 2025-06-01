/**
 * @file ffi_commands.c
 * @brief Command handlers for ffi module
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "polycall/cli/commands/ffi_commands.h"
#include "polycall/cli/command.h"
#include "polycall/core/ffi/ffi_container.h"
#include "polycall/core/polycall/polycall.h"

// Define subcommands
static command_result_t handle_ffi_help(int argc, char** argv, void* context);
static command_result_t handle_ffi_status(int argc, char** argv, void* context);
static command_result_t handle_ffi_configure(int argc, char** argv, void* context);

// ffi subcommands
static subcommand_t ffi_subcommands[] = {
    {
        .name = "help",
        .description = "Show help for ffi commands",
        .usage = "polycall ffi help",
        .handler = handle_ffi_help,
        .requires_context = false
    },
    {
        .name = "status",
        .description = "Show ffi module status",
        .usage = "polycall ffi status",
        .handler = handle_ffi_status,
        .requires_context = true
    },
    {
        .name = "configure",
        .description = "Configure ffi module",
        .usage = "polycall ffi configure [options]",
        .handler = handle_ffi_configure,
        .requires_context = true
    }
};

// ffi command
static command_t ffi_command = {
    .name = "ffi",
    .description = "ffi module commands",
    .usage = "polycall ffi <subcommand>",
    .handler = NULL,
    .subcommands = ffi_subcommands,
    .subcommand_count = sizeof(ffi_subcommands) / sizeof(ffi_subcommands[0]),
    .requires_context = true
};

/**
 * Handle ffi help subcommand
 */
static command_result_t handle_ffi_help(int argc, char** argv, void* context) {
    printf("%s - %s\n", ffi_command.name, ffi_command.description);
    printf("Usage: %s\n\n", ffi_command.usage);
    
    printf("Available subcommands:\n");
    for (int i = 0; i < ffi_command.subcommand_count; i++) {
        printf("  %-15s %s\n", 
               ffi_command.subcommands[i].name, 
               ffi_command.subcommands[i].description);
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle ffi status subcommand
 */
static command_result_t handle_ffi_status(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    
    ffi_container_t* container = polycall_get_service(core_ctx, "ffi_container");
    if (!container) {
        fprintf(stderr, "Error: ffi module not initialized\n");
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    printf("ffi module status: Active\n");
    // Add module-specific status information here
    
    return COMMAND_SUCCESS;
}

/**
 * Handle ffi configure subcommand
 */
static command_result_t handle_ffi_configure(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    
    // Define flags
    command_flag_t flags[] = {
        {
            .name = "enable",
            .short_name = "e",
            .description = "Enable ffi module",
            .requires_value = false,
            .is_present = false
        },
        {
            .name = "disable",
            .short_name = "d",
            .description = "Disable ffi module",
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
    
    ffi_container_t* container = polycall_get_service(core_ctx, "ffi_container");
    if (!container) {
        fprintf(stderr, "Error: ffi module not initialized\n");
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Process flags
    if (flags[0].is_present) {
        printf("Enabling ffi module\n");
        // Enable module
    }
    
    if (flags[1].is_present) {
        printf("Disabling ffi module\n");
        // Disable module
    }
    
    if (flags[2].is_present) {
        printf("Setting ffi configuration file: %s\n", flags[2].value);
        // Set configuration file
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle ffi command
 */
int ffi_command_handler(int argc, char** argv, void* context) {
    if (argc < 1) {
        // No subcommand specified, show help
        return handle_ffi_help(0, NULL, context);
    }
    
    const char* subcommand = argv[0];
    
    // Find and execute subcommand
    for (int i = 0; i < ffi_command.subcommand_count; i++) {
        if (strcmp(ffi_command.subcommands[i].name, subcommand) == 0) {
            return ffi_command.subcommands[i].handler(argc, argv, context);
        }
    }
    
    fprintf(stderr, "Unknown ffi subcommand: %s\n", subcommand);
    return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Register ffi commands
 */
int register_ffi_commands(void) {
    return cli_register_command(&ffi_command);
}
