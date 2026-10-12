# Changelog

All notable changes to LibPolyCall will be documented in this file.

> **Version numbering note:** the shipped library/ABI version has read
> `1.0.1` throughout (`include/polycall_export.h` is the canonical source).
> The `[1.1.0]` entry below was recorded alongside an earlier documentation
> merge and was never matched by an actual library-version bump. It is kept
> as the historical record, immediately below, not as a claim that a 1.1.0
> release ever shipped.

## [Unreleased]

### Changed
- The `run` command is renamed to `start` (registry, help text, JSON
  `command` field, telemetry event names `start.bound`/`start.stopped`/
  `start.plugin_failed`/`start.bind_failed`). The removed `-f` flag's
  error message now points at `polycall start --config PATH`.

### Added
- `start --daemon`: detaches into the background and returns immediately,
  printing the detached process's PID. POSIX: a standard double-fork
  (`setsid` + a second `fork`) with stdout/stderr redirected to the null
  device. Windows: re-execs the same binary with `DETACHED_PROCESS`
  (there is no `fork()`). Plugin-load and bind failures in a daemonized
  process are reported via `telemetry` only, since there is no terminal
  left to print to.
- Debian/Ubuntu packaging (`debian/`): `polycall` (CLI), `libpolycall1`
  (shared library, SONAME `libpolycall.so.1`) and `libpolycall-dev`
  (headers, static library, CMake package), built per release for Debian
  12/13 and Ubuntu 22.04/24.04/26.04 on amd64 and arm64, published to a
  signed third-party APT repository on GitHub Pages by `release.yml`. See
  the README and `docs/PACKAGING.md`.
- `polycall(1)` manual page (`docs/man/polycall.1`), installed by both
  `cmake --install` and `make install`.

### Fixed
- Link libm explicitly (CMake and Make): `src/config/json.c` calls
  `floor()`/`fabs()`, which only GCC at `-O2` inlines; Clang and `-O0`
  builds failed to link the CLI, and the shared library carried an
  unresolved `floor`.
- `make install` now also installs `polycall_telemetry.h`, matching CMake.
- `tests/runtime/roundtrip.sh` no longer fails the C CLI checks when
  Node.js is absent: it parses JSON with Node or Python, or reports NOT RUN.
- Removed the in-source CMake/Visual Studio configure output that had been
  committed (`CMakeCache.txt`, `CMakeFiles/`, `*.vcxproj`, `polycall.sln`,
  ...); its `CMakeCache.txt` made every `cmake ..` build from a fresh
  checkout fail.

## [1.0.0] - 2026-09-16 - Version reset

Library/ABI version set to `1.0.0` (`include/polycall_export.h`), reverting
the `1.0.1` used since the CLI redesign below. No functional change.

## [1.0.1] - 2026-09-11 - CLI Redesign

Delivered on the `polycall-cli-redesign` branch (docs/IMPLEMENTATION.md has
the full account; docs/PLATFORM_REPORT.md and docs/CONFORMANCE.md record
exactly what was verified, where).

### Added
- One predictable CLI contract: argv parser, command registry, central
  exit-code map, `--format json` for finite commands, `doctor`.
- A typed schema-v2 configuration model alongside the unchanged legacy
  loader, with C / Node.js / Python providers and legacy->v2 migration.
- The `polycall_rpc` v1 wire and a real foreground runtime: `run` / `status`
  / `stop` / `call`, reachable identically from the C CLI and from Node.js,
  Python, Go and Java clients.
- An operation-plugin loader (`run --load PATH`, repeatable): add operations
  from a shared library with no core rebuild.
- A cross-language conformance harness comparing raw response bytes
  (SHA-256) across every available client.
- Independent GNU Make and CMake builds, a GitHub Actions CI matrix, and a
  corrected, buildable `Dockerfile`.

### Fixed
- A GNU Make `-j` race that could drop object subdirectories mid-build.
- Several MSVC- and strict-C11-only portability bugs (see
  docs/PLATFORM_REPORT.md) found while verifying on additional toolchains.
- Two bugs in the runtime's `status`/`describe` control response (a
  hardcoded, wrong payload length; unescaped JSON embedded in an
  already-quoted string) found while verifying the plugin loader.
- `polycall_get_version()` no longer hardcodes its own copy of the version
  string; it reads `POLYCALL_ABI_VERSION_STRING`, the same constant
  `include/polycall_export.h` and the CLI's `--version` use.

### Changed
- `polycall -f PATH` removed; `polycall run --config PATH` replaces it.
- No-argument invocation prints help and exits 0 instead of entering an
  implicit REPL; `polycall repl` is now the only way into one.

## [1.1.0] - 2024-09-10

### Added
- COBOL binding (cbl-polycall) for mainframe integration
- Hotwire architecture for hot-swapping components
- DOP (Data-Oriented Programming) adapter
- Socket implementation for real-time communication
- Comprehensive legal framework documentation
- Migration tools and scripts

### Changed
- Unified polycall and libpolycall repositories
- Restructured bindings directory for v1/v2 separation
- Enhanced build system with better CMake support

### Deprecated
- Standalone polycall repository (merged into libpolycall)

### Security
- Enhanced zero-trust security model
- Improved port binding enforcement

## [1.0.0] - 2024-09-10
- Initial release with 5 language bindings
- Core polymorphic routing engine
- Banking system demonstration
