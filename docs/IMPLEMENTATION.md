# PolyCall CLI redesign — implementation delivery

Worktree: local checkout seeded from the `libpolycall-main` snapshot
(SHA-256 `a867ee49…`, the provisional baseline in `source-audit.md`). The four
supplied ZIPs are extracted read-only under `refs/` (git-ignored) and were used
for reference only — **not merged**. Git history: baseline import `c8502a4`,
then one commit per stage.

| Stage | Commit | What shipped |
| --- | --- | --- |
| baseline | `c8502a4` | unmodified `libpolycall-main` |
| 1 | `ab4f3a3` | Make/CMake builds + CLI contract |
| 2 | `059ed5d` | typed schema-v2 config + C/Node/Python providers |
| 3 | `e1a0bd3` | cross-language operation dispatch (`polycall_rpc` v1) |
| 4 | `9bcfd5a` | portability verification + MicroVM strict adapter + platform report |

## Baseline build status that was found (Stage 0)

* `make … all` on the pristine snapshot **failed** at the `verify-objects`
  recipe: `$(MAKE)` expanded to `C:/Program Files (x86)/GnuWin32/bin/make`
  (spaces + parens) inside a `( … )` shell group → MSYS `sh` parse error
  ``syntax error near unexpected token `('``. All 10 objects compiled cleanly
  first.
* No root `CMakeLists.txt` existed (matches the audit).
* The CLI did not implement `--version` / `help` / `doctor`: any invocation
  dropped into the interactive loop, read EOF from stdin, printed `Goodbye!`,
  exited 0. `polycall config …` was the one working surface.

## Stage 1 — build + CLI contract

* GNU Make build rewritten (works with GnuWin32 make 3.81 under MSYS sh):
  isolated `$(BUILD_DIR)` (`obj/`, `lib/`, `bin/` per toolchain), static-pattern
  compile rules so core / CLI / shared objects never shadow each other,
  `-std=gnu11 -MMD -MP`, toolchain fingerprint drops incompatible objects,
  targets `all static shared cli test install clean help provider-c`. Shared
  lib emits an import library on Windows; `install` honours `PREFIX` /
  `DESTDIR`, no `ldconfig`.
* Independent root `CMakeLists.txt` (3.20, C11, no extensions):
  `polycall_static` / `polycall_shared` / `polycall_cli`, `GNUInstallDirs`,
  `CTest`, exported `polycallConfig.cmake`, explicit exports
  (`WINDOWS_EXPORT_ALL_SYMBOLS OFF` + hidden visibility). Never calls `make`.
* `include/polycall_export.h`: `POLYCALL_API` / `POLYCALL_LOCAL` /
  `POLYCALL_CALL`, three separate version contracts. `polycall.h`'s four public
  symbols are decorated; layout and names unchanged.
* New `src/cli/`: one argv parser (globals before/after command, `--`
  terminator, `--opt=value`, no re-tokenising, quoted paths preserved; unknown
  option/command, duplicate scalar, missing value → exit 2), one command
  registry with subcommands and shared help. `help` / `version` / `doctor`
  never touch runtime init, sockets or config. Central exit-code map
  (`include/polycall_cli.h`). `--format json` = one object
  `{schema_version, ok, command, data, error}`. `repl` is explicit only and
  dispatches each line through the same parser. `src/main.c` slimmed 912 → ~25
  lines. `config *` delegates to the unchanged legacy loader.
* Tests: `tests/cli/contract.sh` (29 assertions) + three ABI consumers
  (static link, import-lib link, `dlopen`/`LoadLibrary`).

## Stage 2 — native configuration + migration

* `include/polycall_config2.h` + `src/config/config2.c`: opaque typed model,
  independently versioned from `polycall_config_t` (untouched). Builder API,
  full validation (port range, positive ints, identifiers, duplicate service
  ids, unknown fields, TLS refs must be `env:` / `file:` — never inline
  secrets). Canonical JSON envelope (fixed key order, services sorted by id,
  ports as numbers) — the cross-language equivalence oracle. `src/config/json.c`
  is a small internal JSON DOM.
* Provider hosts (`src/config/provider_host.c`, `include/polycall_provider.h`):
  sub-process host (argv **array**, no shell; explicit cwd; 10 s deadline
  capped at 120 s; 1 MiB output budget — timeout / non-zero exit / oversize
  each fail distinctly) and in-process C host
  (`LoadLibraryExW` / `dlopen` of an **explicit** path → `polycall_config_provider_v1`;
  no directory scan). `doctor` never runs a provider.
* Providers: C (`bindings/c-provider/`), Node (`bindings/node-provider/`),
  Python (`bindings/python-provider/`). All emit the identical canonical
  envelope.
* CLI: `config validate` / `config show` take `--provider c:LIB|node:MOD|python:MOD`
  or `--envelope FILE` (native path) — no selector → unchanged legacy loader
  (explicit selection, never merged). `config show --provenance`.
  `config migrate --from-legacy [--language L] --output F`: legacy **effective**
  model → v2 envelope, preserving values, refusing to overwrite, **reporting**
  unmapped keys. `bindings list` = honest host capability report.
* Tests: `tests/config/model_test.c` (7 groups) + `tests/config/equivalence.sh`
  (18: C == Node envelope byte-identical; failure modes; secret redaction;
  legacy migration equivalence).

## Stage 3 — real cross-language dispatch

* `polycall_rpc` v1 (`src/runtime/rpc_wire.*`, `docs/RPC.md`): 16-byte header
  (`PCR1`, type, correlation id, BE length ≤ 1 MiB) + UTF-8 JSON payload. No C
  struct or pointer on the wire. Partial-IO-safe send/recv. This is a **new,
  additively-versioned** surface; `polycall_protocol.h` is untouched.
* Runtime (`src/runtime/runtime.c`, `include/polycall_runtime.h`): operation
  registry with typed metadata; deterministic built-ins `inventory.get`
  (stock by item id), `debug.echo`, `debug.sleep`. `polycall_runtime_serve()`
  = foreground loopback TCP, port 0 = ephemeral, `on_bound` callback reports
  the endpoint. Control channel `ping` / `describe` / `shutdown`; `shutdown`
  enforces `--auth-token`. `SIGINT`/`SIGTERM` only set a flag — **all cleanup
  runs in normal execution**.
* CLI `run` / `status` / `stop` / `call`. `call` does **exactly one round trip,
  never retries**. Exit map: 3 malformed/invalid input, 4 unknown
  operation/item, 5 transport, 6 deadline, 7 auth, 1 other.
* Clients: `bindings/node-client/inventory_client.mjs`,
  `bindings/python-client/inventory_client.py`.
* Tests: `tests/runtime/roundtrip.sh` (16): same `inventory.get` from the C
  CLI **and** the Node client → identical result; failure modes distinct;
  authenticated shutdown sequence.

## Stage 4 — portability + report

* Verified running from **PowerShell with MSYS2 off `PATH`**: CLI, JSON,
  full `run`/`call`/`status`/`stop`, and `dlopen` of `libpolycall.dll`. DLL
  needs only `KERNEL32` / `msvcrt` / `WS2_32`.
* Edge cases pass: spaced paths, CRLF config, header-dependency rebuild,
  clean/rebuild, staged install **relocation** + external consumer.
* Known limitation: non-ASCII config paths on Windows (`fopen` ANSI code
  page) — fix noted in `docs/PLATFORM_REPORT.md`.
* MicroVM: `bindings/microvm/strict_adapter.mjs` — strict mode requires a
  real round trip (missing runtime = failure, not a pass); demo fallback
  labelled. `tests/microvm/strict_mode.sh` (4).
* `docs/PLATFORM_REPORT.md`: full PASS / FAIL / NOT RUN matrix, exact
  commands, next steps for UCRT64 / MSVC / Linux / macOS / FreeBSD (no
  runners here), intentional compatibility changes, deferred items.

## How to build and test

```sh
# GNU Make (this host: CC=gcc for TDM-GCC; add BUILD_DIR to isolate)
make BUILD_DIR=build/win-tdm CC=gcc all
make BUILD_DIR=build/win-tdm CC=gcc test

# CMake (independent)
cmake -S . -B build/win-cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=gcc \
      -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/win-cmake -j
ctest --test-dir build/win-cmake --output-on-failure

# native Windows (PowerShell / cmd, no MSYS2): use CMake
./build-windows.ps1 -Generator "Ninja" -BuildType Release -Test
```

## Verified toolchains (see docs/PLATFORM_REPORT.md for the full matrix)

| | Windows TDM-GCC | Windows MSVC 19.44 | Linux GCC 15.2 (WSL2) |
| --- | --- | --- | --- |
| build (Make + CMake) | PASS | PASS (CMake+Ninja) | PASS |
| CLI contract | 29 | 29 | 29 |
| config2 model / equivalence / roundtrip | 7 / 18 / 16 | 7 / 18 / 16 | 7 / **19** / **17** |
| microvm strict / ABI consumers | 4 / 3 | 4 / 3 | 4 / 3 |

Python provider/client paths run on Linux (C ≡ Node ≡ Python canonical
envelope); NOT RUN on the Windows hosts (no interpreter). NOT RUN elsewhere:
Linux/Clang (no clang), UCRT64, macOS, FreeBSD (no runners).

## Not published / not deployed

No package publish, no release, no deploy — as instructed.
