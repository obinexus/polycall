#!/bin/sh
# Cross-language conformance harness (docs/TODO.md P2). Generalises
# tests/runtime/roundtrip.sh: the same fixture requests are sent through
# every available polycall_rpc v1 client, each client's raw response is
# SHA-256-hashed, and hashes are compared across languages -- a
# byte-for-byte, not just exit-code, equivalence proof.
#
# Usage: run.sh <polycall> <repo-root> <lib-dir> <shared-ext>
set -u
PC=${1:?}; ROOT=${2:?}; LIBDIR=${3:?}; SHEXT=${4:?}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }
sha()  { sha256sum | awk '{print $1}'; }

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_conf.$$"); mkdir -p "$TMP"
trap 'rm -rf "$TMP"; [ -n "${RUNPID:-}" ] && kill "$RUNPID" 2>/dev/null' EXIT

# ---- discover available clients -------------------------------------
# Each entry: "label|invoke prefix". invoke prefix + "$EP $svc $op $input [$dl]"
CLIENTS=""
CLIENTS="$CLIENTS
c|$PC call"   # special-cased below (different flag-style CLI)

if command -v node >/dev/null 2>&1; then
  CLIENTS="$CLIENTS
node|node $ROOT/tools/rpc-clients/node/polycall_call.mjs"
fi
PY=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
if [ -n "$PY" ]; then
  CLIENTS="$CLIENTS
python|$PY $ROOT/tools/rpc-clients/python/polycall_call.py"
fi
GO_BIN="$ROOT/build/conformance-go/polycall-call"
if command -v go >/dev/null 2>&1; then
  mkdir -p "$ROOT/build/conformance-go"
  ( cd "$ROOT/tools/rpc-clients/go" && go build -o "$GO_BIN" ./cmd/polycall-call ) \
    >"$TMP/go_build.log" 2>&1
  if [ -x "$GO_BIN" ] || [ -x "$GO_BIN.exe" ]; then
    [ -x "$GO_BIN.exe" ] && GO_BIN="$GO_BIN.exe"
    CLIENTS="$CLIENTS
go|$GO_BIN"
  else
    skip "Go client (build failed, see $TMP/go_build.log)"
  fi
else
  skip "Go client (go not on PATH)"
fi
JAVA_OUT="$ROOT/tools/rpc-clients/java/out"
if command -v javac >/dev/null 2>&1 && command -v java >/dev/null 2>&1; then
  mkdir -p "$JAVA_OUT"
  javac -d "$JAVA_OUT" $(find "$ROOT/tools/rpc-clients/java/src/main/java" -name '*.java') \
    >"$TMP/java_build.log" 2>&1
  if [ -f "$JAVA_OUT/org/obinexus/polycall/rpcv1/PolyCallCall.class" ]; then
    CLIENTS="$CLIENTS
java|java -cp $JAVA_OUT org.obinexus.polycall.rpcv1.PolyCallCall"
  else
    skip "Java client (javac failed, see $TMP/java_build.log)"
  fi
else
  skip "Java client (java/javac not on PATH)"
fi
skip "Lua client (no LuaSocket in this environment; wire framing is unit-verified separately -- see docs/backward_compatibility.md)"

note() { printf '%s\n' "$*"; }
note "clients discovered:$(printf '%s' "$CLIENTS" | grep -c '|')"

# ---- start a runtime with the fixture plugin loaded -------------------
EPF="$TMP/endpoint"
PLUGIN="$LIBDIR/fixture_ops_plugin.$SHEXT"
if [ -f "$PLUGIN" ]; then
  "$PC" run --endpoint 127.0.0.1:0 --endpoint-file "$EPF" --auth-token conf-tok \
    --load "$PLUGIN" >"$TMP/run.log" 2>&1 &
else
  "$PC" run --endpoint 127.0.0.1:0 --endpoint-file "$EPF" --auth-token conf-tok \
    >"$TMP/run.log" 2>&1 &
