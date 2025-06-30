/**
 * @file accessibility_container.h
 * @brief Container for accessibility module
 */

#ifndef POLYCALL_ACCESSIBILITY_CONTAINER_H
#define POLYCALL_ACCESSIBILITY_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * accessibility container structure
 */
typedef struct accessibility_container {
    polycall_core_context_t* core_ctx;
    void* module_data;
    // Add component-specific fields here
} accessibility_container_t;

/**
 * Initialize accessibility container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int accessibility_container_init(polycall_core_context_t* core_ctx, accessibility_container_t** container);

/**
 * Register accessibility services
 *
 * @param container accessibility container
 * @return int 0 on success, error code otherwise
 */
int accessibility_register_services(accessibility_container_t* container);

/**
 * Cleanup accessibility container
 *
 * @param container accessibility container
 */
void accessibility_container_cleanup(accessibility_container_t* container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_ACCESSIBILITY_CONTAINER_H */
