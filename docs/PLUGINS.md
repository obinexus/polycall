# Operation plugins (`polycall start --load`)

Status: **implemented**. Completes the third step of the canonical binding
workflow: write a language-specific C binding -> compile to a shared library
with the runtime's export macros -> register it with the C runtime, with no
core rebuild.

## Contract

A plugin is a shared library (`.so` / `.dll` / `.dylib`) exporting exactly one
symbol:

```c
#include "polycall_runtime.h"

int polycall_ops_register(polycall_runtime_t *rt, uint32_t abi_major);
```

`abi_major` is the **host's** `POLYCALL_RUNTIME_ABI_VERSION` at the time the
runtime was built. The plugin must compare it against the value it was
itself compiled against and refuse to register on a mismatch:

```c
if (abi_major != POLYCALL_RUNTIME_ABI_VERSION) {
    return POLYCALL_PLUGIN_ABI_MISMATCH;
}
```

Inside a compatible call, register one or more operations with the existing
`polycall_runtime_register()` (the same function the built-ins use):

```c
static const polycall_op_desc_t desc = {
    "demo", "greet", "greet a name",
    "{\"name\":\"string\"}", "{\"message\":\"string\"}",
    /* idempotent */ true, op_demo_greet, NULL
};

int polycall_ops_register(polycall_runtime_t *rt, uint32_t abi_major)
{
    if (abi_major != POLYCALL_RUNTIME_ABI_VERSION) return POLYCALL_PLUGIN_ABI_MISMATCH;
    if (polycall_runtime_register(rt, &desc) != 0) return POLYCALL_PLUGIN_ERROR;
    return POLYCALL_PLUGIN_OK;
}
```

Return values:

| Value | Meaning |
| --- | --- |
| `POLYCALL_PLUGIN_OK` (0) | registered successfully |
| `POLYCALL_PLUGIN_ABI_MISMATCH` (1) | the host's ABI major is not what the plugin expects; reported distinctly |
| `POLYCALL_PLUGIN_ERROR` (2) | any other registration failure |

A non-zero return rejects the **whole** plugin: none of its operations are
served, and `polycall start --load` exits 4 before binding anything.

## Duplicate `service.operation`: always refused, never "last wins"

`polycall_runtime_register()` refuses a `service.operation` that is already
registered -- whether the collision is with a built-in (`inventory.get`,
`debug.echo`, `debug.sleep`) or with an operation a previously-loaded plugin
already added. The registering call returns `-1` and adds nothing; if a
plugin's `polycall_ops_register` does not itself check that return value and
propagate a non-zero status, the collision is silently absorbed for that one
operation while the rest of the plugin's operations still register. Well-
behaved plugins should treat any `polycall_runtime_register()` failure as
fatal and return `POLYCALL_PLUGIN_ERROR`, which is what the fixture plugin
does. This is a fixed policy, not configurable per invocation.

## Loading

```sh
polycall start --endpoint 127.0.0.1:8080 \
  --load build/lib/fixture_ops_plugin.so \
  --load build/lib/another_plugin.so
```

`--load` is repeatable. Only the explicitly named path is loaded -- parent
directories are never scanned. Loading happens before the endpoint is bound;
an unloadable library, a missing `polycall_ops_register`, or an ABI mismatch
each fail `start` immediately with exit 4 (`POLYCALL_EXIT_UNSUPPORTED`), with no
socket ever opened. Loaded operations appear in `polycall status` (control
action `describe`) exactly like built-ins, with their schema hints and
`idempotent` flag, because `describe` reports whatever is in the shared
operation registry regardless of where it came from.

Libraries are unloaded only in `polycall_runtime_destroy()`, which runs after
`polycall_runtime_serve()` returns -- in normal execution, never from the
`SIGINT`/`SIGTERM` handler (which only sets a flag; see `docs/RPC.md`).

## Building a plugin

The fixture plugin (`tests/fixtures/plugins/fixture_ops_plugin.c`) is the
worked example; `make plugin-fixture` / the CMake `fixture_ops_plugin` and
`fixture_ops_plugin_bad_abi` targets build it both normally and with a
deliberately mismatched `FIXTURE_ABI_MAJOR=999` (exercising the ABI-mismatch
path from one source file). A plugin only needs to:

1. `#include "polycall_runtime.h"` for the types and `polycall_runtime_register`.
2. Export `polycall_ops_register` (`__declspec(dllexport)` on Windows,
   default visibility on ELF/Mach-O -- see the fixture for both).
3. Link against `libpolycall` (static or via the import library) so
   `polycall_runtime_register` resolves.

## Tests

- `tests/cli/contract.sh`: `start --load` with a missing value (exit 2) and a
  nonexistent path (exit 4) -- fast, no server needed.
- `tests/runtime/plugin.sh`: nonexistent path, a real library lacking
  `polycall_ops_register` (reuses `example_provider`), the ABI-mismatch
  build, then the real fixture -- loaded, `start` reports it, `demo.greet` is
  called from the C CLI and (when available) inline Node and Python clients
  with identical results, the built-in operations still work alongside it,
  `status`/`describe` lists `demo.greet` with a correctly-escaped schema hint
  and `idempotent:true`, and the runtime unloads cleanly on `stop`.
