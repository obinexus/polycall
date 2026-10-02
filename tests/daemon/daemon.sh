#!/bin/sh
# polycall daemon lifecycle and fault tests (real processes, no mocks).
#
# Usage: daemon.sh <polycall> <repo-root>
set -u
PC=${1:?polycall path}
ROOT=${2:?repo root}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

WIN=0
case "$(uname -s 2>/dev/null)" in MINGW*|MSYS*|CYGWIN*) WIN=1 ;; esac

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_daemon.$$"); mkdir -p "$TMP"
P1="$TMP/p1"; P2="$TMP/p2"; mkdir -p "$P1" "$P2"
TOKEN="daemon-test-secret-$$-do-not-log"
export POLYCALL_DEV_TOKEN="$TOKEN"
export POLYCALL_TELEMETRY=off

cat >"$P1/Polycallfile" <<'EOF'
# daemon test project
server node 8080:8084
network start
daemon_endpoint=127.0.0.1:0
daemon_max_connections=4
daemon_drain_timeout_ms=4000
daemon_io_timeout_ms=2000
auth_token_env=POLYCALL_DEV_TOKEN
EOF

json_field() { sed -n "s/.*\"$1\":\"\{0,1\}\([^\",}]*\)\"\{0,1\}.*/\1/p" | head -1; }

hard_kill() {
  if [ "$WIN" = 1 ]; then taskkill //F //PID "$1" >/dev/null 2>&1
  else kill -9 "$1" 2>/dev/null; fi
}

stop_all() {
  "$PC" daemon stop --force -t 5000 "$P1/Polycallfile" >/dev/null 2>&1
  "$PC" daemon stop --force -t 5000 "$P2/Polycallfile" >/dev/null 2>&1
}
trap 'stop_all; rm -rf "$TMP"' EXIT

echo "=== configuration failures fail fast, before anything starts ==="
"$PC" daemon start "$TMP/missing/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 3 ] && ok "missing Polycallfile -> exit 3" || bad "missing Polycallfile (rc=$RC: $(cat "$TMP/err"))"
mkdir -p "$TMP/bad"
printf 'daemon_max_connections=lots\n' >"$TMP/bad/Polycallfile"
"$PC" daemon start "$TMP/bad/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 3 ] && grep -q "daemon_max_connections" "$TMP/err" \
  && ok "invalid Polycallfile -> exit 3 naming the key" || bad "invalid Polycallfile (rc=$RC: $(cat "$TMP/err"))"
printf 'tls_enabled=true\ncert_file=/x/c.pem\nkey_file=/x/k.pem\ndaemon_endpoint=127.0.0.1:0\n' >"$TMP/bad/Polycallfile"
"$PC" daemon start "$TMP/bad/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 4 ] && ok "tls_enabled=true refused (exit 4) instead of serving plaintext" \
  || bad "tls refusal (rc=$RC: $(cat "$TMP/err"))"
printf 'daemon_endpoint=0.0.0.0:0\nauth_token_env=NO_SUCH_TOKEN_VAR\n' >"$TMP/bad/Polycallfile"
"$PC" daemon start "$TMP/bad/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 3 ] && ok "non-loopback bind without a token refused (exit 3)" \
  || bad "non-loopback without token (rc=$RC: $(cat "$TMP/err"))"
[ ! -d "$TMP/bad/.polycall" ] && ok "failed starts left no state behind" || bad "state dir created by a failed start"

echo "=== start / status / repeated start ==="
OUT=$("$PC" --format json daemon start -t 15000 "$P1/Polycallfile" 2>"$TMP/err"); RC=$?
PID=$(printf '%s' "$OUT" | json_field pid)
EP=$(printf '%s' "$OUT" | json_field endpoint)
if [ $RC -eq 0 ] && [ -n "$PID" ] && [ -n "$EP" ]; then ok "daemon start -> pid $PID at $EP"
else bad "daemon start (rc=$RC out=$OUT err=$(cat "$TMP/err"))"; echo "--- daemon: $PASS passed, $FAIL failed, $SKIP not run ---"; exit 1; fi
[ -f "$P1/.polycall/daemon.json" ] && [ -f "$P1/.polycall/daemon.pid" ] \
  && ok "state and pid files written" || bad "state files missing"
"$PC" daemon status "$P1/Polycallfile" >"$TMP/st" 2>&1; RC=$?
[ $RC -eq 0 ] && grep -q "running (pid $PID)" "$TMP/st" && ok "status -> running, exit 0" \
  || bad "status (rc=$RC: $(cat "$TMP/st"))"
