# `polycall daemon`

One long-running Polycall runtime (`polycall_rpc` v1, docs/RPC.md) per
Polycallfile, managed by the same `polycall` executable as everything else.
There is no separate daemon binary.

```
polycall daemon run     [options] [Polycallfile]   # foreground
polycall daemon start   [options] [Polycallfile]   # background; returns once healthy
polycall daemon status  [options] [Polycallfile]
polycall daemon stop    [options] [--force] [Polycallfile]
polycall daemon restart [options] [Polycallfile]
```

The Polycallfile defaults to `./Polycallfile`. Every subcommand resolves the
same settings, so `start`, `status` and `stop` given the same file always
address the same daemon.

## Settings

| Polycallfile key | Flag | Default |
| --- | --- | --- |
| `daemon_endpoint` | `--endpoint`, `-e` | `127.0.0.1:8084` |
| `daemon_state_dir` | `--state-dir` | `.polycall` next to the Polycallfile |
| `daemon_log_file` | `--log-file` | `start`: `<state dir>/daemon.log`; `run`: stderr unless `--log-file` |
| `daemon_max_connections` | `--max-connections` | 64 (1–1024) |
| `daemon_drain_timeout_ms` | `--drain-timeout-ms` | 5000 |
| `daemon_io_timeout_ms` | `--io-timeout-ms` | 10000 |
| `auth_token_env` | `--auth-token-env` | `POLYCALL_DEV_TOKEN` |
| — | `--log-format json\|text` | `json` |
| — | `--load <plugin>` (repeatable, max 16) | none |
| — | `--timeout-ms`, `-t` (start/stop wait) | start 15000; stop drain + io + 5000 |

Relative paths in the Polycallfile are relative to the Polycallfile, not
to the current directory.

**Authentication.** `auth_token_env` names an environment variable; the
daemon reads the token from that variable. The token is never accepted on
the command line, is never written to logs or state files, and `start`
passes it to the background process only through its environment (the
test suite checks all three). A daemon on a non-loopback address refuses to
start without a token (exit 3). `stop` must present the same token; a
wrong one is refused with exit 7 and the daemon keeps running.

**TLS.** `tls_enabled=true` is refused with exit 4. The daemon does not
silently serve plaintext in place of TLS.

## State and single instance

The state directory holds:

- `daemon.lock`: an OS file lock (POSIX `fcntl`, Windows exclusive
  `CreateFile`), held for the daemon's whole lifetime.
- `daemon.json`: pid, endpoint, start time, version, Polycallfile, and
  whether auth is enabled. Written atomically.
- `daemon.pid`: written atomically.

The OS releases the lock however the process ends, so a daemon killed with
`SIGKILL` / `TerminateProcess` never blocks the next start. The next `run`
finds the leftover `daemon.json`, logs `daemon.recovered_stale_state` and
replaces it. A second `run` for the same Polycallfile is refused (exit 1,
`daemon.already_running`). A repeated `start` reports the running daemon
and exits 0 without spawning another.

## Lifecycle

- **start**:
  - spawns `polycall daemon run ... --log-file <log>` detached
  - returns only after the child has written its own state file **and**
    answered a health request
  - if the child exits during startup, `start` prints the end of its log
    and returns the child's exit code
  - if it is not ready in time, `start` kills it and exits 6
- **stop**:
  - sends an authenticated `shutdown` control request
  - waits for the lock to be released (bounded by `--timeout-ms`)
  - `--force` terminates the process after that wait
- **run**:
  - `SIGTERM`/`SIGINT` (Ctrl-C / console close on Windows) begin a graceful
    shutdown
  - stops accepting new connections
  - in-flight requests complete within `daemon_drain_timeout_ms`, then the
    remaining connections are closed
  - `SIGHUP` is ignored: there is no configuration reload

**Backpressure.** Past `daemon_max_connections`, a new client gets a
`server.busy` reply (CLI exit 5) and the connection is closed gracefully.
Capacity returns as calls finish. A client that disconnects mid-call does
not affect other calls.

**Logs** are one JSON object per line: `ts`, `level`, `event`, `message`,
plus fields. Events include `daemon.starting`, `daemon.ready`,
`daemon.recovered_stale_state`, `daemon.stopping`, `daemon.stopped`,
`bind.address_in_use`, `bind.permission` and `daemon.plugin_loaded`.

## Exit codes

| Code | Meaning (daemon) |
| --- | --- |
| 0 | ok; `stop` when not running; repeated `start` |
| 1 | already running (`run`); `status`: running but not answering health; state dir / lock not writable; bind permission |
| 2 | usage |
| 3 | missing or invalid Polycallfile; non-loopback without a token |
| 4 | `tls_enabled=true`; plugin load or ABI mismatch |
| 5 | address in use; `status`: not running; `server.busy` |
| 6 | `start` not ready in time; `stop` did not finish (without `--force`) |
| 7 | `stop` with a wrong token |

## Linux: systemd

`-DPOLYCALL_INSTALL_SYSTEMD_UNIT=ON` installs `polycall.service`, which the
Debian and Arch packages include:

- `Type=notify`: readiness comes from `sd_notify`, without a libsystemd
  dependency.
- Runs as `DynamicUser=yes`: no root and no capabilities, with
  `ProtectSystem=strict` and the hardening options in the unit.
- Configuration: `/etc/polycall/Polycallfile`. State: `/run/polycall`.
  Logs: the journal.
- Supply the token with `systemctl edit polycall`:

  ```
  [Service]
  EnvironmentFile=/etc/polycall/polycall.env
  ```

  The file should be mode 0600, root-owned, and contain
  `POLYCALL_DEV_TOKEN=...`. Do not put the secret in the unit or on the
  command line.

For a user-level (non-root) daemon without systemd, `polycall daemon start`
from a login script works the same way.

## Windows

`polycall daemon start` creates a **detached background process**: no
console, its own process group, and no inherited handles except the log
file. It is **not a Windows Service**: there is no Service Control Manager
integration, no `sc create`, and nothing starts it at boot. The lifecycle
is the one above:

- `start`, `status`, `stop [--force]` and `restart` are run by the user.
- `stop` uses the same authenticated control request as on Linux;
  `--force` uses `TerminateProcess`.
- The single-instance lock is an exclusive `CreateFile`, which Windows
  releases when the process ends.
- `SO_EXCLUSIVEADDRUSE` makes a port already held by another process fail
  with `address in use`, instead of being silently shared.

To start it at logon, create a Task Scheduler task (trigger "At log on",
"Run only when user is logged on") that runs
`polycall daemon start C:\path\to\Polycallfile`, with the token variable
set in the user's environment. Running it as a service through a
third-party wrapper is possible but not tested or supported here.

## Tests

`tests/daemon/daemon.sh` (ctest `polycall_daemon`) runs on Linux, Windows
MSVC and MSYS2 UCRT64. It covers:

- missing and invalid Polycallfiles, TLS refused, non-loopback bind
  without a token
- start, status, a served call, and idempotent repeated start
- refusal of a second instance, address in use
- the token absent from logs, state and the command line
- stop with the wrong token
- a graceful stop while a request is in flight (it completes)
- restart, backpressure and recovery, a client vanishing mid-call
- `kill -9` and stale-state recovery
- an unwritable state directory
