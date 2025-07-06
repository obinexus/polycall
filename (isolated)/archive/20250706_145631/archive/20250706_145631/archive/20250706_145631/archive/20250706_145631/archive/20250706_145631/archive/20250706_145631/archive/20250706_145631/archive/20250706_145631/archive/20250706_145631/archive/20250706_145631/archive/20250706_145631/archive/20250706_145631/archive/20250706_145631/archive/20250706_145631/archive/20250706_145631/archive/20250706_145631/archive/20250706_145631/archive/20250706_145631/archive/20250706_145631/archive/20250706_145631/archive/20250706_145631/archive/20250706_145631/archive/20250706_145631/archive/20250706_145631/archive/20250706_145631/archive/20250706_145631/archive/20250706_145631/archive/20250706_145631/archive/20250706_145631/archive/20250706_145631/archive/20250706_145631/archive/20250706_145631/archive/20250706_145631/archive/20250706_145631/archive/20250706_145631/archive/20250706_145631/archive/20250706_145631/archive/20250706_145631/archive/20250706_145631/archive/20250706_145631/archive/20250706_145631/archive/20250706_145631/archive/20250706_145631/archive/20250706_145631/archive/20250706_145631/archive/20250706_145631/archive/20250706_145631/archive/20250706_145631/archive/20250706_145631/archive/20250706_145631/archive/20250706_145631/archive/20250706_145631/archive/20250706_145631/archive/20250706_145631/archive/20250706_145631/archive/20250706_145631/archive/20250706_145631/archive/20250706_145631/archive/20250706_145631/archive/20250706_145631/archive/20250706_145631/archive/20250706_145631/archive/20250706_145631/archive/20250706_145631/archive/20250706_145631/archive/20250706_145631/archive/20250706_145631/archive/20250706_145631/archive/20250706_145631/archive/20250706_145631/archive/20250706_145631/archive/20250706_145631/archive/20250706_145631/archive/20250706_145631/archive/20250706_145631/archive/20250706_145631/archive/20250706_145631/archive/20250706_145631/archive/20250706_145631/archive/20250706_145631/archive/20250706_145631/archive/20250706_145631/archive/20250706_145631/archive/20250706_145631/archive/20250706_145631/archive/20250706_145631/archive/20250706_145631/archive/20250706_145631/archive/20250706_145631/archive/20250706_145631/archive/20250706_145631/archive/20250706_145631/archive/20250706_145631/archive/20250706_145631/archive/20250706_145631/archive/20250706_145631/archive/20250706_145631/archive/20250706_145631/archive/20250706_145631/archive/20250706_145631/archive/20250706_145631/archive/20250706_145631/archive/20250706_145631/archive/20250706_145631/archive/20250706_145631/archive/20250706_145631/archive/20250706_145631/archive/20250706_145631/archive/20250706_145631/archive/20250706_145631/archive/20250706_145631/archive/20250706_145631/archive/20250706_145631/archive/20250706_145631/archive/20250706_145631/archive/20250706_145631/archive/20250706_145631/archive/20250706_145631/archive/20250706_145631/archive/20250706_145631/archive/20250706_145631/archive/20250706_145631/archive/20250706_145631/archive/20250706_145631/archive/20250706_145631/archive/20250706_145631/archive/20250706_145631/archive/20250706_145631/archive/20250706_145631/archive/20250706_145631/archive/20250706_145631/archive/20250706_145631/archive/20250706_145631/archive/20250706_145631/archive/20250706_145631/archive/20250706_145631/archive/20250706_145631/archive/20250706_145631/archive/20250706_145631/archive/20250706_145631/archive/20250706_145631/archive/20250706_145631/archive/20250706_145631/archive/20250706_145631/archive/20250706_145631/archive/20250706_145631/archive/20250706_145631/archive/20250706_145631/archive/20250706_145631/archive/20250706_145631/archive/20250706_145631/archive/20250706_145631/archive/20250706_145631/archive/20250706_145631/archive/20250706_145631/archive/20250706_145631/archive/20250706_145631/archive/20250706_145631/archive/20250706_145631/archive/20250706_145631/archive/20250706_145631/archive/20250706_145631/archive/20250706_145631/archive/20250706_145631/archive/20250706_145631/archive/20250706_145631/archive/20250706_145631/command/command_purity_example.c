// ============================================================================
// Command Interface Standard - command_interface.h
// ============================================================================
#ifndef POLYCALL_COMMAND_INTERFACE_H
#define POLYCALL_COMMAND_INTERFACE_H

#include <stdint.h>