"$PC" --format json daemon status "$P1/Polycallfile" >"$TMP/stj" 2>&1
grep -q '"active_connections"' "$TMP/stj" && grep -q '"status":"ok"' "$TMP/stj" \
  && ok "status JSON carries health (status ok, connection counters)" || bad "status JSON: $(cat "$TMP/stj")"
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}' >"$TMP/call" 2>&1; RC=$?
[ $RC -eq 0 ] && grep -q '"quantity":42' "$TMP/call" && ok "operation served by the daemon" \
  || bad "call through daemon (rc=$RC: $(cat "$TMP/call"))"
OUT2=$("$PC" --format json daemon start "$P1/Polycallfile" 2>&1); RC=$?
PID2=$(printf '%s' "$OUT2" | json_field pid)
[ $RC -eq 0 ] && [ "$PID2" = "$PID" ] && printf '%s' "$OUT2" | grep -q '"already_running":true' \
  && ok "repeated start is idempotent (same pid $PID)" || bad "repeated start (rc=$RC: $OUT2)"
"$PC" daemon run "$P1/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 1 ] && grep -q "already running" "$TMP/err" \
  && ok "a second foreground instance is refused (single-instance lock)" || bad "second run (rc=$RC: $(cat "$TMP/err"))"

echo "=== structured logs, no secrets ==="
LOG="$P1/.polycall/daemon.log"
grep -q '"event":"daemon.ready"' "$LOG" && grep -q '"level":"info"' "$LOG" \
  && ok "JSON log has daemon.ready" || bad "log lacks daemon.ready: $(cat "$LOG")"
if grep -q "$TOKEN" "$LOG" "$P1/.polycall/daemon.json" 2>/dev/null; then bad "the token appears in logs/state"
else ok "the token never appears in logs or state files"; fi
if [ "$WIN" = 0 ] && [ -r "/proc/$PID/cmdline" ]; then
  if tr '\0' ' ' <"/proc/$PID/cmdline" | grep -q "$TOKEN"; then bad "token on the daemon's command line"
  else ok "token not on the daemon's command line"; fi
fi

echo "=== address already in use ==="
PORT=${EP##*:}
cat >"$P2/Polycallfile" <<EOF
daemon_endpoint=127.0.0.1:$PORT
EOF
"$PC" daemon start -t 8000 "$P2/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 5 ] && grep -qi "in use" "$TMP/err" && ok "second daemon on the same port -> exit 5 (address in use)" \
  || bad "address in use (rc=$RC: $(cat "$TMP/err"))"
"$PC" daemon status "$P1/Polycallfile" >/dev/null 2>&1 && ok "first daemon unaffected" || bad "first daemon disturbed"

echo "=== authenticated stop ==="
POLYCALL_DEV_TOKEN=wrong-token "$PC" daemon stop -t 3000 "$P1/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
[ $RC -eq 7 ] && ok "stop with the wrong token -> exit 7" || bad "stop wrong token (rc=$RC: $(cat "$TMP/err"))"
"$PC" daemon status "$P1/Polycallfile" >/dev/null 2>&1 && ok "daemon still running after the refused stop" \
  || bad "refused stop killed the daemon"

echo "=== shutdown with active work drains it ==="
"$PC" call debug sleep --endpoint "$EP" --input-value '{"ms":1500}' --deadline-ms 5000 >"$TMP/slow" 2>&1 &
SLOW=$!
sleep 0.4
"$PC" daemon stop -t 10000 "$P1/Polycallfile" >"$TMP/stop" 2>&1; RC=$?
wait $SLOW; SRC=$?
[ $RC -eq 0 ] && ok "stop -> exit 0" || bad "stop (rc=$RC: $(cat "$TMP/stop"))"
[ $SRC -eq 0 ] && grep -q '"slept_ms":1500' "$TMP/slow" && ok "in-flight request completed during the drain" \
  || bad "in-flight request lost (rc=$SRC: $(cat "$TMP/slow"))"
grep -q '"event":"daemon.stopped"' "$LOG" && ok "log records a clean stop" || bad "no daemon.stopped event"
"$PC" daemon status "$P1/Polycallfile" >/dev/null 2>&1; RC=$?
[ $RC -eq 5 ] && ok "status after stop -> exit 5 (not running)" || bad "status after stop (rc=$RC)"
[ ! -f "$P1/.polycall/daemon.json" ] && ok "state file removed on clean stop" || bad "state file left behind"
"$PC" daemon stop "$P1/Polycallfile" >"$TMP/stop2" 2>&1; RC=$?
[ $RC -eq 0 ] && grep -q "not running" "$TMP/stop2" && ok "stop when not running -> exit 0, says so" \
  || bad "stop when stopped (rc=$RC)"

echo "=== restart ==="
OUT=$("$PC" --format json daemon start "$P1/Polycallfile" 2>&1); PIDA=$(printf '%s' "$OUT" | json_field pid)
OUT=$("$PC" --format json daemon restart "$P1/Polycallfile" 2>&1); RC=$?
PIDB=$(printf '%s' "$OUT" | json_field pid)
EP=$(printf '%s' "$OUT" | json_field endpoint)
[ $RC -eq 0 ] && [ -n "$PIDB" ] && [ "$PIDA" != "$PIDB" ] && ok "restart -> new pid $PIDB (was $PIDA)" \
  || bad "restart (rc=$RC: $OUT)"

echo "=== bounded connections: backpressure, not unbounded queueing ==="
i=0; while [ $i -lt 4 ]; do
  "$PC" call debug sleep --endpoint "$EP" --input-value '{"ms":2500}' --deadline-ms 5000 >/dev/null 2>&1 &
  eval "HOLD$i=\$!"; i=$((i+1))
done
sleep 0.6
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}' >"$TMP/busy" 2>&1; RC=$?
[ $RC -eq 5 ] && grep -qi "busy" "$TMP/busy" \
  && ok "5th concurrent connection over daemon_max_connections=4 -> server.busy (exit 5)" \
  || bad "backpressure (rc=$RC: $(cat "$TMP/busy"))"
