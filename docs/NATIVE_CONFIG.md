# Native configuration (schema v2)

Status: **Stage 2 implemented** for the C, Node and Python providers. C++, Go,
Java, Lua, TypeScript and COBOL are planned and reported as such by
`polycall bindings list`.

## Model

One typed internal model, independently versioned from the legacy
`polycall_config_t` (whose public layout is unchanged). Public API:
`include/polycall_config2.h`. The core validates every value regardless of what
a provider checked.

Fields: project name + optional `extension_namespace`; one or more services,
each with `id`, `language`, `bind_address` (default `127.0.0.1`), `host_port` /
`target_port` (1..65535, JSON numbers), `workspace` (required; relative paths
resolve against the declaring provider's directory and keep that provenance),
`timeout_ms` and `max_connections` (positive integers; defaults 30000 / 64),
and `tls` `{mode: off|server|mutual, cert_ref, key_ref}`. TLS references must be
`env:NAME` or `file:PATH` -- never an inline secret; `config show` prints the
reference, never a resolved value. Duplicate service ids and unknown fields are
errors.

## Canonical envelope

The exchange form between a provider and the core is one UTF-8 JSON object with
a fixed key order and services sorted by id. Two configs with equal effective
values serialise byte-for-byte identically -- this is the cross-language
equivalence oracle (`polycall_config2_to_envelope`). It is transport, not a
user-authored format; nobody hand-edits it.

```json
{"schema_version":2,"project":{"name":"shop","extension_namespace":"obinexus"},
 "services":[{"id":"inventory","language":"c","bind_address":"127.0.0.1",
   "host_port":8080,"target_port":9090,"workspace":"/opt/shop/inventory",
   "timeout_ms":15000,"max_connections":128,
   "tls":{"mode":"off","cert_ref":null,"key_ref":null}}, ...]}
```

## Providers

Authored config is real language values, not a DSL:

| Ecosystem | File | Contract |
| --- | --- | --- |
| C | `bindings/c-provider/example_provider.c` | shared library exporting `polycall_config_provider_v1(builder, ctx)`; loaded from an explicit path via `LoadLibraryExW` / `dlopen` (`polycall_provider_load_c`) |
| Node | `bindings/node-provider/example.config.mjs` + `run.mjs` | ES module exporting a default object; `run.mjs` prints one canonical envelope |
| Python | `bindings/python-provider/example_config.py` + `run.py` | module defining `configure()`; `run.py` prints one canonical envelope |

Sub-process providers are launched with an **argv array** (never a shell
string), an explicit working directory, a **1 MiB** output budget and a **10 s**
default deadline (`--deadline-ms`, capped at 120 s; `--max-bytes`). A non-zero
exit, a timeout, or oversize output each fail with a distinct error. Only the
explicitly named module / library is used -- parent directories are never
scanned, and `doctor` never runs a provider.

CLI selection:

```sh
polycall config validate --provider c:./build/lib/example_provider.so
polycall config show     --provider node:bindings/node-provider/example.config.mjs --provenance
polycall config validate --provider python:bindings/python-provider/example_config.py
polycall config validate --envelope /tmp/generated.json
```

`POLYCALL_PROVIDER_ROOT` (or `--project-root`) locates `bindings/<lang>-provider/run.*`;
`POLYCALL_NODE_PROVIDER` / `POLYCALL_PYTHON_PROVIDER` override the runner path.

## Precedence and legacy coexistence

Native v2 precedence, low to high: compiled defaults -> native global defaults
-> native project config -> selected language override -> allowlisted env fields
-> CLI overrides. This intentionally differs from the v1 order
(`Polycallfile -> Polycallrc -> Polycallrc.<language>`), which the legacy loader
in `src/polycall_config.c` still honours unchanged.

Selection is explicit: `config validate` with a `--provider`/`--envelope`
selector is the native path; with none it is the legacy path. The two are never
merged silently.

## Migration

```sh
polycall --project-root DIR config migrate --from-legacy [--language L] --output native.json
```

Computes the legacy **effective** model (documented v1 precedence, later scalars
win, topology from `Polycallfile`) and projects it onto v2, preserving effective
values: `server L h:t` -> a service; `network_timeout`/`timeout` -> `timeout_ms`;
`workspace_root` -> per-service `workspace`; `tls_enabled` + `cert_file` /
`key_file` -> `tls.mode=server` with `file:` references. Original files are
opened read-only. An existing `--output` is refused. Legacy keys with no v2 home
(`enable_metrics`, `metrics_port`, `auto_discover`, ...) are **reported**, never
silently dropped.
