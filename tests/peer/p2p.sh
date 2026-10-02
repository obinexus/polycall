#!/bin/sh
# Two independent peer PROCESSES (polycall peer serve) exchanging payloads
# over polycall-peer/1, plus fault injection through a TCP proxy.
# Every delivery is verified at the receiver: exact bytes, sender id and
# message id -- never just the sender's exit status.
#
# Usage: p2p.sh <polycall> <repo-root> <fault-proxy-exe>
set -u
PC=${1:?polycall path}
ROOT=${2:?repo root}
PROXY=${3:?fault proxy path}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_p2p.$$"); mkdir -p "$TMP"
export POLYCALL_DEV_TOKEN="p2p-shared-token-$$"
export POLYCALL_TELEMETRY=off
PIDS=""
cleanup() { for p in $PIDS; do kill -9 "$p" 2>/dev/null; done; rm -rf "$TMP"; }
trap cleanup EXIT

wait_file() { _i=0; while [ ! -s "$1" ] && [ $_i -lt 100 ]; do sleep 0.1; _i=$((_i+1)); done; [ -s "$1" ]; }
b64() { printf '%s' "$1" | base64 | tr -d '\n'; }
json_field() { sed -n "s/.*\"$1\":\"\{0,1\}\([^\",}]*\)\"\{0,1\}.*/\1/p" | head -1; }

start_node() { # start_node ID ENDPOINT -> sets NODE_PID, NODE_EP
  rm -f "$TMP/$1.ep"
  "$PC" peer serve --node-id "$1" --endpoint "$2" --endpoint-file "$TMP/$1.ep" >"$TMP/$1.log" 2>&1 &
  NODE_PID=$!
  PIDS="$PIDS $NODE_PID"
  if wait_file "$TMP/$1.ep"; then NODE_EP=$(tr -d '\r\n' <"$TMP/$1.ep"); else NODE_EP=""; fi
}

recv_check() { # recv_check NODE_EP FROM ID B64 LABEL
  _out=$("$PC" peer recv --to "$1" -t 3000 2>&1); _rc=$?
  _f=$(printf '%s' "$_out" | json_field from)
  _id=$(printf '%s' "$_out" | json_field id)
  _p=$(printf '%s' "$_out" | json_field payload_b64)
  if [ $_rc -eq 0 ] && [ "$_f" = "$2" ] && [ "$_id" = "$3" ] && [ "$_p" = "$4" ]; then ok "$5"
  else bad "$5 (rc=$_rc got from=$_f id=$_id payload=$_p; want $2/$3/$4: $_out)"; fi
}

echo "=== two independent peer processes ==="
start_node beta 127.0.0.1:0; BETA_PID=$NODE_PID; EP_B=$NODE_EP
start_node alpha 127.0.0.1:0; ALPHA_PID=$NODE_PID; EP_A=$NODE_EP
if [ -z "$EP_A" ] || [ -z "$EP_B" ]; then
  bad "nodes did not start: $(cat "$TMP/alpha.log" "$TMP/beta.log")"
  echo "--- p2p: $PASS passed, $FAIL failed, $SKIP not run ---"; exit 1
fi
[ "$ALPHA_PID" != "$BETA_PID" ] && ok "alpha (pid $ALPHA_PID, $EP_A) and beta (pid $BETA_PID, $EP_B) are separate processes" \
  || bad "same process"
"$PC" peer health --to "$EP_A" >"$TMP/h" 2>&1 && grep -q '"node_id":"alpha"' "$TMP/h" && ok "alpha /health" || bad "alpha health: $(cat "$TMP/h")"
"$PC" peer health --to "$EP_B" >"$TMP/h" 2>&1 && grep -q '"node_id":"beta"' "$TMP/h" && ok "beta /health" || bad "beta health: $(cat "$TMP/h")"

echo "=== registries are per node; changes are explicit and authenticated ==="
"$PC" peer peers --to "$EP_B" >"$TMP/p" 2>&1 && grep -q '"peers":{}' "$TMP/p" && ok "beta's registry starts empty" || bad "beta peers: $(cat "$TMP/p")"
POLYCALL_DEV_TOKEN= "$PC" peer peers --to "$EP_B" >/dev/null 2>&1; RC=$?
[ $RC -eq 7 ] && ok "reading a registry without the token -> exit 7" || bad "unauthenticated /peers (rc=$RC)"
"$PC" peer register --to "$EP_B" --id alpha --peer-endpoint "$EP_A" >/dev/null 2>&1 && ok "register alpha on beta" || bad "register on beta"
"$PC" peer register --to "$EP_A" --id beta --peer-endpoint "$EP_B" >/dev/null 2>&1 && ok "register beta on alpha" || bad "register on alpha"
"$PC" peer peers --to "$EP_B" >"$TMP/p" 2>&1; grep -q "\"alpha\":\"$EP_A\"" "$TMP/p" && ok "beta now lists alpha" || bad "beta registry: $(cat "$TMP/p")"
"$PC" peer register --to "$EP_B" --id 'bad id' --peer-endpoint "$EP_A" >/dev/null 2>&1; RC=$?
[ $RC -ne 0 ] && ok "invalid registration rejected (exit $RC)" || bad "invalid registration accepted"

