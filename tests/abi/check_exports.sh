#!/bin/sh
# The shared library must export exactly the documented public ABI
# (abi/polycall.symbols): nothing internal leaks, nothing public is lost.
# Usage: check_exports.sh <libpolycall.so> <expected-symbols-file>
set -u
LIB=${1:?shared library}
WANT=${2:?symbol list}
if ! command -v nm >/dev/null 2>&1; then
  echo "NOT RUN  export check (nm not available)"; exit 77
fi
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
nm -D --defined-only "$LIB" | awk '$2 ~ /^[TDBR]$/ {print $3}' | sed 's/@.*//' | sort -u >"$TMP/got"
sort -u "$WANT" | tr -d '\r' >"$TMP/want"
if diff -u "$TMP/want" "$TMP/got"; then
  echo "PASS  $(wc -l <"$TMP/got" | tr -d ' ') exported symbols match abi/polycall.symbols"
  exit 0
fi
echo "FAIL  exported symbols differ from abi/polycall.symbols (diff above: - missing, + unexpected)"
exit 1
