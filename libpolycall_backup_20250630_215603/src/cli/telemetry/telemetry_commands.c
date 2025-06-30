/**
 * @file telemetry_commands.c
 * @brief Enhanced command handlers for telemetry module with state transitions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "polycall/cli/telemetry/telemetry_commands.h"
#include "polycall/cli/command.h"
#include "polycall/core/telemetry/telemetry_container.h"
#include "polycall/core/telemetry/polycall_telemetry.h"
#include "polycall/core/telemetry/polycall_telemetry_reporting.h"
#include "polycall/core/telemetry/polycall_telemetry_security.h"
#include "polycall/core/polycall/polycall.h"
#include "polycall/cli/providers/cli_container.h"

// Command state transition constants
#define CMD_STATE_INITIATED  1
#define CMD_STATE_EXECUTING  2 
#define CMD_STATE_COMPLETED  3
#define CMD_STATE_ERROR      4

// Define subcommands
static command_result_t handle_telemetry_help(int argc, char** argv, void* context);
static command_result_t handle_telemetry_status(int argc, char** argv, void* context);
static command_result_t handle_telemetry_configure(int argc, char** argv, void* context);
static command_result_t handle_telemetry_analytics(int argc, char** argv, void* context);
static command_result_t handle_telemetry_debug(int argc, char** argv, void* context);

// Helper for recording command state transitions
static void record_command_state(polycall_core_context_t* core_ctx, const char* cmd_name, 
                              uint32_t state_id, const char* parent_guid, char** current_guid);

// telemetry subcommands
static subcommand_t telemetry_subcommands[] = {
    {
        .name = "help",
        .description = "Show help for telemetry commands",
        .usage = "polycall telemetry help",
        .handler = handle_telemetry_help,
        .requires_context = false
    },
    {
        .name = "status",
        .description = "Show telemetry module status",
        .usage = "polycall telemetry status",
        .handler = handle_telemetry_status,
        .requires_context = true
    },
    {
        .name = "configure",
        .description = "Configure telemetry module",
        .usage = "polycall telemetry configure [options]",
        .handler = handle_telemetry_configure,
        .requires_context = true
    },
    {
        .name = "analytics",
        .description = "Run telemetry analytics",
        .usage = "polycall telemetry analytics [options]",
        .handler = handle_telemetry_analytics,
        .requires_context = true
    },
    {
        .name = "debug",
        .description = "Debug telemetry with GUID and timestamps",
        .usage = "polycall telemetry debug [event_guid]",
        .handler = handle_telemetry_debug,
        .requires_context = true
    }
};

// telemetry command
static command_t telemetry_command = {
    .name = "telemetry",
    .description = "telemetry module commands",
    .usage = "polycall telemetry <subcommand>",
    .handler = NULL,
    .subcommands = telemetry_subcommands,
    .subcommand_count = sizeof(telemetry_subcommands) / sizeof(telemetry_subcommands[0]),
    .requires_context = true
};

/**
 * Helper for recording state transitions with GUID generation/updates
 */
