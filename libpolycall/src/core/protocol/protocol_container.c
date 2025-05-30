/**
 * @file protocol_container.c
 * @brief Container for protocol module
 */

#include "polycall/core/protocol/protocol_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <stdlib.h>
#include <string.h>

/**
 * Initialize protocol container
 */
int protocol_container_init(polycall_core_context_t* core_ctx, protocol_container_t** container) {
    if (!core_ctx || !container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    protocol_container_t* c = (protocol_container_t*)malloc(sizeof(protocol_container_t));
    if (!c) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(protocol_container_t));
    c->core_ctx = core_ctx;
    
    // Initialize module-specific data
    
    *container = c;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Register protocol services
 */
int protocol_register_services(protocol_container_t* container) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_core_context_t* ctx = container->core_ctx;
    
    // Register services with core context
    polycall_register_service(ctx, "protocol_container", container);
    
    // Register additional module-specific services
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup protocol container
 */
void protocol_container_cleanup(protocol_container_t* container) {
    if (!container) {
        return;
    }
    
    // Free module-specific resources
    
    free(container);
}
