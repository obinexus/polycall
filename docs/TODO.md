# libpolycall — TODO

**Branch:** `polycall-cli-redesign` · **Baseline commit:** `79b0c71` · **Library / ABI:** 1.0.1
**Last reviewed:** 2026-09-11 · **Project status:** _maintainer to set (suspended · in recovery · resumed · completed)_

Phases are ordered by dependency, so finishing one unblocks the next. Every item has acceptance
criteria, so an item counts as done only when its test passes.

---

## Verified baseline (do not regress)

Checked on Linux x86_64 with GCC 13.3, using a serial GNU Make build and CMake + `ctest`.
Windows and macOS were not re-verified in this review.

- [x] `make test` passes: CLI contract 29/29, config equivalence 19/19, runtime roundtrip 17/17, MicroVM strict mode 4/4
- [x] Static, shared/import-library and `dlopen` consumers all link against 1.0.1
- [x] CMake build and `ctest` pass
- [x] Zero compiler warnings under `-Wall -Wextra`
- [x] polycall_rpc v1 (`PCR1` framing) is reachable from the C CLI, `node-client`, `python-client` and the MicroVM strict adapter

---

## P0 — Build and release blockers

### Parallel make race

Symptom: `make -j4 all` fails on every run with `opening dependency file build/obj/config/json.d`.
Cause: the toolchain-fingerprint rule (Makefile, ~line 184) runs `rm -rf $(OBJ_DIR)` and recreates
only `shared/` and `cli/`. Under `-j` this races with the order-only directory targets, so
`obj/config`, `obj/runtime` and their `shared/` twins vanish before compilation.

- [x] Recreate every object subdirectory in the fingerprint rule:
  ```make
  rm -rf $(OBJ_DIR); mkdir -p $(OBJ_DIR) $(OBJ_DIR)/config $(OBJ_DIR)/runtime $(OBJ_DIR)/shared $(OBJ_DIR)/shared/config $(OBJ_DIR)/shared/runtime $(OBJ_DIR)/cli; \
  ```
- **Acceptance:** `rm -rf build && make -j8 all` succeeds on 5 consecutive runs, and `make -j8 test` passes. (This patch met both criteria during the review.)
  - **Done 2026-09-11.** Applied exactly as specified. Verified with `rm -rf build && make -j8 all` for 5 consecutive clean runs, followed by `make -j8 test`, on both Windows (TDM-GCC 10.3.0) and Linux (WSL2, GCC 15.2) — all 10 runs (5+5) passed, both `make -j8 test` runs fully green (29 contract / 7 config2 model / 18-19 equivalence / 16-17 roundtrip / 4 microvm / 3 consumers).

### Dockerfile does not produce a runnable image

These issues come from comparing the Dockerfile with the Makefile outputs. The Dockerfile itself was not run under Docker in this review.

