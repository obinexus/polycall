/**
 * @file config_container.h
 * @brief Container for config module
 */

#ifndef POLYCALL_CONFIG_CONTAINER_H
#define POLYCALL_CONFIG_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * config container structure
 */
typedef struct config_container {
    polycall_core_context_t* core_ctx;
    void* module_data;
    // Add component-specific fields here
} config_container_t;

/**
 * Initialize config container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int config_container_init(polycall_core_context_t* core_ctx, config_container_t** container);

/**
 * Register config services
 *
 * @param container config container
 * @return int 0 on success, error code otherwise
 */
int config_register_services(config_container_t* container);

/**
 * Cleanup config container
 *
 * @param container config container
 */
void config_container_cleanup(config_container_t* container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_CONTAINER_H */
