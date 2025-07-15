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
 * @file command_registry.c
 * @brief Enhanced command registry implementation with IoC integration
 */

#include "polycall/cli/common/command_registry.h"
#include "polycall/cli/command.h"
#include "polycall/core/polycall/polycall.h"
#include "polycall/cli/common/auth_commands.h"
#include "polycall/cli/common/config_commands.h"
#include "polycall/cli/common/edge_commands.h"
#include "polycall/cli/common/ffi_commands.h"
#include "polycall/cli/common/micro_commands.h"
#include "polycall/cli/common/network_commands.h"
#include "polycall/cli/common/protocol_commands.h"
#include "polycall/cli/common/telemetry_commands.h"
#include "polycall/cli/common/accessibility_commands.h"

// Command registry structure
struct command_registry {
    command_t* commands;
    int command_count;
    int capacity;
};

/**
 * Initialize command registry
 */
int command_registry_init(command_registry_t** registry) {
    if (!registry) {
        return -1;
    }
    
    command_registry_t* new_registry = malloc(sizeof(command_registry_t));
    if (!new_registry) {
        return -1;
    }
    
    // Allocate command array
    new_registry->capacity = 64;
    new_registry->commands = malloc(new_registry->capacity * sizeof(command_t));
    if (!new_registry->commands) {
        free(new_registry);
        return -1;
    }
    
    new_registry->command_count = 0;
    *registry = new_registry;
    
    return 0;
}

/**
 * Register a command with the registry
 */
int command_registry_register(command_registry_t* registry, const command_t* command) {
    if (!registry || !command) {
        return -1;
    }
    
    // Check capacity
    if (registry->command_count >= registry->capacity) {
        // Resize registry
        int new_capacity = registry->capacity * 2;
        command_t* new_commands = realloc(registry->commands, 
                                         new_capacity * sizeof(command_t));
        if (!new_commands) {
            return -1;
        }
        
        registry->commands = new_commands;
        registry->capacity = new_capacity;
    }
    
    // Check for duplicate
    for (int i = 0; i < registry->command_count; i++) {
        if (strcmp(registry->commands[i].name, command->name) == 0) {
            return -2; // Command already exists
        }
    }
    
    // Add command
    registry->commands[registry->command_count] = *command;
    registry->command_count++;
    
    return 0;
}

/**
 * Execute a command from the registry
 */
command_result_t command_registry_execute(command_registry_t* registry, 
                                         int argc, char** argv, void* context) {
    if (!registry || argc < 1 || !argv) {
        return COMMAND_ERROR_INVALID_ARGUMENTS;
    }
    
    const char* command_name = argv[0];
    
    // Find command
    for (int i = 0; i < registry->command_count; i++) {
        if (strcmp(registry->commands[i].name, command_name) == 0) {
            if (registry->commands[i].handler) {
                return registry->commands[i].handler(argc, argv, context);
            }
        }
    }
    
    return COMMAND_ERROR_NOT_FOUND;
}

/**
 * Get help for a command
 */
const command_t* command_registry_get_help(command_registry_t* registry, const char* command_name) {
    if (!registry || !command_name) {
        return NULL;
    }
    
    // Find command
    for (int i = 0; i < registry->command_count; i++) {
        if (strcmp(registry->commands[i].name, command_name) == 0) {
            return &registry->commands[i];
        }
    }
    
    return NULL;
}

/**
 * Get all commands
 */
int command_registry_get_all(command_registry_t* registry, command_t* commands, int max_count) {
    if (!registry || !commands || max_count <= 0) {
        return -1;
    }
    
    int count = registry->command_count < max_count ? registry->command_count : max_count;
    for (int i = 0; i < count; i++) {
        commands[i] = registry->commands[i];
    }
    
    return count;
}

/**
 * Cleanup command registry
 */
void command_registry_cleanup(command_registry_t* registry) {
    if (!registry) {
        return;
    }
    
    if (registry->commands) {
        free(registry->commands);
    }
    
    free(registry);
}

// Forward declarations for command handlers
int auth_command_handler(int argc, char** argv, void* context);
int config_command_handler(int argc, char** argv, void* context);
int edge_command_handler(int argc, char** argv, void* context);
int ffi_command_handler(int argc, char** argv, void* context);
int micro_command_handler(int argc, char** argv, void* context);
int network_command_handler(int argc, char** argv, void* context);
int protocol_command_handler(int argc, char** argv, void* context);
int telemetry_command_handler(int argc, char** argv, void* context);
int accessibility_command_handler(int argc, char** argv, void* context);

/**
 * Register all command handlers
 */
int register_all_command_handlers(void) {
    // Register command handlers with the main container
    polycall_register_service(NULL, "auth_command_handler", auth_command_handler);
    polycall_register_service(NULL, "config_command_handler", config_command_handler);
    polycall_register_service(NULL, "edge_command_handler", edge_command_handler);
    polycall_register_service(NULL, "ffi_command_handler", ffi_command_handler);
    polycall_register_service(NULL, "micro_command_handler", micro_command_handler);
    polycall_register_service(NULL, "network_command_handler", network_command_handler);
    polycall_register_service(NULL, "protocol_command_handler", protocol_command_handler);
    polycall_register_service(NULL, "telemetry_command_handler", telemetry_command_handler);
    polycall_register_service(NULL, "accessibility_command_handler", accessibility_command_handler);

    return 0;
}

/**
 * Register all commands
 */
bool register_all_commands(void) {
    // Register core module commands
    if (!register_auth_commands()) {
        return false;
    }
    if (!register_config_commands()) {
        return false;
    }
    if (!register_edge_commands()) {
        return false;
    }
    if (!register_ffi_commands()) {
        return false;
    }
    if (!register_micro_commands()) {
        return false;
    }
    if (!register_network_commands()) {
        return false;
    }
    if (!register_protocol_commands()) {
        return false;
    }
    if (!register_telemetry_commands()) {
        return false;
    }
    if (!register_accessibility_commands()) {
        return false;
    }

    return true;
}
