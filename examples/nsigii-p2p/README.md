# NSIGII P2P on Polycall — two independent processes, no broker

```
┌────────────────────────┐   polycall-peer/1 (HTTP)   ┌────────────────────────┐
│ Go Alpha    :9001      │ ◄────────────────────────► │ Python Beta   :9002    │
│ cgo → libpolycall      │                            │ ctypes → libpolycall   │
│ own registry + inbox   │                            │ (or pure-Python        │
└────────────────────────┘                            │  HTTP fallback)        │
                                                      └────────────────────────┘
```

Each process owns one Polycall peer node. FFI calls stay inside the process
that made them; only the transport connects the two. See
[docs/NSIGII_ADAPTATION.md](../../docs/NSIGII_ADAPTATION.md) for how this
maps to the original reference (preserved in `reference/`) and which of
its defects are fixed where.

## Run

```sh
cmake -S . -B build -DBUILD_TESTING=OFF && cmake --build build
cmake --install build --prefix "$HOME/.local/polycall"
export POLYCALL_DEV_TOKEN=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')
examples/nsigii-p2p/run.sh "$HOME/.local/polycall"
```

Both nodes read commands on stdin and print JSON events on stdout:

| Command | Event |
| --- | --- |
| `register <id> <host:port>` | `{"event":"registered",...}` |
| `send <peer> <message-id> <base64>` | `{"event":"sent","status":"POLYCALL_OK"\|"POLYCALL_E_..."}` |
| `peers` / `health` | `{"event":"peers"...}` / `{"event":"health"...}` |
| (incoming) | `{"event":"message","from":..,"id":..,"payload_b64":..}` |
| `quit` | `{"event":"stopped"}` |

Beta options: `--backend auto|native|python` (`auto` falls back to the pure
Python node when libpolycall is unusable and prints the reason).

## Test

`tests/peer/nsigii_test.py` (ctest `polycall_nsigii_two_process`) runs both
processes and checks, at the receiving process, the exact payload bytes,
the sender id and the message id, in both directions, plus registry
ownership, duplicates, malformed input, either peer killed and rejoining,
partition, delayed delivery with idempotent retry, and the HTTP fallback
with a missing or foreign library.

Windows: cgo needs a current MinGW-w64 GCC (MSYS2 UCRT64). TDM-GCC 10's
binutils produce an executable Windows refuses to load.
