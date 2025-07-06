#ifndef POLYCALL_SECURE_CORE_H
#define POLYCALL_SECURE_CORE_H

#include <stdint.h>
#include "polycall/core/polycall_core.h"

typedef struct {
    void* model_data;
    void* sec_ctx;
    void* acl;
    uint8_t hash[32];
} polycall_protected_model_t;

typedef enum {
    POLYCALL_SEC_LEVEL_PUBLIC,
    POLYCALL_SEC_LEVEL_INTERNAL,
    POLYCALL_SEC_LEVEL_RESTRICTED,
    POLYCALL_SEC_LEVEL_CRITICAL
} polycall_security_level_t;

#define POLYCALL_PROTECT_MODEL(model) \
    polycall_wrap_model_secure(&(model), POLYCALL_SEC_LEVEL_CRITICAL)

polycall_core_error_t polycall_wrap_model_secure(
    void* model,
    polycall_security_level_t level
);

#endif /* POLYCALL_SECURE_CORE_H */
