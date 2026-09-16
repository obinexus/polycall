# Core concepts

## The pieces

**`libpolycall`** (`polycall_static`/`polycall_shared`) — the reusable
native library. It implements configuration parsing (legacy and native
schema-v2), the `polycall_rpc` v1 runtime, and the public C API. It has no
`main()` of its own.

**`polycall`** — the operator CLI (`src/cli/` + `src/main.c`), a thin
dispatcher over a fixed command registry (`src/cli/cli_commands.c`) that
translates parsed command-line options into calls against `libpolycall`
and renders the result as human text or `--format json`. See the
[CLI reference](cli-reference.md).

**The runtime** (`polycall_runtime_t`) — created by `polycall start` (or an
embedding application calling `polycall_runtime_create`/
`polycall_runtime_serve`). It binds `--endpoint host:port`, accepts
connections (one thread per connection), authenticates control actions
against `--auth-token`, and routes each request frame to whichever
registered operation matches its `service.operation`.

**An operation** (`polycall_op_desc_t`) — a `service`, `operation` name
pair with a handler function, an input/output schema hint, and an
`idempotent` flag clients use to decide whether to retry. Two are
built in: `inventory.get` and `debug.sleep`. More can be added at runtime
with no core rebuild via `start --load PATH` (a shared library exporting
`polycall_ops_register`) — see [docs/PLUGINS.md](../docs/PLUGINS.md).
Registering the same `service.operation` twice — from two plugins, or a
plugin and a built-in — is always refused, never "last one wins".

**A reference client** (`tools/rpc-clients/{node,python,go,java}/`) —
minimal, stdlib-only implementations of the `polycall_rpc` v1 wire
protocol used by this repository's own conformance tests
(`tests/conformance/run.sh`) and the [ledger demo](../examples/ledger/).
They exist to prove the protocol is genuinely cross-language, not to be
an installable SDK — see [Language integration](language-integration.md).

**A config provider** (`c:`/`node:`/`python:`) — separate from a runtime
client: code that produces a validated schema-v2 configuration envelope
for `config validate`/`config show`, never touches the network, and never
starts a service. See the [Polycallfile reference](polycallfile-reference.md).

## Request/response, not a shared address space

A `call` round trip is one TCP connection, one framed request, one framed
response — never a retry, even for a deterministic operation. The frame
is a fixed 16-byte header (magic, type, correlation, length) plus a UTF-8
JSON payload; no C struct layout or pointer ever crosses the socket. See
[docs/RPC.md](../docs/RPC.md) for the exact byte layout and the transport
→ exit-code map every client (C CLI included) implements identically.

```text
caller  --(service, operation, deadline_ms, input JSON)-->  runtime  --dispatch-->  operation handler
caller  <---------(output JSON / structured error)--------  runtime  <-------------------┘
```

## Typed exit codes, not just a boolean

Every command maps its outcome onto one shared exit-code contract (0 OK,
1 runtime failure, 2 usage, 3 config, 4 unsupported, 5 transport, 6
deadline, 7 auth — see the [CLI reference](cli-reference.md#exit-codes)).
An operation handler itself returns one of four status classes
(`POLYCALL_OP_OK`/`ERR_INPUT`/`ERR_NOTFOUND`/`ERR_INTERNAL`/`ERR_DEADLINE`),
which the runtime maps onto those same codes centrally — a handler never
picks a process exit code itself.

## Configuration is explicit, never merged from a search path

Both configuration paths — the legacy `Polycallfile` loader and the
native schema-v2 providers — take an explicit path or selector. There is
no parent-directory search, no home-directory fallback for `Polycallfile`
itself, and selecting a `node:`/`python:` provider only ever runs the
exact module and runner script named — never a directory scan. `doctor`
never evaluates a provider or starts a service; it only reports build,
platform, ABI, and core-capability facts.

## Telemetry is a side channel, not part of the protocol

`start`/`call` automatically emit GUID + timestamp correlated events to a
JSONL sink (`.polycall/telemetry.jsonl` by default) — this is separate
from the `polycall_rpc` v1 wire and never affects a call's outcome or
timing budget. See the [CLI reference](cli-reference.md#telemetry).
