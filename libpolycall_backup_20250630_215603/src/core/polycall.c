/**
 * @file polycall.c
 * @brief Core entry point for LibPolyCall framework
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "polycall/core/polycall/polycall.h"
#include "polycall/core/polycall/polycall_container.h"

// Forward declarations for module containers
#include "polycall/core/auth/auth_container.h"
#include "polycall/core/config/config_container.h"
#include "polycall/core/edge/edge_container.h"
#include "polycall/core/ffi/ffi_container.h"
#include "polycall/core/micro/micro_container.h"
#include "polycall/core/network/network_container.h"
#include "polycall/core/protocol/protocol_container.h"
#include "polycall/core/telemetry/telemetry_container.h"
#include "polycall/core/accessibility/accessibility_container.h"

// Global containers
static polycall_container_t* g_polycall_container = NULL;
static auth_container_t* g_auth_container = NULL;
static config_container_t* g_config_container = NULL;
static edge_container_t* g_edge_container = NULL;
static ffi_container_t* g_ffi_container = NULL;
static micro_container_t* g_micro_container = NULL;
static network_container_t* g_network_container = NULL;
static protocol_container_t* g_protocol_container = NULL;
static telemetry_container_t* g_telemetry_container = NULL;
static accessibility_container_t* g_accessibility_container = NULL;

// Command handler prototype
static int polycall_command_handler(int argc, char** argv);

/**
 * Initialize all module containers
 */
static int init_module_containers() {
    g_polycall_container = polycall_container_init();
    g_auth_container = auth_container_init();
    g_config_container = config_container_init();
    g_edge_container = edge_container_init();
    g_ffi_container = ffi_container_init();
    g_micro_container = micro_container_init();
    g_network_container = network_container_init();
    g_protocol_container = protocol_container_init();
    g_telemetry_container = telemetry_container_init();
    g_accessibility_container = accessibility_container_init();
    
    // Verify initialization
    if (!g_polycall_container || !g_auth_container || !g_config_container ||
        !g_edge_container || !g_ffi_container || !g_micro_container ||
        !g_network_container || !g_protocol_container || !g_telemetry_container ||
        !g_accessibility_container) {
        return -1;
    }
    
    // Register module containers with the main container
    polycall_container_register_service(g_polycall_container, "auth_container", g_auth_container);
    polycall_container_register_service(g_polycall_container, "config_container", g_config_container);
    polycall_container_register_service(g_polycall_container, "edge_container", g_edge_container);
    polycall_container_register_service(g_polycall_container, "ffi_container", g_ffi_container);
    polycall_container_register_service(g_polycall_container, "micro_container", g_micro_container);
    polycall_container_register_service(g_polycall_container, "network_container", g_network_container);
    polycall_container_register_service(g_polycall_container, "protocol_container", g_protocol_container);
    polycall_container_register_service(g_polycall_container, "telemetry_container", g_telemetry_container);
    polycall_container_register_service(g_polycall_container, "accessibility_container", g_accessibility_container);
    
    // Register command handler
    polycall_container_register_service(g_polycall_container, "command_handler", polycall_command_handler);
    
    return 0;
}

/**
 * Clean up all module containers
 */
static void cleanup_module_containers() {
    if (g_polycall_container) polycall_container_destroy(g_polycall_container);
    if (g_auth_container) auth_container_destroy(g_auth_container);
    if (g_config_container) config_container_destroy(g_config_container);
    if (g_edge_container) edge_container_destroy(g_edge_container);
    if (g_ffi_container) ffi_container_destroy(g_ffi_container);
    if (g_micro_container) micro_container_destroy(g_micro_container);
    if (g_network_container) network_container_destroy(g_network_container);
    if (g_protocol_container) protocol_container_destroy(g_protocol_container);
    if (g_telemetry_container) telemetry_container_destroy(g_telemetry_container);
    if (g_accessibility_container) accessibility_container_destroy(g_accessibility_container);
    
    g_polycall_container = NULL;
    g_auth_container = NULL;
    g_config_container = NULL;
    g_edge_container = NULL;
    g_ffi_container = NULL;
    g_micro_container = NULL;
    g_network_container = NULL;
    g_protocol_container = NULL;
    g_telemetry_container = NULL;
    g_accessibility_container = NULL;
}

