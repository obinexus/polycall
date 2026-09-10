# Platform verification report

Generated 2026-09-10. Every row is PASS / FAIL / NOT RUN with the exact
commands and the artifacts produced. Nothing here is a support badge; a
NOT RUN row needs a real runner before support is claimed.

## Hosts used

| | |
| --- | --- |
| **Windows 11** (10.0.26200), native | TDM-GCC 10.3.0 (`x86_64-w64-mingw32`, native PE / msvcrt) + GnuWin32 `make` 3.81 (run from MSYS2 / Git-Bash); CMake 4.4.2; **MSVC 19.44** (VS 2022 BuildTools) + Ninja; Node present; **Python absent** |
| **WSL2 Ubuntu** (Linux 6.18) | GCC 15.2, GNU Make 4.4.1, CMake, **Node + Python 3** present; no Clang |

TDM-GCC and MSVC binaries are genuine native Windows executables (TDM-GCC deps:
`KERNEL32` / `msvcrt` / `WS2_32` — **no `msys-2.0.dll`**), verified running from
PowerShell with every MSYS2 directory removed from `PATH`. Neither Windows host
is UCRT64; that row stays NOT RUN.

## Build matrix

| Target | Route | Result | Evidence |
| --- | --- | --- | --- |
| Windows x86-64 | TDM-GCC (MinGW), GNU Make 3.81 | **PASS** | `make BUILD_DIR=build/win-tdm CC=gcc all` clean (`-Wall -Wextra -std=gnu11`); full suite green |
| Windows x86-64 | TDM-GCC (MinGW), CMake `Unix Makefiles` | **PASS** | `cmake -S . -B build/win-cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && cmake --build build/win-cmake -j` clean; `ctest` 100% |
| Windows x86-64 | **MSVC 19.44 + Ninja**, CMake | **PASS** | Developer env → `cmake -S . -B build/msvc -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && cmake --build build/msvc` (42/42, no warnings). Full shell suite green with `CC=cl`. Distinct lib names: `polycall-static.lib` vs import `polycall.lib` + `polycall.dll`. CLI links `polycall-static` with `/STACK:8388608`. |
| **Linux x86-64** | **GCC 15.2**, Make and CMake | **PASS** | WSL2 — `make BUILD_DIR=build/linux-gcc CC=gcc all test` clean; `cmake -S . -B build/linux-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && cmake --build ... && ctest` 100%. ELF `polycall` + versioned `libpolycall.so` (`-soname`) + `libpolycall.a`; `.so` NEEDED = `libc.so.6` only. Python provider + client paths **run** here. |
| Linux x86-64 | Clang, Make and CMake | **NOT RUN** | no clang on the WSL host. Next: `apt install clang && make BUILD_DIR=build/linux-clang CC=clang all test` and `cmake -DCMAKE_C_COMPILER=clang ...`. |
| Windows x86-64 | MSYS2 **UCRT64** GCC, Make and CMake/Ninja | **NOT RUN** | no UCRT64 toolchain installed. Next: in a UCRT64 shell — `command -v gcc; gcc -dumpmachine`, `make BUILD_DIR=build/ucrt64 CC=gcc all test`, `cmake -S . -B build/ucrt64-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && cmake --build build/ucrt64-cmake && ctest --test-dir build/ucrt64-cmake --output-on-failure` |
| macOS | Apple Clang, Make and CMake | **NOT RUN** | no macOS runner. Make selects `.dylib` / `-dynamiclib` / `-install_name @rpath/...`. Next: `make BUILD_DIR=build/macos CC=cc all test` and the CMake equivalent; `otool -L` + install-name relocation check. |
| FreeBSD | Clang, `gmake` and CMake | **NOT RUN** | no FreeBSD runner. Next: `gmake BUILD_DIR=build/freebsd CC=cc all test`; CMake equivalent; runtime smoke test. |

## Test results on the PASS rows

