#ifndef TOPOLOGY_MANAGER_H
#define TOPOLOGY_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct topology_manager topology_manager_t;

enum {
    TOPOLOGY_LAYER_PYTHON = 0,
    TOPOLOGY_LAYER_GO,
    TOPOLOGY_LAYER_NODEJS,
    TOPOLOGY_LAYER_MAX
};

int topology_manager_validate_transition(topology_manager_t* manager,
                                         uint32_t from_layer,
                                         uint32_t to_layer);

#ifdef __cplusplus
}
#endif

#endif // TOPOLOGY_MANAGER_H
