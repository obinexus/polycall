#ifndef POLYCALL_PROVIDER_H
#define POLYCALL_PROVIDER_H

/*
 * Configuration provider hosts (schema v2).
 *
 * A provider expresses configuration in its own language and hands typed
 * values to the C core. Two host mechanisms:
 *
 *   1. Sub-process host  - for Python / Node / Go / ... A provider process is
 *      launched with an explicit argv ARRAY (never a shell string), an
 *      explicit working directory, a wall-clock deadline and a bounded output
 *      budget. It must print exactly one canonical JSON envelope on stdout;
 *      stderr is diagnostic. A non-zero exit is a failure.
 *
 *   2. In-process C host - the provider is a shared library at an explicitly
 *      resolved path exporting `polycall_config_provider_v1`. Only that exact
 *      path is loaded; parent directories are never scanned.
 *
 * Neither host is a sandbox. `doctor` never runs a provider.
 */

#include <stddef.h>
#include <stdint.h>

#include "polycall_export.h"
#include "polycall_config2.h"

POLYCALL_BEGIN_DECLS

#define POLYCALL_PROVIDER_ABI_VERSION 1

/* ---- in-process C provider contract -------------------------------- */

typedef struct {
    uint32_t struct_size;         /* sizeof(polycall_provider_ctx_t)     */
    uint32_t abi_version;         /* POLYCALL_PROVIDER_ABI_VERSION       */
    const char *base_dir;         /* directory the provider lib lives in */
    const char *project_root;     /* --project-root, or NULL            */
    const char *language;         /* --language, or NULL               */
} polycall_provider_ctx_t;

/*
 * Implemented by a C provider shared library. Fill `b` via the builder API.
 * Return 0 on success; non-zero to signal the provider itself failed.
 */
typedef int (POLYCALL_CALL *polycall_config_provider_v1_fn)(
    polycall_config2_builder_t *b, const polycall_provider_ctx_t *ctx);

/* ---- host options ------------------------------------------------- */

typedef struct {
    uint32_t struct_size;         /* sizeof(polycall_provider_opts_t)   */
    const char *cwd;              /* working dir for the sub-process    */
    uint32_t deadline_ms;         /* 0 -> default; capped at MAX        */
    uint32_t max_output_bytes;    /* 0 -> POLYCALL_CFG2_ENVELOPE_MAX    */
    const char *project_root;
    const char *language;
} polycall_provider_opts_t;

POLYCALL_API void POLYCALL_CALL
polycall_provider_opts_init(polycall_provider_opts_t *o);

/*
 * Run a sub-process provider. argv is NULL-terminated; argv[0] is the program
 * (resolved via PATH). On success returns a validated config (caller frees);
 * on failure returns NULL and fills *err. Provider stderr is written through
 * to this process's stderr for diagnostics.
 */
POLYCALL_API polycall_config2_t * POLYCALL_CALL
polycall_provider_run_subprocess(const char *const *argv,
                                 const polycall_provider_opts_t *opts,
                                 polycall_config2_error_t *err);

/*
 * Load an in-process C provider from an explicit shared-library path.
 */
POLYCALL_API polycall_config2_t * POLYCALL_CALL
polycall_provider_load_c(const char *library_path,
                         const polycall_provider_opts_t *opts,
                         polycall_config2_error_t *err);

POLYCALL_END_DECLS

#endif /* POLYCALL_PROVIDER_H */
