# Binding ABI v1

The single C surface every Polycall language binding reaches through its
language's native FFI. Declared in `polycall.h` (`#include <polycall.h>`),
exported by `polycall.dll` (MSVC), `libpolycall.dll` (MinGW/MSYS2) and
`libpolycall.so.1` (Linux). `POLYCALL_FFI_ABI_VERSION` is **1**; a binding
must call `polycall_ffi_abi_version()` after loading the library and refuse
to use it if the value differs from the one it was written for.

Library version 1.1.0 added this surface; it is append-only, so the SONAME
stays `libpolycall.so.1` and every 1.0 symbol keeps its behaviour.

## Rules that make it safe across languages and C runtimes

| Rule | Why |
| --- | --- |
| Only C scalars, NUL-terminated UTF-8 strings, `(pointer, length)` byte buffers and `int32` handles cross the boundary | No struct layout, padding or calling-convention surprises; every FFI can express these |
| The library never returns memory for the caller to free | No allocator mismatch (MSVC vs MinGW CRT, Go/Python/JVM heaps) and nothing to leak |
| Outputs go to caller-owned buffers with an explicit capacity, snprintf-style | On success the text is NUL-terminated and `*out_len` = length; a too-small buffer returns `POLYCALL_E_TOO_LARGE` with `*out_len` = bytes needed (excluding NUL) |
| Every function returns an `int` status | `0` = `POLYCALL_OK`, negative = `POLYCALL_E_*`; never an exception, abort or `errno` |
| Peer handles are integers validated on every call | Unknown, closed or stale handle → `POLYCALL_E_INVALID_HANDLE`; double close and use-after-close are **defined** |
| All functions are thread-safe | Calls on one handle may come from any thread concurrently |
| Calling convention | `cdecl` on 32-bit Windows (`POLYCALL_CALL`), the platform default elsewhere |

## Status codes

| Code | Value | Meaning |
| --- | --- | --- |
| `POLYCALL_OK` | 0 | success |
| `POLYCALL_E_INVALID_ARGUMENT` | -1 | NULL/empty/malformed argument (bad id, endpoint, length) |
| `POLYCALL_E_NO_MEMORY` | -2 | allocation failed |
| `POLYCALL_E_INVALID_HANDLE` | -3 | unknown, closed or stale peer handle |
| `POLYCALL_E_TIMEOUT` | -4 | deadline exceeded (outcome of a send is then *unknown*) |
| `POLYCALL_E_TRANSPORT` | -5 | refused / reset / unreachable (definitely not delivered) |
| `POLYCALL_E_PROTOCOL` | -6 | malformed reply, or acknowledged under the wrong identity |
| `POLYCALL_E_NOT_FOUND` | -7 | unknown peer id, file or operation |
| `POLYCALL_E_AUTH` | -8 | shared token missing or wrong |
| `POLYCALL_E_REMOTE` | -9 | the remote side reported an error (detail in output) |
| `POLYCALL_E_TOO_LARGE` | -10 | over a limit, or caller buffer too small |
| `POLYCALL_E_BUSY` | -11 | backpressure: inbox or connection limit reached — retry later |
| `POLYCALL_E_CANCELLED` | -12 | `polycall_peer_cancel()` woke a blocked receive |
| `POLYCALL_E_CONFIG` | -13 | invalid configuration |
| `POLYCALL_E_ADDRESS_IN_USE` | -14 | bind failed: address in use |
| `POLYCALL_E_UNSUPPORTED` | -15 | valid but not supported by this build (e.g. `tls_enabled=true`) |
| `POLYCALL_E_PERMISSION` | -16 | the OS refused (port, file) |
| `POLYCALL_E_CLOSED` | -17 | the handle was closed while this call was blocked |
| `POLYCALL_E_INTERNAL` | -18 | internal error |

`polycall_strerror(code)` returns a static name for any code (unknown codes
→ `POLYCALL_E_UNKNOWN: ...`). `polycall_last_error(buf, cap)` copies the
calling thread's detail message for its most recent failed call.

