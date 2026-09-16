#!/usr/bin/env python3
"""PolyCall Python provider entry point.

    python run.py <path-to-config-module>

Loads an explicitly named config module (never scans directories), prints
exactly one canonical JSON envelope on stdout, exits 0. Any error: message on
stderr, exit 1. This is the process the C sub-process host spawns.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from polycall_provider import ProviderError, emit_from_module  # noqa: E402


def main() -> int:
    if len(sys.argv) != 2:
        sys.stderr.write("usage: python run.py <config-module>\n")
        return 2
    try:
        emit_from_module(os.path.abspath(sys.argv[1]))
    except ProviderError as e:
        sys.stderr.write(f"polycall-python-provider: {e}\n")
        return 1
    except Exception as e:  # noqa: BLE001
        sys.stderr.write(f"polycall-python-provider: {type(e).__name__}: {e}\n")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
