#include "adapter_registry.h"

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
