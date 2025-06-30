/**
 * @file polycall_polycall.h
 * @brief Stub header for polycall component
 * @author LibPolyCall Implementation Team - Test Infrastructure
 */

#ifndef POLYCALL_POLYCALL_H
#define POLYCALL_POLYCALL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core types
typedef struct polycall_polycall_context polycall_polycall_context_t;
typedef struct polycall_polycall_config polycall_polycall_config_t;

// Error codes
typedef enum {
    POLYCALL_POLYCALL_SUCCESS = 0,
    POLYCALL_POLYCALL_ERROR_GENERIC = -1,
    POLYCALL_POLYCALL_ERROR_INVALID_PARAMETER = -2,
    POLYCALL_POLYCALL_ERROR_NOT_INITIALIZED = -3,
    POLYCALL_POLYCALL_ERROR_ALREADY_INITIALIZED = -4,
    POLYCALL_POLYCALL_ERROR_OUT_OF_MEMORY = -5
} polycall_polycall_error_t;

// Initialization function
polycall_polycall_error_t polycall_polycall_init(
    polycall_polycall_context_t** ctx
);

// Cleanup function
void polycall_polycall_cleanup(
    polycall_polycall_context_t* ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_H */