// Forward declarations for command isolation
typedef struct command_context command_context_t;
typedef struct command_result command_result_t;

// Standard command interface - ALL commands must implement this
typedef struct {
    const char* name;
    const char* version;
    
    // Lifecycle methods
    int (*init)(command_context_t* ctx);
    int (*execute)(command_context_t* ctx, void* params, command_result_t* result);
    int (*cleanup)(command_context_t* ctx);
    
    // Metadata methods
    const char* (*get_description)(void);
    uint32_t (*get_capabilities)(void);
} command_interface_t;

// Command context - injected dependencies only
typedef struct command_context {
    void* protocol_handle;   // Protocol layer access
    void* network_handle;    // Network layer access
    void* auth_handle;       // Auth layer access
    void* config;           // Command-specific config
} command_context_t;

// Command result structure
typedef struct command_result {
    int status_code;
    void* data;
    size_t data_size;
    char error_msg[256];
} command_result_t;

#endif // POLYCALL_COMMAND_INTERFACE_H

// ============================================================================
// Example: Micro Command Implementation - micro_command.c
// ============================================================================
#include "command_interface.h"
#include "base/memory.h"
#include "protocol/protocol_bridge.h"
#include "network/network.h"

// Private micro command state
typedef struct {
    void* routing_table;
    void* isolation_context;
    uint32_t instance_count;
} micro_state_t;

static int micro_init(command_context_t* ctx) {
    micro_state_t* state = memory_alloc(sizeof(micro_state_t));
    if (!state) return -1;
    
    // Initialize using ONLY injected dependencies
    state->routing_table = protocol_create_routing_table(ctx->protocol_handle);
    state->isolation_context = network_create_isolation(ctx->network_handle);
    state->instance_count = 0;
    
    ctx->config = state;
    return 0;
}

static int micro_execute(command_context_t* ctx, void* params, command_result_t* result) {
    micro_state_t* state = (micro_state_t*)ctx->config;
    
    // Process microservice command WITHOUT referencing other commands
    // Use only protocol/network/auth layers
    
    return 0;
}

static int micro_cleanup(command_context_t* ctx) {
    micro_state_t* state = (micro_state_t*)ctx->config;
    if (state) {
        // Cleanup using injected dependencies
        protocol_destroy_routing_table(ctx->protocol_handle, state->routing_table);
        network_destroy_isolation(ctx->network_handle, state->isolation_context);
        memory_free(state);
    }
    return 0;
}

// Export command interface
const command_interface_t micro_command = {
    .name = "micro",
    .version = "2.0.0",
    .init = micro_init,
    .execute = micro_execute,
    .cleanup = micro_cleanup,
    .get_description = micro_get_description,
    .get_capabilities = micro_get_capabilities
};

// ============================================================================
// Command Registry - polycall_registry.c (refactored for purity)
// ============================================================================
#include "command_interface.h"
#include "base/error.h"
#include <string.h>

#define MAX_COMMANDS 32

typedef struct {
    command_interface_t* commands[MAX_COMMANDS];
    command_context_t contexts[MAX_COMMANDS];
    size_t count;
} command_registry_t;

static command_registry_t g_registry = {0};

// Register command with dependency injection
int registry_register_command(command_interface_t* cmd, 
                            void* protocol_handle,
                            void* network_handle,
                            void* auth_handle) {
    if (g_registry.count >= MAX_COMMANDS) {
        return ERROR_REGISTRY_FULL;
    }
    
    // Check for duplicate registration
    for (size_t i = 0; i < g_registry.count; i++) {
        if (strcmp(g_registry.commands[i]->name, cmd->name) == 0) {
            return ERROR_DUPLICATE_COMMAND;
        }
    }
    
    // Set up isolated context for command
    command_context_t* ctx = &g_registry.contexts[g_registry.count];
    ctx->protocol_handle = protocol_handle;
    ctx->network_handle = network_handle;
    ctx->auth_handle = auth_handle;
    ctx->config = NULL;
    
    // Initialize command with its isolated context
    int ret = cmd->init(ctx);
    if (ret != 0) {
        return ret;
    }
    
    g_registry.commands[g_registry.count] = cmd;
    g_registry.count++;
    
    return 0;
}

// Execute command by name - ensures isolation
int registry_execute_command(const char* name, void* params, command_result_t* result) {
    for (size_t i = 0; i < g_registry.count; i++) {
        if (strcmp(g_registry.commands[i]->name, name) == 0) {
            // Execute with isolated context
            return g_registry.commands[i]->execute(&g_registry.contexts[i], params, result);
        }
    }
    return ERROR_COMMAND_NOT_FOUND;
}

