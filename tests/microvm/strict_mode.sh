#!/bin/sh
# Milestone 5: MicroVM strict-mode adapter. A native call MUST require a real
# round trip; a missing runtime is a FAILURE (not a passed integration); the
# demo fallback is explicitly labelled.
#
# Usage: strict_mode.sh <polycall> <repo-root>
set -u
PC=${1:?}; ROOT=${2:?}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

ADAPTER="$ROOT/tools/rpc-clients/microvm/strict_adapter.mjs"
if ! command -v node >/dev/null 2>&1; then
  skip "microvm strict adapter (node not on PATH)"
  echo "--- microvm: 0/0, 1 not run ---"; exit 0
fi

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_mvm.$$"); mkdir -p "$TMP"
trap 'rm -rf "$TMP"; [ -n "${RUNPID:-}" ] && kill "$RUNPID" 2>/dev/null' EXIT

# 1. strict mode with NO runtime -> must fail, must not look like success
out=$(node "$ADAPTER" inventory get '{"item_id":"widget-a"}' --strict 2>&1); rc=$?
if [ $rc -ne 0 ] && printf '%s' "$out" | grep -q "RUNTIME-UNAVAILABLE"; then
  ok "strict + no runtime -> failure (not counted as integration)"
else
  bad "strict + no runtime should fail distinctly (rc=$rc out=$out)"
fi

# 2. demo fallback is explicitly labelled, never mistakable for native
out=$(node "$ADAPTER" inventory get '{"item_id":"widget-a"}' 2>&1)
printf '%s' "$out" | grep -q '"_demo_fallback":true' \
  && ok "demo fallback carries _demo_fallback:true" \
  || bad "demo fallback not labelled: $out"

# 3. strict mode WITH a real runtime -> real value from the real C operation
EPF="$TMP/ep"
"$PC" run --endpoint 127.0.0.1:0 --endpoint-file "$EPF" >"$TMP/run.log" 2>&1 &
RUNPID=$!
i=0; while [ ! -s "$EPF" ] && [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done
EP=$(cat "$EPF" 2>/dev/null)
if [ -z "$EP" ]; then bad "runtime did not start"; else
  out=$(POLYCALL_ENDPOINT="$EP" node "$ADAPTER" inventory get '{"item_id":"widget-a"}' --strict 2>&1); rc=$?
  if [ $rc -eq 0 ] && printf '%s' "$out" | grep -q '"quantity":42'; then
    ok "strict + runtime -> real C result (quantity 42, one round trip)"
  else
    bad "strict + runtime (rc=$rc out=$out)"
  fi
  # cross-check: same value the CLI gets
  cli=$("$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}' \
        | node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{process.stdout.write(JSON.stringify(JSON.parse(s).output))})')
  [ "$out" = "$cli" ] && ok "strict adapter result == CLI result (same implementation)" \
    || bad "adapter vs CLI mismatch: $out != $cli"
  "$PC" stop --endpoint "$EP" >/dev/null 2>&1
fi

echo "--- microvm: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ]