/**
 * Main entry point for polycall
 */
int polycall_main(int argc, char** argv) {
    // Initialize framework
    int result = polycall_init();
    if (result != 0) {
        fprintf(stderr, "Failed to initialize LibPolyCall framework\n");
        return result;
    }
    
    // Get command handler
    int (*handler)(int, char**) = polycall_get_service("command_handler");
    if (!handler) {
        fprintf(stderr, "Command handler not registered\n");
        polycall_cleanup();
        return -1;
    }
    
    // Execute command
    result = handler(argc, argv);
    
    // Cleanup
    polycall_cleanup();
    
    return result;
}

/**
 * Initialize the framework
 */
int polycall_init() {
    // Initialize module containers
    int result = init_module_containers();
    if (result != 0) {
        cleanup_module_containers();
        return result;
    }
    
    return 0;
}

/**
 * Clean up the framework
 */
void polycall_cleanup() {
    cleanup_module_containers();
}

/**
 * Get a service from the container
 */
void* polycall_get_service(const char* service_name) {
    if (!g_polycall_container || !service_name) {
        return NULL;
    }
    
    return polycall_container_get_service(g_polycall_container, service_name);
}

/**
 * Register a service with the container
 */
int polycall_register_service(const char* service_name, void* service) {
    if (!g_polycall_container || !service_name || !service) {
        return -1;
    }
    
    return polycall_container_register_service(g_polycall_container, service_name, service);
}

/**
 * Main command handler
 */
static int polycall_command_handler(int argc, char** argv) {
    if (argc < 2) {
        // No command specified, show help
        printf("LibPolyCall - Cross-language Platform Communication Library\n");
        printf("Usage: %s <command> [options]\n\n", argv[0]);
        printf("Available commands:\n");
        printf("  auth       - Authentication and security commands\n");
        printf("  config     - Configuration management commands\n");
        printf("  edge       - Edge computing commands\n");
        printf("  ffi        - Foreign Function Interface commands\n");
        printf("  micro      - Micro command architecture commands\n");
        printf("  network    - Network communication commands\n");
        printf("  protocol   - Protocol implementation commands\n");
        printf("  telemetry  - Telemetry and monitoring commands\n");
        printf("  version    - Show version information\n");
        printf("  help       - Show this help message\n");
        return 0;
    }
    
    // Get the command
    const char* command = argv[1];
    
    // Handle built-in commands
    if (strcmp(command, "help") == 0) {
        return polycall_command_handler(1, argv);
    } else if (strcmp(command, "version") == 0) {
        printf("LibPolyCall v1.1.0\n");
        return 0;
    }
    
    // Handle module commands
    typedef int (*module_handler_t)(int, char**);
    module_handler_t handler = NULL;
    
    if (strcmp(command, "auth") == 0) {
        handler = polycall_get_service("auth_command_handler");
    } else if (strcmp(command, "config") == 0) {
        handler = polycall_get_service("config_command_handler");
    } else if (strcmp(command, "edge") == 0) {
        handler = polycall_get_service("edge_command_handler");
    } else if (strcmp(command, "ffi") == 0) {
        handler = polycall_get_service("ffi_command_handler");
    } else if (strcmp(command, "micro") == 0) {
        handler = polycall_get_service("micro_command_handler");
    } else if (strcmp(command, "network") == 0) {
        handler = polycall_get_service("network_command_handler");
    } else if (strcmp(command, "protocol") == 0) {
        handler = polycall_get_service("protocol_command_handler");
    } else if (strcmp(command, "telemetry") == 0) {
        handler = polycall_get_service("telemetry_command_handler");
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }
    
    if (handler) {
        // Pass arguments to handler (skipping the command itself)
        return handler(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "Command handler for '%s' not registered\n", command);
        return 1;
    }
}
