/**
 * @file edge_container.c
 * @brief Container for edge module
 */

#include "polycall/core/edge/edge_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <stdlib.h>
#include <string.h>

/**
 * Initialize edge container
 */
int edge_container_init(polycall_core_context_t* core_ctx, edge_container_t** container) {
    if (!core_ctx || !container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    edge_container_t* c = (edge_container_t*)malloc(sizeof(edge_container_t));
    if (!c) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(edge_container_t));
    c->core_ctx = core_ctx;
    
    // Initialize module-specific data
    
    *container = c;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Register edge services
 */
int edge_register_services(edge_container_t* container) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_core_context_t* ctx = container->core_ctx;
    
    // Register services with core context
    polycall_register_service(ctx, "edge_container", container);
    
    // Register additional module-specific services
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup edge container
 */
void edge_container_cleanup(edge_container_t* container) {
    if (!container) {
        return;
    }
    
    // Free module-specific resources
    
    free(container);
}
