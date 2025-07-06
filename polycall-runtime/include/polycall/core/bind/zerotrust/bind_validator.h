#ifndef POLYCALL_BIND_VALIDATOR_H
#define POLYCALL_BIND_VALIDATOR_H

#include "polycall/core/polycall_core.h"
#include "polycall/core/polycall_error.h"

typedef struct {
    void* crypto_ctx;
    void* policy_engine;
    void* audit_log;
} polycall_bind_validator_t;

typedef struct {
    uint8_t challenge[32];
    uint8_t signature[64];
    uint64_t timestamp;
} polycall_bind_request_t;

typedef struct {
    uint8_t proof[64];
    uint32_t validity_period;
} polycall_bind_proof_t;

polycall_core_error_t polycall_bind_validate_trust(
    polycall_bind_validator_t* validator,
    polycall_bind_request_t* request,
    polycall_bind_proof_t* proof
);

#endif /* POLYCALL_BIND_VALIDATOR_H */
