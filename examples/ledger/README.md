# Flagship demo: ledger service

A tiny, deterministic ledger loaded into the runtime as an operation plugin
(`docs/PLUGINS.md`) -- no core rebuild -- and driven from every
`polycall_rpc` v1 client available on the host running it.

## Operations

| Operation | Idempotent | Input | Output |
| --- | --- | --- | --- |
| `ledger.balance` | yes | `{"account":"alice"}` | `{"account","balance"}` |
| `ledger.transfer` | **no** | `{"from":"alice","to":"bob","amount":30}` | `{"from","to","amount","from_balance","to_balance"}` |

Three fixed accounts: `alice=100`, `bob=50`, `carol=0`. This is a demo, not a
real ledger -- balances live in-process and reset every time the runtime
restarts.

## Run it

From a clean checkout, `make` plus one script:

```sh
make BUILD_DIR=build/demo CC=gcc all
sh examples/ledger/demo.sh build/demo gcc
```

or let the script build everything itself:

```sh
sh examples/ledger/demo.sh
```

It starts the runtime with `ledger_plugin` loaded, drives two independent
transfers (different amounts, so the output makes it obvious the second is
its own round trip and not a retried copy of the first -- `call` never
retries, docs/RPC.md), shows an insufficient-funds transfer being rejected
without moving anything, repeats a query/transfer from whichever of the
Node.js, Python and Go `polycall_rpc` v1 clients are on `PATH` (skipping, not
failing, on a missing toolchain -- see `docs/CONFORMANCE.md` for why), and
finally launches `LEDGER_CONCURRENCY` (default 8) simultaneous clients
against the same running runtime -- one round of concurrent `debug.sleep`
calls that only finish quickly if the runtime truly serves them in parallel,
and one round of concurrent `ledger.transfer` calls whose before/after
balances prove no update was lost (`docs/CONCURRENCY.md`, `docs/TODO.md` P4).

## Windows

The same script runs under Git-Bash / MSYS2 / WSL on Windows:

```sh
sh examples/ledger/demo.sh build/demo gcc
```

(On native PowerShell without a POSIX shell, build with
`./build-windows.ps1` first, then run the script from Git-Bash pointed at
that `BUILD_DIR`.)
