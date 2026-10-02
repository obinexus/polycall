#!/bin/sh
# NSIGII P2P on Polycall: Go Alpha (:9001) + Python Beta (:9002).
#
# Fixes over ../reference/run.sh (docs/NSIGII_ADAPTATION.md):
#   - stops at the first failed step (set -eu), never "continues anyway"
#   - builds the Alpha BINARY and runs it directly, so the PID we record is
#     the server's, not that of a `go run` wrapper
#   - waits for each node to report it is listening before going on
#   - on exit (normal, Ctrl-C, or failure) stops exactly the processes it
#     started, and nothing else
#
# Usage: examples/nsigii-p2p/run.sh [POLYCALL_PREFIX]
#   POLYCALL_PREFIX: an installed polycall (default /usr or /usr/local);
#   the shared token comes from $POLYCALL_DEV_TOKEN (generated if unset).
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
PREFIX=${1:-${POLYCALL_PREFIX:-}}
if [ -z "$PREFIX" ]; then
  for p in /usr/local /usr; do [ -f "$p/include/polycall/polycall.h" ] && PREFIX=$p && break; done
fi
[ -n "$PREFIX" ] && [ -f "$PREFIX/include/polycall/polycall.h" ] || {
  echo "run.sh: no installed polycall found; pass its prefix (cmake --install ... --prefix P)" >&2; exit 2; }
command -v go >/dev/null || { echo "run.sh: go is required for the Alpha node" >&2; exit 2; }
PY=$(command -v python3 || command -v python) || { echo "run.sh: python3 is required" >&2; exit 2; }
LIBDIR=$PREFIX/lib
[ -d "$PREFIX/lib64" ] && LIBDIR=$PREFIX/lib64
: "${POLYCALL_DEV_TOKEN:=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')}"
export POLYCALL_DEV_TOKEN

WORK=$(mktemp -d)
ALPHA_PID=""; BETA_PID=""
cleanup() {
  for pid in $ALPHA_PID $BETA_PID; do kill "$pid" 2>/dev/null || true; done
  for pid in $ALPHA_PID $BETA_PID; do wait "$pid" 2>/dev/null || true; done
  rm -rf "$WORK"
}
trap cleanup EXIT INT TERM

echo "== building Go Alpha against $PREFIX"
( cd "$HERE/alpha" && CGO_ENABLED=1 CGO_CFLAGS="-I$PREFIX/include/polycall" \
    CGO_LDFLAGS="-L$LIBDIR -Wl,-rpath,$LIBDIR" go build -o "$WORK/alpha" . )

mkfifo "$WORK/alpha.in" "$WORK/beta.in"
"$WORK/alpha" -listen 127.0.0.1:9001 -peer python-node-beta=127.0.0.1:9002 \
    <"$WORK/alpha.in" >"$WORK/alpha.out" 2>&1 &
ALPHA_PID=$!
exec 3>"$WORK/alpha.in"
POLYCALL_LIBRARY=${POLYCALL_LIBRARY:-$LIBDIR/libpolycall.so.1} "$PY" "$HERE/beta/node.py" \
    --listen 127.0.0.1:9002 --peer go-node-alpha=127.0.0.1:9001 \
    <"$WORK/beta.in" >"$WORK/beta.out" 2>&1 &
BETA_PID=$!
exec 4>"$WORK/beta.in"

wait_listening() { i=0; until grep -q '"event":"listening"' "$1"; do
  i=$((i+1)); [ $i -lt 100 ] || { echo "run.sh: $1 never started:"; cat "$1"; exit 1; }; sleep 0.1; done; }
wait_listening "$WORK/alpha.out"
wait_listening "$WORK/beta.out"
echo "== Alpha (pid $ALPHA_PID) and Beta (pid $BETA_PID) are listening"

echo "send python-node-beta hello-1 $(printf 'hello beta from alpha' | base64)" >&3
echo "send go-node-alpha hello-2 $(printf 'hello alpha from beta' | base64)" >&4
sleep 1
echo "== Alpha events:"; cat "$WORK/alpha.out"
echo "== Beta events:";  cat "$WORK/beta.out"
echo "Running. Ctrl-C stops exactly these two processes."
wait "$ALPHA_PID" "$BETA_PID"
