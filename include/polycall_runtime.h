#ifndef POLYCALL_RUNTIME_H
#define POLYCALL_RUNTIME_H

/*
 * PolyCall runtime: operation registry + foreground server (schema v1).
 *
 * Independently versioned from the core library ABI, the config schema and the
 * legacy polycall_protocol.h wire. The CLI (`run`/`status`/`stop`/`call`) and
 * the language clients all reach the SAME registered C operation through this
 * runtime -- there is no per-language reimplementation and no echo fallback.
 *
 * Wire: polycall_rpc v1 (see src/runtime/rpc_wire.h and docs/RPC.md). Payloads
 * are UTF-8 JSON; no C struct layout or pointer ever crosses the socket.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "polycall_export.h"

POLYCALL_BEGIN_DECLS

#define POLYCALL_RUNTIME_ABI_VERSION 1

typedef struct polycall_runtime polycall_runtime_t;

/* Distinct outcome classes -> mapped to CLI exit codes centrally. */
typedef enum {
    POLYCALL_OP_OK            = 0,
    POLYCALL_OP_ERR_INPUT     = 1,  /* malformed / missing input          */
    POLYCALL_OP_ERR_NOTFOUND  = 2,  /* unknown service/operation or item  */
    POLYCALL_OP_ERR_INTERNAL  = 3,  /* operation raised an internal error */
    POLYCALL_OP_ERR_DEADLINE  = 4   /* operation exceeded its deadline    */
} polycall_op_status_t;

/*
 * An operation handler. `input_json` is the request's "input" member as raw
 * JSON text (never NULL; "null" when absent). The handler writes a JSON value
 * for "output" into out/out_cap (snprintf semantics) and returns a status.
 * On a non-OK status it instead writes {code,message} fields text into
 * err_code/err_msg. Handlers must be deterministic and must not block on the
 * network. `deadline_ms` is advisory; the runtime also enforces it.
 */
typedef polycall_op_status_t (*polycall_op_fn)(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap,
    char *err_code, size_t err_code_cap,
    char *err_msg, size_t err_msg_cap,
    void *user);

typedef struct {
    const char *service;
    const char *operation;
    const char *summary;
    const char *input_schema;   /* one-line hint, e.g. {"item_id":"string"} */
    const char *output_schema;
    bool idempotent;            /* clients must not auto-retry when false   */
    polycall_op_fn fn;
    void *user;
} polycall_op_desc_t;

/* Build a runtime and register the built-in deterministic operations
 * (inventory.get, debug.echo, debug.sleep). Returns NULL on OOM. */
POLYCALL_API polycall_runtime_t * POLYCALL_CALL
polycall_runtime_create(void);

POLYCALL_API void POLYCALL_CALL
polycall_runtime_destroy(polycall_runtime_t *rt);

POLYCALL_API int POLYCALL_CALL
polycall_runtime_register(polycall_runtime_t *rt, const polycall_op_desc_t *desc);

/* Called once, right after bind/listen and before the accept loop, with the
 * actually-bound "host:port". Lets a supervisor learn an ephemeral port. */
typedef void (POLYCALL_CALL *polycall_on_bound_fn)(const char *endpoint,
                                                   void *user);

/* Serve polycall_rpc v1 on a bound TCP socket until a signal is observed or an
 * authenticated shutdown arrives. Foreground / blocking. `bind_host` defaults
 * to 127.0.0.1; pass port 0 for an ephemeral port. `on_bound` (may be NULL) is
 * invoked with the resolved endpoint before the first accept. `auth_token`
 * (may be NULL) gates the shutdown control action. Cleanup happens here, in
 * normal execution, not in a handler.
 *
 * Returns 0 on a clean shutdown, non-zero on a bind/listen failure.
 */
POLYCALL_API int POLYCALL_CALL
polycall_runtime_serve(polycall_runtime_t *rt, const char *bind_host,
                       uint16_t port, const char *auth_token,
                       polycall_on_bound_fn on_bound, void *on_bound_user);

/* Ask the currently-serving runtime in THIS process to stop (signal-safe:
 * only sets a flag). Wired to SIGINT/SIGTERM by the CLI. */
POLYCALL_API void POLYCALL_CALL
polycall_runtime_request_stop(void);

POLYCALL_END_DECLS

#endif /* POLYCALL_RUNTIME_H */
