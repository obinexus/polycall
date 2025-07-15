#ifndef POLYCALL_BIND_AUTHENTICATOR_H
#define POLYCALL_BIND_AUTHENTICATOR_H

#include "polycall/core/polycall_core.h"

typedef struct {
  uint32_t sig_scheme;
  uint32_t hash_algo;
  uint32_t kex_algo;
} polycall_bind_auth_config_t;

typedef struct {
  char *identity_name;
  uint8_t public_key[32];
  uint32_t permissions;
} polycall_bind_identity_t;

typedef struct {
  uint8_t nonce[32];
  uint64_t timestamp;
} polycall_bind_challenge_t;

typedef struct polycall_bind_authenticator polycall_bind_authenticator_t;

polycall_core_error_t
polycall_bind_authenticate(polycall_bind_authenticator_t *auth,
                           polycall_bind_identity_t *identity,
                           polycall_bind_challenge_t *challenge);

#endif /* POLYCALL_BIND_AUTHENTICATOR_H */
