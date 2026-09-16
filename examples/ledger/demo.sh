#!/bin/sh
# Flagship demo (docs/TODO.md P3, P4): a ledger operation plugin, loaded into
# the runtime with no core rebuild, driven from every polycall_rpc v1 client
# available on this host, then from LEDGER_CONCURRENCY (default 8)
# simultaneous clients at once to prove the runtime's concurrency
# (docs/CONCURRENCY.md). From a clean checkout:
#
#   make BUILD_DIR=build/demo CC=gcc all
#   sh examples/ledger/demo.sh build/demo gcc
#
# (or just `sh examples/ledger/demo.sh`, which defaults to BUILD_DIR=build
# and CC=gcc and builds everything itself -- "make plus one script").
set -u
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BUILD_DIR=${1:-build}
CC=${2:-gcc}
SHEXT=${SHEXT:-}
if [ -z "$SHEXT" ]; then
  case "$(uname -s 2>/dev/null)" in
    Darwin) SHEXT=dylib ;;
    MINGW*|MSYS*|CYGWIN*) SHEXT=dll ;;
    *) SHEXT=so ;;
  esac
fi
EXE_EXT=""
case "$(uname -s 2>/dev/null)" in MINGW*|MSYS*|CYGWIN*) EXE_EXT=".exe" ;; esac

say() { printf '\n=== %s ===\n' "$1"; }
fail=0

cd "$ROOT" || exit 1

say "building (make BUILD_DIR=$BUILD_DIR CC=$CC all examples-ledger)"
make BUILD_DIR="$BUILD_DIR" CC="$CC" all examples-ledger || { echo "build failed"; exit 1; }

PC="$BUILD_DIR/bin/polycall$EXE_EXT"
PLUGIN="$BUILD_DIR/lib/ledger_plugin.$SHEXT"
[ -x "$PC" ] || { echo "missing $PC"; exit 1; }
[ -f "$PLUGIN" ] || { echo "missing $PLUGIN"; exit 1; }

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/ledger_demo.$$"); mkdir -p "$TMP"
trap 'rm -rf "$TMP"; [ -n "${RUNPID:-}" ] && kill "$RUNPID" 2>/dev/null' EXIT

say "starting the runtime with the ledger plugin loaded (no core rebuild)"
EPF="$TMP/endpoint"
"$PC" run --endpoint 127.0.0.1:0 --endpoint-file "$EPF" --load "$PLUGIN" \
  >"$TMP/run.log" 2>&1 &
