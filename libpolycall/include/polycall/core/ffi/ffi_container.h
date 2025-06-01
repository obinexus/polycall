/**
 * @file ffi_container.h
 * @brief Container for ffi module
 */

#ifndef POLYCALL_FFI_CONTAINER_H
#define POLYCALL_FFI_CONTAINER_H

#include "polycall/core/polycall/polycall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ffi container structure
 */
typedef struct ffi_container {
    polycall_core_context_t* core_ctx;
    void* module_data;
    // Add component-specific fields here
} ffi_container_t;

/**
 * Initialize ffi container
 *
 * @param core_ctx Core context
 * @param container Pointer to receive container
 * @return int 0 on success, error code otherwise
 */
int ffi_container_init(polycall_core_context_t* core_ctx, ffi_container_t** container);

/**
 * Register ffi services
 *
 * @param container ffi container
 * @return int 0 on success, error code otherwise
 */
int ffi_register_services(ffi_container_t* container);

/**
 * Cleanup ffi container
 *
 * @param container ffi container
 */
void ffi_container_cleanup(ffi_container_t* container);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_CONTAINER_H */
