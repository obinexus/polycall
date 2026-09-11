# polycall_rpc v1 wire protocol

Status: **Stage 3 implemented**. This is a **new, additively-versioned** surface.
The legacy `include/polycall_protocol.h` (message header, handshake/auth state
machine) is **unchanged**; `polycall_rpc` does not modify or reinterpret it.
A future convergence, if any, will be documented and versioned before any wire
behaviour changes.

## Framing

Every message is a 16-byte header followed by a UTF-8 JSON payload. **No C
struct or pointer is ever placed on the wire.**

| Offset | Size | Field | Notes |
| --- | --- | --- | --- |
| 0 | 4 | magic | ASCII `PCR1` |
| 4 | 1 | type | 1 REQUEST, 2 RESPONSE, 3 CONTROL, 4 CONTROL_REPLY |
| 5 | 1 | flags | reserved, 0 |
| 6 | 2 | reserved | 0 |
| 8 | 4 | corr | big-endian; request/response correlation id chosen by the caller |
| 12 | 4 | length | big-endian; payload byte count, `<= 1 MiB` |
| 16 | length | payload | UTF-8 JSON |

The reader rejects a bad magic or an oversize `length` (protocol violation).
`src/runtime/rpc_wire.c` handles partial reads/writes on every send and recv.

## Payloads

**REQUEST** (client -> runtime):
```json
{"service":"inventory","operation":"get","deadline_ms":2000,"input":{"item_id":"widget-a"}}
```
**RESPONSE** (runtime -> client), success and failure alike:
```json
{"ok":true,"output":{"item_id":"widget-a","quantity":42,"in_stock":true}}
{"ok":false,"error":{"code":"item.unknown","message":"no inventory item with id 'nope'"}}
```
**CONTROL** (client -> runtime): `{"action":"ping|describe|shutdown","auth_token":"..."}`
**CONTROL_REPLY**: `{"ok":true,"data":{...}}` or `{"ok":false,"error":{"code","message"}}`

`describe` returns the typed operation inventory (service, operation, summary,
one-line input/output schema hints, `idempotent`). `shutdown` requires the
`--auth-token` the runtime was started with; a mismatch replies
`{"ok":false,"error":{"code":"auth.denied"}}` and the runtime keeps serving.

## Runtime

`polycall run` binds loopback by default (`--endpoint host:port`, port 0 =
ephemeral; the resolved endpoint is printed and, with `--endpoint-file`,
written to a file). It serves each connection on its own thread, bounded to
64 simultaneous connections (`docs/CONCURRENCY.md` has the full threading
model, per-connection deadline and shutdown scope). `SIGINT`/`SIGTERM` only
set a flag; the accept loop notices within 200 ms and **all cleanup runs in
normal execution**, never in the handler.

Built-in operations (deterministic, no network, no I/O):

| service.operation | input | output | idempotent |
| --- | --- | --- | --- |
| `inventory.get` | `{"item_id":"string"}` | `{"item_id","quantity","in_stock"}` | yes |
| `debug.echo` | any | `{"echo":<input>}` | yes |
| `debug.sleep` | `{"ms":integer}` | `{"slept_ms":integer}` (fails `deadline.exceeded` if `ms` > deadline) | **no** |

`inventory.get` stock fixture: `widget-a`=42, `widget-b`=7, `gadget-c`=0.
Register more via `polycall_runtime_register()`.

## `call` semantics

`polycall call SERVICE OPERATION --endpoint host:port [--input FILE|- | --input-value JSON]`
performs **exactly one round trip and never retries** (non-idempotent
operations are therefore safe). Error mapping to the CLI exit contract:

| condition | exit |
| --- | --- |
| success | 0 |
| `request.malformed` / `input.invalid` | 3 |
| `operation.unknown` / `item.unknown` | 4 |
| `deadline.exceeded`, or no reply before the deadline | 6 |
| `auth.denied` | 7 |
| cannot connect / connection dropped | 5 |
| any other operation error | 1 |

Clients in other languages: `bindings/node-client/inventory_client.mjs`,
`bindings/python-client/inventory_client.py` -- each frames one REQUEST and
reads one RESPONSE from the same runtime.
