# Polycallfile reference

`Polycallfile` (exactly that capitalization, no extension, at the project
root) is the legacy source of runtime topology, read by
`config validate`/`config show` when no `--provider`/`--envelope`
selector is given. There is a second, independently versioned
**native schema-v2** path (typed C/Node/Python providers, or a
pre-generated JSON envelope) — see [Native configuration](../docs/NATIVE_CONFIG.md)
for that one. `config migrate --from-legacy` converts a legacy file's
*effective* model into a native envelope, reporting any legacy key it
cannot map (never silently dropping it).

## The legacy grammar

Two line shapes, `#` comments, blank lines ignored:

```text
server LANG HOST_PORT:TARGET_PORT
key=value
```

`server` declares a language server topology entry (host port → target
port). Recognized scalar keys (`src/config/legacy_import.c`):
`network_timeout`, `max_connections`, `workspace_root`/`workspace`,
`log_directory`, `log_level`, `tls_enabled`, `cert_file`, `key_file`,
`require_auth`, `auto_discover`, `discovery_interval`, `enable_metrics`,
`metrics_port`, `max_memory`/`max_memory_per_service`,
`max_cpu_per_service`, `bind_address`, `timeout`. An unrecognized key is
not a parse error — it is carried through and reported by
`config migrate --from-legacy` as unmapped, not dropped.

## The shipped example

The repository root's own `Polycallfile`:

```text
# PolyCall System Configuration

# Language Server Definitions
server node 8080:8084
server python 3001:8084
server java 3002:8082
server go 3003:8083

# Network Configuration
network start
network_timeout=5000
max_connections=1000

# Global Settings
log_directory=/var/log/polycall
workspace_root=/opt/polycall

# Service Discovery
auto_discover=true
discovery_interval=60

# Security Configuration
tls_enabled=true
cert_file=/etc/polycall/cert.pem
key_file=/etc/polycall/key.pem

# Resource Limits
max_memory_per_service=1G
max_cpu_per_service=2

# Monitoring
enable_metrics=true
metrics_port=8090
```

!!! warning "`tls_enabled` does not enable TLS"
    The legacy loader and the native model both accept and validate
    `tls_enabled`/`tls.mode`, but no TLS library is linked anywhere in
    this tree — see [Getting started](getting-started.md). The runtime
    always speaks plaintext `polycall_rpc` v1 today.

## The native schema-v2 envelope

`config validate --provider c:LIB|node:MOD|python:MOD` or
`--envelope FILE` selects this path instead: a typed, versioned JSON
model with `project`, `services[]` (`id`, `language`, `bind_address`,
`host_port`, `target_port`, `workspace`, `timeout_ms`, `max_connections`,
`tls: {mode, cert_ref, key_ref}`), validated against a fixed schema
rather than the permissive legacy key/value bag. Selecting a `node:`/
`python:` provider executes local code: only the explicitly named
module/library is used, and the runner script itself must be named via
`POLYCALL_NODE_PROVIDER`/`POLYCALL_PYTHON_PROVIDER` — there is no
built-in default location. See [Native configuration](../docs/NATIVE_CONFIG.md)
for the full provider contract, error codes, and secret-redaction rules.

## CLI commands that touch it

- `polycall config validate [Polycallfile]` — parses and validates only;
  never connects to anything, never starts a service.
- `polycall config show --format json [Polycallfile]` — prints the
  validated effective model.
- `polycall config load [language]` — reports the legacy load order.
- `polycall config migrate <src> <Polycallrc.lang>` — legacy copy-migrate
  form; refuses to overwrite an existing destination.
- `polycall config migrate --from-legacy [--language L] --output F` —
  legacy effective model → native v2 envelope; refuses to overwrite;
  reports unmapped keys instead of dropping them.

See the [CLI reference](cli-reference.md) for exit codes and
`--format json`.