## Functions

### Library

```c
int         polycall_ffi_abi_version(void);              /* == 1 */
int         polycall_ffi_version(char *buf, int len);    /* "1.1.0"; returns length; len<0 -> E_INVALID_ARGUMENT */
const char *polycall_strerror(int status);               /* static, never NULL */
int         polycall_last_error(char *buf, size_t cap);  /* returns length */
const char *polycall_get_version(void);                  /* 1.0 API, kept */
```

### Configuration

```c
int polycall_ffi_run_config(const char *config_path, int run);
int polycall_ffi_describe(const char *config_path, char *buf, int len);
```

`polycall_ffi_run_config` loads one file — a `Polycallfile`, `Polycallrc`,
`Polycallrc.<language>`, or a binding's `<lang>-polycallrc` — with exactly
the grammar `polycall config validate` uses.

* `run == 0`: validate (unknown keys are warnings).
* `run != 0`: validate **for running with this build**: unknown keys are
  errors and settings the build cannot honour are rejected
  (`tls_enabled=true` → `POLYCALL_E_UNSUPPORTED`, because there is no TLS
  transport — it refuses instead of silently serving plaintext).

It never starts a service and never touches the network. Returns
`POLYCALL_OK`, `E_INVALID_ARGUMENT` (NULL/empty path), `E_NOT_FOUND`
(missing file), `E_CONFIG` (invalid; detail in `polycall_last_error`) or
`E_UNSUPPORTED`. `polycall_ffi_describe` returns a JSON description
(layer, servers, `peer` lines, key/values — secrets never resolved).

### RPC client

```c
int polycall_call(const char *endpoint, const char *service, const char *operation,
                  const char *input_json, uint32_t timeout_ms,
                  char *out, size_t out_cap, size_t *out_len);
```

One `polycall_rpc` v1 round trip to a running runtime / daemon
(`polycall daemon start`). `input_json` NULL → `null`; it must be valid
JSON (else `E_INVALID_ARGUMENT`). On `POLYCALL_OK` `out` is the
operation's output JSON; on `E_REMOTE` / `E_NOT_FOUND` / `E_TIMEOUT` /
`E_BUSY` it is the error object `{"code":..,"message":..}`. Executes once,
never retried; size `out` for `POLYCALL_CALL_MAX_OUTPUT` (1 MiB) when
unsure, because a too-small buffer discards a result that already ran.
`timeout_ms` must be 1..600000.

### Peer nodes (NSIGII-style, no central broker)

```c
typedef int32_t polycall_peer_t;          /* > 0 when valid */
int polycall_peer_open(const char *node_id, const char *bind_endpoint,
                       const char *auth_token, polycall_peer_t *out_handle);
int polycall_peer_close(polycall_peer_t h);
int polycall_peer_endpoint(polycall_peer_t h, char *buf, size_t cap);
int polycall_peer_node_id(polycall_peer_t h, char *buf, size_t cap);
int polycall_peer_register(polycall_peer_t h, const char *peer_id, const char *endpoint);
int polycall_peer_unregister(polycall_peer_t h, const char *peer_id);
int polycall_peer_list(polycall_peer_t h, char *buf, size_t cap, size_t *out_len);
int polycall_peer_ping(polycall_peer_t h, const char *peer, uint32_t timeout_ms);
int polycall_peer_send(polycall_peer_t h, const char *peer, const void *payload,
                       size_t len, const char *message_id, uint32_t timeout_ms);
int polycall_peer_recv(polycall_peer_t h, uint32_t timeout_ms,
                       char *sender, size_t sender_cap,
                       char *message_id, size_t message_id_cap,
                       void *payload, size_t payload_cap, size_t *payload_len);
int polycall_peer_cancel(polycall_peer_t h);
int polycall_peer_health(polycall_peer_t h, char *buf, size_t cap, size_t *out_len);
```

