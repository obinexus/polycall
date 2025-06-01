/**
 * @file polycall_auth.h
 * @brief Stub header for auth component
 * @author LibPolyCall Implementation Team - Test Infrastructure
 */

#ifndef POLYCALL_AUTH_H
#define POLYCALL_AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core types
typedef struct polycall_auth_context polycall_auth_context_t;
typedef struct polycall_auth_config polycall_auth_config_t;

// Error codes
typedef enum {
    POLYCALL_AUTH_SUCCESS = 0,
    POLYCALL_AUTH_ERROR_GENERIC = -1,
    POLYCALL_AUTH_ERROR_INVALID_PARAMETER = -2,
    POLYCALL_AUTH_ERROR_NOT_INITIALIZED = -3,
    POLYCALL_AUTH_ERROR_ALREADY_INITIALIZED = -4,
    POLYCALL_AUTH_ERROR_OUT_OF_MEMORY = -5
} polycall_auth_error_t;

// Initialization function
polycall_auth_error_t polycall_auth_init(
    polycall_auth_context_t** ctx
);

// Cleanup function
void polycall_auth_cleanup(
    polycall_auth_context_t* ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_AUTH_H */
