/**
 * @file polycall_socket_security.h
 * @brief WebSocket Security Integration for LibPolyCall
 * @author Implementation based on existing LibPolyCall architecture
 */

#ifndef POLYCALL_SOCKET_SECURITY_H
#define POLYCALL_SOCKET_SECURITY_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/auth/polycall_auth.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for socket context
typedef struct polycall_socket_context polycall_socket_context_t;

// Socket security context
typedef struct polycall_socket_security_context polycall_socket_security_context_t;

// Maximum scopes and roles per connection
#define POLYCALL_MAX_SOCKET_SCOPES 16
#define POLYCALL_MAX_SOCKET_ROLES 8

/**
 * @brief Authentication result structure
 */
typedef struct {
    bool authenticated;                      // Authentication success flag
    char identity[128];                      // Authenticated identity
    char scopes[POLYCALL_MAX_SOCKET_SCOPES][64]; // Authorized scopes
    size_t scope_count;                      // Number of scopes
    char roles[POLYCALL_MAX_SOCKET_ROLES][64];   // User roles
    size_t role_count;                       // Number of roles
    uint64_t token_expiry;                   // Token expiry timestamp
    char error_message[256];                 // Error message (if authentication failed)
} polycall_socket_auth_result_t;

/**
 * @brief Initialize socket security context
 */
polycall_core_error_t polycall_socket_security_init(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    polycall_socket_security_context_t** security_ctx
);

/**
 * @brief Configure socket security
 */
polycall_core_error_t polycall_socket_security_configure(
    polycall_socket_security_context_t* security_ctx,
    bool enable_token_auth,
    bool require_secure_connection,
    uint32_t max_auth_failures,
    uint32_t token_refresh_interval
);

/**
 * @brief Authenticate WebSocket connection
 */
polycall_core_error_t polycall_socket_authenticate(
    polycall_socket_security_context_t* security_ctx,
    const char* token,
    polycall_socket_auth_result_t* result
);

/**
 * @brief Check if connection is authorized for a specific scope
 */
bool polycall_socket_is_authorized(
    polycall_socket_security_context_t* security_ctx,
    const polycall_socket_auth_result_t* auth_result,
    const char* required_scope
);

/**
 * @brief Check if connection has a specific role
 */
bool polycall_socket_has_role(
    polycall_socket_security_context_t* security_ctx,
    const polycall_socket_auth_result_t* auth_result,
    const char* required_role
);

/**
 * @brief Generate a challenge for WebSocket authentication
 */
polycall_core_error_t polycall_socket_generate_challenge(
    polycall_socket_security_context_t* security_ctx,
    char* challenge_buffer,
    size_t buffer_size
);

/**
 * @brief Verify a challenge response for WebSocket authentication
 */
polycall_core_error_t polycall_socket_verify_challenge(
    polycall_socket_security_context_t* security_ctx,
    const char* challenge,
    const char* response,
    const char* token,
    bool* valid
);

/**
 * @brief Check if a token needs to be refreshed
 */
bool polycall_socket_token_needs_refresh(
    polycall_socket_security_context_t* security_ctx,
    const polycall_socket_auth_result_t* auth_result,
    uint64_t current_time
);

/**
 * @brief Clean up socket security context
 */
void polycall_socket_security_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_socket_security_context_t* security_ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_SOCKET_SECURITY_H */