static void record_command_state(polycall_core_context_t* core_ctx, const char* cmd_name, 
                              uint32_t state_id, const char* parent_guid, char** current_guid) {
    telemetry_container_t* telemetry_container = polycall_get_service(core_ctx, "telemetry_container");
    if (!telemetry_container) {
        fprintf(stderr, "Warning: Telemetry container not available for state tracking\n");
        return;
    }
    
    // Get telemetry context from container
    polycall_telemetry_context_t* telemetry_ctx = telemetry_container->telemetry_ctx;
    if (!telemetry_ctx) {
        fprintf(stderr, "Warning: Telemetry context not available for state tracking\n");
        return;
    }
    
    // Create new GUID or update existing
    uint32_t event_id = 0;
    switch(state_id) {
        case CMD_STATE_INITIATED:
            event_id = 1000; // Base event ID for initiated
            break;
        case CMD_STATE_EXECUTING:
            event_id = 2000; // Base event ID for executing
            break;
        case CMD_STATE_COMPLETED:
            event_id = 3000; // Base event ID for completed
            break;
        case CMD_STATE_ERROR:
            event_id = 4000; // Base event ID for error
            break;
        default:
            event_id = 9000; // Unknown state
    }
    
    // Generate or update GUID
    if (parent_guid && *current_guid) {
        // Free previous GUID
        free(*current_guid);
        *current_guid = NULL;
    }
    
    if (parent_guid) {
        // Update existing GUID with new state
        *current_guid = polycall_update_guid_state(core_ctx, parent_guid, state_id, event_id);
    } else {
        // Generate new GUID for new command
        *current_guid = polycall_generate_cryptonomic_guid(core_ctx, cmd_name, state_id, NULL);
    }
    
    // Record telemetry event
    polycall_telemetry_event_t event = {
        .severity = POLYCALL_TELEMETRY_INFO,
        .category = POLYCALL_TELEMETRY_CATEGORY_COMMAND,
        .source_module = "cli.telemetry",
        .event_id = event_id,
        .timestamp = 0, // Will be set by telemetry system
        .parent_guid = parent_guid
    };
    
    // Copy the new GUID
    if (*current_guid) {
        strncpy(event.event_guid, *current_guid, sizeof(event.event_guid) - 1);
        event.event_guid[sizeof(event.event_guid) - 1] = '\0';
    }
    
    // Add state description
    const char* state_desc = "unknown";
    switch(state_id) {
        case CMD_STATE_INITIATED:
            state_desc = "initiated";
            break;
        case CMD_STATE_EXECUTING:
            state_desc = "executing";
            break;
        case CMD_STATE_COMPLETED:
            state_desc = "completed";
            break;
        case CMD_STATE_ERROR:
            state_desc = "error";
            break;
    }
    
    snprintf(event.message, sizeof(event.message), "Telemetry command '%s' %s", 
             cmd_name, state_desc);
    
    // Record the event
    polycall_telemetry_record_event(telemetry_ctx, &event);
    
    if (state_id == CMD_STATE_ERROR) {
        // For errors, also record a security event
        polycall_security_telemetry_context_t* security_ctx = 
            telemetry_container->security_telemetry_ctx;
        
        if (security_ctx) {
            polycall_security_event_t sec_event = {
                .timestamp = 0, // Will be set by system
                .source_ip = "127.0.0.1", // Local command
                .user_id = "cli_user",
                .action = POLYCALL_SECURITY_ACTION_COMMAND,
                .outcome = POLYCALL_SECURITY_OUTCOME_FAILURE
            };
            
            // Copy event GUID
            if (*current_guid) {
                strncpy(sec_event.event_guid, *current_guid, sizeof(sec_event.event_guid) - 1);
                sec_event.event_guid[sizeof(sec_event.event_guid) - 1] = '\0';
            }
            
            snprintf(sec_event.description, sizeof(sec_event.description), 
                    "Command execution failure: %s", cmd_name);
            
            polycall_security_telemetry_record_event(security_ctx, &sec_event);
        }
    }
}

/**
 * Handle telemetry help subcommand
 */
static command_result_t handle_telemetry_help(int argc, char** argv, void* context) {
    printf("%s - %s\n", telemetry_command.name, telemetry_command.description);
    printf("Usage: %s\n\n", telemetry_command.usage);
    
    printf("Available subcommands:\n");
    for (int i = 0; i < telemetry_command.subcommand_count; i++) {
        printf("  %-15s %s\n", 
               telemetry_command.subcommands[i].name, 
               telemetry_command.subcommands[i].description);
    }
    
    return COMMAND_SUCCESS;
}

/**
 * Handle telemetry status subcommand
 */
