# Cross-language conformance harness

`tests/conformance/run.sh` (docs/TODO.md P2). Generalises
`tests/runtime/roundtrip.sh`: the same fixture requests go through every
available `polycall_rpc` v1 client, and each client's **raw response bytes**
are SHA-256-hashed and compared across languages -- a stronger claim than
"the exit code matched": the wire payload is byte-identical.

## Cases

| Case | Expected exit | What it proves |
| --- | --- | --- |
| success (`inventory.get widget-a`) | 0 | a built-in operation, and its response hash matches across every client |
| `input.invalid` (missing `item_id`) | 3 | malformed/missing input fails predictably |
| `operation.unknown` | 4 | an unregistered `service.operation` fails predictably |
| `deadline.exceeded` (`debug.sleep`) | 6 | a deadline is enforced, not just accepted |
| loaded-plugin op (`demo.greet`) | 0 | a `run --load`-ed operation (P1) is reachable identically, not just built-ins |
| no runtime (`127.0.0.1:1`) | 5 | a missing runtime is a failure, never treated as a pass |
| `auth.denied` (control channel, wrong `stop` token) | 7 | authenticated control actions reject a bad token |
| non-idempotent, called twice independently | -- | structural: every client performs exactly one connect+send+receive per call (verified by reading `CallRaw` / `call` / `client.call` in each language -- no retry loop exists to trigger) |

## Clients exercised

The harness builds/discovers each client at run time and skips (NOT RUN, not
a failure) whatever toolchain is absent on the host it runs on:

| Client | Path | Windows (this repo's host) | Linux (WSL2, this repo's host) |
| --- | --- | --- | --- |
| C | the CLI itself | run | run |
| Node.js | `bindings/node-client/polycall_call.mjs` | run | run |
| Python | `bindings/python-client/polycall_call.py` | NOT RUN (no interpreter) | run |
| Go | `bindings/go-polycall/rpcv1/cmd/polycall-call` | run | NOT RUN (no `go`) |
| Java | `bindings/java-polycall/rpcv1` (`PolyCallCall`) | run | NOT RUN (no `java`/`javac`) |
| Lua | `bindings/lua-polycall/rpcv1` | NOT RUN everywhere (no LuaSocket) | NOT RUN everywhere (no LuaSocket) |

Results on this pass: **Windows 33/33 passed** (C, Node, Go, Java; Python and
Lua NOT RUN); **Linux 25/25 passed** (C, Node, Python; Go, Java and Lua NOT
RUN). No host used for this pass has every toolchain at once; nothing here
claims a combination that wasn't actually run.

## A binding is "stable" only after it passes this in full

Per the P2 acceptance criterion, a binding earns a "stable" mark in its own
docs only once it has passed every case above on a host where its toolchain
is present. As of this pass: **C, Node.js and Python are stable by this
definition** (each has passed every case on at least one host). Go and Java
have passed every case they could run (Windows) but have not been exercised
on Linux at all -- call them **verified-on-one-platform**, not yet "stable"
in the cross-platform sense, until a Linux Go/JDK run confirms the same.
Lua's wire framing is unit-verified (byte-identical to a Python reference
frame, see `bindings/lua-polycall/rpcv1/frame_test.lua`) but the harness
itself has never run for Lua -- **not stable**.

## Binding classification: bidirectional (square) vs. asymmetric driver (rectangle)

A **bidirectional ("square")** binding can both call into the runtime *and*
be called by it -- for example, hosting operations the runtime dispatches to,
the way a C plugin (`docs/PLUGINS.md`) does. An **asymmetric driver
("rectangle")** only calls out; nothing calls back into it.

Every binding covered by this harness -- C's own CLI, Node, Python, Go, Java,
and (once its framing is live-tested) Lua -- is currently an **asymmetric
driver**: each opens a connection, sends one request, reads one response, and
closes it. None of them host operations that the C runtime calls back into;
only the C plugin ABI (`polycall_ops_register`, in-process, same address
space) does that today. A language becomes "bidirectional" only if it grows
a way to receive dispatch from the runtime -- for example, a small
long-lived listener process registering itself and answering `describe`-style
calls -- which is a separate, larger piece of design work than a client,
not attempted here.
