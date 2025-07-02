/**
 * @file network_container.h
 * @brief Container for network module
 */

#ifndef POLYCALL_NETWORK_CONTAINER_H
#define POLYCALL_NETWORK_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * network container structure
 */
typedef struct network_container {
    polycall_core_context_t* core_ctx;
    void* module_data;
    // Add component-specific fields here
} network_container_t;

/**
 * Initialize network container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int network_container_init(polycall_core_context_t* core_ctx, network_container_t** container);

/**
 * Register network services
 *
 * @param container network container
 * @return int 0 on success, error code otherwise
 */
int network_register_services(network_container_t* container);

/**
 * Cleanup network container
 *
 * @param container network container
 */
void network_container_cleanup(network_container_t* container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_CONTAINER_H */
