/**
 * @file ffi_container.c
 * @brief Container for ffi module
 */

#include "polycall/core/ffi/ffi_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <stdlib.h>
#include <string.h>

/**
 * Initialize ffi container
 */
int ffi_container_init(polycall_core_context_t* core_ctx, ffi_container_t** container) {
    if (!core_ctx || !container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    ffi_container_t* c = (ffi_container_t*)malloc(sizeof(ffi_container_t));
    if (!c) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(ffi_container_t));
    c->core_ctx = core_ctx;
    
    // Initialize module-specific data
    
    *container = c;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Register ffi services
 */
int ffi_register_services(ffi_container_t* container) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_core_context_t* ctx = container->core_ctx;
    
    // Register services with core context
    polycall_register_service(ctx, "ffi_container", container);
    
    // Register additional module-specific services
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup ffi container
 */
void ffi_container_cleanup(ffi_container_t* container) {
    if (!container) {
        return;
    }
    
    // Free module-specific resources
    
    free(container);
}