- [x] `make static` builds only `libpolycall.a`, not the executable → use `make static cli` (or `make all`)
- [x] Artifacts land in `build/bin/` and `build/lib/`, not `bin/` and `lib/` → fix the `strip` path and both `COPY --from=builder` paths
- [x] The executable is a dynamically linked glibc PIE, but the runtime stage is `FROM scratch` → link fully static (e.g. a musl builder) or use a base image that ships libc
- [x] `ARG POLYCALL_VERSION=1.0.0` → 1.0.1 (see P5)
- **Acceptance:** `docker build` succeeds; `docker run --rm <image> --version` prints `polycall 1.0.1`; `docker run --rm <image> doctor` exits 0.
  - **Partially done 2026-09-11.** All four fixes applied: `RUN make clean && make static cli && strip build/bin/polycall`; both `COPY --from=builder` lines now read from `/src/build/bin` and `/src/build/lib`; runtime stage switched from `FROM scratch` to `FROM debian:bookworm-slim` (chosen over a musl static build — `ldd` on the built binary shows only `libc.so.6` + the dynamic linker, and bookworm-slim matches the `gcc:12-bookworm` builder's glibc ABI); `ARG POLYCALL_VERSION=1.0.1`. Added a `HEALTHCHECK` running `polycall doctor`.
  - Verified without Docker (no engine on this host): ran the exact `RUN` build line (`make clean && make static cli && strip build/bin/polycall`) directly — succeeds, produces `build/bin/polycall` (117 KB stripped) and `build/lib/libpolycall.a`, both at the paths the `COPY` lines now reference. **`docker build` / `docker run` themselves: NOT RUN — no Docker engine available on this host or in WSL2.** Next verification step: `docker build -t polycall:test .` then `docker run --rm polycall:test --version` and `docker run --rm polycall:test doctor`.

### Continuous integration

- [x] There is no `.github/workflows/`. Add a CI matrix covering:
  - Linux GNU Make with `-j`
  - Linux CMake + `ctest`
  - Windows via `build-windows.ps1` (MSVC)
  - Windows MSYS2 Make
  - macOS Make
- **Acceptance:** every PR runs `tests/run_all.sh` through both build systems, and the parallel-build job would have caught the race above.
  - **Done 2026-09-11.** Added `.github/workflows/ci.yml` with 5 jobs matching the list above (`linux-make` also runs GCC and Clang; `linux-make` and `windows-msys2-make` both build with `-j"$(nproc)"`, which would have caught the P0 race). Every job ends by running the test suite (`make test`, `ctest`, or `build-windows.ps1 -Test`, each of which runs `tests/run_all.sh`). YAML validated (`yaml.safe_load`); **the workflow itself has NOT RUN on GitHub Actions yet** — no CI executed as part of this pass. Next step: push and confirm all 5 jobs go green on a real PR.

---

## P1 — Operation plugin loader (`polycall run --load`)

This completes step three of the canonical workflow: write a language-specific C binding → compile to `.so` with
unified ABI flags → register with the C DRIVER daemon. Today `run` serves only the built-ins
(`inventory.get`, `debug.echo`, `debug.sleep`), so adding an operation means editing C and rebuilding
the binary.

- [x] Declare the exported entry point in `include/polycall_runtime.h`:
  ```c
  POLYCALL_API int POLYCALL_CALL
  polycall_ops_register(polycall_runtime_t *rt, uint32_t abi_major);
  ```
  - Declared as the `polycall_ops_register_fn` typedef (the plugin, not the host, implements the symbol -- matches the existing `polycall_config_provider_v1_fn` convention in `polycall_provider.h`), plus `POLYCALL_PLUGIN_OK` / `POLYCALL_PLUGIN_ABI_MISMATCH` / `POLYCALL_PLUGIN_ERROR` return codes and `polycall_runtime_load_plugin()`.
- [x] Implement `polycall run --load PATH` (repeatable) in `src/cli/cmd_runtime.c`: `dlopen` / `LoadLibrary`, resolve the symbol, check the ABI major version, call register
- [x] Map failures onto the existing exit contract: an unloadable library, missing symbol or ABI mismatch → exit 4
- [x] Decide and document what happens when two plugins register the same `service.operation` (refuse vs last-wins)
  - **Decision: refuse.** `polycall_runtime_register()` already rejects a duplicate `service.operation` (built-in vs. plugin, or plugin vs. plugin); the fixture plugin propagates that failure as `POLYCALL_PLUGIN_ERROR`. Documented in `docs/PLUGINS.md`.
- [x] Unload libraries in the normal shutdown path, never in a signal handler (same rule as `docs/RPC.md`)
- [x] Loaded operations appear in `polycall status` (`describe`) with schema hints and the `idempotent` flag
- [x] Update the `docs/CLI.md` command table and help text, and add CLI contract tests for the new flag
- [x] Add a fixture plugin under `tests/fixtures/plugins/`; `tests/runtime/` covers load, call, ABI mismatch and missing symbol
- **Acceptance:** the fixture plugin is callable from the C CLI and the Node and Python clients with no core rebuild, on Linux and Windows.
  - **Done 2026-09-11.** Verified on Windows (TDM-GCC, both Make and CMake) and Linux (WSL2, GCC 15.2, both Make and CMake): `tests/runtime/plugin.sh`, 11/11 on Linux (C CLI + Node client + Python client all get `{"message":"hello, world"}` from `demo.greet`, byte-identical), 10/11 on Windows (Python NOT RUN — no interpreter on that host; C CLI + Node client both pass there too). Also covers: nonexistent plugin path, a real library missing `polycall_ops_register` (reused `example_provider`), and the deliberately ABI-mismatched build — each fails fast with exit 4 and no socket bound.
  - While verifying `status`/`describe`, found and fixed two pre-existing bugs it depended on (both predate this TODO, never caught because nothing asserted on `describe`'s content): (1) the `status` control request's payload length was hardcoded to 20 bytes for a 21-byte JSON literal, silently truncating it to invalid JSON, so `describe` always fell through to "unknown control action"; (2) the `describe` reply spliced operation schema-hint strings (themselves containing literal `"` characters, e.g. `{"item_id":"string"}`) directly into an already-quoted JSON string without escaping, corrupting the whole reply. Both fixed in `src/cli/cmd_runtime.c` / `src/runtime/runtime.c`.

**Open decision:** whether `Polycallfile` gains a plugin key. `docs/CONFIGURATION_STANDARD.md` gives topology to `Polycallfile`, so decide after the CLI flag lands. **Still open** — `--load` today is CLI-only, by design (P1 didn't require a config-file surface); revisit once there's a concrete need for plugin lists to persist across invocations.

---

## P2 — Bindings onto polycall_rpc v1

Only `node-client`, `python-client` and `microvm` frame `PCR1`. The remaining bindings target the legacy
protocol in `include/polycall_protocol.h`.

- [ ] **Go:** add a polycall_rpc v1 client alongside `bindings/go-polycall/pkg/client.go`. Go is the orchestration language, so it goes first.
- [ ] **Java:** `bindings/java-polycall`
- [ ] **Lua:** `bindings/lua-polycall`
- [ ] **Zig:** `bindings/zig-polycall`
- [ ] **Python:** fold `bindings/python-client` into `bindings/pypolycall`, or document why they stay separate
- [ ] **COBOL:** `bindings/cbl-polycall`; reconcile with the nlink → polybuild orchestration described in its README
- [ ] For each binding, decide whether its legacy-protocol client is kept, deprecated or removed, and record the decision in `docs/backward_compatibility.md`

### Cross-language conformance harness

- [ ] Generalise `tests/runtime/roundtrip.sh` into `tests/conformance/`, with shared request fixtures and expected responses
- [ ] Each client emits normalised responses, and their SHA-256 hashes are compared across languages
- [ ] Cover these cases:
  - success
  - `input.invalid` (exit 3)
  - `operation.unknown` (exit 4)
  - no runtime (exit 5)
  - `deadline.exceeded` (exit 6)
  - `auth.denied` (exit 7)
  - a non-idempotent operation is never retried
- [ ] Classify each binding as bidirectional (square) or asymmetric driver (rectangle)
- **Acceptance:** a binding is marked stable in the docs only after it passes the full conformance set.

---

## P3 — Flagship demo: ledger service

- [ ] `examples/ledger/` plugin, loaded through P1, with `ledger.balance` (idempotent) and `ledger.transfer` (non-idempotent)
- [ ] Driven from the Node, Python and Go clients against a single runtime
- [ ] Show that `transfer` makes exactly one round trip and is never retried
- [ ] Stretch goal: a GnuCOBOL `cobc -m` implementation behind a thin C shim, producing the same conformance output as the C version
- **Acceptance:** from a clean checkout, `make` plus one script runs the whole demo.

---

## P4 — Runtime concurrency

The runtime is single-threaded and serves one connection at a time. `docs/RPC.md` scopes concurrency as a
separate change.

- [ ] Write the scope document first: threading model, per-connection deadlines, shutdown semantics
- [ ] Serve multiple concurrent clients without changing the wire format
- [ ] Keep the existing rule: signal handlers only set a flag, and all cleanup runs in normal code
- **Acceptance:** the ledger demo runs with N concurrent clients, and all existing tests pass unchanged.

---

## P5 — Version and documentation alignment (SemVerX)

The current sources of truth are `CMakeLists.txt` (`VERSION 1.0.1`) and `include/polycall_export.h`
(`POLYCALL_ABI_VERSION_STRING "1.0.1"`).

- [ ] README line 3 says v1.0.0 → change to 1.0.1
- [ ] The README section "Version 1.1.0 - Unified Architecture (2024-09-10)" and the top `[1.1.0]` entry in `docs/CHANGELOG.md` conflict with 1.0.1 → reconcile them and add a 1.0.1 entry for the CLI redesign
- [ ] The README points to `obinexus/libpolycall-v1trial` and "v1trial" → point to this repository instead
- [ ] `docs/VMILESTONE.md` marks the Go, Java and Lua bindings stable, but they are not on polycall_rpc v1 → re-mark them from P2 conformance results
- [ ] Update the Dockerfile version `ARG` (tracked in P0)
- [ ] Make one version source feed Make, CMake, the Dockerfile and the bindings
- [ ] Tag the release per SemVerX once P0 is clear

### README fixes

- [ ] Line 15 has broken image syntax, `![LibPolycall Version Favicon)(./favicon.png)` → fix to `![LibPolycall Version Favicon](./favicon.png)`
- [ ] The README ends on an empty "Web IaaS Architecture" heading → fill it or remove it
- [ ] Add a quick start drawn from `docs/CLI.md`: build, test, `polycall run`, `polycall call`

---

## P6 — Repository hygiene

- [ ] Remove backup artifacts; git history already preserves them:
  - `bindings/cbl-polycall/src/MAIN.CBL.backup`
  - `bindings/cbl-polycall/src/POLYCALL.CBL.backup`
  - `bindings/cbl-polycall/Makefile.backup`
  - `bindings/cbl-polycall/Makefile.v1.0.backup`
  - `bindings/cbl-polycall/build.bat.v1.0.backup`
  - `bindings/pypolycall/backup_20250603_233522/`
- [ ] Remove the byte-identical duplicate docs:
  - `docs/LibPolyCall Architecture Definition Document` (no extension) duplicates the `.md` version
  - `docs/web-polycall-integration-guide (1).md` duplicates `docs/web-polycall-integration-guide.md`
- [ ] Neither Make nor CMake builds `test/test_polystate.c` or `test/test_polystate_machine.c` → either wire them into `tests/run_all.sh` or remove them, then merge `test/` into `tests/`
- [ ] `LICENSE` ("Use It, Respect It") and `LICENSE.md` (OBINexus NT Open Access, MIT text) differ, and the Dockerfile label declares MIT → maintainer decides the canonical licence and makes all three consistent
- [ ] Consider slug filenames for docs that contain spaces or en dashes. Do **not** rename `tests/fixtures/with space/`, which is an intentional fixture.