| Suite | Win TDM-GCC | Win MSVC | Linux GCC |
| --- | --- | --- | --- |
| CLI contract (`tests/cli/contract.sh`) | 29 PASS | 29 PASS | 29 PASS |
| config2 typed model (`tests/config/model_test.c`) | 7 groups PASS | 7 groups PASS | 7 groups PASS |
| provider equivalence + migration (`tests/config/equivalence.sh`) | 18 PASS, 1 NOT RUN¹ | 18 PASS, 1 NOT RUN¹ | **19 PASS** |
| runtime roundtrip (`tests/runtime/roundtrip.sh`) | 16 PASS, 1 NOT RUN¹ | 16 PASS, 1 NOT RUN¹ | **17 PASS** |
| MicroVM strict-mode adapter (`tests/microvm/strict_mode.sh`) | 4 PASS | 4 PASS | 4 PASS |
| ABI consumers: static / import-lib / dlopen | 3 PASS | 3 PASS | 3 PASS |

¹ NOT RUN = Python provider/client path; the Windows hosts have no Python
interpreter. Linux exercises it (C ≡ Node ≡ **Python** canonical envelope; the
Python client's `inventory.get` result equals the C CLI's).

Key checks inside those suites:

* `polycall --version` immediate single line, no banner; unknown option/command
  → exit 2; `--format json` emits one object for success and failure; help
  never binds a port; no implicit REPL on closed stdin; `--` terminates
  options; quoted and spaced paths survive argv.
* C provider envelope **byte-identical** to the Node provider envelope for the
  same two-service topology; ports serialised as JSON numbers; invalid port /
  unknown field / provider load-failure / hung-provider deadline / oversize
  output each fail with a distinct code; inline TLS secret rejected and never
  echoed; `env:` / `file:` references shown but never resolved by `config
  show`; validation starts no service.
* Legacy `Polycallfile` → `config migrate --from-legacy` preserves effective
  ports / timeout / TLS refs, reports unmapped keys (`enable_metrics`,
  `metrics_port`, …) instead of dropping them, refuses to overwrite the
  output, leaves the source file intact.
* One deterministic C operation `inventory.get` reached from the C CLI **and**
  the Node client through the same `polycall run` process → identical result
  `{"item_id":"widget-a","quantity":42,"in_stock":true}`. Failure modes fail
  distinctly: unknown item / operation → 4, malformed / missing input → 3,
  past-deadline → 6, missing runtime → 5. Authenticated shutdown: wrong token
  → 7 and the runtime keeps serving; right token → 0 and it exits; `status`
  afterwards → 5. Cleanup runs in normal execution (SIGINT only sets a flag).
* MicroVM strict adapter: no runtime → distinct failure (never a pass); demo
  fallback carries `"_demo_fallback": true`; strict result == CLI result.

## PE / linkage verification (Windows PASS rows)

```
objdump -p build/win-tdm/lib/libpolycall.dll   # exports = the 33 POLYCALL_API
                                                #  functions only; json_*/pcr_*
                                                #  and all statics are hidden
objdump -p build/win-tdm/lib/libpolycall.dll | grep "DLL Name:"
    KERNEL32.dll   msvcrt.dll   WS2_32.dll      # no msys-2.0.dll
objdump -p build/win-tdm/bin/polycall.exe | grep "DLL Name:"
    KERNEL32.dll   msvcrt.dll                   # CLI links the core statically
```

Run outside MSYS2 (PowerShell, every MSYS dir removed from `PATH`): `polycall
--version`, `doctor`, `--format json doctor` (parsed by `ConvertFrom-Json`),
`polycall run` + `call` + `status` + `stop`, and the `dlopen_consumer.exe`
loading `libpolycall.dll` via `LoadLibrary` — all succeed.

## Portability edge cases (Windows PASS rows)

