#!/bin/sh
# Stage 3 gate: one deterministic C operation (inventory.get) reached from the
# C CLI AND a Node client AND (when available) a Python client through the SAME
# runtime -- identical results, and failure modes that fail distinctly.
# No mocks; a missing runtime is a failure; non-idempotent calls are not retried.
#
# Usage: roundtrip.sh <polycall> <repo-root>
set -u
PC=${1:?polycall path}
ROOT=${2:?repo root}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_rt.$$"); mkdir -p "$TMP"
EPF="$TMP/endpoint"
TOK="stage3-token"
NODE_CLIENT="$ROOT/tools/rpc-clients/node/inventory_client.mjs"
PY_CLIENT="$ROOT/tools/rpc-clients/python/inventory_client.py"

RUNLOG="$TMP/run.log"
"$PC" start --endpoint 127.0.0.1:0 --endpoint-file "$EPF" --auth-token "$TOK" \
  >"$RUNLOG" 2>&1 &
RUNPID=$!
cleanup() { kill "$RUNPID" 2>/dev/null; rm -rf "$TMP"; }
trap cleanup EXIT

# wait for the runtime to publish its endpoint
i=0
while [ ! -s "$EPF" ] && [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done
if [ ! -s "$EPF" ]; then bad "runtime did not start"; cat "$RUNLOG"; echo "--- $PASS/$FAIL/$SKIP ---"; exit 1; fi
EP=$(cat "$EPF")
echo "=== runtime up at $EP (pid $RUNPID) ==="

# helper: extract the compact "output" object from a {"ok":true,"output":...} reply
out_field() { node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{try{const o=JSON.parse(s);process.stdout.write(JSON.stringify(o.output??o.error))}catch(e){process.exit(7)}})'; }

echo "=== same C operation, three entry points ==="

C_RAW=$("$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}'); c_rc=$?
C_OUT=$(printf '%s' "$C_RAW" | out_field)
[ $c_rc -eq 0 ] && [ "$C_OUT" = '{"item_id":"widget-a","quantity":42,"in_stock":true}' ] \
  && ok "C CLI  inventory.get widget-a -> quantity 42" \
  || bad "C CLI inventory.get (rc=$c_rc out=$C_OUT)"

if command -v node >/dev/null 2>&1; then
  N_RAW=$(node "$NODE_CLIENT" "$EP" widget-a); n_rc=$?
  N_OUT=$(printf '%s' "$N_RAW" | out_field)
  { [ $n_rc -eq 0 ] && [ "$N_OUT" = "$C_OUT" ]; } \
    && ok "Node client inventory.get widget-a == C result" \
    || bad "Node client (rc=$n_rc out=$N_OUT want=$C_OUT)"
else
  skip "Node client (node not on PATH)"
fi

if command -v python >/dev/null 2>&1 || command -v python3 >/dev/null 2>&1; then
  PY=$(command -v python || command -v python3)
  P_RAW=$("$PY" "$PY_CLIENT" "$EP" widget-a); p_rc=$?
  P_OUT=$(printf '%s' "$P_RAW" | out_field)
  { [ $p_rc -eq 0 ] && [ "$P_OUT" = "$C_OUT" ]; } \
    && ok "Python client inventory.get widget-a == C result" \
    || bad "Python client (rc=$p_rc out=$P_OUT want=$C_OUT)"
else
  skip "Python client (python not on PATH)"
fi

echo "=== known fixtures ==="
Z=$("$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"gadget-c"}' | out_field)
[ "$Z" = '{"item_id":"gadget-c","quantity":0,"in_stock":false}' ] \
  && ok "gadget-c -> quantity 0, in_stock false" || bad "gadget-c fixture ($Z)"

echo "=== failure modes fail distinctly (no mocks) ==="
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"nope"}' >/dev/null 2>&1
[ $? -eq 4 ] && ok "unknown item -> exit 4" || bad "unknown item exit"

"$PC" call inventory teleport --endpoint "$EP" --input-value '{}' >/dev/null 2>&1
[ $? -eq 4 ] && ok "unknown operation -> exit 4" || bad "unknown operation exit"

"$PC" call inventory get --endpoint "$EP" --input-value 'not json' >"$TMP/mal.out" 2>&1
rc=$?; { [ $rc -eq 3 ] && grep -qi "malformed\|invalid" "$TMP/mal.out"; } \
  && ok "malformed payload -> exit 3" || bad "malformed payload (rc=$rc)"

"$PC" call inventory get --endpoint "$EP" --input-value '{}' >/dev/null 2>&1
[ $? -eq 3 ] && ok "missing required input -> exit 3" || bad "missing input exit"

"$PC" call debug sleep --endpoint "$EP" --input-value '{"ms":3000}' --deadline-ms 150 >/dev/null 2>&1
[ $? -eq 6 ] && ok "operation past deadline -> exit 6" || bad "deadline exit"

"$PC" call inventory get --endpoint 127.0.0.1:1 --input-value '{"item_id":"widget-a"}' >/dev/null 2>&1
[ $? -eq 5 ] && ok "missing runtime -> exit 5 (failure, not a stub)" || bad "missing runtime exit"

if command -v node >/dev/null 2>&1; then
  node "$NODE_CLIENT" "$EP" nope >/dev/null 2>&1
  [ $? -eq 4 ] && ok "Node client unknown item -> non-zero, distinct" || bad "Node client failure exit"
fi

echo "=== authenticated control-channel shutdown ==="
"$PC" stop --endpoint "$EP" --auth-token wrong-token >/dev/null 2>&1
[ $? -eq 7 ] && ok "stop with wrong token -> exit 7 (auth)" || bad "stop wrong token exit"

"$PC" status --endpoint "$EP" >/dev/null 2>&1
[ $? -eq 0 ] && ok "runtime still up after refused stop" || bad "runtime died on refused stop"

"$PC" stop --endpoint "$EP" --auth-token "$TOK" >/dev/null 2>&1
[ $? -eq 0 ] && ok "stop with correct token -> exit 0" || bad "stop correct token exit"

# give it a moment to unwind
j=0
while kill -0 "$RUNPID" 2>/dev/null && [ $j -lt 40 ]; do sleep 0.1; j=$((j+1)); done
if kill -0 "$RUNPID" 2>/dev/null; then bad "runtime did not exit after authenticated stop"
else ok "runtime exited after authenticated stop"; fi

"$PC" status --endpoint "$EP" >/dev/null 2>&1
[ $? -eq 5 ] && ok "status after shutdown -> exit 5 (transport)" || bad "status after shutdown"

grep -q "stopped cleanly" "$RUNLOG" && ok "run reported a clean shutdown (cleanup in normal code)" \
  || bad "run did not report clean shutdown: $(cat "$RUNLOG")"

echo "--- roundtrip: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ]