echo "=== payloads both directions, verified at the receiver ==="
"$PC" peer send --from alpha --to "$EP_B" --id m-a2b --payload "hello beta" >/dev/null 2>&1 && ok "alpha -> beta sent" || bad "alpha -> beta send"
recv_check "$EP_B" alpha m-a2b "$(b64 'hello beta')" "beta received exactly 'hello beta' from alpha"
"$PC" peer send --from beta --to "$EP_A" --id m-b2a --payload "hello alpha" >/dev/null 2>&1 && ok "beta -> alpha sent" || bad "beta -> alpha send"
recv_check "$EP_A" beta m-b2a "$(b64 'hello alpha')" "alpha received exactly 'hello alpha' from beta"
U=$(printf 'h\303\251llo \342\200\224 \344\270\226\347\225\214 \360\237\214\215')
"$PC" peer send --from alpha --to "$EP_B" --id m-utf8 --payload "$U" >/dev/null 2>&1
recv_check "$EP_B" alpha m-utf8 "$(b64 "$U")" "UTF-8 payload byte-identical"
i=0; : >"$TMP/bin.in"; while [ $i -lt 256 ]; do printf "\\$(printf '%03o' $i)" >>"$TMP/bin.in"; i=$((i+1)); done
"$PC" peer send --from alpha --to "$EP_B" --id m-bin --payload-file "$TMP/bin.in" >/dev/null 2>&1 && ok "binary (incl. NUL) sent from a file" || bad "binary send"
"$PC" peer recv --to "$EP_B" -t 3000 --raw >"$TMP/bin.out" 2>/dev/null
cmp -s "$TMP/bin.in" "$TMP/bin.out" && ok "256-byte binary payload identical at beta" || bad "binary payload differs"
"$PC" peer send --from alpha --to "$EP_B" --id m-empty --payload "" >/dev/null 2>&1
recv_check "$EP_B" alpha m-empty "" "empty payload delivered"

echo "=== duplicates and retries (at-least-once + receiver de-duplication) ==="
"$PC" peer send --from alpha --to "$EP_B" --id m-dup --payload once >/dev/null 2>&1
"$PC" peer send --from alpha --to "$EP_B" --id m-dup --payload once >/dev/null 2>&1; RC=$?
[ $RC -eq 0 ] && ok "re-sending the same id is acknowledged" || bad "dup re-send (rc=$RC)"
recv_check "$EP_B" alpha m-dup "$(b64 once)" "first copy received"
"$PC" peer recv --to "$EP_B" -t 500 >/dev/null 2>&1; RC=$?
[ $RC -eq 6 ] && ok "no second copy (recv times out, exit 6)" || bad "duplicate delivered twice (rc=$RC)"

echo "=== authentication ==="
POLYCALL_DEV_TOKEN=wrong "$PC" peer send --from mallory --to "$EP_B" --payload x >/dev/null 2>&1; RC=$?
[ $RC -eq 7 ] && ok "wrong token -> exit 7, not delivered" || bad "wrong token (rc=$RC)"
"$PC" peer recv --to "$EP_B" -t 300 >/dev/null 2>&1; RC=$?
[ $RC -eq 6 ] && ok "nothing from mallory was queued" || bad "unauthenticated message queued"

echo "=== limits and malformed input ==="
head -c 1048577 /dev/zero >"$TMP/big" 2>/dev/null || dd if=/dev/zero of="$TMP/big" bs=1048577 count=1 2>/dev/null
"$PC" peer send --from alpha --to "$EP_B" --payload-file "$TMP/big" >/dev/null 2>&1; RC=$?
[ $RC -eq 2 ] && ok "1 MiB + 1 payload refused before sending (exit 2)" || bad "oversize (rc=$RC)"
head -c 1048576 /dev/zero >"$TMP/max" 2>/dev/null || dd if=/dev/zero of="$TMP/max" bs=1048576 count=1 2>/dev/null
"$PC" peer send --from alpha --to "$EP_B" --id m-max --payload-file "$TMP/max" -t 10000 >/dev/null 2>&1 && ok "exactly 1 MiB delivered" || bad "1 MiB send"
"$PC" peer recv --to "$EP_B" -t 5000 --raw >"$TMP/max.out" 2>/dev/null
cmp -s "$TMP/max" "$TMP/max.out" && ok "1 MiB payload identical" || bad "1 MiB payload differs"
"$PC" peer send --from alpha --to "$EP_B" --id 'bad id!' --payload x >/dev/null 2>&1; RC=$?
[ $RC -eq 2 ] && ok "invalid message id -> exit 2" || bad "invalid id (rc=$RC)"
"$PC" peer send --from alpha --to nobody --payload x >/dev/null 2>&1; RC=$?
[ $RC -eq 4 ] && ok "unknown peer id -> exit 4" || bad "unknown peer (rc=$RC)"

