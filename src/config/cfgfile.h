#ifndef POLYCALL_INTERNAL_CFGFILE_H
#define POLYCALL_INTERNAL_CFGFILE_H

/*
 * Library access to ONE configuration file (Polycallfile, Polycallrc,
 * Polycallrc.<language>, or a binding's *-polycallrc) using exactly the
 * grammar and validation `polycall config validate` applies -- the same
 * parser in src/polycall_config.c, not a second one. Internal; not installed.
 *
 * Diagnostics are captured into `err` (first error only) instead of being
 * printed, so library callers (bindings, the daemon) get them as data.
 */

#include <stdbool.h>
#include <stddef.h>

#include "core/pc_buf.h"

typedef struct polycall_cfgfile polycall_cfgfile_t;

enum {
    POLYCALL_CFGFILE_OK = 0,
    POLYCALL_CFGFILE_INVALID_ARGUMENT = 1,
    POLYCALL_CFGFILE_NOT_FOUND = 2,
    POLYCALL_CFGFILE_INVALID = 3,
    POLYCALL_CFGFILE_NO_MEMORY = 4
};

/* flags */
#define POLYCALL_CFGFILE_STRICT          0x1u  /* unknown keys are errors          */
#define POLYCALL_CFGFILE_LEGACY_REQUIRED 0x2u  /* `config validate` required fields */

polycall_cfgfile_t *polycall_cfgfile_load(const char *path, unsigned flags,
                                          int *status, char *err, size_t errcap);
void polycall_cfgfile_free(polycall_cfgfile_t *cf);

const char *polycall_cfgfile_get(const polycall_cfgfile_t *cf, const char *key);
unsigned    polycall_cfgfile_warnings(const polycall_cfgfile_t *cf);
int         polycall_cfgfile_is_project(const polycall_cfgfile_t *cf);
int         polycall_cfgfile_network(const polycall_cfgfile_t *cf); /* -1/0/1 */
size_t      polycall_cfgfile_peer_count(const polycall_cfgfile_t *cf);
int         polycall_cfgfile_peer_at(const polycall_cfgfile_t *cf, size_t i,
                                     const char **id, const char **endpoint);

/* Compact JSON description of the effective file. Values are never
 * resolved: auth_token_env is a variable NAME, cert_file a path. */
int polycall_cfgfile_describe(const polycall_cfgfile_t *cf, pc_buf_t *out);

/* validators shared with the daemon / peer CLI */
bool polycall_cfg_valid_peer_id(const char *id);
bool polycall_cfg_valid_env_name(const char *name);
bool polycall_cfg_valid_endpoint(const char *ep, bool allow_port_zero);

#endif /* POLYCALL_INTERNAL_CFGFILE_H */
