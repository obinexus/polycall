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
#include "polycall/core/auth/auth_integration.h"

 * @file auth_integration.c
 * @brief Implementation of authentication integration with other LibPolyCall
components
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the interfaces for integrating authentication with
 * other LibPolyCall components, including protocol, micro command, edge
command,
 * and telemetry subsystems.
 */

#include "polycall/core/auth/polycall_auth_audit.h"
#include "polycall/core/auth/polycall_auth_context.h"
#include "polycall/core/auth/polycall_auth_integration.h"
#include "polycall/core/auth/polycall_auth_token.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for message handler callbacks
static polycall_core_error_t handle_auth_login_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response);

static polycall_core_error_t handle_auth_token_refresh_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response);

static polycall_core_error_t handle_auth_token_validate_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response);

static polycall_core_error_t handle_auth_token_revoke_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response);

// Protocol message authentication middleware
static polycall_core_error_t
protocol_auth_middleware(polycall_core_context_t *core_ctx,
                         polycall_auth_context_t *auth_ctx,
                         polycall_protocol_context_t *protocol_ctx,
                         polycall_message_t *message, bool *allowed);

// Micro command authentication middleware
static polycall_core_error_t micro_auth_middleware(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_micro_context_t *micro_ctx, const char *command_name,
    const char *token, bool *allowed, char **identity_id);

// Edge command authentication middleware
static polycall_core_error_t edge_auth_middleware(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_edge_context_t *edge_ctx, const char *command_name,
    const char *token, bool *allowed, char **identity_id);

// Telemetry authentication event handler
static void telemetry_auth_event_handler(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_telemetry_context_t *telemetry_ctx, const audit_event_t *event);

/**
 * @brief Register with the protocol system
 */
