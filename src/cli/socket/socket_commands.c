/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file socket_commands.c
 * @brief CLI Commands for LibPolyCall WebSocket Functionality
 * @author Implementation based on existing LibPolyCall architecture
 *
 * Exposes WebSocket functionality to the CLI, including server creation,
 * client connections, message exchange, and security configuration.
 */

#include "polycall/cli/command.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/socket/polycall_socket.h"
#include "polycall/core/socket/polycall_socket_protocol.h"
#include "polycall/core/socket/polycall_socket_security.h"
#include <string.h>

// CLI context with socket dependency
typedef struct {
  polycall_core_context_t *core_ctx;
  polycall_socket_context_t *socket_ctx;
  polycall_auth_context_t *auth_ctx;
  command_registry_t *registry;
} socket_cli_context_t;

// Active connections table for the CLI
#define MAX_CLI_CONNECTIONS 16
static struct {
  polycall_socket_connection_t *connection;
  char name[64];
  bool in_use;
} active_connections[MAX_CLI_CONNECTIONS];

// Find connection by name or index
static polycall_socket_connection_t *
find_connection(const char *name_or_index) {
  // Try to parse as index first
  char *end;
  long index = strtol(name_or_index, &end, 10);

  if (*end == '\0' && index >= 0 && index < MAX_CLI_CONNECTIONS) {
    if (active_connections[index].in_use) {
      return active_connections[index].connection;
    }
  }

  // Try to find by name
  for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
    if (active_connections[i].in_use &&
        strcmp(active_connections[i].name, name_or_index) == 0) {
      return active_connections[i].connection;
    }
  }

  return NULL;
}

// Store connection with name
static int store_connection(polycall_socket_connection_t *connection,
                            const char *name) {
  for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
    if (!active_connections[i].in_use) {
      active_connections[i].connection = connection;
      active_connections[i].in_use = true;

      if (name && name[0] != '\0') {
        strncpy(active_connections[i].name, name,
                sizeof(active_connections[i].name) - 1);
      } else {
        snprintf(active_connections[i].name,
                 sizeof(active_connections[i].name) - 1, "conn_%d", i);
      }

      return i;
    }
  }

  return -1;
}

// Message handler callback for connections
static void socket_message_handler(polycall_socket_connection_t *connection,
                                   const void *data, size_t data_size,
                                   polycall_socket_data_type_t data_type,
                                   void *user_data) {
  // Find connection index
  int index = -1;
  for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
    if (active_connections[i].in_use &&
        active_connections[i].connection == connection) {
      index = i;
      break;
    }
  }

  // Print message info
  printf("Received message on connection %d (%s):\n", index,
         index >= 0 ? active_connections[index].name : "unknown");

  // Print data based on type
  if (data_type == POLYCALL_SOCKET_DATA_TEXT) {
    printf("Text message (%zu bytes): %.*s\n", data_size, (int)data_size,
           (const char *)data);
  } else {
    printf("Binary message (%zu bytes)\n", data_size);
    // Optionally print hex dump for binary data
  }
}

/*** Command implementations ***/

// Command: socket-init
static int cmd_socket_init(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx != NULL) {
    printf("Socket system already initialized\n");
    return 0;
  }

  // Create default config
  polycall_socket_config_t config = polycall_socket_create_default_config();

  // Parse options (if any)
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--max-connections") == 0 && i + 1 < argc) {
      config.max_connections = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
      config.connection_timeout_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--no-tls") == 0) {
      config.use_tls = false;
    } else if (strcmp(argv[i], "--ping-interval") == 0 && i + 1 < argc) {
      config.ping_interval_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
      config.worker_threads = atoi(argv[++i]);
    }
  }

  // Initialize socket context
  polycall_core_error_t error =
      polycall_socket_init(ctx->core_ctx, &ctx->socket_ctx, &config);

  if (error != POLYCALL_CORE_SUCCESS) {
    printf("Failed to initialize socket system: %s\n",
           polycall_core_error_to_string(error));
    return 1;
  }

  printf("Socket system initialized\n");
  return 0;
}

// Command: socket-connect
static int cmd_socket_connect(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx == NULL) {
    printf("Socket system not initialized. Use 'socket-init' first.\n");
    return 1;
  }

  if (argc < 2) {
    printf("Usage: socket-connect <url> [--name <name>] [--no-tls] [--timeout "
           "<ms>]\n");
    return 1;
  }

  const char *url = argv[1];
  const char *name = NULL;
  polycall_socket_connect_options_t options = {0};
  options.timeout_ms = 30000;
  options.use_tls = true;

  // Parse options
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
      name = argv[++i];
    } else if (strcmp(argv[i], "--no-tls") == 0) {
      options.use_tls = false;
    } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
      options.timeout_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--auto-reconnect") == 0) {
      options.auto_reconnect = true;
    }
  }

  // Connect to server
  polycall_socket_connection_t *connection = NULL;
  polycall_core_error_t error =
      polycall_socket_connect(ctx->socket_ctx, url, &options, &connection);

  if (error != POLYCALL_CORE_SUCCESS) {
    printf("Failed to connect: %s\n", polycall_core_error_to_string(error));
    return 1;
  }

  // Register message handler
  error = polycall_socket_register_handler(connection, socket_message_handler,
                                           NULL);

  if (error != POLYCALL_CORE_SUCCESS) {
    printf("Warning: Failed to register message handler: %s\n",
           polycall_core_error_to_string(error));
  }

  // Store connection
  int index = store_connection(connection, name);
  if (index < 0) {
    printf(
        "Warning: Failed to store connection. Too many active connections.\n");
  }

  printf("Connected to %s (index: %d, name: %s)\n", url, index,
         index >= 0 ? active_connections[index].name : "unknown");

  return 0;
}