// ============================================================================
// Hot-wire Core Implementation - hotwire_core.c
// ============================================================================
#ifndef POLYCALL_HOTWIRE_H
#define POLYCALL_HOTWIRE_H

#include "command_interface.h"

// Hot-wire adapter for dynamic command routing
typedef struct {
    const char* name;
    void* (*init)(const char* config);
    int (*route)(void* instance, const char* cmd, void* params, void* result);
    void (*cleanup)(void* instance);
} hotwire_adapter_t;

// Hot-wire routing entry
typedef struct {
    char pattern[128];
    char target_command[64];
    void* transform_func;
} hotwire_route_t;

// Hot-wire core API
int hotwire_register_adapter(hotwire_adapter_t* adapter);
int hotwire_add_route(const char* pattern, const char* target_cmd, void* transform);
int hotwire_process(const char* input, command_result_t* result);

#endif // POLYCALL_HOTWIRE_H

// Implementation
#include "hotwire_core.h"
#include "base/memory.h"
#include <regex.h>

typedef struct {
    hotwire_adapter_t* adapters[16];
    hotwire_route_t routes[64];
    size_t adapter_count;
    size_t route_count;
} hotwire_state_t;

static hotwire_state_t g_hotwire = {0};

int hotwire_add_route(const char* pattern, const char* target_cmd, void* transform) {
    if (g_hotwire.route_count >= 64) return -1;
    
    hotwire_route_t* route = &g_hotwire.routes[g_hotwire.route_count];
    strncpy(route->pattern, pattern, sizeof(route->pattern) - 1);
    strncpy(route->target_command, target_cmd, sizeof(route->target_command) - 1);
    route->transform_func = transform;
    
    g_hotwire.route_count++;
    return 0;
}

// ============================================================================
// Topo Command Implementation - topo_command.c
// ============================================================================
#include "command_interface.h"
#include "network/network.h"

typedef struct {
    void* mesh_topology;
    void* node_registry;
    uint32_t topology_version;
} topo_state_t;

static int topo_init(command_context_t* ctx) {
    topo_state_t* state = memory_alloc(sizeof(topo_state_t));
    if (!state) return -1;
    
    // Initialize topology using network layer only
    state->mesh_topology = network_create_mesh(ctx->network_handle);
    state->node_registry = network_create_node_registry(ctx->network_handle);
    state->topology_version = 1;
    
    ctx->config = state;
    return 0;
}

static int topo_execute(command_context_t* ctx, void* params, command_result_t* result) {
    topo_state_t* state = (topo_state_t*)ctx->config;
    
    // Define mesh network topology WITHOUT depending on other commands
    // Pure topology operations only
    
    return 0;
}

// Export topo command
const command_interface_t topo_command = {
    .name = "topo",
    .version = "2.0.0",
    .init = topo_init,
    .execute = topo_execute,
    .cleanup = topo_cleanup,
    .get_description = topo_get_description,
    .get_capabilities = topo_get_capabilities
};

// ============================================================================
// CLI Main with Command Purity - cli/main.c
// ============================================================================
#include "command_interface.h"
#include "polycall_registry.h"
#include <stdio.h>
#include <string.h>

// External command declarations
extern const command_interface_t micro_command;
extern const command_interface_t telemetry_command;
extern const command_interface_t guid_command;
extern const command_interface_t edge_command;
extern const command_interface_t crypto_command;
extern const command_interface_t topo_command;

int main(int argc, char* argv[]) {
    // Initialize core infrastructure
    void* protocol = protocol_init();
    void* network = network_init();
    void* auth = auth_init();
    
    // Register all commands with isolated contexts
    registry_register_command(&micro_command, protocol, network, auth);
    registry_register_command(&telemetry_command, protocol, network, auth);
    registry_register_command(&guid_command, protocol, network, auth);
    registry_register_command(&edge_command, protocol, network, auth);
    registry_register_command(&crypto_command, protocol, network, auth);
    registry_register_command(&topo_command, protocol, network, auth);
    
    // Parse and execute command
    if (argc < 2) {
        printf("Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }
    
    command_result_t result = {0};
    int ret = registry_execute_command(argv[1], &argv[2], &result);
    
    if (ret != 0) {
        fprintf(stderr, "Command failed: %s\n", result.error_msg);
        return ret;
    }
    
    // Cleanup
    protocol_cleanup(protocol);
    network_cleanup(network);
    auth_cleanup(auth);
    
    return 0;
}