polycall_core_error_t polycall_auth_register_with_protocol(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Store protocol context in integrator
  auth_ctx->integrator->protocol_hook.context = protocol_ctx;

  // Register authentication middleware with protocol
  polycall_core_error_t result = polycall_protocol_register_auth_middleware(
      core_ctx, protocol_ctx, protocol_auth_middleware, auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Set up message handlers for authentication-related messages
  result =
      polycall_auth_setup_message_handlers(core_ctx, auth_ctx, protocol_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Create audit event
  audit_event_t *event = polycall_auth_create_audit_event(
      core_ctx, POLYCALL_AUDIT_EVENT_CUSTOM, NULL, "protocol", "register", true,
      NULL);

  if (event) {
    event->details = strdup("{\"component\":\"protocol\"}");
    polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
    polycall_auth_free_audit_event(core_ctx, event);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register with the micro command system
 */
polycall_core_error_t
polycall_auth_register_with_micro(polycall_core_context_t *core_ctx,
                                  polycall_auth_context_t *auth_ctx,
                                  polycall_micro_context_t *micro_ctx) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !micro_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Store micro context in integrator
  auth_ctx->integrator->micro_hook.context = micro_ctx;

  // Register authentication middleware with micro command system
  polycall_core_error_t result = polycall_micro_register_auth_middleware(
      core_ctx, micro_ctx, (micro_auth_middleware_t)micro_auth_middleware,
      auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Create audit event
  audit_event_t *event =
      polycall_auth_create_audit_event(core_ctx, POLYCALL_AUDIT_EVENT_CUSTOM,
                                       NULL, "micro", "register", true, NULL);

  if (event) {
    event->details = strdup("{\"component\":\"micro\"}");
    polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
    polycall_auth_free_audit_event(core_ctx, event);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register with the edge command system
 */
polycall_core_error_t
polycall_auth_register_with_edge(polycall_core_context_t *core_ctx,
                                 polycall_auth_context_t *auth_ctx,
                                 polycall_edge_context_t *edge_ctx) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !edge_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Store edge context in integrator
  auth_ctx->integrator->edge_hook.context = edge_ctx;

  // Register authentication middleware with edge command system
  polycall_core_error_t result = polycall_edge_register_auth_middleware(
      core_ctx, edge_ctx, (edge_auth_middleware_t)edge_auth_middleware,
      auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Create audit event
  audit_event_t *event =
      polycall_auth_create_audit_event(core_ctx, POLYCALL_AUDIT_EVENT_CUSTOM,
                                       NULL, "edge", "register", true, NULL);

  if (event) {
    event->details = strdup("{\"component\":\"edge\"}");
    polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
    polycall_auth_free_audit_event(core_ctx, event);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register with the telemetry system
 */
polycall_core_error_t polycall_auth_register_with_telemetry(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_telemetry_context_t *telemetry_ctx) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !telemetry_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Store telemetry context in integrator
  auth_ctx->integrator->telemetry_hook.context = telemetry_ctx;

  // Register authentication event handler with telemetry system
  polycall_core_error_t result = polycall_telemetry_register_auth_events(
      core_ctx, telemetry_ctx,
      (telemetry_auth_event_handler_t)telemetry_auth_event_handler, auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Create audit event
  audit_event_t *event = polycall_auth_create_audit_event(
      core_ctx, POLYCALL_AUDIT_EVENT_CUSTOM, NULL, "telemetry", "register",
      true, NULL);

  if (event) {
    event->details = strdup("{\"component\":\"telemetry\"}");
    polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
    polycall_auth_free_audit_event(core_ctx, event);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Validate a protocol message
 */
polycall_core_error_t polycall_auth_validate_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    const polycall_message_t *message, char **identity_id, bool *allowed) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !message || !identity_id || !allowed) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Initialize result
  *identity_id = NULL;
  *allowed = false;

  // Check if message type is exempt from authentication
  if (polycall_message_is_auth_exempt(core_ctx, message)) {
    *allowed = true;
    return POLYCALL_CORE_SUCCESS;
  }

  // Get token from message
  const char *token = polycall_message_get_auth_token(core_ctx, message);
  if (!token) {
    // No token provided
    return POLYCALL_CORE_SUCCESS;
  }

  // Validate token
  polycall_core_error_t result =
      polycall_auth_validate_token(core_ctx, auth_ctx, token, identity_id);

  if (result != POLYCALL_CORE_SUCCESS || !*identity_id) {
    return result;
  }

  // Check permission for this message
  const char *resource = polycall_message_get_resource(core_ctx, message);
  const char *action = polycall_message_get_action(core_ctx, message);

  if (!resource || !action) {
    // Cannot determine resource or action
    return POLYCALL_CORE_SUCCESS;
  }

  result = polycall_auth_check_permission(core_ctx, auth_ctx, *identity_id,
                                          resource, action, allowed);

  if (result != POLYCALL_CORE_SUCCESS) {
    if (*identity_id) {
      polycall_core_free(core_ctx, *identity_id);
      *identity_id = NULL;
    }
    return result;
  }

  // If not allowed, log access denied
  if (!*allowed) {
    audit_event_t *event = polycall_auth_create_audit_event(
        core_ctx, POLYCALL_AUDIT_EVENT_ACCESS_DENIED, *identity_id, resource,
        action, false, "Permission denied for message");

    if (event) {
      polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
      polycall_auth_free_audit_event(core_ctx, event);
    }
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Set up authentication message handlers
 */
polycall_core_error_t polycall_auth_setup_message_handlers(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Register login message handler
  polycall_core_error_t result = polycall_protocol_register_message_handler(
      core_ctx, protocol_ctx, "auth.login",
      (protocol_message_handler_t)handle_auth_login_message, auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Register token refresh message handler
  result = polycall_protocol_register_message_handler(
      core_ctx, protocol_ctx, "auth.token.refresh",
      (protocol_message_handler_t)handle_auth_token_refresh_message, auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Register token validation message handler
  result = polycall_protocol_register_message_handler(
      core_ctx, protocol_ctx, "auth.token.validate",
      (protocol_message_handler_t)handle_auth_token_validate_message, auth_ctx);

  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Register token revocation message handler
  result = polycall_protocol_register_message_handler(
      core_ctx, protocol_ctx, "auth.token.revoke",
      (protocol_message_handler_t)handle_auth_token_revoke_message, auth_ctx);

  return result;
}

/**
 * @brief Initialize auth integrator
 * Internal function called from auth_context.c
 */
auth_integrator_t *init_auth_integrator(polycall_core_context_t *ctx) {
  if (!ctx) {
    return NULL;
  }

  // Allocate integrator context
  auth_integrator_t *integrator =
      polycall_core_malloc(ctx, sizeof(auth_integrator_t));
  if (!integrator) {
    return NULL;
  }

  // Initialize integrator context
  memset(integrator, 0, sizeof(auth_integrator_t));
  integrator->core_ctx = ctx;

  return integrator;
}

/**
 * @brief Cleanup auth integrator
 * Internal function called from auth_context.c
 */
void cleanup_auth_integrator(polycall_core_context_t *ctx,
                             auth_integrator_t *integrator) {
  if (!ctx || !integrator) {
    return;
  }

  // No need to free hook contexts as they are owned by other subsystems

  // Free integrator context
  polycall_core_free(ctx, integrator);
}

/**
 * @brief Handle authentication login message
 */
static polycall_core_error_t handle_auth_login_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx || !message || !response) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get username and password from message
  const char *username =
      polycall_message_get_param(core_ctx, message, "username");
  const char *password =
      polycall_message_get_param(core_ctx, message, "password");

  if (!username || !password) {
    // Create error response
    *response = polycall_message_create_error(
        core_ctx, message, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
        "Missing username or password");

    return POLYCALL_CORE_SUCCESS;
  }

  // Authenticate user
  char *access_token = NULL;
  char *refresh_token = NULL;

  polycall_core_error_t result = polycall_auth_authenticate(
      core_ctx, auth_ctx, username, password, &access_token, &refresh_token);

  if (result != POLYCALL_CORE_SUCCESS || !access_token || !refresh_token) {
    // Create error response
    const char *error_message = (result == POLYCALL_CORE_ERROR_ACCESS_DENIED)
                                    ? "Invalid username or password"
                                    : "Authentication failed";

    *response =
        polycall_message_create_error(core_ctx, message, result, error_message);

    return POLYCALL_CORE_SUCCESS;
  }

  // Create success response
  *response = polycall_message_create_response(core_ctx, message);
  if (!*response) {
    polycall_core_free(core_ctx, access_token);
    polycall_core_free(core_ctx, refresh_token);
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Add tokens to response
  polycall_message_set_param(core_ctx, *response, "access_token", access_token);
  polycall_message_set_param(core_ctx, *response, "refresh_token",
                             refresh_token);

  // Free tokens
  polycall_core_free(core_ctx, access_token);
  polycall_core_free(core_ctx, refresh_token);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Handle token refresh message
 */
static polycall_core_error_t handle_auth_token_refresh_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx || !message || !response) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get refresh token from message
  const char *refresh_token =
      polycall_message_get_param(core_ctx, message, "refresh_token");

  if (!refresh_token) {
    // Create error response
    *response = polycall_message_create_error(
        core_ctx, message, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
        "Missing refresh token");

    return POLYCALL_CORE_SUCCESS;
  }

  // Refresh token
  char *access_token = NULL;

  polycall_core_error_t result = polycall_auth_refresh_token(
      core_ctx, auth_ctx, refresh_token, &access_token);

  if (result != POLYCALL_CORE_SUCCESS || !access_token) {
    // Create error response
    *response = polycall_message_create_error(core_ctx, message, result,
                                              "Failed to refresh token");

    return POLYCALL_CORE_SUCCESS;
  }

  // Create success response
  *response = polycall_message_create_response(core_ctx, message);
  if (!*response) {
    polycall_core_free(core_ctx, access_token);
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Add access token to response
  polycall_message_set_param(core_ctx, *response, "access_token", access_token);

  // Free token
  polycall_core_free(core_ctx, access_token);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Handle token validation message
 */
static polycall_core_error_t handle_auth_token_validate_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx || !message || !response) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get token from message
  const char *token = polycall_message_get_param(core_ctx, message, "token");

  if (!token) {
    // Create error response
    *response = polycall_message_create_error(
        core_ctx, message, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
        "Missing token");

    return POLYCALL_CORE_SUCCESS;
  }

  // Validate token
  char *identity_id = NULL;

  polycall_core_error_t result =
      polycall_auth_validate_token(core_ctx, auth_ctx, token, &identity_id);

  if (result != POLYCALL_CORE_SUCCESS || !identity_id) {
    // Create error response
    *response = polycall_message_create_error(core_ctx, message, result,
                                              "Invalid token");

    return POLYCALL_CORE_SUCCESS;
  }

  // Create success response
  *response = polycall_message_create_response(core_ctx, message);
  if (!*response) {
    polycall_core_free(core_ctx, identity_id);
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Add identity ID to response
  polycall_message_set_param(core_ctx, *response, "identity_id", identity_id);
  polycall_message_set_param(core_ctx, *response, "is_valid", "true");

  // Free identity ID
  polycall_core_free(core_ctx, identity_id);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Handle token revocation message
 */
static polycall_core_error_t handle_auth_token_revoke_message(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_protocol_context_t *protocol_ctx, polycall_message_t *message,
    polycall_message_t **response) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx || !message || !response) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get token from message
  const char *token = polycall_message_get_param(core_ctx, message, "token");

  if (!token) {
    // Create error response
    *response = polycall_message_create_error(
        core_ctx, message, POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
        "Missing token");

    return POLYCALL_CORE_SUCCESS;
  }

  // Revoke token
  polycall_core_error_t result =
      polycall_auth_revoke_token(core_ctx, auth_ctx, token);

  if (result != POLYCALL_CORE_SUCCESS) {
    // Create error response
    *response = polycall_message_create_error(core_ctx, message, result,
                                              "Failed to revoke token");

    return POLYCALL_CORE_SUCCESS;
  }

  // Create success response
  *response = polycall_message_create_response(core_ctx, message);
  if (!*response) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Add success indicator to response
  polycall_message_set_param(core_ctx, *response, "revoked", "true");

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Protocol message authentication middleware
 */
static polycall_core_error_t
protocol_auth_middleware(polycall_core_context_t *core_ctx,
                         polycall_auth_context_t *auth_ctx,
                         polycall_protocol_context_t *protocol_ctx,
                         polycall_message_t *message, bool *allowed) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !protocol_ctx || !message || !allowed) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Initialize result
  *allowed = false;

  // Check if message type is exempt from authentication
  const char *message_type = polycall_message_get_type(core_ctx, message);

  // List of exempt message types (authentication-related)
  const char *exempt_types[] = {"auth.login", "auth.token.refresh",
                                "auth.token.validate", NULL};

  // Check if message type is in exempt list
  for (const char **type = exempt_types; *type != NULL; type++) {
    if (strcmp(message_type, *type) == 0) {
      *allowed = true;
      return POLYCALL_CORE_SUCCESS;
    }
  }

  // For non-exempt messages, validate token
  char *identity_id = NULL;

  polycall_core_error_t result = polycall_auth_validate_message(
      core_ctx, auth_ctx, message, &identity_id, allowed);

  if (result != POLYCALL_CORE_SUCCESS) {
    if (identity_id) {
      polycall_core_free(core_ctx, identity_id);
    }
    return result;
  }

  // If allowed, store identity ID in message for later use
  if (*allowed && identity_id) {
    polycall_message_set_identity(core_ctx, message, identity_id);
  }

  // Free identity ID
  if (identity_id) {
    polycall_core_free(core_ctx, identity_id);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Micro command authentication middleware
 */
static polycall_core_error_t micro_auth_middleware(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_micro_context_t *micro_ctx, const char *command_name,
    const char *token, bool *allowed, char **identity_id) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !micro_ctx || !command_name || !allowed ||
      !identity_id) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Initialize result
  *allowed = false;
  *identity_id = NULL;

  // Check if command is exempt from authentication
  // List of exempt commands (authentication-related)
  const char *exempt_commands[] = {"auth.login", "auth.token.refresh",
                                   "auth.token.validate", NULL};

  // Check if command is in exempt list
  for (const char **cmd = exempt_commands; *cmd != NULL; cmd++) {
    if (strcmp(command_name, *cmd) == 0) {
      *allowed = true;
      return POLYCALL_CORE_SUCCESS;
    }
  }

  // For non-exempt commands, token is required
  if (!token) {
    return POLYCALL_CORE_SUCCESS;
  }

  // Validate token
  polycall_core_error_t result =
      polycall_auth_validate_token(core_ctx, auth_ctx, token, identity_id);

  if (result != POLYCALL_CORE_SUCCESS || !*identity_id) {
    if (*identity_id) {
      polycall_core_free(core_ctx, *identity_id);
      *identity_id = NULL;
    }
    return result;
  }

  // Check permission for this command
  char resource[256];
  snprintf(resource, sizeof(resource), "micro:%s", command_name);

  result = polycall_auth_check_permission(core_ctx, auth_ctx, *identity_id,
                                          resource, "execute", allowed);

  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_core_free(core_ctx, *identity_id);
    *identity_id = NULL;
    return result;
  }

  // If not allowed, log access denied
  if (!*allowed) {
    audit_event_t *event = polycall_auth_create_audit_event(
        core_ctx, POLYCALL_AUDIT_EVENT_ACCESS_DENIED, *identity_id, resource,
        "execute", false, "Permission denied for micro command");

    if (event) {
      polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
      polycall_auth_free_audit_event(core_ctx, event);
    }

    polycall_core_free(core_ctx, *identity_id);
    *identity_id = NULL;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Edge command authentication middleware
 */
static polycall_core_error_t edge_auth_middleware(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_edge_context_t *edge_ctx, const char *command_name,
    const char *token, bool *allowed, char **identity_id) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !edge_ctx || !command_name || !allowed ||
      !identity_id) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Initialize result
  *allowed = false;
  *identity_id = NULL;

  // Check if command is exempt from authentication
  // List of exempt commands (authentication-related)
  const char *exempt_commands[] = {"auth.login", "auth.token.refresh",
                                   "auth.token.validate", NULL};

  // Check if command is in exempt list
  for (const char **cmd = exempt_commands; *cmd != NULL; cmd++) {
    if (strcmp(command_name, *cmd) == 0) {
      *allowed = true;
      return POLYCALL_CORE_SUCCESS;
    }
  }

  // For non-exempt commands, token is required
  if (!token) {
    return POLYCALL_CORE_SUCCESS;
  }

  // Validate token
  polycall_core_error_t result =
      polycall_auth_validate_token(core_ctx, auth_ctx, token, identity_id);

  if (result != POLYCALL_CORE_SUCCESS || !*identity_id) {
    if (*identity_id) {
      polycall_core_free(core_ctx, *identity_id);
      *identity_id = NULL;
    }
    return result;
  }

  // Check permission for this command
  char resource[256];
  snprintf(resource, sizeof(resource), "edge:%s", command_name);

  result = polycall_auth_check_permission(core_ctx, auth_ctx, *identity_id,
                                          resource, "execute", allowed);

  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_core_free(core_ctx, *identity_id);
    *identity_id = NULL;
    return result;
  }

  // If not allowed, log access denied
  if (!*allowed) {
    audit_event_t *event = polycall_auth_create_audit_event(
        core_ctx, POLYCALL_AUDIT_EVENT_ACCESS_DENIED, *identity_id, resource,
        "execute", false, "Permission denied for edge command");

    if (event) {
      polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
      polycall_auth_free_audit_event(core_ctx, event);
    }

    polycall_core_free(core_ctx, *identity_id);
    *identity_id = NULL;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Telemetry authentication event handler
 */
static void telemetry_auth_event_handler(
    polycall_core_context_t *core_ctx, polycall_auth_context_t *auth_ctx,
    polycall_telemetry_context_t *telemetry_ctx, const audit_event_t *event) {
  // Validate parameters
  if (!core_ctx || !auth_ctx || !telemetry_ctx || !event) {
    return;
  }

  // Convert audit event to telemetry event
  // This would typically involve mapping audit event fields to telemetry
  // metrics

  // Example: Track authentication attempts
  if (event->type == POLYCALL_AUDIT_EVENT_LOGIN) {
    polycall_telemetry_track_counter(
        core_ctx, telemetry_ctx, "auth.login.attempts", 1,
        event->success ? "result:success" : "result:failure");
  }

  // Example: Track token validations
  if (event->type == POLYCALL_AUDIT_EVENT_TOKEN_VALIDATE) {
    polycall_telemetry_track_counter(
        core_ctx, telemetry_ctx, "auth.token.validations", 1,
        event->success ? "result:success" : "result:failure");
  }

  // Example: Track access denied events
  if (event->type == POLYCALL_AUDIT_EVENT_ACCESS_DENIED) {
    polycall_telemetry_track_counter(
        core_ctx, telemetry_ctx, "auth.access.denied", 1,
        event->resource ? event->resource : "resource:unknown");
  }
}
