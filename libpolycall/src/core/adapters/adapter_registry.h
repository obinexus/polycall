#ifndef ADAPTER_REGISTRY_H
#define ADAPTER_REGISTRY_H

#include <pthread.h>
#include <stdint.h>

#include "adapter_base.h"

typedef struct adapter_registry {
    adapter_base_t* adapters[TOPOLOGY_LAYER_MAX];
    pthread_rwlock_t rwlock;
    topology_manager_t* manager;
} adapter_registry_t;

int adapter_registry_init(adapter_registry_t* registry, topology_manager_t* manager);
int adapter_registry_register(adapter_registry_t* registry,
                             uint32_t layer_id,
                             adapter_base_t* adapter);
adapter_base_t* adapter_registry_get(adapter_registry_t* registry, uint32_t layer_id);

#endif // ADAPTER_REGISTRY_H
