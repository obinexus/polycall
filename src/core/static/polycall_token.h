/**
 * @file polycall_token.h
 * @brief Token management system for the PolyCall protocol
 * @author LibPolyCall Implementation Team
 */

#ifndef POLYCALL_TOKEN_H
#define POLYCALL_TOKEN_H

#include "polycall/core/polycall/polycall_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Token error codes
 */
typedef enum {
  POLYCALL_TOKEN_ERROR_NONE = 0,          /**< No error */
  POLYCALL_TOKEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
  POLYCALL_TOKEN_ERROR_OUT_OF_MEMORY,     /**< Out of memory */
  POLYCALL_TOKEN_ERROR_NOT_FOUND,         /**< Token not found */
  POLYCALL_TOKEN_ERROR_ALREADY_EXISTS,    /**< Token already exists */
  POLYCALL_TOKEN_ERROR_EXPIRED,           /**< Token has expired */
  POLYCALL_TOKEN_ERROR_INVALID,           /**< Token is invalid */
  POLYCALL_TOKEN_ERROR_INTERNAL,          /**< Internal error */
} polycall_token_error_t;

/**
 * @brief Token context
 */
typedef struct polycall_token_context polycall_token_context_t;

/**
 * @brief Token object
 */
typedef struct polycall_token polycall_token_t;

/**
 * @brief Create a token context
 *
 * @param core_ctx Core context
 * @param token_ctx Pointer to store the created token context
 * @return polycall_token_error_t Error code
 */
polycall_token_error_t
polycall_token_context_create(polycall_core_context_t *core_ctx,
                              polycall_token_context_t **token_ctx);

/**
 * @brief Destroy a token context
 *
 * @param token_ctx Token context
 */
void polycall_token_context_destroy(polycall_token_context_t *token_ctx);

/**
 * @brief Create a token
 *
 * @param token_ctx Token context
 * @param content Token content
 * @param token Pointer to store the created token
 * @return polycall_token_error_t Error code
 */
polycall_token_error_t
polycall_token_create(polycall_token_context_t *token_ctx, const char *content,
                      polycall_token_t **token);

/**
 * @brief Destroy a token
 *
 * @param token Token to destroy
 */
void polycall_token_destroy(polycall_token_t *token);

/**
 * @brief Register a token for validation
 *
 * @param token_ctx Token context
 * @param token Token to register
 * @return polycall_token_error_t Error code
 */
polycall_token_error_t
polycall_token_register(polycall_token_context_t *token_ctx,
                        polycall_token_t *token);

/**
 * @brief Unregister a token
 *
 * @param token_ctx Token context
 * @param token Token to unregister
 * @return polycall_token_error_t Error code
 */
polycall_token_error_t
polycall_token_unregister(polycall_token_context_t *token_ctx,
                          polycall_token_t *token);

/**
 * @brief Validate a token
 *
 * @param token_ctx Token context
 * @param content Token content to validate
 * @return bool True if valid, false otherwise
 */
bool polycall_token_validate(polycall_token_context_t *token_ctx,
                             const char *content);

/**
 * @brief Get token content
 *
 * @param token Token
 * @return const char* Token content
 */
const char *polycall_token_get_content(polycall_token_t *token);

/**
 * @brief Set token expiration time
 *
 * @param token Token
 * @param expiration_ms Expiration time in milliseconds
 * @return polycall_token_error_t Error code
 */
polycall_token_error_t polycall_token_set_expiration(polycall_token_t *token,
                                                     uint64_t expiration_ms);

/**
 * @brief Check if token is expired
 *
 * @param token Token
 * @return bool True if expired, false otherwise
 */
bool polycall_token_is_expired(polycall_token_t *token);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_TOKEN_H */