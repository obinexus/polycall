/**
#include "polycall/core/auth/auth_context.h"
#include "polycall/core/auth/polycall_auth_context.h"
#include <stdbool.h>
#include <stddef.h>
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"


 * @file polycall_auth_context.c
 * @brief Implementation of the authentication context
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the authentication context that manages identity, credentials,
 * and authorization for LibPolyCall components.
 */

 #include "polycall/core/auth/polycall_auth_context.h"

// Public API implementation

polycall_auth_config_t polycall_auth_create_default_config(void)
{
    polycall_auth_config_t config;
    memset(&config, 0, sizeof(config));

    // Default values
    config.enable_token_validation = true;
    config.enable_access_control = true;
    config.enable_audit_logging = true;
    config.token_validity_period_sec = 3600;     // 1 hour
    config.refresh_token_validity_sec = 2592000; // 30 days
    config.enable_credential_hashing = true;
    config.token_signing_secret = NULL; // Must be provided by the caller

    return config;
}

polycall_core_error_t polycall_auth_init(
    polycall_core_context_t *core_ctx,
    polycall_auth_context_t **auth_ctx,
    const polycall_auth_config_t *config)
{
    // Validate parameters
    if (!core_ctx || !auth_ctx || !config)
    {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Check required config values
    if (!config->token_signing_secret)
    {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Allocate auth context
    polycall_auth_context_t *new_ctx = polycall_core_malloc(core_ctx, sizeof(polycall_auth_context_t));
    if (!new_ctx)
    {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    // Initialize context
    memset(new_ctx, 0, sizeof(polycall_auth_context_t));
    new_ctx->core_ctx = core_ctx;

    // Copy configuration
    memcpy(&new_ctx->config, config, sizeof(polycall_auth_config_t));

    // Create a copy of the token signing secret
    size_t secret_len = strlen(config->token_signing_secret) + 1;
    new_ctx->config.token_signing_secret = polycall_core_malloc(core_ctx, secret_len);
    if (!new_ctx->config.token_signing_secret)
    {
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(new_ctx->config.token_signing_secret, config->token_signing_secret, secret_len);

    // Initialize components
    new_ctx->identities = init_identity_registry(core_ctx, 32);
    if (!new_ctx->identities)
    {
        polycall_core_free(core_ctx, new_ctx->config.token_signing_secret);
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }

    new_ctx->credentials = init_credential_store(core_ctx, config->enable_credential_hashing);
    if (!new_ctx->credentials)
    {
        cleanup_identity_registry(core_ctx, new_ctx->identities);
        polycall_core_free(core_ctx, new_ctx->config.token_signing_secret);
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }

    new_ctx->token_service = init_token_service(core_ctx, config->token_signing_secret,
                                                config->token_validity_period_sec,
                                                config->refresh_token_validity_sec);
    if (!new_ctx->token_service)
    {
        cleanup_credential_store(core_ctx, new_ctx->credentials);
        cleanup_identity_registry(core_ctx, new_ctx->identities);
        polycall_core_free(core_ctx, new_ctx->config.token_signing_secret);
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }

    new_ctx->policies = init_policy_manager(core_ctx);
    if (!new_ctx->policies)
    {
        cleanup_token_service(core_ctx, new_ctx->token_service);
        cleanup_credential_store(core_ctx, new_ctx->credentials);
        cleanup_identity_registry(core_ctx, new_ctx->identities);
        polycall_core_free(core_ctx, new_ctx->config.token_signing_secret);
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }

    new_ctx->integrator = init_auth_integrator(core_ctx);
    if (!new_ctx->integrator)
    {
        cleanup_policy_manager(core_ctx, new_ctx->policies);
        cleanup_token_service(core_ctx, new_ctx->token_service);
        cleanup_credential_store(core_ctx, new_ctx->credentials);
        cleanup_identity_registry(core_ctx, new_ctx->identities);
        polycall_core_free(core_ctx, new_ctx->config.token_signing_secret);
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }

    *auth_ctx = new_ctx;
    return POLYCALL_CORE_SUCCESS;
}

void polycall_auth_cleanup(
    polycall_core_context_t *core_ctx,
    polycall_auth_context_t *auth_ctx)
{
    if (!core_ctx || !auth_ctx)
    {
        return;
    }

    // Clean up components in reverse order of initialization
    cleanup_auth_integrator(core_ctx, auth_ctx->integrator);
    cleanup_policy_manager(core_ctx, auth_ctx->policies);
    cleanup_token_service(core_ctx, auth_ctx->token_service);
    cleanup_credential_store(core_ctx, auth_ctx->credentials);
    cleanup_identity_registry(core_ctx, auth_ctx->identities);

    // Free current identity if set
    if (auth_ctx->current_identity)
    {
        polycall_core_free(core_ctx, auth_ctx->current_identity);
    }

    // Free configuration
    polycall_core_free(core_ctx, auth_ctx->config.token_signing_secret);

    // Free context itself
    polycall_core_free(core_ctx, auth_ctx);
}

polycall_core_error_t polycall_auth_get_current_identity(
    polycall_core_context_t *core_ctx,
    polycall_auth_context_t *auth_ctx,
    char **identity_id)
{
    // Validate parameters
    if (!core_ctx || !auth_ctx || !identity_id)
    {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Check if current identity is set
    if (!auth_ctx->current_identity)
    {
        *identity_id = NULL;
        return POLYCALL_CORE_ERROR_NOT_FOUND;
    }

    // Duplicate the identity string
    size_t len = strlen(auth_ctx->current_identity) + 1;
    *identity_id = polycall_core_malloc(core_ctx, len);
    if (!*identity_id)
    {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    memcpy(*identity_id, auth_ctx->current_identity, len);
    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_auth_authenticate(
    polycall_core_context_t *core_ctx,
    polycall_auth_context_t *auth_ctx,
    const char *username,
    const char *password,
    char **access_token,
    char **refresh_token)
{
    // Validate parameters
    if (!core_ctx || !auth_ctx || !username || !password || !access_token || !refresh_token)
    {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Find identity in registry
    pthread_mutex_lock(&auth_ctx->identities->mutex);

    int identity_index = -1;
    for (size_t i = 0; i < auth_ctx->identities->count; i++)
    {
        identity_attributes_t *attributes = auth_ctx->identities->attributes[i];
        if (attributes && attributes->name && strcmp(attributes->name, username) == 0)
        {
            identity_index = (int)i;
            break;
        }
    }

    // Identity not found
    if (identity_index == -1)
    {
        pthread_mutex_unlock(&auth_ctx->identities->mutex);
        return POLYCALL_CORE_ERROR_NOT_FOUND;
    }

    // Check if identity is active
    if (!auth_ctx->identities->attributes[identity_index]->is_active)
    {
        pthread_mutex_unlock(&auth_ctx->identities->mutex);
        return POLYCALL_CORE_ERROR_ACCESS_DENIED;
    }

    // Get identity ID and hashed password
    char *identity_id = auth_ctx->identities->identity_ids[identity_index];
    char *stored_hash = auth_ctx->identities->hashed_passwords[identity_index];

    // Unlock registry mutex
    pthread_mutex_unlock(&auth_ctx->identities->mutex);

    // Verify password
    bool password_valid = verify_password(auth_ctx->credentials, password, stored_hash);
    if (!password_valid)
    {
        return POLYCALL_CORE_ERROR_ACCESS_DENIED;
    }

    // Set current identity
    if (auth_ctx->current_identity)
    {
        polycall_core_free(core_ctx, auth_ctx->current_identity);
    }

    size_t id_len = strlen(identity_id) + 1;
    auth_ctx->current_identity = polycall_core_malloc(core_ctx, id_len);
    if (!auth_ctx->current_identity)
    {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(auth_ctx->current_identity, identity_id, id_len);

    // Calculate expiry times
    uint64_t now = get_current_timestamp();
    uint64_t access_expiry = now + auth_ctx->token_service->access_token_validity;
    uint64_t refresh_expiry = now + auth_ctx->token_service->refresh_token_validity;

    // Generate tokens
    char *new_access_token = generate_token(auth_ctx->token_service, identity_id,
                                            POLYCALL_TOKEN_TYPE_ACCESS, access_expiry);
    if (!new_access_token)
    {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    char *new_refresh_token = generate_token(auth_ctx->token_service, identity_id,
                                             POLYCALL_TOKEN_TYPE_REFRESH, refresh_expiry);
    if (!new_refresh_token)
    {
        polycall_core_free(core_ctx, new_access_token);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    // Update last login timestamp
    pthread_mutex_lock(&auth_ctx->identities->mutex);
    auth_ctx->identities->attributes[identity_index]->last_login_timestamp = now;
    pthread_mutex_unlock(&auth_ctx->identities->mutex);

    // Create audit event
    audit_event_t *event = polycall_auth_create_audit_event(
        core_ctx,
        POLYCALL_AUDIT_EVENT_LOGIN,
        identity_id,
        NULL,
        NULL,
        true,
        NULL);

    if (event)
    {
        event->source_ip = NULL;  // Would be set in a real implementation
        event->user_agent = NULL; // Would be set in a real implementation
        polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
        polycall_auth_free_audit_event(core_ctx, event);
    }

    // Return tokens
    *access_token = new_access_token;
    *refresh_token = new_refresh_token;

    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_auth_validate_token(
    polycall_core_context_t *core_ctx,
    polycall_auth_context_t *auth_ctx,
    const char *token,
    char **identity_id)
{
    // Validate parameters
    if (!core_ctx || !auth_ctx || !token || !identity_id)
    {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Validate token
    token_validation_result_t *result = validate_token_internal(auth_ctx->token_service, token);
    if (!result)
    {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    if (!result->is_valid)
    {
        // Create audit event for invalid token
        audit_event_t *event = polycall_auth_create_audit_event(
            core_ctx,
            POLYCALL_AUDIT_EVENT_TOKEN_VALIDATE,
            NULL, // No identity
            NULL,
            NULL,
            false,
            result->error_message);

        if (event)
        {
            polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
            polycall_auth_free_audit_event(core_ctx, event);
        }

        free(result->error_message);
        free(result);
        return POLYCALL_CORE_ERROR_INVALID_TOKEN;
    }

    // Token is valid, extract identity ID
    char *subject = result->claims->subject;
    if (!subject)
    {
        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_INVALID_TOKEN;
    }

    // Set current identity
    if (auth_ctx->current_identity)
    {
        polycall_core_free(core_ctx, auth_ctx->current_identity);
    }

    size_t id_len = strlen(subject) + 1;
    auth_ctx->current_identity = polycall_core_malloc(core_ctx, id_len);
    if (!auth_ctx->current_identity)
    {
        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(auth_ctx->current_identity, subject, id_len);

    // Return identity ID
    *identity_id = polycall_core_malloc(core_ctx, id_len);
    if (!*identity_id)
    {
        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(*identity_id, subject, id_len);

    // Create audit event for successful validation
    audit_event_t *event = polycall_auth_create_audit_event(
        core_ctx,
        POLYCALL_AUDIT_EVENT_TOKEN_VALIDATE,
        subject,
        NULL,
        NULL,
        true,
        NULL);

    if (event)
    {
        polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
        polycall_auth_free_audit_event(core_ctx, event);
    }

    // Clean up
    free(result->claims);
    free(result);

    return POLYCALL_CORE_SUCCESS;
}

polycall_core_error_t polycall_auth_refresh_token(
    polycall_core_context_t *core_ctx,
    polycall_auth_context_t *auth_ctx,
    const char *refresh_token,
    char **access_token)
{
    // Validate parameters
    if (!core_ctx || !auth_ctx || !refresh_token || !access_token)
    {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Validate refresh token
    token_validation_result_t *result = validate_token_internal(auth_ctx->token_service, refresh_token);
    if (!result)
    {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    if (!result->is_valid)
    {
        // Create audit event for invalid token
        audit_event_t *event = polycall_auth_create_audit_event(
            core_ctx,
            POLYCALL_AUDIT_EVENT_TOKEN_REFRESH,
            NULL, // No identity
            NULL,
            NULL,
            false,
            result->error_message);

        if (event)
        {
            polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
            polycall_auth_free_audit_event(core_ctx, event);
        }

        free(result->error_message);
        free(result);
        return POLYCALL_CORE_ERROR_INVALID_TOKEN;
    }

    // Check if it's a refresh token
    if (result->claims->token_id[0] != 'R')
    {
        // Create audit event for invalid token type
        audit_event_t *event = polycall_auth_create_audit_event(
            core_ctx,
            POLYCALL_AUDIT_EVENT_TOKEN_REFRESH,
            result->claims->subject,
            NULL,
            NULL,
            false,
            "Not a refresh token");

        if (event)
        {
            polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
            polycall_auth_free_audit_event(core_ctx, event);
        }

        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_INVALID_TOKEN;
    }

    // Token is valid, extract identity ID
    char *subject = result->claims->subject;
    if (!subject)
    {
        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_INVALID_TOKEN;
    }

    // Set current identity
    if (auth_ctx->current_identity)
    {
        polycall_core_free(core_ctx, auth_ctx->current_identity);
    }

    size_t id_len = strlen(subject) + 1;
    auth_ctx->current_identity = polycall_core_malloc(core_ctx, id_len);
    if (!auth_ctx->current_identity)
    {
        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(auth_ctx->current_identity, subject, id_len);

    // Calculate expiry time for new access token
    uint64_t now = get_current_timestamp();
    uint64_t access_expiry = now + auth_ctx->token_service->access_token_validity;

    // Generate new access token
    char *new_access_token = generate_token(auth_ctx->token_service, subject,
                                            POLYCALL_TOKEN_TYPE_ACCESS, access_expiry);
    if (!new_access_token)
    {
        free(result->claims);
        free(result);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    // Create audit event for successful token refresh
    audit_event_t *event = polycall_auth_create_audit_event(
        core_ctx,
        POLYCALL_AUDIT_EVENT_TOKEN_REFRESH,
        subject,
        NULL,
        NULL,
        true,
        NULL);

    if (event)
    {
        polycall_auth_log_audit_event(core_ctx, auth_ctx, event);
        polycall_auth_free_audit_event(core_ctx, event);
    }

    // Clean up token validation result
    free(result->claims);
    free(result);

    // Return new access token
    *access_token = new_access_token;

    return POLYCALL_CORE_SUCCESS;
}