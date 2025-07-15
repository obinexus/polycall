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
#include "polycall/polycall_config.h"

 * @file auth_config.c
 * @brief Implementation of authentication configuration functions
 * @author Integration with Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements configuration loading and validation for the LibPolyCall
 * authentication module, following zero-trust security principles.
 */

#include "polycall/polycall_config.h"
/**
 * @brief Authentication configuration validator
 */
typedef struct {
  polycall_core_context_t *core_ctx; // Core context
  polycall_auth_config_t *config;    // Config being validated
  char error_buffer[256];            // Error message buffer
} auth_config_validator_t;

/**
 * @brief Default values for authentication configuration
 */
static const polycall_auth_config_t DEFAULT_AUTH_CONFIG = {
    .enable_token_validation = true,
    .enable_access_control = true,
    .enable_audit_logging = true,
    .token_validity_period_sec = 3600,     // 1 hour
    .refresh_token_validity_sec = 2592000, // 30 days
    .enable_credential_hashing = true,
    .token_signing_secret = NULL, // Must be provided
    .flags = 0,
    .user_data = NULL};

// Forward declarations for internal functions
static polycall_core_error_t
validate_auth_config(auth_config_validator_t *validator,
                     const polycall_auth_config_t *config);

/**
 * @brief Load authentication configuration from file
 *
 * @param core_ctx Core context
 * @param config_file Configuration file path
 * @param config Pointer to receive the configuration
 * @return Error code
 */
