/**
 * @file ffi_core.c
 * @brief FFI Core Coordinator - Sinphasé Compliant
 * 
 * Cost Target: C ≤ 0.4 (Autonomous Zone)
 * Responsibilities: Coordinate isolated bridges (star topology)
 * Dependencies: NONE directly - communicates with bridges via API
 */

#include "polycall/ffi/ffi_core.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// FFI Core context (minimal)
typedef struct {
    bool initialized;
    size_t registered_bridges;
    char bridge_names[8][32];  // Maximum 8 bridges
} polycall_ffi_core_context_t;

static polycall_ffi_core_context_t g_ffi_core_ctx = {0};

/**
 * @brief Initialize FFI core coordinator
 */
polycall_ffi_error_t polycall_ffi_core_init(void) {
    if (g_ffi_core_ctx.initialized) {
        return POLYCALL_FFI_ERROR_ALREADY_INITIALIZED;
    }
    
    g_ffi_core_ctx.initialized = true;
    g_ffi_core_ctx.registered_bridges = 0;
    
    return POLYCALL_FFI_SUCCESS;
}

/**
 * @brief Register a bridge with core coordinator
 */
polycall_ffi_error_t polycall_ffi_core_register_bridge(
    const char* bridge_name
) {
    if (!g_ffi_core_ctx.initialized) {
        return POLYCALL_FFI_ERROR_NOT_INITIALIZED;
    }
    
    if (!bridge_name || strlen(bridge_name) >= 32) {
        return POLYCALL_FFI_ERROR_INVALID_PARAMETER;
    }
    
    if (g_ffi_core_ctx.registered_bridges >= 8) {
        return POLYCALL_FFI_ERROR_BRIDGE_LIMIT_EXCEEDED;
    }
    
    strncpy(g_ffi_core_ctx.bridge_names[g_ffi_core_ctx.registered_bridges], 
            bridge_name, 31);
    g_ffi_core_ctx.bridge_names[g_ffi_core_ctx.registered_bridges][31] = '\0';
    g_ffi_core_ctx.registered_bridges++;
    
    return POLYCALL_FFI_SUCCESS;
}

/**
 * @brief Get registered bridge count
 */
polycall_ffi_error_t polycall_ffi_core_get_bridge_count(
    size_t* count
) {
    if (!count) {
        return POLYCALL_FFI_ERROR_INVALID_PARAMETER;
    }
    
    *count = g_ffi_core_ctx.registered_bridges;
    return POLYCALL_FFI_SUCCESS;
}

/**
 * @brief Cleanup FFI core
 */
polycall_ffi_error_t polycall_ffi_core_cleanup(void) {
    if (!g_ffi_core_ctx.initialized) {
        return POLYCALL_FFI_ERROR_NOT_INITIALIZED;
    }
    
    g_ffi_core_ctx.initialized = false;
    g_ffi_core_ctx.registered_bridges = 0;
    
    return POLYCALL_FFI_SUCCESS;
}