fi
RUNPID=$!
i=0
while [ ! -s "$EPF" ] && [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done
if [ ! -s "$EPF" ]; then
  bad "runtime did not start"; cat "$TMP/run.log"
  echo "--- conformance: 0 passed, 1 failed, 0 not run ---"
  exit 1
fi
EP=$(cat "$EPF")
note "runtime up at $EP"

# invoke CLIENT_LABEL EP SVC OP INPUT [DEADLINE] -> stdout in $OUT, exit in $RC
invoke() {
  _label=$1; _ep=$2; _svc=$3; _op=$4; _input=$5; _dl=${6:-}
  _prefix=$(printf '%s\n' "$CLIENTS" | awk -F'|' -v l="$_label" '$1==l{print $2}')
  if [ "$_label" = "c" ]; then
    if [ -n "$_dl" ]; then
      OUT=$(eval "$_prefix" "$_svc" "$_op" --endpoint "$_ep" --input-value "'$_input'" --deadline-ms "$_dl" 2>"$TMP/err"); RC=$?
    else
      OUT=$(eval "$_prefix" "$_svc" "$_op" --endpoint "$_ep" --input-value "'$_input'" 2>"$TMP/err"); RC=$?
    fi
  else
    OUT=$(eval "$_prefix" "$_ep" "$_svc" "$_op" "'$_input'" "$_dl" 2>"$TMP/err"); RC=$?
  fi
}

# run_case NAME SVC OP INPUT EXPECT_RC [DEADLINE]
run_case() {
  _name=$1; _svc=$2; _op=$3; _input=$4; _expect=$5; _dl=${6:-}
  note ""
  note "=== case: $_name (expect exit $_expect) ==="
  _first_label=""; _first_hash=""; _any=0
  for _l in $(printf '%s\n' "$CLIENTS" | awk -F'|' 'NF{print $1}'); do
    invoke "$_l" "$EP" "$_svc" "$_op" "$_input" "$_dl"
    _any=1
    if [ "$RC" -eq "$_expect" ]; then
      ok "$_l: $_name -> exit $RC"
    else
      bad "$_l: $_name (rc=$RC want=$_expect, out=$OUT, err=$(cat "$TMP/err" 2>/dev/null))"
    fi
    if [ "$RC" -eq 0 ]; then
      _h=$(printf '%s' "$OUT" | sha)
      if [ -z "$_first_hash" ]; then
        _first_label=$_l; _first_hash=$_h
      elif [ "$_h" = "$_first_hash" ]; then
        ok "$_l: $_name response hash == $_first_label's (byte-identical)"
      else
        bad "$_l: $_name response hash != $_first_label's ($OUT)"
      fi
    fi
  done
  [ "$_any" -eq 0 ] && skip "$_name (no clients available)"
}

run_case "success (inventory.get widget-a)"      inventory get '{"item_id":"widget-a"}' 0
run_case "input.invalid (missing item_id)"       inventory get '{}'                     3
run_case "operation.unknown"                     inventory teleport '{}'                4
run_case "deadline.exceeded (debug.sleep)"       debug sleep '{"ms":3000}'              6 200
run_case "loaded-plugin op (demo.greet)"         demo greet '{"name":"conformance"}'     0

note ""
note "=== case: no runtime (transport) ==="
for _l in $(printf '%s\n' "$CLIENTS" | awk -F'|' 'NF{print $1}'); do
  invoke "$_l" "127.0.0.1:1" inventory get '{"item_id":"widget-a"}'
  [ "$RC" -eq 5 ] && ok "$_l: no runtime -> exit 5" || bad "$_l: no runtime (rc=$RC)"
done

note ""
note "=== case: auth.denied (control channel, wrong stop token) ==="
"$PC" stop --endpoint "$EP" --auth-token wrong-token >/dev/null 2>&1
[ $? -eq 7 ] && ok "c: stop wrong token -> exit 7" || bad "c: stop wrong token"
if command -v go >/dev/null 2>&1 && [ -x "$GO_BIN" -o -x "$GO_BIN.exe" ]; then
  note "  (Go/Java rpcv1 packages expose Control()/control() for this; the C CLI" \
       "  path above is what's wired into a runnable CLI check here)"
fi

note ""
note "=== structural: non-idempotent operations are never retried ==="
note "  every client here performs exactly one connect+send+receive per call"
note "  (verified by code review of each CallRaw/call/client.call -- no loop,"
note "  no re-send on error); two independent debug.sleep calls below must"
note "  each be judged solely on their own outcome, not a retried one."
invoke "c" "$EP" debug sleep '{"ms":50}' 500
[ "$RC" -eq 0 ] && ok "c: independent debug.sleep call 1 succeeds" || bad "c: debug.sleep call 1 (rc=$RC)"
invoke "c" "$EP" debug sleep '{"ms":3000}' 200
[ "$RC" -eq 6 ] && ok "c: independent debug.sleep call 2 hits its own deadline" || bad "c: debug.sleep call 2 (rc=$RC)"

"$PC" stop --endpoint "$EP" --auth-token conf-tok >/dev/null 2>&1
j=0
while kill -0 "$RUNPID" 2>/dev/null && [ $j -lt 40 ]; do sleep 0.1; j=$((j+1)); done

note ""
note "--- conformance: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ]
