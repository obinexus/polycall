/**
 * @file protocol_commands.c
 * @brief Command handlers for protocol module
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "polycall/cli/commands/protocol_commands.h"
#include "polycall/cli/command.h"
#include "polycall/core/protocol/protocol_container.h"
#include "polycall/core/polycall/polycall.h"

// Define subcommands
static command_result_t handle_protocol_help(int argc, char** argv, void* context);
static command_result_t handle_protocol_status(int argc, char** argv, void* context);
static command_result_t handle_protocol_configure(int argc, char** argv, void* context);

// protocol subcommands
static subcommand_t protocol_subcommands[] = {
    {
        .name = "help",
        .description = "Show help for protocol commands",
        .usage = "polycall protocol help",
        .handler = handle_protocol_help,
        .requires_context = false
    },
    {
        .name = "status",
        .description = "Show protocol module status",
        .usage = "polycall protocol status",
        .handler = handle_protocol_status,
        .requires_context = true
    },
    {
        .name = "configure",
        .description = "Configure protocol module",
        .usage = "polycall protocol configure [options]",
        .handler = handle_protocol_configure,
        .requires_context = true
    }
};

// protocol command
static command_t protocol_command = {
    .name = "protocol",
    .description = "protocol module commands",
    .usage = "polycall protocol <subcommand>",
    .handler = NULL,
    .subcommands = protocol_subcommands,
    .subcommand_count = sizeof(protocol_subcommands) / sizeof(protocol_subcommands[0]),
    .requires_context = true
};

/**
 * Handle protocol help subcommand
 */
static command_result_t handle_protocol_help(int argc, char** argv, void* context) {
    printf("%s - %s\n", protocol_command.name, protocol_command.description);
    printf("Usage: %s\n\n", protocol_command.usage);
    
    printf("Available subcommands:\n");
    for (int i = 0; i < protocol_command.subcommand_count; i++) {
        printf("  %-15s %s\n", 
               protocol_command.subcommands[i].name, 
               protocol_command.subcommands[i].description);
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle protocol status subcommand
 */
static command_result_t handle_protocol_status(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    
    protocol_container_t* container = polycall_get_service(core_ctx, "protocol_container");
    if (!container) {
        fprintf(stderr, "Error: protocol module not initialized\n");
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    printf("protocol module status: Active\n");
    // Add module-specific status information here
    
    return COMMAND_SUCCESS;
}

/**
 * Handle protocol configure subcommand
 */
static command_result_t handle_protocol_configure(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    
    // Define flags
    command_flag_t flags[] = {
        {
            .name = "enable",
            .short_name = "e",
            .description = "Enable protocol module",
            .requires_value = false,
            .is_present = false
        },
        {
            .name = "disable",
            .short_name = "d",
            .description = "Disable protocol module",
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
    
    protocol_container_t* container = polycall_get_service(core_ctx, "protocol_container");
    if (!container) {
        fprintf(stderr, "Error: protocol module not initialized\n");
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Process flags
    if (flags[0].is_present) {
        printf("Enabling protocol module\n");
        // Enable module
    }
    
    if (flags[1].is_present) {
        printf("Disabling protocol module\n");
        // Disable module
    }
    
    if (flags[2].is_present) {
        printf("Setting protocol configuration file: %s\n", flags[2].value);
        // Set configuration file
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle protocol command
 */
int protocol_command_handler(int argc, char** argv, void* context) {
    if (argc < 1) {
        // No subcommand specified, show help
        return handle_protocol_help(0, NULL, context);
    }
    
    const char* subcommand = argv[0];
    
    // Find and execute subcommand
    for (int i = 0; i < protocol_command.subcommand_count; i++) {
        if (strcmp(protocol_command.subcommands[i].name, subcommand) == 0) {
            return protocol_command.subcommands[i].handler(argc, argv, context);
        }
    }
    
    fprintf(stderr, "Unknown protocol subcommand: %s\n", subcommand);
    return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Register protocol commands
 */
int register_protocol_commands(void) {
    return cli_register_command(&protocol_command);
}
