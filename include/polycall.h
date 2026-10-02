#ifndef POLYCALL_H
#define POLYCALL_H

/*
 * polycall.h -- the public consumer header of the Polycall C library.
 *
 *   #include <polycall.h>       (with -I<prefix>/include/polycall, which
 *                                `pkg-config --cflags polycall` and the
 *                                CMake target polycall::polycall provide)
 *   #include <polycall/polycall.h>   (no extra -I needed)
 *
 * The library keeps its historical identity (libpolycall.so.1 /
 * polycall.dll, SONAME and symbol names unchanged). <libpolycall.h> is a
 * deprecated forwarding header for older consumers; new code includes
 * this file.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "polycall_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define POLYCALL_MAX_NAME_LENGTH 32
#define POLYCALL_MAX_STATES 32
#define POLYCALL_MAX_TRANSITIONS 64

/* Forward declarations */
struct polycall_context;
typedef struct polycall_context* polycall_context_t;

/* Type definitions */
typedef void (*PolyCall_StateAction)(polycall_context_t ctx);

/* Status codes */
typedef enum {
    POLYCALL_SUCCESS = 0,
    POLYCALL_ERROR_INVALID_PARAMETERS,
    POLYCALL_ERROR_INITIALIZATION_FAILED,
    POLYCALL_ERROR_OUT_OF_MEMORY,
    POLYCALL_ERROR
} polycall_status_t;

/* Configuration structure */
typedef struct polycall_config {
    unsigned int flags;
    size_t memory_pool_size;
    void* user_data;
} polycall_config_t;

/* API Functions */

/**
 * Initialize the PolyCall library with configuration
 * 
 * @param ctx Pointer to receive the created context
 * @param config Pointer to configuration structure
 * @return Status code indicating success or failure
 */
POLYCALL_API polycall_status_t POLYCALL_CALL polycall_init_with_config(
    polycall_context_t* ctx,
    const polycall_config_t* config
);

/**
 * Clean up and release resources associated with a PolyCall context
 *
 * @param ctx Context to clean up
 */
POLYCALL_API void POLYCALL_CALL polycall_cleanup(polycall_context_t ctx);

/**
 * Get the version string of the PolyCall library
 *
 * @return Null-terminated version string
 */
POLYCALL_API const char* POLYCALL_CALL polycall_get_version(void);

/**
 * Get the last error message from the PolyCall library
 *
 * @param ctx Context to get error from
 * @return Null-terminated error message string
 */
POLYCALL_API const char* POLYCALL_CALL polycall_get_last_error(polycall_context_t ctx);

/* ======================================================================
 * Binding ABI v1
 *
 * The one surface every language binding reaches through its native FFI
 * (ctypes, cgo, JNI/JNA, P/Invoke, Fiddle, PHP FFI, NIFs, CFFI, DPI-C ...).
 * Rules that make it safe across languages and C runtimes:
 *
 *   - Only C scalars, NUL-terminated UTF-8 strings, (pointer, length)
 *     byte buffers and int32 handles cross the boundary. No struct layout,
 *     no callback, no varargs.
 *   - The library never returns memory for the caller to free. Every
 *     output goes into a CALLER-OWNED buffer with an explicit capacity, so
 *     there is no allocator-ownership mismatch between, e.g., an MSVC
 *     consumer and a MinGW-built DLL, and nothing can leak across FFI.
 *   - Output buffers follow snprintf rules: on success the text is
 *     NUL-terminated and *out_len (when non-NULL) receives its length.
 *     When the buffer is too small the call returns POLYCALL_E_TOO_LARGE
 *     and *out_len receives the size needed (excluding the NUL).
 *   - Every function returns an int status: POLYCALL_OK (0) or a negative
 *     POLYCALL_E_* code. polycall_strerror() names a code;
 *     polycall_last_error() gives the calling thread's detail message.
 *   - Peer handles are integers validated on every call: an unknown,
 *     closed or stale handle is POLYCALL_E_INVALID_HANDLE -- never a crash
 *     -- so double close and use-after-close are defined behaviour.
 *   - All functions are thread-safe. Calls on the same peer handle may be
 *     made concurrently from any thread.
 * ====================================================================== */

#define POLYCALL_FFI_ABI_VERSION 1

