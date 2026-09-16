# Language integration

## Support matrix

| Language | Status | Evidence |
| --- | --- | --- |
| C | **Verified** | the CLI itself, plus `tests/fixtures/providers/c-provider/example_provider.c` (config provider) and `examples/ledger/ledger_plugin.c` (operation plugin) |
| Node.js | **Verified** | `tools/rpc-clients/node/` (RPC v1 client) and `tests/fixtures/providers/node-provider/` (config provider) |
| Python | **Verified** | `tools/rpc-clients/python/` and `tests/fixtures/providers/python-provider/`; exercised by `tests/config/equivalence.sh` and `tests/conformance/run.sh` |
| Go | **Verified** | `tools/rpc-clients/go/` (RPC v1 client, `go build` clean, stdlib only) |
| Java | **Verified** | `tools/rpc-clients/java/` (RPC v1 client, plain `javac`, no Maven) |
| Lua, COBOL, and any other language | **Not implemented in this repository** | no reference client or provider ships here |

!!! danger "No `bindings/` directory currently exists"
    An earlier revision of this repository had a top-level `bindings/`
    directory with one subdirectory per language, including full SDK
    scaffolding for languages with no working example. It was removed:
    installable per-language client packages are now published
    separately (npm, PyPI, Maven, ... under the `obinexus` org), and what
    remains in this repository — `tools/rpc-clients/` — is deliberately
    minimal reference code that exists to prove `polycall_rpc` v1 is
    genuinely cross-language, not a distributable SDK.

## What a reference client actually is

Each one under `tools/rpc-clients/<lang>/` implements the same thing: the
16-byte frame header plus JSON envelope described in
[docs/RPC.md](../docs/RPC.md), enough to send one `REQUEST` and read one
`RESPONSE`. No async framework, no code generation, no external
dependency — stdlib/JDK-only in every case. They are what
`tests/conformance/run.sh` uses to SHA-256-hash and compare every
client's raw response against the C CLI's own, byte for byte.

## Config providers vs. RPC clients

These are two different roles, easy to conflate because both happen to
exist for Node and Python:

- A **config provider** (`config validate --provider node:MOD`) produces
  a validated configuration envelope. It never touches the network. See
  [Polycallfile reference](polycallfile-reference.md).
- An **RPC client** (`tools/rpc-clients/`) speaks `polycall_rpc` v1 to a
  running `polycall run` instance. It never touches configuration files.

A `[service.ID].language` value in the legacy `Polycallfile` (or a
service's `language` field in the native envelope) is informational
only — it documents intent, it does not gate which process is actually
allowed to register or call an operation.

## C, in more depth

The C CLI links `libpolycall` directly — see
[C library and ABI](c-abi-and-linking.md) for the public headers, the
provider API, and the operation-plugin ABI (`run --load`) that lets a
compiled C shared library add operations to the runtime with no core
rebuild.