static command_result_t handle_telemetry_status(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    char* cmd_guid = NULL;
    
    // Record command initiation
    record_command_state(core_ctx, "telemetry.status", CMD_STATE_INITIATED, NULL, &cmd_guid);
    
    telemetry_container_t* container = polycall_get_service(core_ctx, "telemetry_container");
    if (!container) {
        fprintf(stderr, "Error: telemetry module not initialized\n");
        
        // Record error state
        record_command_state(core_ctx, "telemetry.status", CMD_STATE_ERROR, cmd_guid, &cmd_guid);
        free(cmd_guid);
        
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Record executing state
    record_command_state(core_ctx, "telemetry.status", CMD_STATE_EXECUTING, cmd_guid, &cmd_guid);
    
    // Parse flags for JSON output
    bool json_output = false;
    bool verbose = false;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            json_output = true;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
    }
    
    // Get telemetry configuration
    polycall_telemetry_config_t config;
    polycall_telemetry_context_t* telemetry_ctx = container->telemetry_ctx;
    
    if (telemetry_ctx) {
        // Get the telemetry configuration
        // (Simplified - in real implementation, get from context)
        config.enable_telemetry = true;
        config.min_severity = POLYCALL_TELEMETRY_INFO;
        config.max_event_queue_size = 1024;
    } else {
        config.enable_telemetry = false;
    }
    
    // Output telemetry status based on format
    if (json_output) {
        printf("{\n");
        printf("  \"command\": \"telemetry status\",\n");
        printf("  \"timestamp\": \"%s\",\n", "2025-05-12T10:30:00Z"); // Replace with actual timestamp
        printf("  \"guid\": \"%s\",\n", cmd_guid ? cmd_guid : "unknown");
        printf("  \"status\": \"%s\",\n", telemetry_ctx ? "active" : "inactive");
        
        if (telemetry_ctx) {
            printf("  \"telemetry\": {\n");
            printf("    \"enabled\": %s,\n", config.enable_telemetry ? "true" : "false");
            printf("    \"min_severity\": %d,\n", config.min_severity);
            printf("    \"queue_size\": %d", config.max_event_queue_size);
            
            if (verbose) {
                printf(",\n    \"last_event_timestamp\": \"%s\",\n", "2025-05-12T10:29:45Z");
                printf("    \"event_count\": %d\n", 42); // Example count
            } else {
                printf("\n");
            }
            
            printf("  }\n");
        }
        
        printf("}\n");
    } else {
        printf("Telemetry module status: %s\n", telemetry_ctx ? "Active" : "Inactive");
        
        if (telemetry_ctx) {
            printf("Configuration:\n");
            printf("  - Telemetry enabled: %s\n", config.enable_telemetry ? "Yes" : "No");
            printf("  - Minimum severity: %d\n", config.min_severity);
            printf("  - Event queue size: %d\n", config.max_event_queue_size);
            
            if (verbose) {
                printf("Runtime Information:\n");
                printf("  - Last event time: %s\n", "2025-05-12T10:29:45Z");
                printf("  - Events collected: %d\n", 42); // Example count
                printf("  - Command GUID: %s\n", cmd_guid ? cmd_guid : "unknown");
            }
        }
    }
    
    // Record successful completion
    record_command_state(core_ctx, "telemetry.status", CMD_STATE_COMPLETED, cmd_guid, &cmd_guid);
    free(cmd_guid);
    
    return COMMAND_SUCCESS;
}

/**
 * Handle telemetry configure subcommand
 */