/* status codes (append-only; values are never reused) */
#define POLYCALL_OK                    0
#define POLYCALL_E_INVALID_ARGUMENT  (-1)  /* NULL/empty/malformed argument   */
#define POLYCALL_E_NO_MEMORY         (-2)
#define POLYCALL_E_INVALID_HANDLE    (-3)  /* unknown, closed or stale handle */
#define POLYCALL_E_TIMEOUT           (-4)
#define POLYCALL_E_TRANSPORT         (-5)  /* refused / reset / unreachable   */
#define POLYCALL_E_PROTOCOL          (-6)  /* malformed reply, wrong identity */
#define POLYCALL_E_NOT_FOUND         (-7)  /* unknown peer id / file / op     */
#define POLYCALL_E_AUTH              (-8)  /* shared token missing or wrong   */
#define POLYCALL_E_REMOTE            (-9)  /* the remote side reported an error */
#define POLYCALL_E_TOO_LARGE        (-10)  /* over a limit, or buffer too small */
#define POLYCALL_E_BUSY             (-11)  /* backpressure: queue/conn limit  */
#define POLYCALL_E_CANCELLED        (-12)  /* polycall_peer_cancel() woke it  */
#define POLYCALL_E_CONFIG           (-13)  /* invalid configuration           */
#define POLYCALL_E_ADDRESS_IN_USE   (-14)
#define POLYCALL_E_UNSUPPORTED      (-15)  /* valid, but not in this build    */
#define POLYCALL_E_PERMISSION       (-16)  /* OS refused (port, file, ...)    */
#define POLYCALL_E_CLOSED           (-17)  /* handle closed while blocked     */
#define POLYCALL_E_INTERNAL         (-18)

/* limits */
#define POLYCALL_PEER_ID_MAX        64        /* buffer size incl. NUL; ids are
                                                 1..63 of [A-Za-z0-9._-]       */
#define POLYCALL_MESSAGE_ID_MAX     64        /* same alphabet as peer ids     */
#define POLYCALL_ENDPOINT_MAX       128       /* "host:port" incl. NUL         */
#define POLYCALL_PEER_MAX_PAYLOAD   (1u << 20) /* 1 MiB per message           */
#define POLYCALL_CALL_MAX_OUTPUT    (1u << 20)

/* peer handle: > 0 when valid */
typedef int32_t polycall_peer_t;

/* ---- library ---------------------------------------------------------- */

/** POLYCALL_FFI_ABI_VERSION of the loaded library (bindings must check it). */
POLYCALL_API int POLYCALL_CALL polycall_ffi_abi_version(void);

/** Version string into buf (snprintf rules). Returns the string length
 *  (>= 0), or POLYCALL_E_INVALID_ARGUMENT when len < 0. */
POLYCALL_API int POLYCALL_CALL polycall_ffi_version(char *buf, int len);

/** Static, never-NULL name for a status code ("POLYCALL_E_TIMEOUT: ..."). */
POLYCALL_API const char * POLYCALL_CALL polycall_strerror(int status);

/** Detail of the most recent failure on THIS thread (snprintf rules).
 *  Returns its length; empty when the last call on this thread succeeded. */
POLYCALL_API int POLYCALL_CALL polycall_last_error(char *buf, size_t cap);

/* ---- configuration ------------------------------------------------------ */

/**
 * Load and validate one configuration file -- a Polycallfile, Polycallrc,
 * Polycallrc.<language>, or a binding's *-polycallrc -- with exactly the
 * grammar `polycall config validate` uses.
 *
 * run == 0: validate (unknown keys are warnings).
 * run != 0: validate for running with this build: unknown keys are errors
 *           and settings this build cannot honour (tls_enabled=true; TLS is
 *           not implemented) are rejected with POLYCALL_E_UNSUPPORTED.
 *
 * Never starts a service and never touches the network. Returns
 * POLYCALL_OK, POLYCALL_E_INVALID_ARGUMENT, POLYCALL_E_NOT_FOUND (missing
 * file), POLYCALL_E_CONFIG (invalid) or POLYCALL_E_UNSUPPORTED.
 */
POLYCALL_API int POLYCALL_CALL polycall_ffi_run_config(const char *config_path,
                                                       int run);

/** JSON description of the file at config_path (layer, servers, peers,
 *  key/values; secrets are never resolved). snprintf rules; returns the
 *  JSON length (>= 0) or a negative status. */
POLYCALL_API int POLYCALL_CALL polycall_ffi_describe(const char *config_path,
                                                     char *buf, int len);

/* ---- RPC: call an operation on a running runtime / daemon ---------------- */

/**
 * One polycall_rpc v1 round trip to `endpoint` ("host:port"). input_json
 * may be NULL (sent as null). On POLYCALL_OK `out` receives the operation's
 * "output" JSON value; on POLYCALL_E_REMOTE it receives the error object
 * {"code":..,"message":..}. The call executes once and is never retried
 * here: if `out` is too small (POLYCALL_E_TOO_LARGE, *out_len = needed) the
 * result is discarded -- size `out` for POLYCALL_CALL_MAX_OUTPUT when unsure.
 */
POLYCALL_API int POLYCALL_CALL polycall_call(const char *endpoint,
                                             const char *service,
                                             const char *operation,
                                             const char *input_json,
                                             uint32_t timeout_ms,
                                             char *out, size_t out_cap,
                                             size_t *out_len);

