# Runtime concurrency (P4)

Scope document written first, per `docs/TODO.md`, before touching
`src/runtime/runtime.c`. The runtime was single-threaded (one connection at
a time; see the old note in `docs/RPC.md`); this describes what changes and
what deliberately does not.

## Threading model

One thread per connection. `polycall_runtime_serve()`'s accept loop stays on
a single (the caller's) thread and keeps its existing 200 ms `select()` poll
on the listening socket unchanged; the only difference is that instead of
calling `handle_client()` inline and blocking until that client disconnects,
it hands the accepted socket to a new, detached thread
(`_beginthreadex` on Windows, `pthread_create` elsewhere) and immediately
loops back to `accept()` again. `handle_client()` itself, and everything it
calls (`dispatch_request`, `control_reply`, every `polycall_op_fn`), is
unchanged code, now just reached from more than one thread at once.

Concurrency is bounded: `RT_MAX_CONNS` (64) caps how many connections may be
live simultaneously. A connection accepted beyond the cap is closed
immediately, before any frame is read -- from the client's point of view
that's a connection failure (maps to exit code 5, `transport`, via the
existing central exit-code contract), not a hang or a silently queued
connection. 64 is comfortably above anything this project's demos or test
suite drive concurrently; it exists to bound thread/memory use, not to model
a specific production capacity.

## What's shared, and what isn't

- **The operation registry** (`rt->ops`, `rt->op_count`): populated once,
  before `polycall_runtime_serve()` is ever called -- built-ins at
  `polycall_runtime_create()`, then every `--load` plugin registers via
  `polycall_runtime_load_plugin()` inside `polycall_cmd_run()`, all *before*
  `serve()` is invoked -- and never mutated again (there is no "hot load a
  plugin while serving" path anywhere in the CLI). Concurrent handler
  threads only ever *read* it. No lock needed.
- **`rt->plugin_handles`/`plugin_count`**: same shape -- write-once before
  `serve()`, read only by `unload_plugins()` from
  `polycall_runtime_destroy()`, which (see Shutdown semantics below) never
  runs until every handler thread has already exited. No lock needed.
- **`g_stop`**: unchanged type and purpose (a process-wide `sig_atomic_t`
  flag). Before P4 only the signal handler wrote it; now a handler thread
  also sets it when it processes a `{"action":"shutdown"}` control message.
  Both writers only ever set it 0->1, never clear it back, so concurrent
  writes are harmless -- there's nothing to race.
- **`g_active_conns` + `g_conn_mutex`**: the only genuinely new shared
  mutable state this change introduces, and it exists purely for
  bookkeeping -- enforcing `RT_MAX_CONNS` and letting `serve()` know when
  every connection thread has actually finished. A small mutex around a
  counter; nothing more.
- **Whatever an operation plugin touches is the plugin's own business.** The
  runtime does not serialize calls into `polycall_op_fn` handlers -- two
  clients can be inside two different (or the same) operation's handler at
  the same time. `inventory.get`, `debug.echo` and `debug.sleep` touch no
  shared mutable state and needed no changes. `examples/ledger/ledger_plugin.c`
  *does* mutate shared state (its in-memory account table) and picked up its
  own mutex for exactly that reason -- see below.

## Per-connection deadlines

Each REQUEST frame already carries its own `deadline_ms` (`docs/RPC.md`);
that was always interpreted independently per request and still is.
Concurrency doesn't add, and doesn't need, any global deadline clock: one
client's slow request (e.g. a long `debug.sleep`) cannot delay another
client's deadline enforcement, because they're never on the same thread.

Separately, each connection thread now polls for shutdown while idle: before
reading a new frame it waits (up to 200 ms at a time, matching the accept
loop's own interval) for at least one byte to be available, and only then
calls the existing, unchanged, blocking `pcr_recv(c, &f, 0)` to read that
frame to completion. This is a *connection* idle check, not a per-frame
timeout -- once any byte of a frame has arrived, `pcr_recv` blocks through
to the end of that frame exactly as it always has, so a frame that
legitimately arrives slowly (a large payload trickling in) is never
abandoned mid-read. What it fixes: before P4, a client that opened a
connection and then sent nothing at all could block that connection (and,
pre-concurrency, the *entire* server) from ever noticing a shutdown request.
After P4 an idle connection's thread notices within ~200 ms; a connection
mid-request still finishes that request first (see below).

## Shutdown semantics

- The signal handler is untouched: `on_signal()` still only calls
  `polycall_runtime_request_stop()` (sets `g_stop`). No cleanup and no
  thread work happens in the handler, before or after P4.
- A `{"action":"shutdown"}` control message (with the correct
  `--auth-token`) also just sets `g_stop` -- unchanged behavior, now simply
  reachable from whichever connection thread happened to receive it instead
  of always the single serving thread.
- The accept loop notices `g_stop` within 200 ms (unchanged) and stops
  accepting new connections.
- **New in P4:** `polycall_runtime_serve()` does not return until every
  in-flight connection thread has actually finished -- polled via
  `g_active_conns` under `g_conn_mutex`, 50 ms granularity. This matters
  because the caller, `polycall_cmd_run()`, calls `polycall_runtime_destroy(rt)`
  the instant `serve()` returns; a handler thread still touching `rt` after
  that would be a use-after-free. Before P4 this was automatic (the single
  serving thread literally could not return while inside `handle_client()`);
  after P4 it has to be explicit, so it now is.
- Shutdown is graceful, not forced: an idle connection thread exits within
  ~200 ms of noticing `g_stop`; a connection thread in the middle of a
  request finishes that request (and sends its response) before checking
  `g_stop` and exiting. In-flight work drains; it is never severed
  mid-response.

## What did not change

- The wire format (`docs/RPC.md`'s 16-byte header + JSON payload) is
  untouched -- concurrency is invisible on the wire, exactly as scoped.
- Every operation handler signature (`polycall_op_fn`), the plugin ABI
  (`docs/PLUGINS.md`), and the CLI contract (exit codes, flags) are
  untouched.
- `polycall call` still performs exactly one round trip per invocation and
  never retries; concurrency on the runtime side has no bearing on that.

## Verification

`examples/ledger/demo.sh` gained a concurrent phase: `N` simultaneous
`ledger.transfer` calls (default `N=8`, override with `LEDGER_CONCURRENCY`)
from the C CLI, launched as background client processes so they genuinely
overlap in the runtime rather than running one after another, followed by a
balance check that the books still balance (sum of all account balances is
conserved) and match the arithmetic sum of every transfer's amount --
`docs/TODO.md`'s P4 acceptance criterion. The rest of the demo, and the
existing full test suite, run unchanged afterward on the same runtime
process to confirm nothing about single-connection behavior regressed.
