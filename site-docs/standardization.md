# Standardization

!!! danger "The LPV1 ten-document standards suite does not exist in this repository"
    This page used to summarize a `docs/standards/` directory of ten
    normative specification documents (`01-identity-scope-and-source-consolidation.md`
    through `10-verification-migration-and-release.md`) with a
    requirement-ID matrix (`CFG-*`, `CLI-*`, `ABI-*`, ...). That material
    belongs to a different repository, `obinexus/libpolycall`. Nothing
    under that path exists in `obinexus/polycall`, and none of those
    requirement IDs are referenced anywhere in this codebase.

## What this repository actually tracks instead

There is no separate normative-spec suite; the contract lives directly
alongside the code it describes, staged and dated:

| Document | Covers |
| --- | --- |
| [docs/CLI.md](../docs/CLI.md) | the full CLI contract: commands, exit codes, `--format json`, by delivery stage |
| [docs/RPC.md](../docs/RPC.md) | the `polycall_rpc` v1 wire byte layout and exit-code map |
| [docs/NATIVE_CONFIG.md](../docs/NATIVE_CONFIG.md) | the schema-v2 configuration model and provider contract |
| [docs/PLUGINS.md](../docs/PLUGINS.md) | the operation-plugin ABI (`run --load`) |
| [docs/CONCURRENCY.md](../docs/CONCURRENCY.md) | the one-thread-per-connection runtime model |
| [docs/CONFORMANCE.md](../docs/CONFORMANCE.md) | the cross-language RPC v1 equivalence results, per client, per host |
| [docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) | the broader OBINexus project context this repository sits inside |
| [docs/CHANGELOG.md](../docs/CHANGELOG.md) | dated, versioned entries — including what shipped, not just what's planned |

`docs/CLI.md`'s own status line is the closest equivalent to a
conformance summary this repository has: it names exactly which stage
(1–3) each command reached and points at the test that proves it
(`tests/cli/contract.sh`, `tests/config/equivalence.sh`,
`tests/conformance/run.sh`, ...) — read that first if you're looking for
"how much of the contract is actually implemented."