/* ---- peer-to-peer node (NSIGII-style, no central broker) ---------------- */

/**
 * Open a peer node named node_id. With bind_endpoint ("host:port", port 0 =
 * ephemeral) the node listens for peers on its own thread (HTTP/1.1,
 * protocol "polycall-peer/1", see docs/PEER_PROTOCOL.md); with NULL it is
 * send-only. auth_token (NULL/"" = none) is the shared secret every
 * request to this node must carry and that this node presents to peers; a
 * node bound to a non-loopback address without a token is refused
 * (POLYCALL_E_CONFIG). The registry and inbox belong to this node only --
 * two nodes (in this or any other process) never share state implicitly.
 */
POLYCALL_API int POLYCALL_CALL polycall_peer_open(const char *node_id,
                                                  const char *bind_endpoint,
                                                  const char *auth_token,
                                                  polycall_peer_t *out_handle);

/** Stop the listener, wake blocked calls (POLYCALL_E_CLOSED), release
 *  everything. A second close returns POLYCALL_E_INVALID_HANDLE. */
POLYCALL_API int POLYCALL_CALL polycall_peer_close(polycall_peer_t h);

/** Bound "host:port" ("" for a send-only node). snprintf rules. */
POLYCALL_API int POLYCALL_CALL polycall_peer_endpoint(polycall_peer_t h,
                                                      char *buf, size_t cap);
POLYCALL_API int POLYCALL_CALL polycall_peer_node_id(polycall_peer_t h,
                                                     char *buf, size_t cap);

/** Add or replace peer_id -> endpoint in THIS node's registry. */
POLYCALL_API int POLYCALL_CALL polycall_peer_register(polycall_peer_t h,
                                                      const char *peer_id,
                                                      const char *endpoint);
/** POLYCALL_E_NOT_FOUND when peer_id is not registered. */
POLYCALL_API int POLYCALL_CALL polycall_peer_unregister(polycall_peer_t h,
                                                        const char *peer_id);
/** THIS node's registry as a JSON object {"id":"host:port",...}. */
POLYCALL_API int POLYCALL_CALL polycall_peer_list(polycall_peer_t h,
                                                  char *buf, size_t cap,
                                                  size_t *out_len);

/** GET /health on `peer` (registered id or "host:port"). POLYCALL_OK only
 *  when it answers healthy -- and, for a registered id, under that id. */
POLYCALL_API int POLYCALL_CALL polycall_peer_ping(polycall_peer_t h,
                                                  const char *peer,
                                                  uint32_t timeout_ms);

/**
 * Deliver `len` bytes (binary-safe, <= POLYCALL_PEER_MAX_PAYLOAD) to `peer`
 * (registered id or "host:port"). Exactly ONE delivery attempt: POLYCALL_OK
 * means the receiver stored the message (or already had this message_id
 * from this sender) and acknowledged it under the expected id. Any other
 * result means "not known to be delivered": the caller may retry with the
 * SAME message_id, and the receiver drops the duplicate (at-least-once
 * delivery + receiver de-duplication). message_id NULL/"" generates one.
 */
POLYCALL_API int POLYCALL_CALL polycall_peer_send(polycall_peer_t h,
                                                  const char *peer,
                                                  const void *payload,
                                                  size_t len,
                                                  const char *message_id,
                                                  uint32_t timeout_ms);

/**
 * Take the oldest received message. timeout_ms 0 = do not wait,
 * UINT32_MAX = wait indefinitely (until a message, cancel or close).
 * sender / message_id receive the sender's node id and the message id
 * (size them POLYCALL_PEER_ID_MAX / POLYCALL_MESSAGE_ID_MAX). If payload_cap
 * is too small: POLYCALL_E_TOO_LARGE, *payload_len = needed, and the message
 * stays queued. Payload bytes are not NUL-terminated.
 */
POLYCALL_API int POLYCALL_CALL polycall_peer_recv(polycall_peer_t h,
                                                  uint32_t timeout_ms,
                                                  char *sender, size_t sender_cap,
                                                  char *message_id, size_t message_id_cap,
                                                  void *payload, size_t payload_cap,
                                                  size_t *payload_len);

/** Wake every polycall_peer_recv() currently blocked on h with
 *  POLYCALL_E_CANCELLED (later calls wait normally). */
POLYCALL_API int POLYCALL_CALL polycall_peer_cancel(polycall_peer_t h);

/** This node's health as JSON (node id, endpoint, peers, inbox depth and
 *  capacity, counters). */
POLYCALL_API int POLYCALL_CALL polycall_peer_health(polycall_peer_t h,
                                                    char *buf, size_t cap,
                                                    size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_H */