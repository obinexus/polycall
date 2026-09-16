# PolyCall CLI contract

Status: **Stages 1-3 implemented**. `help`, `version`/`--version`, `doctor`, the
argv parser, exit-code mapping and `--format json` are in place
(`tests/cli/contract.sh`). `config validate` / `config show` take a native
schema-v2 selector (`--provider c:LIB | node:MOD | python:MOD`, or
`--envelope FILE`) and validate through the typed C model; with no selector they
fall back to the unchanged legacy loader. `config migrate --from-legacy` writes
a native envelope from the legacy effective model. `start` / `status` / `stop` / `call`
drive a real foreground runtime over `polycall_rpc` v1 -- the C CLI and the Node
/ Python clients all reach the same registered C operation; a missing runtime is
a failure, not a stub, and `call` never retries. `start` and `call` each emit
GUID + timestamp correlated `telemetry` events automatically; `telemetry
emit`/`show`/`status` read and write the same sink directly. See
`docs/NATIVE_CONFIG.md` and `docs/RPC.md`.

## Synopsis

```
polycall [global-options] <command> [subcommand] [arguments]
```

* No arguments -> root help on stdout, exit 0. Non-interactive input never
  starts a REPL. `polycall repl` is the only way into the interactive session,
  and it dispatches every line through this same parser and registry.
* `--version` / `-V` prints one line (`polycall <version>`) and exits 0 without
  touching runtime init, the network, or any configuration file.
* `--help` / `-h` prints help for the resolved command and exits 0.
* Unknown option, unknown command, unknown subcommand, a repeated scalar
  option, a missing option value, or a trailing argument that cannot be placed
  is a usage error (exit 2).
* `--` ends option parsing; every following token is positional. argv is never
  re-tokenised (no second `strtok` pass); quoted paths from the OS survive
  intact.
* The removed `-f PATH` flag reports exit 2 and points at `polycall start
  --config PATH`.

## Global options

| Option | Meaning |
| --- | --- |
| `--project-root PATH` | project root for resolving relative paths |
| `--config PATH` | explicit configuration file / provider |
| `--language LANG` | language layer / adapter to select |
| `--format text\|json` | output format for finite commands (default `text`) |
| `--no-color` | disable ANSI colour |
| `--quiet` | suppress non-essential text on stderr |
| `--timeout-ms N` | deadline (non-negative integer) for commands that support one |
| `-h`, `--help` | show help and exit |
| `-V`, `--version` | show version and exit |

Globals are accepted before or after the command tokens, but never after `--`.
`--opt=value` and `--opt value` are both accepted for scalar options.

## Commands

| Command | Stage | Behaviour |
| --- | --- | --- |
| `help [command [subcommand]]` | 1 | deterministic help text; no side effects |
| `version` | 1 | same as `--version` |
| `doctor` | 1 | read-only build / platform / ABI / core-capability report; never starts a service or evaluates a provider |
| `config validate --provider c:LIB\|node:MOD\|python:MOD` | 2 | run the provider, validate through the typed model |
| `config validate --envelope FILE` | 2 | validate a pre-generated canonical envelope |
| `config validate [path]` | 1 (legacy bridge) | no selector -> unchanged legacy loader |
| `config show <selector> [--provenance]` | 2 | print the canonical envelope + winning source per field |
| `config load [language]` | 1 (legacy bridge) | report legacy load order |
| `config migrate <src> <Polycallrc.lang>` | 1 (legacy bridge) | copy-migrate a legacy file, refuses to overwrite |
| `config migrate --from-legacy [--language L] --output F` | 2 | legacy effective model -> v2 envelope; refuses to overwrite; reports unmapped keys |
| `telemetry emit --event NAME [--service S] [--operation OP] [--status ST] [--detail TEXT] [--correlation-id GUID]` | -- | append one structured event to the sink; prints the correlation id used |
| `telemetry show [--limit N]` | -- | print the most recent recorded events (default 20, newest last) |
| `telemetry status` | -- | whether telemetry is enabled and where it writes; never starts the runtime |
| `repl` | 1 | explicit interactive session over this registry |
| `start --endpoint host:port [--auth-token T] [--endpoint-file F] [--load PATH ...] [--daemon]` | 3 | foreground runtime; port 0 = ephemeral; SIGINT stops it, cleanup in normal code; `--load` (repeatable) adds operations from a plugin shared library before binding -- see `docs/PLUGINS.md`; `--daemon` detaches into the background and returns immediately, printing the detached process's PID |
| `status --endpoint host:port` | 3 | describe registered operations over the control channel; never infers health from a file |
| `stop --endpoint host:port [--auth-token T]` | 3 | authenticated shutdown; exit 7 on token mismatch, 5 on no runtime |
| `call SERVICE OPERATION --endpoint host:port [--input F\|- \| --input-value JSON]` | 3 | one round trip, no retry; see `docs/RPC.md` for the exit-code map |

## Telemetry

Every `start` bind and clean stop, and every `call` round trip, appends one
compact JSON line (JSONL) to a sink file:

```json
{"schema_version":1,"ts":"2026-01-01T00:00:00.000Z","correlation_id":"3fa8...-4...-8...","event":"call.start","service":"inventory","operation":"get"}
{"schema_version":1,"ts":"2026-01-01T00:00:00.002Z","correlation_id":"3fa8...-4...-8...","event":"call.end","service":"inventory","operation":"get","status":"ok","duration_ns":1998600}
```

`correlation_id` is a random RFC 4122 v4 GUID shared by both ends of a paired
event (`call.start`/`call.end`); `ts` is millisecond-resolution UTC wall clock
for cross-process correlation, and `duration_ns` (when present) comes from a
monotonic clock, never the wall clock. Emitting is best-effort and silent: it
never fails or blocks the operation it describes.

- Sink: `<project-root>/.polycall/telemetry.jsonl`, or `$POLYCALL_TELEMETRY_LOG` to override.
- Disable entirely: `POLYCALL_TELEMETRY=off` (also accepts `0`/`false`/`no`).
- `telemetry emit --event NAME [--service S] [--operation OP] [--status ST] [--detail TEXT] [--correlation-id GUID]`
  writes one event by hand -- e.g. from a loaded operation plugin or an
  external script -- into the same sink, and prints the correlation id used.
- `telemetry show [--limit N]` prints the most recent events (default 20).
- `telemetry status` reports whether telemetry is enabled and the resolved
  sink path; it never starts the runtime.

## `--format json`

A finite command with `--format json` prints exactly one UTF-8 JSON object on
stdout, for success and failure alike, with no banner and no ANSI:

```json
{"schema_version":1,"ok":true,"command":"doctor","data":{...},"error":null}
```

On failure `ok` is `false`, `data` is `null`, and `error` is
`{"code","message","hint"}` with a stable `code`. Diagnostics/logs go to stderr.
Usage errors are emitted as JSON too when `--format json` was parsed before the
offending token.

## Exit codes

| Code | Meaning |
| --- | --- |
| 0 | completed successfully |
| 1 | runtime / internal failure |
| 2 | CLI usage error |
| 3 | invalid or missing selected configuration |
| 4 | missing dependency, adapter, or unsupported capability |
| 5 | transport / endpoint unavailable |
| 6 | operation deadline exceeded |
| 7 | authentication / authorization rejected |
| 130 | user interruption (SIGINT) |

Library status values are mapped onto these centrally in the CLI; raw enum
values are never used as the shell contract.
