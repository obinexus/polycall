/**
 * @file polycall_token.c
 * @brief Implementation of the token management system for PolyCall protocol
 * @author LibPolyCall Implementation Team
 */

#include "polycall/core/protocol/polycall_token.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <string.h>
#include <time.h>

/**
 * @brief Maximum registered tokens
 */
#define POLYCALL_TOKEN_MAX_REGISTERED 256

/**
 * @brief Token data structure
 */
struct polycall_token {
    char* content;              /**< Token content string */
    size_t content_len;         /**< Content length */
    uint64_t creation_time;     /**< Creation timestamp (milliseconds) */
    uint64_t expiration_time;   /**< Expiration time (milliseconds) */
    bool is_registered;         /**< Registration status */
};

/**
 * @brief Token context data structure
 */
struct polycall_token_context {
    polycall_core_context_t* core_ctx;            /**< Core context */
    polycall_token_t* registered_tokens[POLYCALL_TOKEN_MAX_REGISTERED]; /**< Registered tokens */
    size_t registered_count;                      /**< Number of registered tokens */
};

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/**
 * @brief Create a token context
 */
polycall_token_error_t polycall_token_context_create(
    polycall_core_context_t* core_ctx,
    polycall_token_context_t** token_ctx
) {
    if (!core_ctx || !token_ctx) {
        return POLYCALL_TOKEN_ERROR_INVALID_PARAMETER;
    }
    
    polycall_token_context_t* ctx = polycall_memory_alloc(core_ctx, sizeof(polycall_token_context_t));
    if (!ctx) {
        return POLYCALL_TOKEN_ERROR_OUT_OF_MEMORY;
    }
    
    memset(ctx, 0, sizeof(polycall_token_context_t));
    ctx->core_ctx = core_ctx;
    ctx->registered_count = 0;
    
    *token_ctx = ctx;
    return POLYCALL_TOKEN_ERROR_NONE;
}

/**
 * @brief Destroy a token context
 */
void polycall_token_context_destroy(polycall_token_context_t* token_ctx) {
    if (!token_ctx) {
        return;
    }
    
    // Unregister and destroy all tokens
    for (size_t i = 0; i < token_ctx->registered_count; i++) {
        if (token_ctx->registered_tokens[i]) {
            token_ctx->registered_tokens[i]->is_registered = false;
        }
    }
    
    polycall_memory_free(token_ctx->core_ctx, token_ctx);
}

/**
 * @brief Create a token
 */
polycall_token_error_t polycall_token_create(
    polycall_token_context_t* token_ctx,
    const char* content,
    polycall_token_t** token
) {
    if (!token_ctx || !content || !token) {
        return POLYCALL_TOKEN_ERROR_INVALID_PARAMETER;
    }
    
    size_t content_len = strlen(content);
    if (content_len == 0) {
        return POLYCALL_TOKEN_ERROR_INVALID_PARAMETER;
    }
    
    polycall_token_t* new_token = polycall_memory_alloc(token_ctx->core_ctx, sizeof(polycall_token_t));
    if (!new_token) {
        return POLYCALL_TOKEN_ERROR_OUT_OF_MEMORY;
    }
    
    new_token->content = polycall_memory_alloc(token_ctx->core_ctx, content_len + 1);
    if (!new_token->content) {
        polycall_memory_free(token_ctx->core_ctx, new_token);
        return POLYCALL_TOKEN_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(new_token->content, content, content_len);
    new_token->content[content_len] = '\0';
    new_token->content_len = content_len;
    new_token->creation_time = get_current_time_ms();
    new_token->expiration_time = 0; // No expiration by default
    new_token->is_registered = false;
    
    *token = new_token;
    return POLYCALL_TOKEN_ERROR_NONE;
}

/**
 * @brief Destroy a token
 */
void polycall_token_destroy(polycall_token_t* token) {
    if (!token) {
        return;
    }
    
    if (token->content) {
        polycall_memory_free(NULL, token->content);
        token->content = NULL;
    }
    
    polycall_memory_free(NULL, token);
}

/**
 * @brief Register a token for validation
 */
polycall_token_error_t polycall_token_register(
    polycall_token_context_t* token_ctx,
    polycall_token_t* token
) {
    if (!token_ctx || !token) {
        return POLYCALL_TOKEN_ERROR_INVALID_PARAMETER;
    }
    
    if (token->is_registered) {
        return POLYCALL_TOKEN_ERROR_ALREADY_EXISTS;
    }
    
    if (token_ctx->registered_count >= POLYCALL_TOKEN_MAX_REGISTERED) {
        return POLYCALL_TOKEN_ERROR_INTERNAL;
    }
    
    token_ctx->registered_tokens[token_ctx->registered_count++] = token;
    token->is_registered = true;
    
    return POLYCALL_TOKEN_ERROR_NONE;
}

/**
 * @brief Unregister a token
 */
polycall_token_error_t polycall_token_unregister(
    polycall_token_context_t* token_ctx,
    polycall_token_t* token
) {
    if (!token_ctx || !token) {
        return POLYCALL_TOKEN_ERROR_INVALID_PARAMETER;
    }
    
    if (!token->is_registered) {
        return POLYCALL_TOKEN_ERROR_NOT_FOUND;
    }
    
    // Find the token in the registered list
    size_t index = 0;
    for (; index < token_ctx->registered_count; index++) {
        if (token_ctx->registered_tokens[index] == token) {
            break;
        }
    }
    
    if (index == token_ctx->registered_count) {
        return POLYCALL_TOKEN_ERROR_NOT_FOUND;
    }
    
    // Remove the token by shifting remaining tokens
    for (size_t i = index; i < token_ctx->registered_count - 1; i++) {
        token_ctx->registered_tokens[i] = token_ctx->registered_tokens[i + 1];
    }
    
    token_ctx->registered_count--;
    token->is_registered = false;
    
    return POLYCALL_TOKEN_ERROR_NONE;
}

/**
 * @brief Validate a token
 */
bool polycall_token_validate(
    polycall_token_context_t* token_ctx,
    const char* content
) {
    if (!token_ctx || !content) {
        return false;
    }
    
    uint64_t current_time = get_current_time_ms();
    
    // Search for a matching token
    for (size_t i = 0; i < token_ctx->registered_count; i++) {
        polycall_token_t* token = token_ctx->registered_tokens[i];
        
        // Check expiration
        if (token->expiration_time > 0 && current_time > token->creation_time + token->expiration_time) {
            // Token is expired, remove it
            polycall_token_unregister(token_ctx, token);
            i--; // Adjust index since we removed an element
            continue;
        }
        
        // Check content match
        if (strcmp(token->content, content) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Get token content
 */
const char* polycall_token_get_content(polycall_token_t* token) {
    if (!token) {
        return NULL;
    }
    
    return token->content;
}

/**
 * @brief Set token expiration time
 */
polycall_token_error_t polycall_token_set_expiration(
    polycall_token_t* token,
    uint64_t expiration_ms
) {
    if (!token) {
        return POLYCALL_TOKEN_ERROR_INVALID_PARAMETER;
    }
    
    token->expiration_time = expiration_ms;
    return POLYCALL_TOKEN_ERROR_NONE;
}

/**
 * @brief Check if token is expired
 */
bool polycall_token_is_expired(polycall_token_t* token) {
    if (!token || token->expiration_time == 0) {
        return false;
    }
    
    uint64_t current_time = get_current_time_ms();
    return current_time > token->creation_time + token->expiration_time;
}