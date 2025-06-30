#include "polycall/core/adapters/adapter_orchestrator.h"
#include "adapter_registry.h"
#include <pthread.h>
#include <stdlib.h>

int adapter_orchestrate_transition(adapter_registry_t* registry,
                                   uint64_t thread_id,
                                   uint32_t from_layer,
                                   uint32_t to_layer) {
    if (!registry) return -1;
    pthread_rwlock_rdlock(&registry->rwlock);
    adapter_base_t* from_adapter = registry->adapters[from_layer];
    adapter_base_t* to_adapter = registry->adapters[to_layer];
    pthread_rwlock_unlock(&registry->rwlock);

    if (!from_adapter || !to_adapter) return -1;

    int result = 0;
    if (from_adapter->vtable && from_adapter->vtable->exit_layer) {
        result = from_adapter->vtable->exit_layer(from_adapter, thread_id);
        if (result != 0) return result;
    }

    if (to_adapter->vtable && to_adapter->vtable->enter_layer) {
        result = to_adapter->vtable->enter_layer(to_adapter, thread_id, to_layer);
    }

    return result;
}


int adapter_orchestrate_transition(adapter_registry_t* registry,
                                  uint64_t thread_id,
                                  uint32_t from_layer,
                                  uint32_t to_layer) {
    pthread_rwlock_rdlock(&registry->rwlock);
    adapter_base_t* from = registry->adapters[from_layer];
    adapter_base_t* to = registry->adapters[to_layer];
    pthread_rwlock_unlock(&registry->rwlock);
    if (!from || !to) return -1;
    int result = 0;
    if (from->vtable && from->vtable->exit_layer) {
        result = from->vtable->exit_layer(from, thread_id);
        if (result != 0) return result;
    }
    if (to->vtable && to->vtable->enter_layer) {
        result = to->vtable->enter_layer(to, thread_id, to_layer);
    }
    return result;
}
