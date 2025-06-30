#ifndef ADAPTER_REGISTRY_H
#define ADAPTER_REGISTRY_H

#include <pthread.h>
#include <stdint.h>
#include "polycall/core/adapters/adapter_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOPOLOGY_LAYER_MAX 32

typedef struct adapter_registry {
    adapter_base_t* adapters[TOPOLOGY_LAYER_MAX];
    pthread_rwlock_t rwlock;
    struct topology_manager* manager;
} adapter_registry_t;

int adapter_registry_init(adapter_registry_t* registry, struct topology_manager* manager);
int adapter_registry_register(adapter_registry_t* registry, uint32_t layer_id, adapter_base_t* adapter);
adapter_base_t* adapter_registry_get(adapter_registry_t* registry, uint32_t layer_id);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_REGISTRY_H */
