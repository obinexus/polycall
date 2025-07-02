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
 * @file auth_container.c
 * @brief Container for auth module
 */

#include "polycall/core/auth/auth_container.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <string.h>

/**
 * Initialize auth container
 */
int auth_container_init(polycall_core_context_t* core_ctx, auth_container_t** container) {
    if (!core_ctx || !container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    auth_container_t* c = (auth_container_t*)malloc(sizeof(auth_container_t));
    if (!c) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(c, 0, sizeof(auth_container_t));
    c->core_ctx = core_ctx;
    
    // Initialize module-specific data
    
    *container = c;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Register auth services
 */
int auth_register_services(auth_container_t* container) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_core_context_t* ctx = container->core_ctx;
    
    // Register services with core context
    polycall_register_service(ctx, "auth_container", container);
    
    // Register additional module-specific services
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * Cleanup auth container
 */
void auth_container_cleanup(auth_container_t* container) {
    if (!container) {
        return;
    }
    
    // Free module-specific resources
    
    free(container);
}
