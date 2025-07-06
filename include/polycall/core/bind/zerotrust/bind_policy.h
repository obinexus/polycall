#ifndef POLYCALL_BIND_POLICY_H
#define POLYCALL_BIND_POLICY_H

#include "polycall/core/polycall_core.h"

typedef enum {
  POLYCALL_BIND_POLICY_ALLOW,
  POLYCALL_BIND_POLICY_DENY,
  POLYCALL_BIND_POLICY_CHALLENGE
} polycall_bind_policy_decision_t;

typedef struct polycall_bind_policy_engine polycall_bind_policy_engine_t;
typedef struct polycall_bind_context polycall_bind_context_t;
typedef struct polycall_bind_operation polycall_bind_operation_t;

polycall_bind_policy_decision_t
polycall_bind_evaluate_policy(polycall_bind_policy_engine_t *engine,
                              polycall_bind_context_t *context,
                              polycall_bind_operation_t *operation);

#endif /* POLYCALL_BIND_POLICY_H */
