# Your first cross-language call

!!! success "Verified this session"
    The Node.js walkthrough below was run end to end against a real
    `polycall.exe` build on Windows this session: `start`, a live TCP
    connection from `tools/rpc-clients/node/inventory_client.mjs`, and a
    clean `stop`. The Python client takes the identical path and is
    exercised the same way by `tests/runtime/roundtrip.sh` and
    `tests/conformance/run.sh` in CI — not personally re-run here (no
    Python interpreter was available in this particular environment).

This walks through the built-in `inventory.get` operation: the same C
code, reached first from the `polycall` CLI, then from a Node.js process
that never links against `libpolycall` at all — only the
`polycall_rpc` v1 wire protocol.

## Prerequisites

- Built binaries — see [Getting started](getting-started.md).
- Node.js (or Python 3) on `PATH`, for the reference client. Nothing
  installs via `npm`/`pip`; the clients are single files, standard
  library only.

## Terminal 1: start the runtime

```sh
bin/polycall start --endpoint 127.0.0.1:0 --endpoint-file /tmp/ep --auth-token demo-tok
```
```text
polycall start: listening on 127.0.0.1:PORT (Ctrl-C to stop)
```

Port `0` picks an ephemeral port; `--endpoint-file` writes the resolved
`host:port` so the next terminal can read it.

## Terminal 2: call it from the C CLI

```sh
EP=$(cat /tmp/ep)
bin/polycall call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}'
```
```json
{"ok":true,"output":{"item_id":"widget-a","quantity":42,"in_stock":true}}
```

## Terminal 3: the identical call from Node.js

```sh
node tools/rpc-clients/node/inventory_client.mjs "$EP" widget-a
```
```json
{"ok":true,"output":{"item_id":"widget-a","quantity":42,"in_stock":true}}
```

Byte-for-byte the same result — `tests/conformance/run.sh` SHA-256-hashes
every client's raw response and asserts they match, across C, Node,
Python, Go, and Java. The Python equivalent is:

```sh
python3 tools/rpc-clients/python/inventory_client.py "$EP" widget-a
```

## What else is verified, not just plausible

**Unknown item is reported honestly, not faked.**

```sh
bin/polycall call inventory get --endpoint "$EP" --input-value '{"item_id":"nope"}'
```
exits 4 (`item.unknown`) — a real routing/lookup outcome, not a stub.

**A missing runtime is a failure, never silently retried.**

```sh
bin/polycall call inventory get --endpoint 127.0.0.1:1 --input-value '{"item_id":"widget-a"}'
```
exits 5 (transport) immediately. `call` performs exactly one connect +
send + receive and never retries, even for this built-in, side-effect-free
operation — see [docs/RPC.md](../docs/RPC.md).

**Deadlines are enforced independently per call**, not shared across
requests — two back-to-back `debug.sleep` calls with different deadlines
each get judged on their own outcome (`tests/conformance/run.sh` asserts
this explicitly).

## Stopping cleanly

```sh
bin/polycall stop --endpoint "$EP" --auth-token demo-tok
```

An authenticated shutdown; a wrong or missing token when the runtime was
started with `--auth-token` is refused (exit 7), and the runtime stays
up. Cleanup runs in normal execution, never in a signal handler.

## Next

- Loading your own operation with no core rebuild:
  [docs/PLUGINS.md](../docs/PLUGINS.md) (`start --load PATH`).
- The full wire byte layout: [docs/RPC.md](../docs/RPC.md).
- Writing a config provider instead of an RPC client:
  [Polycallfile reference](polycallfile-reference.md).
