#ifndef ADAPTER_BASE_H
#define ADAPTER_BASE_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include "polycall/core/topology/topology_manager.h"
#include "polycall/core/trace/polycall_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adapter_vtable {
  int (*init)(void *adapter, topology_manager_t *manager);
  int (*enter_layer)(void *adapter, uint64_t thread_id, uint32_t layer_id);
  int (*exit_layer)(void *adapter, uint64_t thread_id);
  int (*validate_transition)(void *adapter, uint32_t from, uint32_t to);
  int (*emit_trace)(void *adapter, trace_event_t *event);
  int (*cleanup)(void *adapter);
} adapter_vtable_t;

typedef struct adapter_base {
  adapter_vtable_t *vtable;
  topology_manager_t *manager;
  atomic_int ref_count;
  pthread_mutex_t mutex;
  void *language_specific_data;
  uint32_t adapter_layer_id;
} adapter_base_t;

int adapter_base_init(adapter_base_t *adapter, topology_manager_t *manager);
int adapter_base_acquire(adapter_base_t *adapter);
int adapter_base_release(adapter_base_t *adapter);

int adapter_execute_transition(adapter_base_t *adapter, uint64_t thread_id,
                               uint32_t target_layer);

#ifdef __cplusplus
}
#endif

#endif // ADAPTER_BASE_H
