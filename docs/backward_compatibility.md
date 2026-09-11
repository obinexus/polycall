# Hotwiring for Backward Compatibility

Hotwiring in LibPolyCall allows developers to replace legacy API functionality without breaking existing integrations. By routing old command paths to new implementation modules, applications can gradually adopt updated features while remaining functional.

## Approach

1. **Compatibility Layer** – Provide thin wrappers around deprecated functions. These wrappers forward requests to new modules using the Sinphasé command interface.
2. **Incremental Replacement** – Update one feature at a time. Each replacement includes cost analysis to confirm that the new code stays within acceptable complexity thresholds.
3. **Telemetry Hooks** – Maintain telemetry in both old and new code paths to verify behaviour and gather usage statistics.
4. **Isolation Triggers** – If a new module exceeds the configured cost threshold, it can be isolated into a separate component until optimised.

This strategy ensures stable evolution of the runtime while following the single-pass, hierarchical structuring pattern promoted by the Sinphasé framework.

---

# Legacy-protocol bindings: keep / deprecate / remove decisions

Per `docs/TODO.md` P2. For each existing binding directory, what was actually
found in this checkout (static inspection only, dated 2026-09-11) and the
decision on its legacy-protocol client now that a `polycall_rpc` v1 client
exists alongside it.

| Binding | What's actually there | Decision |
| --- | --- | --- |
| `go-polycall` | `pkg/client.go` imports `.../go-polycall/internal`, a package that **does not exist anywhere in this checkout** (`find bindings/go-polycall -iname internal` returns nothing) -- the legacy client does not compile. `config/` and `examples/` reference the same missing package. | **Keep, mark non-functional.** Not deleted (removing code without knowing whether `internal/` exists in a sibling snapshot or was simply never committed needs a maintainer call, not a unilateral delete). Use `bindings/go-polycall/rpcv1/` instead -- built, tested, works today. |
| `java-polycall` | `org.obinexus.ffi.LibPolyCallJNI` is a **stub**: `// TODO: Implement JNI interface`, empty class body. `org.obinexus.core.ProtocolBinding` / `ProtocolHandler` / `StateManager` / `TelemetryObserver` are real code (283 lines total) implementing a host:port protocol client, but depend on Maven-resolved `org.slf4j` and were not built or run as part of this pass (no network access confirmed for `mvn` dependency resolution; `pom.xml` was not exercised). | **Keep, mark unverified.** The JNI stub is aspirational scaffolding, not a second working integration path -- don't document it as one. `ProtocolBinding` et al. may well work; it just wasn't proven here. Use `bindings/java-polycall/rpcv1/` instead -- built, tested, works today, and needs only the JDK (no Maven, no external dependency). |
| `lua-polycall` | `polycall/core.lua`'s `connect()` and `authenticate()` are literally `error("ADAPTER COMPLIANCE: ...")` -- calling them always fails. This is scaffolding that documents an intent, not a working client. | **Keep, mark as scaffolding.** Use `bindings/lua-polycall/rpcv1/` instead: `frame.lua` (wire framing) is real and unit-verified, including a byte-for-byte comparison against a reference frame from the already-verified Python client. The live socket path (`client.lua`, `polycall_call.lua`) needs LuaSocket, which is **not installed** in this environment and could not be added without an interactive `sudo` password (`apt-cache policy lua-socket` shows it as an available, uninstalled candidate). Live round-trip testing is **NOT RUN**; next step: `sudo apt install lua-socket` (or `luarocks install luasocket`), then run `tests/conformance/` for Lua. |
| `zig-polycall` | A substantial (1162-line) from-scratch reimplementation across `c.zig`, `core.zig`, `network.zig`, `protocol.zig`, `state_machine.zig`, `types.zig` -- not a stub, but not evaluated here. | **Deferred, untouched.** No `zig` toolchain is installed and none could be added without an interactive `sudo` password (`apt-cache policy zig` shows a candidate, uninstalled). No `polycall_rpc` v1 client was written for Zig in this pass: writing Zig code with no compiler available to check it against would be exactly the "fabricated / unverified" work this project has avoided everywhere else. Next step: install Zig (`sudo apt install zig` or the official tarball), then add `bindings/zig-polycall/rpcv1/`. |
| `pypolycall` | A separate, older package (`pypolycall/__init__.py` plus a `backup_20250603_233522/` directory, itself evidence of past instability) implementing -- or attempting to implement -- the legacy protocol. | **Keep separate from `bindings/python-client/` and `bindings/python-provider/`.** Those two are deliberately minimal, stdlib-only reference implementations of the *new* surfaces (`polycall_rpc` v1 client, schema-v2 config provider) built and fully tested in Stages 2-3. Folding them into `pypolycall` now would couple tested, working code to an unaudited legacy package with its own dependency and structural history. Revisit once `pypolycall` itself has been reviewed and, if kept, migrated. |
| `cbl-polycall` | The README describes a build pipeline `nlink -> polybuild -> CBLPolyCall` (README.md:330) that this pass did not investigate. Multiple backup files exist alongside the current sources (`Makefile.backup`, `Makefile.v1.0.backup`, `build.bat.v1.0.backup`, `MAIN.CBL.backup`, `POLYCALL.CBL.backup`), suggesting the same build has churned before. | **Deferred, untouched.** No `cobc` (GnuCOBOL) is installed and none could be added without an interactive `sudo` password. Reconciling with `nlink`/`polybuild` needs its own investigation pass with a working COBOL toolchain, as `docs/TODO.md` itself anticipates -- not a quick client addition. Next step: install GnuCOBOL (`sudo apt install gnucobol`), read the full `nlink`/`polybuild` pipeline description, then decide whether a `polycall_rpc` v1 client fits inside that pipeline or beside it. |

## Toolchains this pass could not install

`lua-socket`, `zig`, and `gnucobol` are all available via `apt` in the WSL2
environment used for Linux verification (`apt-cache policy` confirms
candidates) but installing any of them needs `sudo`, and this environment has
no passwordless `sudo` (`sudo -n true` fails with "interactive authentication
is required"). Nothing was installed non-interactively; nothing was worked
around. A maintainer with shell access to that environment (or an equivalent
one) can unblock all three with:

```sh
sudo apt install lua-socket gnucobol zig
```

## Bindings actually built and verified against a real running runtime

`bindings/<lang>-client/` or `bindings/<lang>-polycall/rpcv1/`, all speaking
`polycall_rpc` v1 alongside (never replacing) the directory's existing code:

| Language | Location | Verified |
| --- | --- | --- |
| C | the CLI itself (`polycall call`) | Windows + Linux |
| Node.js | `bindings/node-client/` | Windows + Linux |
| Python | `bindings/python-client/` | Linux (no interpreter on the Windows host used here) |
| Go | `bindings/go-polycall/rpcv1/` | Windows (`go build`, `go vet` clean; no Go toolchain in the WSL2 environment used) |
| Java | `bindings/java-polycall/rpcv1/` | Windows (JDK 25; no `java`/`javac` in the WSL2 environment used) |
| Lua | `bindings/lua-polycall/rpcv1/` | wire framing only (Linux, byte-verified against Python); live socket round trip NOT RUN (no LuaSocket) |
