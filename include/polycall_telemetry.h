#ifndef POLYCALL_TELEMETRY_H
#define POLYCALL_TELEMETRY_H

/*
 * PolyCall telemetry: GUID + timestamp correlated structured events.
 *
 * Independently versioned from the core library ABI. Every event carries a
 * v4 GUID correlation id and a millisecond UTC wall-clock timestamp; a
 * duration (when present) is computed from polycall_telemetry_mono_ns(),
 * never from the wall clock, which can jump and must never be used for
 * elapsed-time math.
 *
 * Events are appended as one compact JSON object per line (JSONL) to a sink
 * file -- never to stdout/stderr, and never blocking or failing the
 * operation they describe: emit() is a best-effort side channel, silently a
 * no-op when disabled or when the sink cannot be written. Disable with
 * POLYCALL_TELEMETRY=off; override the sink with POLYCALL_TELEMETRY_LOG.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "polycall_export.h"

POLYCALL_BEGIN_DECLS

/* "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx" + NUL */
#define POLYCALL_TELEMETRY_GUID_LEN 37

/* Cryptographically random RFC 4122 version-4 GUID, lowercase hex. */
POLYCALL_API void POLYCALL_CALL
polycall_telemetry_new_guid(char out[POLYCALL_TELEMETRY_GUID_LEN]);

/* Millisecond-resolution UTC wall clock, ISO 8601:
 * "YYYY-MM-DDThh:mm:ss.sssZ" + NUL. For display and cross-process
 * correlation only -- never used for duration math. */
#define POLYCALL_TELEMETRY_TS_LEN 25
POLYCALL_API void POLYCALL_CALL
polycall_telemetry_timestamp(char out[POLYCALL_TELEMETRY_TS_LEN]);

/* Monotonic nanosecond counter from an arbitrary, process-local epoch.
 * Subtract two readings for an elapsed duration; a raw reading is never
 * persisted or compared across processes. */
POLYCALL_API uint64_t POLYCALL_CALL
polycall_telemetry_mono_ns(void);

typedef struct {
    const char *event;          /* required, e.g. "call.start" */
    const char *correlation_id; /* GUID; NULL/empty -> one is generated */
    const char *service;        /* optional */
    const char *operation;      /* optional */
    const char *status;         /* optional, e.g. "ok" / "error" */
    int64_t duration_ns;        /* negative to omit */
    const char *detail;         /* optional short freeform string */
} polycall_telemetry_event_t;

/* Append one structured JSONL record to the sink (see
 * polycall_telemetry_sink_path). Silent no-op when telemetry is disabled,
 * `ev`/`ev->event` is NULL/empty, or the sink cannot be opened -- this never
 * fails or blocks the caller's operation. `project_root` may be NULL. */
POLYCALL_API void POLYCALL_CALL
polycall_telemetry_emit(const char *project_root,
                        const polycall_telemetry_event_t *ev);

#define POLYCALL_TELEMETRY_PATH_MAX 1024

/* Effective sink path: $POLYCALL_TELEMETRY_LOG if set and non-empty, else
 * <project_root>/.polycall/telemetry.jsonl, else ./.polycall/telemetry.jsonl.
 * `project_root` may be NULL. snprintf semantics on buf/buf_cap. */
POLYCALL_API void POLYCALL_CALL
polycall_telemetry_sink_path(const char *project_root, char *buf, size_t buf_cap);

/* False when $POLYCALL_TELEMETRY is "off" / "0" / "false" / "no"
 * (case-insensitive); true otherwise, including when unset (the default). */
POLYCALL_API bool POLYCALL_CALL
polycall_telemetry_enabled(void);

POLYCALL_END_DECLS

#endif /* POLYCALL_TELEMETRY_H */
