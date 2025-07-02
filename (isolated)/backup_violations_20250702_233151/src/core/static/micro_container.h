/**
 * @file micro_container.h
 * @brief Container for micro module
 */

#ifndef POLYCALL_MICRO_CONTAINER_H
#define POLYCALL_MICRO_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * micro container structure
 */
typedef struct micro_container {
    polycall_core_context_t* core_ctx;
    void* module_data;
    // Add component-specific fields here
} micro_container_t;

/**
 * Initialize micro container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int micro_container_init(polycall_core_context_t* core_ctx, micro_container_t** container);

/**
 * Register micro services
 *
 * @param container micro container
 * @return int 0 on success, error code otherwise
 */
int micro_register_services(micro_container_t* container);

/**
 * Cleanup micro container
 *
 * @param container micro container
 */
void micro_container_cleanup(micro_container_t* container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_MICRO_CONTAINER_H */
