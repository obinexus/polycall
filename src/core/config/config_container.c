/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file config_container.c
 * @brief Container for config module
 */

#include "polycall/core/config/config_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <string.h>

/**
 * Initialize config container
 */
int config_container_init(polycall_core_context_t* core_ctx, config_container_t** container) {
    if (!core_ctx || !container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    config_container_t* c = (config_container_t*)malloc(sizeof(config_container_t));
    if (!c) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(config_container_t));
    c->core_ctx = core_ctx;
    
    // Initialize module-specific data
    
    *container = c;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Register config services
 */
int config_register_services(config_container_t* container) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_core_context_t* ctx = container->core_ctx;
    
    // Register services with core context
    polycall_register_service(ctx, "config_container", container);
    
    // Register additional module-specific services
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup config container
 */
void config_container_cleanup(config_container_t* container) {
    if (!container) {
        return;
    }
    
    // Free module-specific resources
    
    free(container);
}
