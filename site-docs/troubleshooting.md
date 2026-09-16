# Troubleshooting

## `call` exits 5 — "could not reach the runtime"

Transport failure: no process is listening at `--endpoint host:port`, or
it isn't `polycall start` (or isn't `polycall_rpc` v1). Check the runtime's
own terminal for `polycall start: listening on host:port` — if it never
printed that, `start` failed to bind (see below). `call` performs exactly
one connect + send + receive and never retries, even here.

## `start` fails to bind

If the port is already in use, `start` fails at startup before printing
`listening on`. Bind `--endpoint 127.0.0.1:0` (ephemeral port) plus
`--endpoint-file PATH` instead of guessing a free port, and read the
resolved endpoint back from that file — this is what every test script
and reference client in this repository does.

## `start --load PATH` exits 4

Three distinct causes, reported distinctly (never a generic failure):

- the path doesn't exist, or the library fails to load (`polycall start: <os loader error>`)
- the library has no `polycall_ops_register` symbol
- the plugin's ABI major version doesn't match the host's (message
  mentions "ABI")

None of these ever bind a socket first — a bad `--load` fails fast, with
nothing partially started. See [docs/PLUGINS.md](../docs/PLUGINS.md).

## `config validate --provider node:...`/`python:...` fails with `provider.runner_unset`

```text
polycall config: no node runner script configured  [POLYCALL_NODE_PROVIDER]  (provider.runner_unset)
```

There is no built-in default runner location — set
`POLYCALL_NODE_PROVIDER`/`POLYCALL_PYTHON_PROVIDER` to the runner
script's path explicitly before invoking `config validate`/`config show`.
`tests/fixtures/providers/{node,python}-provider/run.{mjs,py}` are the
runners this repository's own tests point at; see
[Native configuration](../docs/NATIVE_CONFIG.md).

## `config validate` exits 3

The selected configuration (legacy file or native provider/envelope) is
invalid, or the provider process failed, hung past its deadline, or
produced oversize output — the error's `code`/`message` (or, in text
mode, the printed reason) says which. `doctor` never evaluates a
provider, so a passing `doctor` says nothing about whether a specific
provider or Polycallfile validates.

## Authentication rejected (exit 7)

`stop --auth-token` and any `call`/`status` reaching an auth-gated action
need the exact token the runtime was started with
(`start --auth-token T`). A wrong or missing token is refused distinctly
from "no runtime answered" (exit 5) — the runtime is up and responding,
it just declined this specific request. `status` with no token still
works; only `stop` and control actions that mutate state require one.

## Deadline exceeded (exit 6)

`call --deadline-ms N` (or the global `--timeout-ms`, default 5000ms) is
enforced by both the client and the runtime. A `debug.sleep` call
requesting more time than its deadline allows is the reliable way to
reproduce this on purpose — see
[Your first cross-language call](first-python-call.md).

## Loader/linking errors for a C consumer

A missing-library error when *running* (not compiling) a consumer linked
against the shared library is the OS loader failing to find
`libpolycall.so`/`.dll`/`.dylib` at run time — separate from the
`-L`/`-l` link-time flags. Place the shared library beside your consumer
binary, or set an explicit loader search path
(`LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH`/PATH). See
[C library and ABI](c-abi-and-linking.md).

## Telemetry sink is empty

`POLYCALL_TELEMETRY=off` (or `0`/`false`/`no`) disables emission
entirely — `telemetry status` reports whether it's currently enabled and
the resolved sink path. Emission is best-effort and silent by design: a
sink directory that can't be created, or a disk that can't be written to,
never fails the `start`/`call` it was trying to record.
