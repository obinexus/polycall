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
#include <node_api.h>

typedef struct nodejs_adapter {
    adapter_base_t base;
    napi_env env;
    napi_ref callback_ref;
    uv_async_t* async_handle;
} nodejs_adapter_t;

static int nodejs_adapter_init(void* adapter, topology_manager_t* manager) {
    nodejs_adapter_t* na = (nodejs_adapter_t*)adapter;
    if (adapter_base_init(&na->base, manager) != 0) return -1;
    na->base.adapter_layer_id = TOPOLOGY_LAYER_NODEJS;
    return 0;
}

static int nodejs_adapter_enter_layer(void* adapter,
                                     uint64_t thread_id,
                                     uint32_t layer_id) {
    nodejs_adapter_t* na = (nodejs_adapter_t*)adapter;
    napi_handle_scope scope;
    napi_open_handle_scope(na->env, &scope);
    int result = adapter_execute_transition(&na->base, thread_id, layer_id);
    if (result == 0 && na->async_handle) {
        uv_async_send(na->async_handle);
    }
    napi_close_handle_scope(na->env, scope);
    return result;
}

static int nodejs_adapter_exit_layer(void* adapter, uint64_t thread_id) {
    (void)adapter; (void)thread_id;
    return 0;
}

static int nodejs_adapter_cleanup(void* adapter) {
    (void)adapter;
    return 0;
}

adapter_base_t* create_nodejs_adapter(topology_manager_t* manager, napi_env env) {
    nodejs_adapter_t* adapter = calloc(1, sizeof(nodejs_adapter_t));
    if (!adapter) return NULL;
    adapter->env = env;
    static adapter_vtable_t vt = {
        .init = nodejs_adapter_init,
        .enter_layer = nodejs_adapter_enter_layer,
        .exit_layer = nodejs_adapter_exit_layer,
        .validate_transition = NULL,
        .emit_trace = NULL,
        .cleanup = nodejs_adapter_cleanup
    };
    adapter->base.vtable = &vt;
    if (adapter->base.vtable->init(adapter, manager) != 0) {
        free(adapter);
        return NULL;
    }
    return &adapter->base;
}
