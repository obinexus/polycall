# PolyCall

<img src="assets/favicon.png" alt="PolyCall" width="96" style="float:right; margin: 0 0 1em 1em;">

**polycall** is a program-first cross-language runtime: one C CLI
(`polycall`) and one wire protocol (`polycall_rpc` v1), reached
identically from every language client. A caller sends a service name, an
operation name, and a JSON input; the runtime routes it to whichever
process registered that `service.operation` and returns a typed JSON
result or a structured error — regardless of which language implements
the operation.

```text
polycall [global-options] <command> [subcommand] [arguments]
```

A concrete example, exercised by this repository's own test suite: the
built-in `inventory.get` operation is reached identically from the C CLI
and from Node.js and Python reference clients (`tools/rpc-clients/`)
talking to the *same* running instance over TCP — not three separate
reimplementations.

## What's actually in this repository

!!! success "Verified"
    Built, run, and checked against a real `polycall` binary this session
    (`tests/cli/contract.sh`, `tests/runtime/roundtrip.sh`,
    `tests/conformance/run.sh`), or exercised directly at a terminal.

!!! warning "Registered, not yet implemented"
    The CLI recognizes the command and returns a definite, honest error
    (never a stub that pretends to succeed) — see the exit-code table in
    the [CLI reference](cli-reference.md).

| Area | Status |
| --- | --- |
| `help`, `version`, `doctor`, `--format json` | Verified |
| `config validate`/`config show` against a legacy `Polycallfile`, or a native C/Node/Python provider | Verified |
| `start`/`status`/`stop`/`call` over `polycall_rpc` v1, including `start --load PATH` operation plugins | Verified |
| `telemetry emit`/`show`/`status` (GUID + timestamp correlated events, auto-emitted by `start`/`call`) | Verified |
| Cross-language reference clients: Node.js, Python, Go, Java (`tools/rpc-clients/`) | Verified |
| `config migrate` (legacy → native envelope) | Verified |
| A packaged, installable per-language client SDK (npm, PyPI, Maven, ...) shipped from *this* repository | Not implemented here — published as separate packages under the `obinexus` org |

## Where to go next

- New to the project and want to build it: [Getting started](getting-started.md).
- Want to see it work end to end: [Your first cross-language call](first-python-call.md).
- Looking up a specific command, flag, or exit code: [CLI reference](cli-reference.md).
- Configuring a project: [Polycallfile reference](polycallfile-reference.md).
- Want the vocabulary before diving in: [Core concepts](core-concepts.md).
- Calling from Node, Python, Go, or Java today: [Language integration](language-integration.md).

Deeper technical reference (wire protocol byte layout, plugin ABI,
concurrency model, conformance results) lives at [docs/](../docs/) —
[start there](../docs/) for the full reference set.
