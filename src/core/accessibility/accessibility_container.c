/**
 * @file accessibility_container.c
 * @brief Container for accessibility module
 */

#include "polycall/core/accessibility/accessibility_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <stdlib.h>
#include <string.h>

/**
 * Initialize accessibility container
 */
int accessibility_container_init(polycall_core_context_t* core_ctx, accessibility_container_t** container) {
    if (!core_ctx || !container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    accessibility_container_t* c = (accessibility_container_t*)malloc(sizeof(accessibility_container_t));
    if (!c) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(accessibility_container_t));
    c->core_ctx = core_ctx;
    
    // Initialize module-specific data
    
    *container = c;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Register accessibility services
 */
int accessibility_register_services(accessibility_container_t* container) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_core_context_t* ctx = container->core_ctx;
    
    // Register services with core context
    polycall_register_service(ctx, "accessibility_container", container);
    
    // Register additional module-specific services
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup accessibility container
 */
void accessibility_container_cleanup(accessibility_container_t* container) {
    if (!container) {
        return;
    }
    
    // Free module-specific resources
    
    free(container);
}
