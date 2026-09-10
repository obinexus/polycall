#ifndef POLYCALL_CONFIG2_H
#define POLYCALL_CONFIG2_H

/*
 * PolyCall typed configuration model, schema v2.
 *
 * This is a SEPARATE, independently versioned surface from the legacy
 * `polycall_config_t` in polycall.h. That struct's public layout is
 * unchanged; nothing here touches it.
 *
 * The model is the single internal representation. Language providers
 * (C / Python / Node / ...) express native values and hand them to a builder;
 * the C core validates every value regardless of what the provider checked.
 * The model is never serialised as a C memory layout -- the canonical
 * exchange form is the JSON envelope (see polycall_config2_to_envelope).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "polycall_export.h"

POLYCALL_BEGIN_DECLS

#define POLYCALL_CONFIG2_SCHEMA_VERSION 2
#define POLYCALL_CONFIG2_ABI_VERSION    1

/* ---- value limits (documented, enforced by the core) ------------------- */
#define POLYCALL_CFG2_MAX_SERVICES     64
#define POLYCALL_CFG2_NAME_MAX         64    /* identifiers, languages       */
#define POLYCALL_CFG2_PATH_MAX         512   /* workspace / cert refs        */
#define POLYCALL_CFG2_ADDR_MAX         128   /* bind address                 */
#define POLYCALL_CFG2_NS_MAX           64    /* extension namespace          */
#define POLYCALL_CFG2_ENVELOPE_MAX     (1u << 20)  /* 1 MiB provider output  */
#define POLYCALL_CFG2_PROVIDER_DEADLINE_MS_DEFAULT 10000
#define POLYCALL_CFG2_PROVIDER_DEADLINE_MS_MAX     120000

typedef enum {
    POLYCALL_TLS_OFF    = 0,
    POLYCALL_TLS_SERVER = 1,   /* server presents a certificate            */
    POLYCALL_TLS_MUTUAL = 2    /* server + client certificates             */
} polycall_tls_mode_t;

/* Where a resolved value came from -- retained for `config show --provenance` */
typedef enum {
    POLYCALL_SRC_DEFAULT   = 0, /* compiled-in default                     */
    POLYCALL_SRC_NATIVE_GLOBAL = 1,
    POLYCALL_SRC_NATIVE_PROJECT = 2,
    POLYCALL_SRC_LANG_OVERRIDE  = 3,
    POLYCALL_SRC_ENV       = 4, /* allowlisted environment field           */
    POLYCALL_SRC_CLI       = 5, /* --set on the command line               */
    POLYCALL_SRC_LEGACY    = 6  /* imported from a v1 file during migrate   */
} polycall_cfg_source_t;

/* ---- opaque handles -------------------------------------------------- */
typedef struct polycall_config2         polycall_config2_t;        /* validated */
typedef struct polycall_config2_builder polycall_config2_builder_t;

/* ---- structured error ---------------------------------------------- */
typedef struct {
    uint32_t struct_size;      /* set to sizeof(polycall_config2_error_t) */
    char     code[48];         /* stable, e.g. "port.range"              */
    char     message[256];
    char     field[128];       /* dotted path, e.g. "services[1].host_port" */
} polycall_config2_error_t;

POLYCALL_API void POLYCALL_CALL
polycall_config2_error_init(polycall_config2_error_t *err);

/* ---- read view of one service (extensible via struct_size) --------- */
typedef struct {
    uint32_t struct_size;     /* caller sets sizeof(polycall_service_view_t) */
    const char *id;
    const char *language;
    const char *bind_address;
    uint16_t    host_port;
    uint16_t    target_port;
    const char *workspace;
    uint32_t    timeout_ms;
    uint32_t    max_connections;
    polycall_tls_mode_t tls_mode;
    const char *tls_cert_ref;   /* env:/file: reference, never a secret     */
    const char *tls_key_ref;
} polycall_service_view_t;

/* ================================================================== */
/* builder                                                             */
/* ================================================================== */

POLYCALL_API polycall_config2_builder_t * POLYCALL_CALL
polycall_config2_builder_create(void);

POLYCALL_API void POLYCALL_CALL
polycall_config2_builder_free(polycall_config2_builder_t *b);

/* Directory the declaring provider lives in; relative paths in the model
 * resolve against it and keep that provenance. May be NULL. */
POLYCALL_API void POLYCALL_CALL
polycall_config2_builder_set_base_dir(polycall_config2_builder_t *b,
                                      const char *dir);

POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_set_project(polycall_config2_builder_t *b,
                                     const char *project_name,
                                     const char *extension_namespace);

/* Returns the new service index (>=0) or -1 on error (see *err). */
POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_add_service(polycall_config2_builder_t *b,
                                     const char *id, const char *language,
                                     polycall_config2_error_t *err);

POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_service_set_ports(polycall_config2_builder_t *b,
                                           int svc_index,
                                           uint32_t host_port,
                                           uint32_t target_port,
                                           polycall_config2_error_t *err);

POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_service_set_bind(polycall_config2_builder_t *b,
                                          int svc_index, const char *bind_address,
                                          polycall_config2_error_t *err);

POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_service_set_workspace(polycall_config2_builder_t *b,
                                               int svc_index, const char *path,
                                               polycall_config2_error_t *err);

POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_service_set_limits(polycall_config2_builder_t *b,
                                            int svc_index, uint32_t timeout_ms,
                                            uint32_t max_connections,
                                            polycall_config2_error_t *err);

POLYCALL_API int POLYCALL_CALL
polycall_config2_builder_service_set_tls(polycall_config2_builder_t *b,
                                         int svc_index, polycall_tls_mode_t mode,
                                         const char *cert_ref, const char *key_ref,
                                         polycall_config2_error_t *err);

/* Validate and freeze. On success returns a config (caller frees with
 * polycall_config2_free); on failure returns NULL and fills *err. */
POLYCALL_API polycall_config2_t * POLYCALL_CALL
polycall_config2_builder_build(polycall_config2_builder_t *b,
                               polycall_config2_error_t *err);

/* ================================================================== */
/* validated config: read + serialise                                  */
/* ================================================================== */

POLYCALL_API void POLYCALL_CALL
polycall_config2_free(polycall_config2_t *cfg);

POLYCALL_API const char * POLYCALL_CALL
polycall_config2_project_name(const polycall_config2_t *cfg);

POLYCALL_API const char * POLYCALL_CALL
polycall_config2_extension_namespace(const polycall_config2_t *cfg);

POLYCALL_API int POLYCALL_CALL
polycall_config2_service_count(const polycall_config2_t *cfg);

/* view->struct_size must be set by the caller. Returns 0 on success. */
POLYCALL_API int POLYCALL_CALL
polycall_config2_service_at(const polycall_config2_t *cfg, int index,
                            polycall_service_view_t *view);

/*
 * Canonical JSON envelope: object keys emitted in a fixed order, booleans as
 * booleans, ports as numbers, arguments as arrays, explicit units already
 * normalised (ms, bytes). Two configs with equal effective values produce
 * byte-identical envelopes -- this is the cross-language equivalence oracle.
 *
 * Returns the number of bytes that WOULD be written (excluding NUL), like
 * snprintf; writes at most cap-1 bytes plus a NUL when cap > 0.
 */
POLYCALL_API int POLYCALL_CALL
polycall_config2_to_envelope(const polycall_config2_t *cfg,
                             char *buf, size_t cap);

/*
 * Parse + validate one envelope. `with_provenance` values, when present, are
 * recorded. Returns a config (caller frees) or NULL with *err set.
 */
POLYCALL_API polycall_config2_t * POLYCALL_CALL
polycall_config2_from_envelope(const char *text, size_t len,
                               polycall_config2_error_t *err);

/* Provenance for one dotted field path, for `config show --provenance`.
 * Returns POLYCALL_SRC_DEFAULT when nothing more specific was recorded. */
POLYCALL_API polycall_cfg_source_t POLYCALL_CALL
polycall_config2_provenance(const polycall_config2_t *cfg, const char *field);

/*
 * Compute the legacy (v1) effective model using the documented legacy
 * precedence (Polycallfile -> Polycallrc -> Polycallrc.<language>, later
 * scalars win) and project it onto the schema-v2 model, preserving the
 * effective values. Original files are never modified.
 *
 * `project_root` may be NULL (means "."). `language` may be NULL (skip the
 * third layer). Legacy keys that have no v2 home are appended to `unresolved`
 * as "file:line key" lines (never silently dropped); pass a buffer or NULL.
 *
 * Returns a validated config (caller frees) or NULL with *err set.
 */
POLYCALL_API polycall_config2_t * POLYCALL_CALL
polycall_config2_import_legacy(const char *project_root, const char *language,
                               char *unresolved, size_t unresolved_cap,
                               polycall_config2_error_t *err);

POLYCALL_END_DECLS

#endif /* POLYCALL_CONFIG2_H */