| Case | Result |
| --- | --- |
| Path with a space (`tests/fixtures/with space/Polycallfile`) | PASS |
| CRLF config file (`tests/fixtures/crlf/Polycallfile`) | PASS |
| Toolchain-fingerprint change drops stale objects | PASS (stamp in `$(BUILD_DIR)/.toolchain`) |
| Touch a public header → only dependents recompile; no-op build is quiet | PASS (`-MMD -MP`) |
| `make clean` removes `$(BUILD_DIR)`; rebuild from clean | PASS |
| Staged `make install PREFIX=<dir>`, then **relocate** the tree, then compile an external consumer against the moved headers + `libpolycall.a` | PASS |
| `make install DESTDIR=<dir> PREFIX=/usr/local` | PASS on POSIX semantics; on MSYS2 an absolute POSIX `PREFIX` is rewritten by the path-conversion layer — use `MSYS2_ARG_CONV_EXCL='*'` or a Windows-style `PREFIX` (documented in the Makefile) |
| **Non-ASCII directory** in a config path on Windows | **FAIL (known limitation)** — the file-opening code (`src/polycall_config.c`, and `read_file` in `src/cli/cmd_config2.c`, and `src/config/legacy_import.c`) calls `fopen()`, which uses the ANSI code page on Windows, so a UTF-8 argv path with non-ASCII bytes does not resolve. Fix: a `_wfopen` / `CreateFileW` wrapper via UTF-8→UTF-16 at those three call sites. Not yet done; ASCII paths incl. spaces are unaffected. |

## Portability fixes made to the baseline C (Stage 4)

These pre-existing issues only surfaced with strict `-std=c11` on glibc or with
MSVC; GCC/Clang on `gnu11` tolerated them:

* `src/network.c` / `src/polycall_protocol.c` used `usleep` and `ssize_t` — hidden
  under strict C11 on glibc. Fix: GCC/Clang builds now use **`-std=gnu11`**
  (Make and CMake); MSVC keeps `/std:c11` and gets an `ssize_t` typedef
  (`SSIZE_T`) in `network.h`.
* `src/polycall_state_machine.c` used `__attribute__((unused))` (GCC-only). Fix:
  replaced with `(void)param;` casts.
* MSVC has a 1 MB default stack; three CLI helpers held a `~1 MiB` envelope
  buffer on the stack → `STATUS_STACK_OVERFLOW`. Fix: those buffers are now
  heap-allocated (`envelope_dup`), and the CLI target links `/STACK:8388608` on
  MSVC to match GCC's ~8 MB.
* `src/cli/cli.c` `--help` path built the positional array in an inner block
  whose lifetime ended before it was used (use-after-scope). GCC left the stack
  slot intact; MSVC clobbered it → `help <cmd>` printed "no such command".
  Fixed by hoisting the array to the enclosing scope.
* The CLI's `python:` provider now spawns `python3` on POSIX / `python` on
  Windows, with a `POLYCALL_PYTHON` override.
* The Make build now fails fast with a clear message when run without a POSIX
  shell (cmd.exe / PowerShell) instead of spewing `tr`/`mkdir -p` errors.

## Intentional compatibility changes

* GCC/Clang builds compile with `-std=gnu11` (was implicitly `gnu11` in the
  baseline; my Stage 1 `-std=c11` broke POSIX visibility on Linux — reverted).
* `polycall -f PATH` is **removed**; it now exits 2 pointing at
  `polycall run --config PATH`. (`docs/USAGE.md` still shows the old form.)
* No-argument invocation prints help and exits 0 instead of dropping into the
  interactive loop. `polycall repl` is the only way in.
* The GNU Make build no longer emits a `bin/polycall` next to the sources; all
  outputs go under `$(BUILD_DIR)`. The `static` target now builds the static
  **library**, not a static executable.
* `make install` no longer runs `ldconfig`.
* `polycall_get_version()` / the CLI `--version` report `1.0.1` consistently
  (the old `main.c` printed a separate `PPI_VERSION` `1.0.0`).

## Not in scope / deferred

* Concurrency in `polycall run` (one connection at a time today).
* TLS on the wire (`tls.mode` is validated and carried in config; the runtime
  socket is plaintext loopback).
* Non-loopback control access and finer shutdown authorization.
* Phone-call media / signaling / NAT traversal / mobile / PSTN — a separate
  later SDK task, not started.
