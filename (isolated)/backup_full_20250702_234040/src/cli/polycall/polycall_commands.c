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

// polycall_commands.c
#include "polycall/cli/providers/cli_container.h"
#include "polycall/cli/common/command_registry.h"
#include "polycall/core/polycall/polycall.h"
#include <string.h>

// Initialize core context
static int cmd_core_init(void* container, int argc, char** argv, void* context) {
    polycall_cli_container_t* cli = (polycall_cli_container_t*)container;
    
    // Parse options
    polycall_config_t config = polycall_create_default_config();
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--config-file") == 0 && i + 1 < argc) {
            config.config_file = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--debug") == 0) {
            config.flags |= POLYCALL_FLAG_DEBUG;
        } else if (strcmp(argv[i], "--secure") == 0) {
            config.flags |= POLYCALL_FLAG_SECURE;
        } else if (strcmp(argv[i], "--memory-pool-size") == 0 && i + 1 < argc) {
            config.memory_pool_size = atol(argv[i + 1]);
            i++;
        }
    }
    
    // Initialize core context
    polycall_context_t* ctx = NULL;
    polycall_error_t err = polycall_init(&ctx, &config);
    if (err != POLYCALL_OK) {
        printf("Error initializing core context: %s\n", polycall_get_error_message(ctx));
        return 1;
    }
    
    // Register core context in container
    if (cli->register_service(cli, "polycall_context", ctx) != 0) {
        printf("Error: Failed to register core context in container\n");
        polycall_cleanup(ctx);
        return 1;
    }
    
    // Also register as core_context for backward compatibility
    cli->register_service(cli, "core_context", ctx);
    
    printf("Core context initialized successfully\n");
    return 0;
}

// Show core version
static int cmd_core_version(void* container, int argc, char** argv, void* context) {
    // Get version directly - does not require context
    polycall_version_t version = polycall_get_version();
    
    printf("LibPolyCall Version: %s\n", version.string);
    printf("- Major: %u\n", version.major);
    printf("- Minor: %u\n", version.minor);
    printf("- Patch: %u\n", version.patch);
    
    return 0;
}

// Cleanup core context
static int cmd_core_cleanup(void* container, int argc, char** argv, void* context) {
    polycall_cli_container_t* cli = (polycall_cli_container_t*)container;
    
    // Get core context
    polycall_context_t* ctx = cli->resolve_service(cli, "polycall_context");
    if (!ctx) {
        printf("Error: Core context not initialized\n");
        return 1;
    }
    
    // Cleanup core context
    polycall_cleanup(ctx);
    
    // Unregister from container
    cli->register_service(cli, "polycall_context", NULL);
    cli->register_service(cli, "core_context", NULL);
    
    printf("Core context cleaned up successfully\n");
    return 0;
}

// Register core commands
void polycall_register_core_commands(void* registry) {
    if (!registry) {
        return;
    }
    
    // Define commands
    static const polycall_command_t init_cmd = {
        .execute = cmd_core_init,
        .name = "init",
        .description = "Initialize core context",
        .usage = "core init [--config-file <file>] [--debug] [--secure] [--memory-pool-size <size>]",
        .dependencies = NULL
    };
    
    static const polycall_command_t version_cmd = {
        .execute = cmd_core_version,
        .name = "version",
        .description = "Show core version",
        .usage = "core version",
        .dependencies = NULL
    };
    
    static const polycall_command_t cleanup_cmd = {
        .execute = cmd_core_cleanup,
        .name = "cleanup",
        .description = "Cleanup core context",
        .usage = "core cleanup",
        .dependencies = (const char*[]){"polycall_context", NULL}
    };
    
    // Register commands
    polycall_command_registry_register(registry, "core", &init_cmd);
    polycall_command_registry_register(registry, "core", &version_cmd);
    polycall_command_registry_register(registry, "core", &cleanup_cmd);
}
