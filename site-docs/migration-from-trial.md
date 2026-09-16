# Historical migration notes

This page tracks real, dated changes to this repository that make older
material — an earlier README, a previous build, an earlier version of
this documentation site itself — inaccurate if taken at face value.

## The `bindings/` directory was removed

An earlier revision of this repository had a top-level `bindings/`
directory with one subdirectory per language (`go-polycall`,
`java-polycall`, `lua-polycall`, `cbl-polycall`, `node-polycall`,
`pypolycall`, `zig-polycall`, plus provider/client/plugin fixtures used
by this repository's own build and tests).

- Parts that were genuinely load-bearing — the example C provider, the
  Node/Python config providers, and the Node/Python/Go/Java RPC v1
  reference clients — were relocated, not deleted: see
  `tests/fixtures/providers/` and `tools/rpc-clients/`.
- Everything else (the standalone per-language SDK scaffolding with no
  working example, referenced by nothing in the build or tests) was
  removed outright. Installable client packages for those languages are
  published separately under the `obinexus` npm/PyPI/Maven namespaces,
  not vendored in this repository.
- The CLI's own `bindings list` command was removed along with it —
  reporting "binding support" was reporting information the CLI doesn't
  actually track; see [Language integration](language-integration.md).

If you're looking for `bindings/<lang>/...` and it isn't there, this is
why.

## This documentation site was rewritten against the wrong project

The version of `site-docs/` that shipped before this rewrite — including
its `cli-reference.md`, `polycallfile-reference.md`, and
`core-concepts.md` — accurately described a *different, related*
repository (`obinexus/libpolycall`, its `runtime start`/`call invoke`
command grammar, its TOML-like Polycallfile schema, and its
`docs/standards/` LPV1 specification suite). None of that matches this
repository, `obinexus/polycall`: different command names, different
config grammar, different exit codes, no `docs/standards/` directory at
all. Every page under `site-docs/` was checked against this repository's
actual CLI, config loader, and wire protocol and corrected — if a page
still references something that doesn't exist here, that's a bug in this
correction, not intentional.

## `telemetry` is new

`polycall telemetry emit`/`show`/`status`, and automatic GUID + timestamp
correlated events from `start`/`call`, did not exist in earlier revisions.
See the [CLI reference](cli-reference.md#telemetry).