* **Identifiers** (`node_id`, `peer_id`, `message_id`): 1–63 bytes of
  `[A-Za-z0-9._-]`. Buffers: `POLYCALL_PEER_ID_MAX` /
  `POLYCALL_MESSAGE_ID_MAX` = 64 incl. NUL. Endpoints: `host:port`,
  `POLYCALL_ENDPOINT_MAX` = 128.
* **open**: `bind_endpoint` `"127.0.0.1:0"` = ephemeral port; `NULL` =
  send-only node (no listener). A non-loopback bind without a token is
  refused (`E_CONFIG`). `auth_token` NULL/"" = no authentication.
* **Ownership**: the registry and inbox belong to *that node only*. Two
  nodes — in the same process or in different processes, in any language —
  never share state implicitly. A registry changes only through
  `polycall_peer_register` on that node, or the authenticated
  `POST /register` of that node. Receiving a message never registers its
  sender.
* **send**: payload is binary-safe (embedded NULs allowed), at most
  `POLYCALL_PEER_MAX_PAYLOAD` = 1 MiB (more → `E_TOO_LARGE` before any I/O);
  `peer` is a registered id or a `host:port`. **Exactly one delivery
  attempt.** `POLYCALL_OK` means the receiver stored the message (or
  already had this `(sender, message_id)`) and acknowledged it under the
  expected node id. Anything else means *not known to be delivered*.
* **Retry / idempotency**: delivery is at-least-once when the caller
  retries; the receiver de-duplicates on `(sender id, message_id)` over its
  last 1024 messages, so retrying with the **same** `message_id` is safe
  and yields exactly one copy. `E_TRANSPORT` = definitely not delivered;
  `E_TIMEOUT` = unknown (it may still arrive — retry with the same id).
* **recv**: `timeout_ms` 0 = poll, `UINT32_MAX` = wait until a message,
  `polycall_peer_cancel` (`E_CANCELLED`) or close (`E_CLOSED`). If
  `payload_cap` is too small: `E_TOO_LARGE`, `*payload_len` = size, and the
  message stays queued (retry with a bigger buffer). Payload bytes are not
  NUL-terminated.
* **Backpressure**: the inbox holds 256 messages / 64 MiB; when full the
  receiver answers 503 and `send` returns `E_BUSY`.
* **close**: wakes blocked receivers (`E_CLOSED`), stops the listener,
  joins every worker. A second close → `E_INVALID_HANDLE`.

## How a binding should load the library

1. Honour an explicit path in an environment variable first
   (`POLYCALL_LIBRARY`), then the platform name: `polycall.dll` (MSVC
   build) / `libpolycall.dll` (MinGW) on Windows, `libpolycall.so.1` on
   Linux, `libpolycall.1.dylib` on macOS.
2. Resolve **every** symbol it uses up front. A missing symbol (an old
   1.0 library) or `polycall_ffi_abi_version() != 1` must produce a clear,
   language-native error naming the library and the problem — never a
   crash on first use.
3. Never free anything the library returned (it returns nothing to free).
4. Keep strings UTF-8; pass binary payloads as `(pointer, length)`.

## What every binding's tests must cover (against the real library)

Version + ABI check; `run_config` (valid, missing, invalid, strict,
unsupported TLS); `polycall_call` against a running `polycall daemon` or
`polycall start` (success, unknown operation, deadline, invalid input,
no runtime); two peer nodes exchanging payloads **in both directions**
with exact bytes, sender id and message id verified at the receiver;
empty, UTF-8, binary-with-NUL, exactly 1 MiB and 1 MiB + 1 payloads;
registry ownership; duplicate message id delivered once; auth failure;
send to a dead peer (`E_TRANSPORT`); receive timeout; too-small buffer
leaves the message queued; cancel and close waking a blocked receive;
double close / calls after close / invalid handles; concurrent senders.
Run in the language's own runtime; report each check PASS/FAIL/SKIP —
never count a skipped check as passed.
