# Getting started

## Two independent build systems

!!! success "Verified this session"
    Both paths build the same CLI and pass the same test suite
    (`tests/run_all.sh`) — CI (`.github/workflows/ci.yml`) exercises both
    on every push, plus Windows MSYS2 GCC and macOS.

- **CMake**, cross-platform, the one to use on native Windows (MSVC):
  ```powershell
  .\build-windows.ps1 -Generator "Visual Studio 17 2022" -BuildType Release -Test
  ```
  or directly:
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
  cmake --build build --config Release --parallel
  ctest --test-dir build -C Release --output-on-failure
  ```
- **GNU Make**, POSIX-only (Linux, macOS, *BSD, or MSYS2/Git-Bash on
  Windows — never plain PowerShell/cmd.exe):
  ```sh
  make all
  make test
  ```

Both produce `bin/polycall` (`.exe` on Windows), a static library, and a
shared library. Neither build requires npm, a JVM, or a Python install —
the core is C11 only. Node/Python/Go/Java are only needed if you also want
to run the cross-language reference clients under `tools/rpc-clients/` or
their tests.

Windows link libraries (`ws2_32`) and POSIX's `-lpthread`/`-ldl` are added
automatically by whichever build system you use — you never pass them
yourself.

!!! warning "TLS is declared, not implemented"
    The native config schema accepts `tls.mode = "server"`/`"mutual"` and
    validates certificate/key references, but no TLS library is linked
    anywhere in this tree (`grep -r openssl src/` finds nothing) — the
    runtime always speaks plaintext `polycall_rpc` v1 today. Don't rely on
    a configured TLS mode to actually encrypt the socket.

## First run

```sh
bin/polycall --version
bin/polycall doctor
bin/polycall config validate Polycallfile
```

`doctor` is read-only: it never starts a service, opens a socket, or
evaluates a configuration provider. `config validate` against the
repository's own `Polycallfile` (the legacy `server LANG PORT:PORT` /
`key=value` format — see the [Polycallfile reference](polycallfile-reference.md))
should print `configuration is valid` and exit 0.

## Start the runtime and make a call

```sh
bin/polycall run --endpoint 127.0.0.1:0 --endpoint-file /tmp/ep &
sleep 1
EP=$(cat /tmp/ep)
bin/polycall call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}'
bin/polycall stop --endpoint "$EP"
```

Port `0` binds an ephemeral port; `--endpoint-file` is how a supervisor
(or this example) learns which one. `inventory.get` and `debug.sleep` are
the two built-in deterministic operations available with no plugin and no
provider — see [Core concepts](core-concepts.md) for what registers an
operation and [CLI reference](cli-reference.md) for the full `call`
contract and exit codes.

## Running the test suite

```sh
sh tests/run_all.sh          # everything: CLI contract, config equivalence,
                              # runtime roundtrip, plugin loader, conformance,
                              # microvm strict mode, three consumer link modes
sh tests/cli/contract.sh bin/polycall tests/fixtures   # CLI contract only
```

`tests/run_all.sh` is driven by the build systems (`make test` /
`ctest`), which export `BUILD_DIR`/`CC`/`EXE_EXT`/`SHARED_EXT` for it; run
by hand it falls back to sensible defaults.

## What's next

Continue to [CLI reference](cli-reference.md) for every command, or
[Polycallfile reference](polycallfile-reference.md) to configure your own
services.
