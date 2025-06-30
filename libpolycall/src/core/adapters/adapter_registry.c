#include "adapter_registry.h"
#include <stdlib.h>

int adapter_registry_init(adapter_registry_t* registry, topology_manager_t* manager) {
    if (!registry || !manager) return -1;
    registry->manager = manager;
    pthread_rwlock_init(&registry->rwlock, NULL);
    for (size_t i = 0; i < TOPOLOGY_LAYER_MAX; i++) {
        registry->adapters[i] = NULL;
    }
    return 0;
}

int adapter_registry_register(adapter_registry_t* registry,
                             uint32_t layer_id,
                             adapter_base_t* adapter) {
    if (!registry || !adapter || layer_id >= TOPOLOGY_LAYER_MAX) return -1;
    pthread_rwlock_wrlock(&registry->rwlock);
    registry->adapters[layer_id] = adapter;
    pthread_rwlock_unlock(&registry->rwlock);
    return 0;
}

adapter_base_t* adapter_registry_get(adapter_registry_t* registry, uint32_t layer_id) {
    if (!registry || layer_id >= TOPOLOGY_LAYER_MAX) return NULL;
    pthread_rwlock_rdlock(&registry->rwlock);
    adapter_base_t* a = registry->adapters[layer_id];
    pthread_rwlock_unlock(&registry->rwlock);
    return a;
}