i=0; while [ $i -lt 4 ]; do eval "wait \$HOLD$i"; i=$((i+1)); done
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}' >/dev/null 2>&1 \
  && ok "capacity returns once the slow calls finish" || bad "daemon did not recover after backpressure"

echo "=== peer disconnect / reconnect ==="
"$PC" call debug sleep --endpoint "$EP" --input-value '{"ms":3000}' --deadline-ms 5000 >/dev/null 2>&1 &
GONE=$!
sleep 0.3
kill "$GONE" 2>/dev/null; wait "$GONE" 2>/dev/null
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-b"}' >"$TMP/after" 2>&1; RC=$?
[ $RC -eq 0 ] && grep -q '"quantity":7' "$TMP/after" && ok "a client vanishing mid-call does not disturb the next one" \
  || bad "after disconnect (rc=$RC: $(cat "$TMP/after"))"
"$PC" daemon status "$P1/Polycallfile" >/dev/null 2>&1 && ok "daemon healthy after the disconnect" || bad "daemon unhealthy after disconnect"

echo "=== recovery after forced termination ==="
PIDK=$(cat "$P1/.polycall/daemon.pid" 2>/dev/null | tr -d '\r\n')
hard_kill "$PIDK"
sleep 0.5
"$PC" daemon status "$P1/Polycallfile" >"$TMP/st" 2>&1; RC=$?
[ $RC -eq 5 ] && grep -q "stale" "$TMP/st" && ok "after kill -9: status reports not running (stale state)" \
  || bad "status after hard kill (rc=$RC: $(cat "$TMP/st"))"
OUT=$("$PC" --format json daemon start "$P1/Polycallfile" 2>&1); RC=$?
EP=$(printf '%s' "$OUT" | json_field endpoint)
[ $RC -eq 0 ] && ok "start after a hard kill succeeds (lock was released by the OS)" || bad "restart after kill (rc=$RC: $OUT)"
grep -q '"event":"daemon.recovered_stale_state"' "$LOG" && ok "stale state recovery logged" \
  || bad "no stale-state recovery event"
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}' >/dev/null 2>&1 \
  && ok "recovered daemon serves" || bad "recovered daemon does not serve"
"$PC" daemon stop "$P1/Polycallfile" >/dev/null 2>&1

echo "=== permission failure ==="
if [ "$WIN" = 0 ] && [ "$(id -u)" = 0 ]; then
  skip "permission failure (running as root: file modes do not restrict root)"
else
  mkdir -p "$P2/.polycall"
  printf 'daemon_endpoint=127.0.0.1:0\n' >"$P2/Polycallfile"
  : >"$P2/.polycall/daemon.lock"
  if [ "$WIN" = 1 ]; then attrib +R "$(cygpath -w "$P2/.polycall/daemon.lock")" >/dev/null 2>&1
  else chmod 0400 "$P2/.polycall/daemon.lock"; fi
  "$PC" daemon start -t 5000 "$P2/Polycallfile" >/dev/null 2>"$TMP/err"; RC=$?
  [ $RC -eq 1 ] && grep -qi "permission\|denied\|cannot open lock" "$TMP/err" \
    && ok "unwritable state -> exit 1 with a permission error" || bad "permission failure (rc=$RC: $(cat "$TMP/err"))"
  if [ "$WIN" = 1 ]; then attrib -R "$(cygpath -w "$P2/.polycall/daemon.lock")" >/dev/null 2>&1
  else chmod 0600 "$P2/.polycall/daemon.lock"; fi
fi

echo "--- daemon: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ] || exit 1
[ "$SKIP" -eq 0 ] || exit 77
exit 0
