---
title: PolyCall documentation
---

# PolyCall

**polycall** is a program-first cross-language runtime: one C CLI and one
wire protocol (`polycall_rpc` v1), reached identically from every language
client. Source: [github.com/obinexus/polycall](https://github.com/obinexus/polycall).

```
polycall [global-options] <command> [subcommand] [arguments]
```

## Start here

- [CLI reference](CLI.md) -- every command, global options, exit codes, `--format json`
- [RPC v1](RPC.md) -- the wire protocol behind `run` / `status` / `stop` / `call`
- [Native configuration](NATIVE_CONFIG.md) -- `config validate` / `config show`, C/Node/Python providers
- [Operation plugins](PLUGINS.md) -- load your own operations with `polycall run --load PATH`
- [Concurrency model](CONCURRENCY.md) -- one thread per connection

## Telemetry

- [CLI reference: telemetry](CLI.md) -- `telemetry emit` / `telemetry show` / `telemetry status`

Every `run` bind/stop and every `call` round trip emits a GUID + millisecond-UTC-timestamp
correlated JSONL event to `<project-root>/.polycall/telemetry.jsonl` (override with
`POLYCALL_TELEMETRY_LOG`, disable with `POLYCALL_TELEMETRY=off`). `telemetry emit` lets
external scripts and loaded plugins log into the same sink.

## Reference

- [Architecture](ARCHITECTURE.md)
- [Conformance](CONFORMANCE.md) -- cross-language RPC v1 equivalence results
- [Features](FEATURES.md)
- [Changelog](CHANGELOG.md)
- [Platform report](PLATFORM_REPORT.md)

## Language bindings

Client SDKs for other languages are published separately (npm, PyPI, Maven,
etc. under the `obinexus` org) rather than vendored in this repository. This
repo carries only the minimal reference clients needed by its own test suite,
under `tools/rpc-clients/`.
