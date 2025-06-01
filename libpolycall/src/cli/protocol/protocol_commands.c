// protocol_commands.c
#include "polycall/cli/providers/cli_container.h"
#include "polycall/cli/common/command_registry.h"
#include "polycall/core/protocol/polycall_protocol_context.h"
#include "polycall/core/polycall/polycall_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Protocol initialization command
static int cmd_protocol_init(void* container, int argc, char** argv, void* context) {
    polycall_cli_container_t* cli = (polycall_cli_container_t*)container;
    
    // Get core context from container
    polycall_core_context_t* core_ctx = cli->resolve_service(cli, "core_context");
    if (!core_ctx) {
        printf("Error: Core context not initialized\n");
        return 1;
    }
    
    // Parse command options
    polycall_protocol_config_t config = {0};
    
    // Default configuration
    config.max_message_size = 4096;
    
    // Parse arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--max-message-size") == 0 && i + 1 < argc) {
            config.max_message_size = atoi(argv[i + 1]);
            i++;
        }
        // Add more configuration options as needed
    }
    
    // Create protocol context
    polycall_protocol_context_t* protocol_ctx = malloc(sizeof(polycall_protocol_context_t));
    if (!protocol_ctx) {
        printf("Error: Failed to allocate protocol context\n");
        return 1;
    }
    
    // Initialize protocol
    NetworkEndpoint* endpoint = NULL; // Would be provided by network module
    if (!polycall_protocol_init(protocol_ctx, core_ctx, endpoint, &config)) {
        printf("Error: Failed to initialize protocol\n");
        free(protocol_ctx);
        return 1;
    }
    
    // Register protocol context in container
    if (cli->register_service(cli, "protocol_context", protocol_ctx) != 0) {
        printf("Error: Failed to register protocol context\n");
        // Cleanup protocol context
        free(protocol_ctx);
        return 1;
    }
    
    printf("Protocol initialized successfully\n");
    return 0;
}

// Command to display protocol state
static int cmd_protocol_state(void* container, int argc, char** argv, void* context) {
    polycall_cli_container_t* cli = (polycall_cli_container_t*)container;
    
    // Get protocol context
    polycall_protocol_context_t* protocol_ctx = cli->resolve_service(cli, "protocol_context");
    if (!protocol_ctx) {
        printf("Error: Protocol not initialized\n");
        return 1;
    }
    
    // Get current state
    polycall_protocol_state_t state = polycall_protocol_get_state(protocol_ctx);
    
    // Convert state to string
    const char* state_str = "UNKNOWN";
    switch (state) {
        case POLYCALL_PROTOCOL_STATE_INIT:
            state_str = "INITIALIZED";
            break;
        case POLYCALL_PROTOCOL_STATE_HANDSHAKE:
            state_str = "HANDSHAKE";
            break;
        case POLYCALL_PROTOCOL_STATE_AUTH:
            state_str = "AUTHENTICATING";
            break;
        case POLYCALL_PROTOCOL_STATE_READY:
            state_str = "READY";
            break;
        case POLYCALL_PROTOCOL_STATE_ERROR:
            state_str = "ERROR";
            break;
        case POLYCALL_PROTOCOL_STATE_CLOSED:
            state_str = "CLOSED";
            break;
    }
    
    printf("Protocol State: %s\n", state_str);
    return 0;
}

// Command to initiate protocol handshake
static int cmd_protocol_handshake(void* container, int argc, char** argv, void* context) {
    polycall_cli_container_t* cli = (polycall_cli_container_t*)container;
    
    // Get protocol context
    polycall_protocol_context_t* protocol_ctx = cli->resolve_service(cli, "protocol_context");
    if (!protocol_ctx) {
        printf("Error: Protocol not initialized\n");
        return 1;
    }
    
    // Start handshake
    if (!polycall_protocol_start_handshake(protocol_ctx)) {
        printf("Error: Failed to start handshake\n");
        return 1;
    }
    
    printf("Handshake initiated\n");
    return 0;
}

// Command to send protocol message
static int cmd_protocol_send(void* container, int argc, char** argv, void* context) {
    polycall_cli_container_t* cli = (polycall_cli_container_t*)container;
    
    // Get protocol context
    polycall_protocol_context_t* protocol_ctx = cli->resolve_service(cli, "protocol_context");
    if (!protocol_ctx) {
        printf("Error: Protocol not initialized\n");
        return 1;
    }
    
    // Check arguments
    if (argc < 2) {
        printf("Usage: protocol send <message_type> <payload>\n");
        return 1;
    }
    
    // Parse message type
    polycall_protocol_msg_type_t type = POLYCALL_PROTOCOL_MSG_COMMAND;
    if (strcmp(argv[0], "handshake") == 0) {
        type = POLYCALL_PROTOCOL_MSG_HANDSHAKE;
    } else if (strcmp(argv[0], "auth") == 0) {
        type = POLYCALL_PROTOCOL_MSG_AUTH;
    } else if (strcmp(argv[0], "error") == 0) {
        type = POLYCALL_PROTOCOL_MSG_ERROR;
    } else if (strcmp(argv[0], "heartbeat") == 0) {
        type = POLYCALL_PROTOCOL_MSG_HEARTBEAT;
    }
    
    // Get payload
    const char* payload = argv[1];
    size_t payload_length = strlen(payload);
    
    // Send message
    if (!polycall_protocol_send(protocol_ctx, type, payload, payload_length, POLYCALL_PROTOCOL_FLAG_NONE)) {
        printf("Error: Failed to send message\n");
        return 1;
    }
    
    printf("Message sent successfully\n");
    return 0;
}

// Register protocol commands
void polycall_register_protocol_commands(void* registry) {
    if (!registry) {
        return;
    }
    
    // Define commands
    static const polycall_command_t init_cmd = {
        .execute = cmd_protocol_init,
        .name = "init",
        .description = "Initialize protocol subsystem",
        .usage = "protocol init [--max-message-size <size>]",
        .dependencies = (const char*[]){"core_context", NULL}
    };
    
    static const polycall_command_t state_cmd = {
        .execute = cmd_protocol_state,
        .name = "state",
        .description = "Show current protocol state",
        .usage = "protocol state",
        .dependencies = (const char*[]){"protocol_context", NULL}
    };
    
    static const polycall_command_t handshake_cmd = {
        .execute = cmd_protocol_handshake,
        .name = "handshake",
        .description = "Initiate protocol handshake",
        .usage = "protocol handshake",
        .dependencies = (const char*[]){"protocol_context", NULL}
    };
    
    static const polycall_command_t send_cmd = {
        .execute = cmd_protocol_send,
        .name = "send",
        .description = "Send protocol message",
        .usage = "protocol send <message_type> <payload>",
        .dependencies = (const char*[]){"protocol_context", NULL}
    };
    
    // Register commands
    polycall_command_registry_register(registry, "protocol", &init_cmd);
    polycall_command_registry_register(registry, "protocol", &state_cmd);
    polycall_command_registry_register(registry, "protocol", &handshake_cmd);
    polycall_command_registry_register(registry, "protocol", &send_cmd);
}