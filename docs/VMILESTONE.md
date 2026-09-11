# Release Milestone: v1.0.stable

## Pre-Release Checklist ✅
- [x] All tests passing
- [x] Documentation updated
- [x] Banking demo functional
- [x] All bindings compiled
- [x] SemVerX version tagged

## Components Status

Original entries above predate the 2026-09 CLI redesign and are left as the
historical record. The table below re-marks each binding specifically for
`polycall_rpc` v1 (docs/RPC.md), per the cross-language conformance harness
(`tests/conformance/run.sh`, results in `docs/CONFORMANCE.md`) -- "stable"
here means it passed every harness case on at least one host; a binding
being "stable" for the *legacy* protocol (`include/polycall_protocol.h`) is
a separate question, see `docs/backward_compatibility.md`.

| Binding | `polycall_rpc` v1 status |
| --- | --- |
| Core C runtime | stable -- reference implementation, verified Windows (TDM-GCC, MSVC) + Linux (GCC) |
| Node.js | stable -- passed the full harness on Windows and Linux |
| Python | stable -- passed the full harness on Linux; no interpreter on the Windows host used, so not cross-platform-confirmed there |
| Go | verified-on-one-platform -- passed the full harness on Windows; not yet run on Linux (no `go` toolchain in the environment used) |
| Java | verified-on-one-platform -- passed the full harness on Windows; not yet run on Linux (no `java`/`javac` in the environment used) |
| Lua | not stable -- wire framing is unit-verified (byte-identical to a Python reference frame); the live harness has never run for it (no LuaSocket available, see `docs/backward_compatibility.md`) |
| Zig, COBOL | no `polycall_rpc` v1 client exists yet (deferred, no toolchain available -- `docs/backward_compatibility.md`) |

## Release Assets
- [ ] Source code (auto)
- [ ] Pre-compiled binaries
- [ ] Docker image
- [ ] Installation script