// Command: socket-send
static int cmd_socket_send(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx == NULL) {
    printf("Socket system not initialized. Use 'socket-init' first.\n");
    return 1;
  }

  if (argc < 3) {
    printf(
        "Usage: socket-send <connection-name-or-index> <message> [--binary]\n");
    return 1;
  }

  // Find connection
  polycall_socket_connection_t *connection = find_connection(argv[1]);
  if (!connection) {
    printf("Connection not found: %s\n", argv[1]);
    return 1;
  }

  // Determine data type
  polycall_socket_data_type_t data_type = POLYCALL_SOCKET_DATA_TEXT;
  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "--binary") == 0) {
      data_type = POLYCALL_SOCKET_DATA_BINARY;
      break;
    }
  }

  // Send message
  polycall_core_error_t error =
      polycall_socket_send(connection, argv[2], strlen(argv[2]), data_type);

  if (error != POLYCALL_CORE_SUCCESS) {
    printf("Failed to send message: %s\n",
           polycall_core_error_to_string(error));
    return 1;
  }

  printf("Message sent\n");
  return 0;
}

// Command: socket-close
static int cmd_socket_close(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx == NULL) {
    printf("Socket system not initialized. Use 'socket-init' first.\n");
    return 1;
  }

  if (argc < 2) {
    printf("Usage: socket-close <connection-name-or-index> [--code "
           "<close-code>] [--reason <reason>]\n");
    return 1;
  }

  // Find connection
  polycall_socket_connection_t *connection = find_connection(argv[1]);
  if (!connection) {
    printf("Connection not found: %s\n", argv[1]);
    return 1;
  }

  // Parse options
  uint16_t close_code = 1000; // Normal closure
  const char *reason = "Closed by user";

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--code") == 0 && i + 1 < argc) {
      close_code = (uint16_t)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--reason") == 0 && i + 1 < argc) {
      reason = argv[++i];
    }
  }

  // Close connection
  polycall_core_error_t error =
      polycall_socket_close(connection, close_code, reason);

  if (error != POLYCALL_CORE_SUCCESS) {
    printf("Failed to close connection: %s\n",
           polycall_core_error_to_string(error));
    return 1;
  }

  // Find connection in active list and mark as not in use
  for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
    if (active_connections[i].in_use &&
        active_connections[i].connection == connection) {
      printf("Closed connection %d (%s)\n", i, active_connections[i].name);
      active_connections[i].in_use = false;
      break;
    }
  }

  return 0;
}

// Command: socket-list
static int cmd_socket_list(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx == NULL) {
    printf("Socket system not initialized. Use 'socket-init' first.\n");
    return 1;
  }

  printf("Active WebSocket connections:\n");
  printf("--------------------------------------------------------------\n");
  printf("| Index | Name                 | URL                         |\n");
  printf("--------------------------------------------------------------\n");

  int count = 0;
  for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
    if (active_connections[i].in_use) {
      printf("| %-5d | %-20s | %-27s |\n", i, active_connections[i].name,
             active_connections[i].connection->url);
      count++;
    }
  }

  printf("--------------------------------------------------------------\n");
  printf("Total: %d active connection(s)\n", count);

  return 0;
}

// Command: socket-server
static int cmd_socket_server(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx == NULL) {
    printf("Socket system not initialized. Use 'socket-init' first.\n");
    return 1;
  }

  if (argc < 3) {
    printf(
        "Usage: socket-server start <bind-address> <port> [--name <name>]\n");
    printf("       socket-server stop <server-name-or-index>\n");
    return 1;
  }

  // TODO: Implement server management
  printf("WebSocket server functionality not yet implemented\n");

  return 0;
}

// Command: socket-auth
static int cmd_socket_auth(int argc, char **argv, void *user_data) {
  socket_cli_context_t *ctx = (socket_cli_context_t *)user_data;

  if (ctx->socket_ctx == NULL || ctx->auth_ctx == NULL) {
    printf("Socket or auth system not initialized.\n");
    return 1;
  }

  if (argc < 2) {
    printf("Usage: socket-auth configure [--token-auth <on|off>] "
           "[--require-secure <on|off>]\n");
    printf("       socket-auth token <connection-name-or-index> <token>\n");
    return 1;
  }

  // TODO: Implement authentication configuration
  printf("WebSocket authentication functionality not yet implemented\n");

  return 0;
}

// Function to register all socket commands
void register_socket_commands(command_registry_t *registry,
                              polycall_core_context_t *core_ctx,
                              polycall_auth_context_t *auth_ctx) {
  // Create context for commands
  socket_cli_context_t *ctx = malloc(sizeof(socket_cli_context_t));
  if (!ctx) {
    fprintf(stderr, "Failed to allocate socket CLI context\n");
    return;
  }

  ctx->core_ctx = core_ctx;
  ctx->socket_ctx = NULL; // Will be initialized by socket-init command
  ctx->auth_ctx = auth_ctx;
  ctx->registry = registry;

  // Register commands
  command_register(registry, "socket-init", "Initialize WebSocket subsystem",
                   cmd_socket_init, ctx);
  command_register(registry, "socket-connect", "Connect to WebSocket server",
                   cmd_socket_connect, ctx);
  command_register(registry, "socket-send",
                   "Send message over WebSocket connection", cmd_socket_send,
                   ctx);
  command_register(registry, "socket-close", "Close WebSocket connection",
                   cmd_socket_close, ctx);
  command_register(registry, "socket-list", "List active WebSocket connections",
                   cmd_socket_list, ctx);
  command_register(registry, "socket-server", "Manage WebSocket servers",
                   cmd_socket_server, ctx);
    command_register(registry, "socket-auth", "Configure WebSocket authentic