RUNPID=$!
i=0
while [ ! -s "$EPF" ] && [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done
[ -s "$EPF" ] || { echo "runtime did not start"; cat "$TMP/run.log"; exit 1; }
EP=$(cat "$EPF")
echo "runtime up at $EP"

check() { # check LABEL EXPECTED_SUBSTRING ACTUAL
  if printf '%s' "$3" | grep -qF "$2"; then
    printf 'PASS  %s\n' "$1"
  else
    printf 'FAIL  %s (want to see %s, got: %s)\n' "$1" "$2" "$3"
    fail=1
  fi
}

call_c() { "$PC" call "$1" "$2" --endpoint "$EP" --input-value "$3"; }

say "starting balances (C CLI)"
out=$(call_c ledger balance '{"account":"alice"}'); echo "$out"
check "alice starts at 100" '"balance":100' "$out"
out=$(call_c ledger balance '{"account":"bob"}'); echo "$out"
check "bob starts at 50" '"balance":50' "$out"

say "transfer 1: alice -> bob, 30 (C CLI)"
out=$(call_c ledger transfer '{"from":"alice","to":"bob","amount":30}'); echo "$out"
check "transfer 1 result" '"from_balance":70,"to_balance":80' "$out"

say "transfer 2: a SECOND, independent alice -> bob transfer of a DIFFERENT amount (15) -- distinct numbers prove this is its own round trip, not a re-send of transfer 1's 30"
out=$(call_c ledger transfer '{"from":"alice","to":"bob","amount":15}'); echo "$out"
check "transfer 2 result (70-15=55, 80+15=95 -- not transfer 1's 70/80 again)" '"from_balance":55,"to_balance":95' "$out"

say "balances after both transfers"
out=$(call_c ledger balance '{"account":"alice"}'); echo "$out"
check "alice: 100 - 30 - 15 = 55" '"balance":55' "$out"
out=$(call_c ledger balance '{"account":"bob"}'); echo "$out"
check "bob: 50 + 30 + 15 = 95" '"balance":95' "$out"

say "insufficient funds is rejected, and the rejected attempt moved nothing"
out=$(call_c ledger transfer '{"from":"carol","to":"alice","amount":50}' 2>&1); echo "$out"
check "carol (balance 0) cannot send 50" "funds.insufficient" "$out"
out=$(call_c ledger balance '{"account":"carol"}'); echo "$out"
check "carol is still at 0 after the rejected transfer" '"balance":0' "$out"

say "the same ledger, driven from every polycall_rpc v1 client available on this host"
if command -v node >/dev/null 2>&1; then
  out=$(node "$ROOT/tools/rpc-clients/node/polycall_call.mjs" "$EP" ledger balance '{"account":"alice"}')
  echo "node:   $out"
  check "Node.js sees the same balance" '"balance":55' "$out"
else
  echo "NOT RUN  Node.js (node not on PATH)"
fi

PY=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
if [ -n "$PY" ]; then
  out=$("$PY" "$ROOT/tools/rpc-clients/python/polycall_call.py" "$EP" ledger balance '{"account":"bob"}')
  echo "python: $out"
  check "Python sees the same balance" '"balance":95' "$out"
else
  echo "NOT RUN  Python (no interpreter on PATH)"
fi

if command -v go >/dev/null 2>&1; then
  GO_BIN="$TMP/polycall-call$EXE_EXT"
  if ( cd "$ROOT/tools/rpc-clients/go" && go build -o "$GO_BIN" ./cmd/polycall-call ) >"$TMP/go_build.log" 2>&1; then
    out=$("$GO_BIN" "$EP" ledger transfer '{"from":"alice","to":"carol","amount":20}')
    echo "go:     $out"
    check "Go performs a real transfer (alice 55 -> 35, carol 0 -> 20)" '"from_balance":35,"to_balance":20' "$out"
  else
    echo "NOT RUN  Go (build failed, see $TMP/go_build.log)"
  fi
else
  echo "NOT RUN  Go (go not on PATH)"
fi

N=${LEDGER_CONCURRENCY:-8}
SLEEP_MS=500

say "P4: $N concurrent debug.sleep calls prove the runtime genuinely overlaps connections (docs/CONCURRENCY.md), not just that this script backgrounds them -- if it still served one at a time, $N x ${SLEEP_MS}ms would take ~$((N * SLEEP_MS / 1000))s"
t0=$(date +%s)
pids=""
i=0
while [ $i -lt $N ]; do
  call_c debug sleep "{\"ms\":$SLEEP_MS}" >"$TMP/sleep_$i.out" 2>&1 &
  pids="$pids $!"
  i=$((i + 1))
done
# wait only on the client PIDs just launched -- a bare `wait` would also
# block on $RUNPID, the runtime server itself, which never exits on its own
wait $pids
t1=$(date +%s)
elapsed=$((t1 - t0))
echo "elapsed: ${elapsed}s"
if [ "$elapsed" -le 2 ]; then
  printf 'PASS  %s\n' "$N concurrent sleeps overlapped (${elapsed}s, not ~$((N * SLEEP_MS / 1000))s serial)"
else
  printf 'FAIL  %s\n' "$N concurrent sleeps took ${elapsed}s -- looks serialized"
  fail=1
fi
i=0
while [ $i -lt $N ]; do
  out=$(cat "$TMP/sleep_$i.out")
  check "sleep #$i completed" '"slept_ms"' "$out"
  i=$((i + 1))
done

say "P4: $N concurrent ledger.transfer calls (alice -> bob, 1 each) prove no lost update under real concurrency"
out=$(call_c ledger balance '{"account":"alice"}')
alice_before=$(printf '%s' "$out" | sed -n 's/.*"balance":\(-\{0,1\}[0-9]*\).*/\1/p')
out=$(call_c ledger balance '{"account":"bob"}')
bob_before=$(printf '%s' "$out" | sed -n 's/.*"balance":\(-\{0,1\}[0-9]*\).*/\1/p')
echo "before: alice=$alice_before bob=$bob_before"
pids=""
i=0
while [ $i -lt $N ]; do
  call_c ledger transfer '{"from":"alice","to":"bob","amount":1}' >"$TMP/xfer_$i.out" 2>&1 &
  pids="$pids $!"
  i=$((i + 1))
done
wait $pids
i=0
while [ $i -lt $N ]; do
  out=$(cat "$TMP/xfer_$i.out")
  check "concurrent transfer #$i succeeded" '"from_balance"' "$out"
  i=$((i + 1))
done
out=$(call_c ledger balance '{"account":"alice"}')
alice_after=$(printf '%s' "$out" | sed -n 's/.*"balance":\(-\{0,1\}[0-9]*\).*/\1/p')
out=$(call_c ledger balance '{"account":"bob"}')
bob_after=$(printf '%s' "$out" | sed -n 's/.*"balance":\(-\{0,1\}[0-9]*\).*/\1/p')
echo "after:  alice=$alice_after bob=$bob_after"
want_alice=$((alice_before - N))
want_bob=$((bob_before + N))
if [ "$alice_after" -eq "$want_alice" ] && [ "$bob_after" -eq "$want_bob" ]; then
  printf 'PASS  %s\n' "$N concurrent transfers, zero lost updates (alice $alice_before->$alice_after, bob $bob_before->$bob_after)"
else
  printf 'FAIL  %s\n' "$N concurrent transfers: expected alice=$want_alice bob=$want_bob, got alice=$alice_after bob=$bob_after"
  fail=1
fi

say "final state, and every client agrees (C CLI)"
out=$(call_c ledger balance '{"account":"alice"}'); echo "alice: $out"
out=$(call_c ledger balance '{"account":"bob"}');   echo "bob:   $out"
out=$(call_c ledger balance '{"account":"carol"}'); echo "carol: $out"

"$PC" stop --endpoint "$EP" >/dev/null 2>&1
j=0
while kill -0 "$RUNPID" 2>/dev/null && [ $j -lt 40 ]; do sleep 0.1; j=$((j+1)); done

say "$([ $fail -eq 0 ] && echo 'DEMO PASSED' || echo 'DEMO HAD FAILURES')"
exit $fail
