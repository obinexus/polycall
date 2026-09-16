# Contributing and releases

## Building and testing a change

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

or, on any POSIX shell:

```sh
make all
sh tests/run_all.sh
```

See [Getting started](getting-started.md) for the full toolchain list —
CI (`.github/workflows/ci.yml`) exercises both build systems, on Linux,
macOS, and Windows (MSVC and MSYS2 GCC), on every push.

Before proposing a CLI change: `src/cli/cli_commands.c` is the single
command registry `polycall --help` and [CLI reference](cli-reference.md)
both derive from. A new command goes through a registry change with its
behavior, exit codes, and a `tests/cli/contract.sh` case together — not a
speculative entry that only partially works.

## Documentation

This site's source is `site-docs/` (the MkDocs `docs_dir`, configured by
`mkdocs.yml` at the repository root), deliberately separate from the
pre-existing `docs/` directory — the deeper technical reference this site
links out to, plus material from the wider OBINexus project this
repository sits inside.

```sh
pip install mkdocs
mkdocs serve      # live-reloading preview at http://127.0.0.1:8000
mkdocs build      # what the Pages workflow runs
```

If you edit or add a page, update `nav:` in `mkdocs.yml` and keep
filenames lowercase kebab-case. A claim of "Verified" on this site should
mean someone actually ran the command against a real build — if you find
one that no longer holds, correct it in the same change that changes the
behavior.

## Compatibility

The ABI version (`include/polycall_export.h`), the wire protocol version
(`polycall_rpc` v1), and the config schema version
(`polycall_config2.h`) are three independent contracts — see
[C library and ABI](c-abi-and-linking.md#version). None of the three is
frozen by a published external spec yet; treat a version bump in any of
them as a real compatibility question, not a cosmetic string change.

## License

This project uses OBINexus's own "Use It, Respect It" license (see
[`LICENSE`](https://github.com/obinexus/polycall/blob/main/LICENSE) in
the repository root) — not a standard OSI template. Read it directly
rather than assuming MIT/Apache/BSD terms.

## About

<img src="assets/founder.png" alt="Nnamdi Michael Okpala" width="120" style="float:left; margin: 0 1.5em 1em 0; border-radius: 4px;">

PolyCall is an OBINexus project led by **Nnamdi Michael Okpala**.

<div style="clear:both;"></div>
