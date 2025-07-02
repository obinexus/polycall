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

// repl_service_provider.c
#include "polycall/cli/providers/repl_service_provider.h"
#include "polycall/cli/providers/cli_container.h"
#include "polycall/core/polycall/polycall_repl.h"

// Initialize REPL services
polycall_core_error_t polycall_repl_register_services(
    polycall_cli_container_t* container
) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get core context from container
    polycall_core_context_t* core_ctx = container->resolve_service(
        container, "core_context");
    if (!core_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get config context from container if available
    polycall_config_context_t* config_ctx = container->resolve_service(
        container, "config_context");
    
    // Create default REPL configuration
    polycall_repl_config_t repl_config = {
        .show_prompts = true,
        .echo_commands = true,
        .save_history = true,
        .history_file = ".polycall_history",
        .config_ctx = config_ctx,
        .output_width = 80,
        .color_output = true,
        .verbose = false
    };
    
    // Initialize REPL context
    polycall_repl_context_t* repl_ctx = NULL;
    polycall_core_error_t result = polycall_repl_init(
        core_ctx, &repl_ctx, &repl_config);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Register REPL context in container
    container->register_service(container, "repl_context", repl_ctx);
    
    return POLYCALL_CORE_SUCCESS;
}

// Cleanup REPL services
void polycall_repl_cleanup_services(
    polycall_cli_container_t* container
) {
    if (!container) {
        return;
    }
    
    // Get core context from container
    polycall_core_context_t* core_ctx = container->resolve_service(
        container, "core_context");
    if (!core_ctx) {
        return;
    }
    
    // Get REPL context from container
    polycall_repl_context_t* repl_ctx = container->resolve_service(
        container, "repl_context");
    if (!repl_ctx) {
        return;
    }
    
    // Cleanup REPL context
    polycall_repl_cleanup(core_ctx, repl_ctx);
    
    // Unregister from container
    container->register_service(container, "repl_context", NULL);
}
