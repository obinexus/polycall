# C library and ABI

## Artifact types

| Extension | What it is |
| --- | --- |
| `.a` / `polycall-static.lib` | Static archive; the linker copies what your program calls into your executable, no runtime dependency on `libpolycall` itself. |
| `.so` / `.dylib` / `.dll` | Shared library, loaded at run time (Linux/macOS/Windows respectively). |

CMake builds both `polycall_static` and `polycall_shared` from the same
`POLYCALL_CORE_SOURCES` list every time; the shared target sets
`C_VISIBILITY_PRESET hidden` and `WINDOWS_EXPORT_ALL_SYMBOLS OFF`, so only
symbols explicitly marked `POLYCALL_API` are exported — not an
accidental "export everything" build. The CLI (`polycall_cli`) links
against the **static** target. The GNU Make build produces the POSIX
equivalents (`lib/libpolycall.a`, `lib/libpolycall.so`) plus a Windows
import library (`libpolycall.dll.a`) when cross-building for Windows via
MinGW.

## Public headers

Everything under `include/`, `polycall_`/`POLYCALL_` prefixed:

| Header | Covers |
| --- | --- |
| `polycall.h` | version query, `polycall_init_with_config`/`polycall_cleanup` |
| `polycall_export.h` | `POLYCALL_API`/`POLYCALL_CALL` linkage macros, the ABI version triplet |
| `polycall_cli.h` | `polycall_cli_main`, the process exit-code enum |
| `polycall_runtime.h` | the `polycall_rpc` v1 runtime: create/destroy/register, `start --load` plugin ABI, `polycall_runtime_serve` |
| `polycall_config2.h` | the native schema-v2 config model |
| `polycall_provider.h` | the C/Node/Python config provider contract |
| `polycall_telemetry.h` | GUID/timestamp generation and the JSONL event sink |

`POLYCALL_API`/`POLYCALL_LOCAL`/`POLYCALL_CALL` (`polycall_export.h`)
control DLL import/export and calling convention for every public
symbol — define `POLYCALL_BUILD_SHARED` to build the DLL,
`POLYCALL_USE_SHARED` to consume it; define neither for static linkage
(the default).

## The plugin ABI

`start --load PATH` loads a shared library exporting exactly one symbol:

```c
int polycall_ops_register(polycall_runtime_t *rt, uint32_t abi_major);
```

`abi_major` is the host's `POLYCALL_RUNTIME_ABI_VERSION`; the plugin must
compare it against its own and refuse to register on a mismatch
(`POLYCALL_PLUGIN_ABI_MISMATCH`), which the loader reports distinctly
from any other load failure. Only the exact path given is loaded — no
directory scan — and an unloadable library, missing symbol, or ABI
mismatch fails `start` before it binds anything (exit 4). See
[docs/PLUGINS.md](../docs/PLUGINS.md) for the full contract, and
`tests/fixtures/providers/c-provider/example_provider.c` /
`examples/ledger/ledger_plugin.c` for two real, built, tested examples.

## Linking a consumer

```sh
cc -std=c11 -Iinclude -c consumer.c -o consumer.o
cc consumer.o -Lbuild/lib -lpolycall -o consumer   # or polycall-static on MSVC
```

`tests/run_all.sh` builds and runs three independent consumers on every
CI run as a real link-compatibility check, not just a compile check:
static linkage, import-library/shared linkage, and explicit
`dlopen`/`LoadLibrary` — see `tests/consumers/`.

## Version

A single source of truth: `POLYCALL_ABI_VERSION_MAJOR`/`MINOR`/`PATCH` in
`include/polycall_export.h`. `CMakeLists.txt` parses these three macros
directly out of the header with a regex to set the CMake project
version (so `SOVERSION`/`VERSION` on the shared library can never drift
from the header), and `src/polycall.c` derives its reported version
string from the same macros rather than a second hardcoded literal.
`polycall --version` / `polycall_get_version()` report it.
