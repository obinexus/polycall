/**
 * @file polycall_config.h
 * @brief Stub header for config component
 * @author LibPolyCall Implementation Team - Test Infrastructure
 */

#ifndef POLYCALL_CONFIG_H
#define POLYCALL_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core types
typedef struct polycall_config_context polycall_config_context_t;
typedef struct polycall_config_config polycall_config_config_t;

// Error codes
typedef enum {
    POLYCALL_CONFIG_SUCCESS = 0,
    POLYCALL_CONFIG_ERROR_GENERIC = -1,
    POLYCALL_CONFIG_ERROR_INVALID_PARAMETER = -2,
    POLYCALL_CONFIG_ERROR_NOT_INITIALIZED = -3,
    POLYCALL_CONFIG_ERROR_ALREADY_INITIALIZED = -4,
    POLYCALL_CONFIG_ERROR_OUT_OF_MEMORY = -5
} polycall_config_error_t;

// Initialization function
polycall_config_error_t polycall_config_init(
    polycall_config_context_t** ctx
);

// Cleanup function
void polycall_config_cleanup(
    polycall_config_context_t* ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_H */