polycall_core_error_t
polycall_auth_load_config(polycall_core_context_t *core_ctx,
                          const char *config_file,
                          polycall_auth_config_t *config) {
  // Validate parameters
  if (!core_ctx || !config_file || !config) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Initialize with default values
  memcpy(config, &DEFAULT_AUTH_CONFIG, sizeof(polycall_auth_config_t));

  // Read configuration file
  polycall_config_context_t *config_ctx = NULL;
  polycall_core_error_t result = polycall_config_init(core_ctx, &config_ctx);
  if (result != POLYCALL_CORE_SUCCESS) {
    return result;
  }

  // Load configuration file
  result = polycall_config_load_file(core_ctx, config_ctx, config_file);
  if (result != POLYCALL_CORE_SUCCESS) {
    polycall_config_cleanup(core_ctx, config_ctx);
    return result;
  }

  // Read configuration values
  polycall_config_get_bool(core_ctx, config_ctx, "auth.enable_token_validation",
                           &config->enable_token_validation);
  polycall_config_get_bool(core_ctx, config_ctx, "auth.enable_access_control",
                           &config->enable_access_control);
  polycall_config_get_bool(core_ctx, config_ctx, "auth.enable_audit_logging",
                           &config->enable_audit_logging);
  polycall_config_get_uint32(core_ctx, config_ctx,
                             "auth.token_validity_period_sec",
                             &config->token_validity_period_sec);
  polycall_config_get_uint32(core_ctx, config_ctx,
                             "auth.refresh_token_validity_sec",
                             &config->refresh_token_validity_sec);
  polycall_config_get_bool(core_ctx, config_ctx,
                           "auth.enable_credential_hashing",
                           &config->enable_credential_hashing);

  // Get token signing secret (required)
  const char *signing_secret = NULL;
  result = polycall_config_get_string(
      core_ctx, config_ctx, "auth.token_signing_secret", &signing_secret);
  if (result == POLYCALL_CORE_SUCCESS && signing_secret) {
    size_t secret_len = strlen(signing_secret) + 1;
    config->token_signing_secret = polycall_core_malloc(core_ctx, secret_len);
    if (!config->token_signing_secret) {
      polycall_config_cleanup(core_ctx, config_ctx);
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy((void *)config->token_signing_secret, signing_secret, secret_len);
  }

  // Get additional flags
  polycall_config_get_uint32(core_ctx, config_ctx, "auth.flags",
                             &config->flags);

  // Validate the configuration
  auth_config_validator_t validator = {
      .core_ctx = core_ctx, .config = config, .error_buffer = {0}};

  result = validate_auth_config(&validator, config);
  if (result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_LOG(core_ctx, POLYCALL_LOG_ERROR,
                 "Authentication configuration validation failed: %s",
                 validator.error_buffer);
    if (config->token_signing_secret) {
      polycall_core_free(core_ctx, (void *)config->token_signing_secret);
      config->token_signing_secret = NULL;
    }
    polycall_config_cleanup(core_ctx, config_ctx);
    return result;
  }

  // Clean up
  polycall_config_cleanup(core_ctx, config_ctx);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Validate authentication configuration
 *
 * @param validator Configuration validator
 * @param config Configuration to validate
 * @return Error code
 */
static polycall_core_error_t
validate_auth_config(auth_config_validator_t *validator,
                     const polycall_auth_config_t *config) {
  // Check if token signing secret is provided
  if (!config->token_signing_secret) {
    snprintf(validator->error_buffer, sizeof(validator->error_buffer),
             "Token signing secret is required");
    return POLYCALL_CORE_ERROR_INVALID_CONFIGURATION;
  }

  // Validate token validity period
  if (config->token_validity_period_sec < 300 ||
      config->token_validity_period_sec > 86400) {
    snprintf(validator->error_buffer, sizeof(validator->error_buffer),
             "Token validity period must be between 5 minutes and 24 hours");
    return POLYCALL_CORE_ERROR_INVALID_CONFIGURATION;
  }

  // Validate refresh token validity period
  if (config->refresh_token_validity_sec < 3600 ||
      config->refresh_token_validity_sec > 31536000) {
    snprintf(
        validator->error_buffer, sizeof(validator->error_buffer),
        "Refresh token validity period must be between 1 hour and 365 days");
    return POLYCALL_CORE_ERROR_INVALID_CONFIGURATION;
  }

  // Check for sufficient token signing secret length
  if (strlen(config->token_signing_secret) < 16) {
    snprintf(validator->error_buffer, sizeof(validator->error_buffer),
             "Token signing secret must be at least 16 characters long");
    return POLYCALL_CORE_ERROR_INVALID_CONFIGURATION;
  }

  // Ensure refresh token has longer validity than access token
  if (config->refresh_token_validity_sec <= config->token_validity_period_sec) {
    snprintf(
        validator->error_buffer, sizeof(validator->error_buffer),
        "Refresh token validity must be greater than access token validity");
    return POLYCALL_CORE_ERROR_INVALID_CONFIGURATION;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Free resources associated with authentication configuration
 *
 * @param core_ctx Core context
 * @param config Configuration to clean up
 */
void polycall_auth_cleanup_config(polycall_core_context_t *core_ctx,
                                  polycall_auth_config_t *config) {
  if (!core_ctx || !config) {
    return;
  }

  // Free token signing secret
  if (config->token_signing_secret) {
    polycall_core_free(core_ctx, (void *)config->token_signing_secret);
    config->token_signing_secret = NULL;
  }

  // Reset to default values
  memcpy(config, &DEFAULT_AUTH_CONFIG, sizeof(polycall_auth_config_t));
}

/**
 * @brief Merge two authentication configurations
 *
 * Values from the override configuration take precedence over the base
 * configuration.
 *
 * @param core_ctx Core context
 * @param base Base configuration
 * @param override Override configuration
 * @param result Pointer to receive the merged configuration
 * @return Error code
 */
polycall_core_error_t polycall_auth_merge_configs(
    polycall_core_context_t *core_ctx, const polycall_auth_config_t *base,
    const polycall_auth_config_t *override, polycall_auth_config_t *result) {
  // Validate parameters
  if (!core_ctx || !base || !override || !result) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Copy base configuration
  memcpy(result, base, sizeof(polycall_auth_config_t));

  // Override with values from override configuration
  result->enable_token_validation = override->enable_token_validation;
  result->enable_access_control = override->enable_access_control;
  result->enable_audit_logging = override->enable_audit_logging;
  result->token_validity_period_sec = override->token_validity_period_sec;
  result->refresh_token_validity_sec = override->refresh_token_validity_sec;
  result->enable_credential_hashing = override->enable_credential_hashing;
  result->flags = override->flags;

  // Handle token signing secret
  if (override->token_signing_secret) {
    // If base had a secret, free it
    if (result->token_signing_secret) {
      polycall_core_free(core_ctx, (void *)result->token_signing_secret);
    }

    // Copy override's secret
    size_t secret_len = strlen(override->token_signing_secret) + 1;
    result->token_signing_secret = polycall_core_malloc(core_ctx, secret_len);
    if (!result->token_signing_secret) {
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy((void *)result->token_signing_secret, override->token_signing_secret,
           secret_len);
  }

  // Validate merged configuration
  auth_config_validator_t validator = {
      .core_ctx = core_ctx, .config = result, .error_buffer = {0}};

  polycall_core_error_t validation_result =
      validate_auth_config(&validator, result);
  if (validation_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_LOG(core_ctx, POLYCALL_LOG_ERROR,
                 "Merged authentication configuration validation failed: %s",
                 validator.error_buffer);
    if (result->token_signing_secret) {
      polycall_core_free(core_ctx, (void *)result->token_signing_secret);
      result->token_signing_secret = NULL;
    }
    return validation_result;
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Apply zero-trust security constraints to authentication configuration
 *
 * This function ensures that authentication configuration meets zero-trust
 * security requirements, overriding unsafe settings if necessary.
 *
 * @param core_ctx Core context
 * @param config Configuration to secure
 * @return Error code
 */
polycall_core_error_t
polycall_auth_apply_zero_trust_constraints(polycall_core_context_t *core_ctx,
                                           polycall_auth_config_t *config) {
  // Validate parameters
  if (!core_ctx || !config) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Enforce zero-trust security principles

  // 1. Always enforce token validation
  config->enable_token_validation = true;

  // 2. Always enforce access control
  config->enable_access_control = true;

  // 3. Always enable audit logging
  config->enable_audit_logging = true;

  // 4. Enforce credential hashing
  config->enable_credential_hashing = true;

  // 5. Enforce reasonable token validity periods
  // Access tokens should be short-lived in zero-trust model
  if (config->token_validity_period_sec > 3600) {
    config->token_validity_period_sec = 3600; // Max 1 hour
  }

  // 6. Enforce strong signing secret
  if (!config->token_signing_secret ||
      strlen(config->token_signing_secret) < 32) {
    // Log warning, but don't automatically generate a secret
    // as this should be explicitly provided in a secure manner
    POLYCALL_LOG(core_ctx, POLYCALL_LOG_WARNING,
                 "Zero-trust security requires a strong token signing secret "
                 "(32+ chars)");
  }

  return POLYCALL_CORE_SUCCESS;
}