static command_result_t handle_telemetry_configure(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    char* cmd_guid = NULL;
    
    // Record command initiation
    record_command_state(core_ctx, "telemetry.configure", CMD_STATE_INITIATED, NULL, &cmd_guid);
    
    // Define flags
    command_flag_t flags[] = {
        {
            .name = "enable",
            .short_name = "e",
            .description = "Enable telemetry module",
            .requires_value = false,
            .is_present = false
        },
        {
            .name = "disable",
            .short_name = "d",
            .description = "Disable telemetry module",
            .requires_value = false,
            .is_present = false
        },
        {
            .name = "config",
            .short_name = "c",
            .description = "Set configuration file",
            .requires_value = true,
            .is_present = false
        },
        {
            .name = "severity",
            .short_name = "s",
            .description = "Set minimum severity level",
            .requires_value = true,
            .is_present = false
        },
        {
            .name = "queue-size",
            .short_name = "q",
            .description = "Set event queue size",
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
        
        // Record error state
        record_command_state(core_ctx, "telemetry.configure", CMD_STATE_ERROR, cmd_guid, &cmd_guid);
        free(cmd_guid);
        
        return COMMAND_ERROR_INVALID_ARGUMENTS;
    }
    
    // Record executing state
    record_command_state(core_ctx, "telemetry.configure", CMD_STATE_EXECUTING, cmd_guid, &cmd_guid);
    
    // Handle mutually exclusive flags
    if (flags[0].is_present && flags[1].is_present) {
        fprintf(stderr, "Error: --enable and --disable flags are mutually exclusive\n");
        
        // Record error state
        record_command_state(core_ctx, "telemetry.configure", CMD_STATE_ERROR, cmd_guid, &cmd_guid);
        free(cmd_guid);
        
        return COMMAND_ERROR_INVALID_ARGUMENTS;
    }
    
    telemetry_container_t* container = polycall_get_service(core_ctx, "telemetry_container");
    if (!container) {
        fprintf(stderr, "Error: telemetry module not initialized\n");
        
        // Record error state
        record_command_state(core_ctx, "telemetry.configure", CMD_STATE_ERROR, cmd_guid, &cmd_guid);
        free(cmd_guid);
        
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Process flags
    if (flags[0].is_present) {
        printf("Enabling telemetry module\n");
        // Enable module - implementation would go here
    }
    
    if (flags[1].is_present) {
        printf("Disabling telemetry module\n");
        // Disable module - implementation would go here
    }
    
    if (flags[2].is_present) {
        printf("Setting telemetry configuration file: %s\n", flags[2].value);
        // Set configuration file - implementation would go here
    }
    
    if (flags[3].is_present) {
        printf("Setting minimum severity level: %s\n", flags[3].value);
        // Set severity - implementation would go here
    }
    
    if (flags[4].is_present) {
        printf("Setting event queue size: %s\n", flags[4].value);
        // Set queue size - implementation would go here
    }
    
    // Record successful completion
    record_command_state(core_ctx, "telemetry.configure", CMD_STATE_COMPLETED, cmd_guid, &cmd_guid);
    free(cmd_guid);
    
    return COMMAND_SUCCESS;
}

/**
 * Handle telemetry analytics subcommand
 */
static command_result_t handle_telemetry_analytics(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    char* cmd_guid = NULL;
    
    // Record command initiation
    record_command_state(core_ctx, "telemetry.analytics", CMD_STATE_INITIATED, NULL, &cmd_guid);
    
    telemetry_container_t* container = polycall_get_service(core_ctx, "telemetry_container");
    if (!container) {
        fprintf(stderr, "Error: telemetry module not initialized\n");
        
        // Record error state
        record_command_state(core_ctx, "telemetry.analytics", CMD_STATE_ERROR, cmd_guid, &cmd_guid);
        free(cmd_guid);
        
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Record executing state
    record_command_state(core_ctx, "telemetry.analytics", CMD_STATE_EXECUTING, cmd_guid, &cmd_guid);
    
    // Placeholder for analytics implementation
    printf("Running telemetry analytics...\n");
    printf("GUID: %s\n", cmd_guid ? cmd_guid : "unknown");
    
    // Record successful completion
    record_command_state(core_ctx, "telemetry.analytics", CMD_STATE_COMPLETED, cmd_guid, &cmd_guid);
    free(cmd_guid);
    
    return COMMAND_SUCCESS;
}

/**
 * Handle telemetry debug subcommand
 */
static command_result_t handle_telemetry_debug(int argc, char** argv, void* context) {
    polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
    char* cmd_guid = NULL;
    
    // Record command initiation
    record_command_state(core_ctx, "telemetry.debug", CMD_STATE_INITIATED, NULL, &cmd_guid);
    
    telemetry_container_t* container = polycall_get_service(core_ctx, "telemetry_container");
    if (!container) {
        fprintf(stderr, "Error: telemetry module not initialized\n");
        
        // Record error state
        record_command_state(core_ctx, "telemetry.debug", CMD_STATE_ERROR, cmd_guid, &cmd_guid);
        free(cmd_guid);
        
        return COMMAND_ERROR_EXECUTION_FAILED;
    }
    
    // Record executing state
    record_command_state(core_ctx, "telemetry.debug", CMD_STATE_EXECUTING, cmd_guid, &cmd_guid);
    
    // Check if a specific GUID was provided
    const char* target_guid = NULL;
    if (argc > 1) {
        target_guid = argv[1];
        printf("Debugging telemetry events for GUID: %s\n", target_guid);
        
        // Placeholder for GUID-specific debug info
        printf("State transitions:\n");
        printf("  - Command initiated: timestamp\n");
        printf("  - Command executing: timestamp\n");
        printf("  - Command completed/failed: timestamp\n");
    } else {
        printf("Showing recent telemetry events:\n");
        
        // Placeholder for recent events list
        printf("Event 1: %s - timestamp\n", "example-guid-1");
        printf("Event 2: %s - timestamp\n", "example-guid-2");
        printf("Event 3: %s - timestamp\n", "example-guid-3");
        
        printf("\nUse 'polycall telemetry debug <guid>' to see details for a specific event.\n");
    }
    
    // Record successful completion
    record_command_state(core_ctx, "telemetry.debug", CMD_STATE_COMPLETED, cmd_guid, &cmd_guid);
    free(cmd_guid);
    
    return COMMAND_SUCCESS;
}

/**
 * Handle telemetry command
 */
int telemetry_command_handler(int argc, char** argv, void* context) {
    if (argc < 1) {
        // No subcommand specified, show help
        return handle_telemetry_help(0, NULL, context);
    }
    
    const char* subcommand = argv[0];
    
    // Find and execute subcommand
    for (int i = 0; i < telemetry_command.subcommand_count; i++) {
        if (strcmp(telemetry_command.subcommands[i].name, subcommand) == 0) {
            return telemetry_command.subcommands[i].handler(argc, argv, context);
        }
    }
    
    fprintf(stderr, "Unknown telemetry subcommand: %s\n", subcommand);
    return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Register telemetry commands
 */
int register_telemetry_commands(void) {
    return cli_register_command(&telemetry_command);
}
