#ifndef ADAPTER_ORCHESTRATOR_H
#define ADAPTER_ORCHESTRATOR_H

#include <stdint.h>
#include "polycall/core/adapters/adapter_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

int adapter_orchestrate_transition(adapter_registry_t* registry,
                                   uint64_t thread_id,
                                   uint32_t from_layer,
                                   uint32_t to_layer);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_ORCHESTRATOR_H */
