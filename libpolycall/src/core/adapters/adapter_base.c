
#include "polycall/core/adapters/adapter_base.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

int adapter_base_init(adapter_base_t* adapter, topology_manager_t* manager) {
    if (!adapter || !manager) return -1;
    adapter->manager = manager;
    atomic_init(&adapter->ref_count, 1);
    pthread_mutex_init(&adapter->mutex, NULL);
    adapter->language_specific_data = NULL;
    return 0;
}

int adapter_base_acquire(adapter_base_t* adapter) {
    if (!adapter) return -1;
    atomic_fetch_add_explicit(&adapter->ref_count, 1, memory_order_relaxed);
    return 0;
}

int adapter_base_release(adapter_base_t* adapter) {
    if (!adapter) return -1;
    if (atomic_fetch_sub_explicit(&adapter->ref_count, 1, memory_order_acq_rel) == 1) {
        if (adapter->vtable && adapter->vtable->cleanup) {
            adapter->vtable->cleanup(adapter);
        }
        pthread_mutex_destroy(&adapter->mutex);
        free(adapter);
    }
    return 0;
}

int adapter_execute_transition(adapter_base_t* adapter,
                               uint64_t thread_id,
                               uint32_t target_layer) {
    if (!adapter || !adapter->manager) return -1;

    pthread_mutex_lock(&adapter->mutex);
    int result = topology_manager_validate_transition(
        adapter->manager,
        adapter->adapter_layer_id,
        target_layer);
    pthread_mutex_unlock(&adapter->mutex);
    return result;
}