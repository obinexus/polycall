/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

#include "adapter_base.h"
#include <Python.h>

typedef struct python_adapter {
  adapter_base_t base;
  PyObject *callback_dict;
  PyGILState_STATE gil_state;
} python_adapter_t;

static int python_adapter_init(void *adapter, topology_manager_t *manager) {
  python_adapter_t *py = (python_adapter_t *)adapter;
  if (adapter_base_init(&py->base, manager) != 0)
    return -1;
  py->base.adapter_layer_id = TOPOLOGY_LAYER_PYTHON;
  py->callback_dict = PyDict_New();
  if (!py->callback_dict)
    return -1;
  return 0;
}

static int python_adapter_enter_layer(void *adapter, uint64_t thread_id,
                                      uint32_t layer_id) {
  python_adapter_t *py = (python_adapter_t *)adapter;
  py->gil_state = PyGILState_Ensure();
  int result = adapter_execute_transition(&py->base, thread_id, layer_id);
  if (result != 0) {
    PyGILState_Release(py->gil_state);
  }
  return result;
}

static int python_adapter_exit_layer(void *adapter, uint64_t thread_id) {
  python_adapter_t *py = (python_adapter_t *)adapter;
  PyGILState_Release(py->gil_state);
  (void)thread_id;
  return 0;
}

static int python_adapter_cleanup(void *adapter) {
  python_adapter_t *py = (python_adapter_t *)adapter;
  Py_XDECREF(py->callback_dict);
  return 0;
}

adapter_base_t *create_python_adapter(topology_manager_t *manager) {
  python_adapter_t *adapter = calloc(1, sizeof(python_adapter_t));
  if (!adapter)
    return NULL;
  static adapter_vtable_t vt = {.init = python_adapter_init,
                                .enter_layer = python_adapter_enter_layer,
                                .exit_layer = python_adapter_exit_layer,
                                .validate_transition = NULL,
                                .emit_trace = NULL,
                                .cleanup = python_adapter_cleanup};
  adapter->base.vtable = &vt;
  if (adapter->base.vtable->init(adapter, manager) != 0) {
    free(adapter);
    return NULL;
  }
  return &adapter->base;
}