echo "=== a peer dies: the survivor stays healthy and reports failed delivery ==="
kill -9 "$BETA_PID" 2>/dev/null; wait "$BETA_PID" 2>/dev/null
sleep 0.3
"$PC" peer send --from alpha --to "$EP_B" --payload "into the void" -t 2000 >"$TMP/dead" 2>&1; RC=$?
[ $RC -eq 5 ] && ok "send to dead beta -> exit 5 (failed delivery, not a false success)" || bad "send to dead (rc=$RC: $(cat "$TMP/dead"))"
"$PC" peer health --to "$EP_A" >/dev/null 2>&1 && ok "alpha remains healthy while beta is down" || bad "alpha unhealthy"
"$PC" peer health --to "$EP_B" >/dev/null 2>&1; RC=$?
[ $RC -eq 5 ] && ok "beta health -> exit 5 while down" || bad "dead beta health (rc=$RC)"

echo "=== restart / rejoin ==="
start_node beta "$EP_B"; BETA_PID=$NODE_PID
[ "$NODE_EP" = "$EP_B" ] && ok "beta restarted on its old endpoint $EP_B" || bad "beta restart ($NODE_EP)"
"$PC" peer peers --to "$EP_B" >"$TMP/p" 2>&1; grep -q '"peers":{}' "$TMP/p" \
  && ok "restarted beta has a fresh registry (state is per process)" || bad "registry survived restart?! $(cat "$TMP/p")"
"$PC" peer send --from alpha --to "$EP_B" --id m-rejoin --payload back >/dev/null 2>&1 && ok "alpha -> restarted beta" || bad "send after rejoin"
recv_check "$EP_B" alpha m-rejoin "$(b64 back)" "rejoined beta receives"

if [ ! -x "$PROXY" ] && [ ! -x "$PROXY.exe" ]; then
  skip "partition / delay / reset faults (fault proxy not built: $PROXY)"
else
  proxy() { # proxy MODE [DELAY] -> PROXY_PID, PROXY_EP (in front of beta)
    rm -f "$TMP/proxy.ep"
    "$PROXY" "$TMP/proxy.ep" "$EP_B" "$@" >/dev/null 2>&1 &
    PROXY_PID=$!; PIDS="$PIDS $PROXY_PID"
    wait_file "$TMP/proxy.ep" && PROXY_EP=$(tr -d '\r\n' <"$TMP/proxy.ep")
  }
  echo "=== partition (blackhole link) ==="
  proxy blackhole
  "$PC" peer send --from alpha --to "$PROXY_EP" --id m-part --payload lost -t 800 >/dev/null 2>&1; RC=$?
  [ $RC -eq 6 ] && ok "send across a partition -> exit 6 within its deadline" || bad "partition (rc=$RC)"
  "$PC" peer health --to "$EP_A" >/dev/null 2>&1 && "$PC" peer health --to "$EP_B" >/dev/null 2>&1 \
    && ok "both sides stay independently healthy during the partition" || bad "health during partition"
  kill -9 "$PROXY_PID" 2>/dev/null
  proxy forward
  "$PC" peer send --from alpha --to "$PROXY_EP" --id m-part --payload lost -t 3000 >/dev/null 2>&1 \
    && ok "after the partition heals, the retry with the same id succeeds" || bad "post-partition retry"
  recv_check "$EP_B" alpha m-part "$(b64 lost)" "the healed message arrives once"
  kill -9 "$PROXY_PID" 2>/dev/null

  echo "=== delayed delivery: deadline expires, message still arrives, retry is de-duplicated ==="
  proxy delay 1500
  "$PC" peer send --from alpha --to "$PROXY_EP" --id m-slow --payload slow -t 500 >/dev/null 2>&1; RC=$?
  [ $RC -eq 6 ] && ok "slow link: sender reports a timeout (outcome unknown)" || bad "slow link (rc=$RC)"
  sleep 2
  "$PC" peer send --from alpha --to "$EP_B" --id m-slow --payload slow -t 2000 >/dev/null 2>&1; RC=$?
  [ $RC -eq 0 ] && ok "retry with the same id is acknowledged" || bad "retry after timeout (rc=$RC)"
  recv_check "$EP_B" alpha m-slow "$(b64 slow)" "the delayed message was delivered"
  "$PC" peer recv --to "$EP_B" -t 500 >/dev/null 2>&1; RC=$?
  [ $RC -eq 6 ] && ok "late original + retry produced exactly one copy" || bad "duplicate after delayed retry (rc=$RC)"
  kill -9 "$PROXY_PID" 2>/dev/null

  echo "=== link reset ==="
  proxy reset
  "$PC" peer send --from alpha --to "$PROXY_EP" --payload x -t 2000 >/dev/null 2>&1; RC=$?
  [ $RC -eq 5 ] && ok "connection reset -> exit 5" || bad "reset (rc=$RC)"
  kill -9 "$PROXY_PID" 2>/dev/null
fi

echo "--- p2p: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ] || exit 1
[ "$SKIP" -eq 0 ] || exit 77
exit 0
