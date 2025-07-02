// polycall/core/auth/polycall_auth_adapter.c
#include "polycall/core/auth/polycall_auth_adapter.h"
#include <string.h>

// Generate a session identifier
polycall_core_error_t polycall_auth_generate_session_id(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    char* buffer,
    size_t buffer_size) {
    
    if (!core_ctx || !auth_ctx || !buffer || buffer_size < POLYCALL_MAX_ID_LEN) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create new identifier
    polycall_identifier_t identifier;
    polycall_core_error_t result = polycall_identifier_create(
        core_ctx, &identifier, auth_ctx->id_format);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Copy to output buffer
    strncpy(buffer, identifier.string, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return POLYCALL_CORE_SUCCESS;
}

// Generate a token identifier
polycall_core_error_t polycall_auth_generate_token_id(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    polycall_token_type_t token_type,
    char* buffer,
    size_t buffer_size) {
    
    if (!core_ctx || !auth_ctx || !buffer || buffer_size < POLYCALL_MAX_ID_LEN) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create new identifier
    polycall_identifier_t identifier;
    polycall_core_error_t result = polycall_identifier_create(
        core_ctx, &identifier, auth_ctx->id_format);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Add token type prefix
    char prefix = 'X';  // Default
    switch (token_type) {
        case POLYCALL_TOKEN_TYPE_ACCESS:
            prefix = 'A';
            break;
        case POLYCALL_TOKEN_TYPE_REFRESH:
            prefix = 'R';
            break;
        case POLYCALL_TOKEN_TYPE_API_KEY:
            prefix = 'K';
            break;
    }
    
    // Create token ID with prefix and identifier
    if (buffer_size < POLYCALL_MAX_ID_LEN + 2) {
        return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
    }
    
    buffer[0] = prefix;
    buffer[1] = '-';
    strncpy(buffer + 2, identifier.string, buffer_size - 3);
    buffer[buffer_size - 1] = '\0';
    
    return POLYCALL_CORE_SUCCESS;
}

// Set identifier format for auth component
polycall_core_error_t polycall_auth_set_identifier_format(
    polycall_core_context_t* core_ctx,
    polycall_auth_context_t* auth_ctx,
    polycall_identifier_format_t format) {
    
    if (!core_ctx || !auth_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    auth_ctx->id_format = format;
    
    return POLYCALL_CORE_SUCCESS;
}