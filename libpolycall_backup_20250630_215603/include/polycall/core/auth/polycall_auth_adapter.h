// polycall/core/auth/polycall_auth_adapter.h
#ifndef POLYCALL_AUTH_ADAPTER_H
#define POLYCALL_AUTH_ADAPTER_H

#include "polycall/core/common/polycall_identifier.h"
#include "polycall/core/auth/polycall_auth_context.h"

// Generate a unique session identifier
polycall_core_error_t polycall_auth_generate_session_id(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    char* buffer,
    size_t buffer_size);

// Generate a unique token identifier
polycall_core_error_t polycall_auth_generate_token_id(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    polycall_token_type_t token_type,
    char* buffer,
    size_t buffer_size);

// Set preferred identifier format for auth component
polycall_core_error_t polycall_auth_set_identifier_format(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    polycall_identifier_format_t format);

#endif // POLYCALL_AUTH_ADAPTER_H