/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

#include "adapter_base.h"

typedef struct go_adapter {
    adapter_base_t base;
    uintptr_t go_handle;
    pthread_t owner_thread;
} go_adapter_t;

static int go_adapter_init(void* adapter, topology_manager_t* manager) {
    go_adapter_t* go = (go_adapter_t*)adapter;
    if (adapter_base_init(&go->base, manager) != 0) return -1;
    go->base.adapter_layer_id = TOPOLOGY_LAYER_GO;
    go->owner_thread = pthread_self();
    return 0;
}

static int go_adapter_enter_layer(void* adapter, uint64_t thread_id, uint32_t layer_id) {
    go_adapter_t* go = (go_adapter_t*)adapter;
    if (!pthread_equal(pthread_self(), go->owner_thread)) {
        return -1;
    }
    return adapter_execute_transition(&go->base, thread_id, layer_id);
}

static int go_adapter_exit_layer(void* adapter, uint64_t thread_id) {
    (void)adapter; (void)thread_id;
    return 0;
}

static int go_adapter_cleanup(void* adapter) {
    (void)adapter;
    return 0;
}

adapter_base_t* create_go_adapter(topology_manager_t* manager, uintptr_t handle) {
    go_adapter_t* adapter = calloc(1, sizeof(go_adapter_t));
    if (!adapter) return NULL;
    adapter->go_handle = handle;
    static adapter_vtable_t vt = {
        .init = go_adapter_init,
        .enter_layer = go_adapter_enter_layer,
        .exit_layer = go_adapter_exit_layer,
        .validate_transition = NULL,
        .emit_trace = NULL,
        .cleanup = go_adapter_cleanup
    };
    adapter->base.vtable = &vt;
    if (adapter->base.vtable->init(adapter, manager) != 0) {
        free(adapter);
        return NULL;
    }
    return &adapter->base;
}
