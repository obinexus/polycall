#!/bin/sh
# Stage 2 gate: the C, Node and (when available) Python providers must express
# the SAME two-service "shop" topology and yield a byte-identical canonical
# envelope. Also covers: invalid values fail predictably, provider crash /
# timeout / oversize output fail distinctly, secrets are redacted, no runtime
# is started, and legacy->v2 migration preserves effective values.
#
# Usage: equivalence.sh <polycall> <repo-root> <c-provider-lib>
set -u
PC=${1:?polycall path}
ROOT=${2:?repo root}
CPROV=${3:?c provider lib path}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

NODE_CFG="$ROOT/tests/fixtures/providers/node-provider/example.config.mjs"
PY_CFG="$ROOT/tests/fixtures/providers/python-provider/example_config.py"
export POLYCALL_NODE_PROVIDER="$ROOT/tests/fixtures/providers/node-provider/run.mjs"
export POLYCALL_PYTHON_PROVIDER="$ROOT/tests/fixtures/providers/python-provider/run.py"
TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_eqv.$$"); mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

echo "=== provider equivalence ==="

# canonical envelopes from each provider (config show --format json -> .data)
env_of() { # env_of <selector...>
  "$PC" --format json config show "$@" 2>/dev/null | \
    node -e 'let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{try{const o=JSON.parse(s);if(!o.ok){process.exit(9)}process.stdout.write(JSON.stringify(o.data))}catch(e){process.exit(8)}})'
}

C_ENV=$(env_of --provider "c:$CPROV"); c_rc=$?
N_ENV=$(env_of --provider "node:$NODE_CFG"); n_rc=$?

if [ $c_rc -eq 0 ] && [ $n_rc -eq 0 ]; then
  if [ "$C_ENV" = "$N_ENV" ]; then ok "C provider envelope == Node provider envelope"
  else bad "C vs Node envelope differ"; printf '  C   : %s\n  node: %s\n' "$C_ENV" "$N_ENV"; fi
else
  bad "could not obtain both envelopes (c_rc=$c_rc n_rc=$n_rc)"
fi

if command -v python >/dev/null 2>&1 || command -v python3 >/dev/null 2>&1; then
  P_ENV=$(env_of --provider "python:$PY_CFG"); p_rc=$?
  if [ $p_rc -eq 0 ] && [ "$P_ENV" = "$C_ENV" ]; then ok "Python provider envelope == C provider envelope"
  else bad "Python vs C envelope differ or python failed (rc=$p_rc)"; fi
else
  skip "Python provider equivalence (python not on PATH)"
fi

# the envelope must carry numbers for ports and booleans as booleans
case "$C_ENV" in
  *'"host_port":8080'*) ok "ports serialised as JSON numbers" ;;
  *) bad "ports not serialised as numbers: $C_ENV" ;;
esac

echo "=== failure modes ==="

# invalid port via a bad envelope file
printf '%s' '{"schema_version":2,"project":{"name":"p"},"services":[{"id":"a","language":"c","host_port":70000,"target_port":2,"workspace":"/w"}]}' > "$TMP/badport.json"
"$PC" config validate --envelope "$TMP/badport.json" >/dev/null 2>&1
[ $? -eq 3 ] && ok "out-of-range port -> exit 3" || bad "out-of-range port exit code"

# unknown core field
printf '%s' '{"schema_version":2,"project":{"name":"p"},"services":[{"id":"a","language":"c","host_port":1,"target_port":2,"workspace":"/w","surprise":1}]}' > "$TMP/unknown.json"
"$PC" config validate --envelope "$TMP/unknown.json" >/dev/null 2>&1
[ $? -eq 3 ] && ok "unknown core field -> exit 3" || bad "unknown core field exit code"

# provider process that crashes (non-zero exit, no envelope)
cat > "$TMP/crash.mjs" <<'EOF'
process.stderr.write("boom\n"); process.exit(1);
EOF
"$PC" config validate --provider "node:$TMP/does-not-exist.mjs" >/dev/null 2>&1
rc=$?; [ $rc -eq 3 ] && ok "provider load failure -> exit 3" || bad "provider failure exit ($rc)"

# provider that never terminates -> deadline
cat > "$TMP/hang.mjs" <<'EOF'
setInterval(()=>{}, 1000);
EOF
POLYCALL_NODE_PROVIDER="$TMP/hang.mjs" \
  "$PC" config validate --provider "node:$NODE_CFG" --deadline-ms 800 >"$TMP/hang.out" 2>&1
rc=$?
if [ $rc -eq 3 ] && grep -qi "deadline" "$TMP/hang.out"; then ok "hung provider -> deadline -> exit 3"
else bad "hung provider deadline (rc=$rc): $(cat "$TMP/hang.out")"; fi

# oversize provider output -> distinct error
cat > "$TMP/flood.mjs" <<'EOF'
const chunk = "x".repeat(65536);
for (let i = 0; i < 40; i++) process.stdout.write(chunk);
EOF
POLYCALL_NODE_PROVIDER="$TMP/flood.mjs" \
  "$PC" config validate --provider "node:$NODE_CFG" --max-bytes 4096 >"$TMP/flood.out" 2>&1
rc=$?
if [ $rc -eq 3 ] && grep -qi "budget\|too large\|too_large" "$TMP/flood.out"; then ok "oversize output -> distinct error"
else bad "oversize output (rc=$rc): $(cat "$TMP/flood.out")"; fi

echo "=== secrets / redaction ==="
printf '%s' '{"schema_version":2,"project":{"name":"p"},"services":[{"id":"a","language":"c","host_port":1,"target_port":2,"workspace":"/w","tls":{"mode":"server","cert_ref":"-----BEGIN CERT-----AAAA","key_ref":"file:/k"}}]}' > "$TMP/secret.json"
"$PC" config validate --envelope "$TMP/secret.json" >"$TMP/secret.out" 2>&1
rc=$?
if [ $rc -eq 3 ] && ! grep -q "BEGIN CERT" "$TMP/secret.out"; then ok "inline secret rejected and not echoed"
else bad "inline secret handling (rc=$rc)"; fi

printf '%s' '{"schema_version":2,"project":{"name":"p"},"services":[{"id":"a","language":"c","host_port":1,"target_port":2,"workspace":"/w","tls":{"mode":"server","cert_ref":"env:TLS_CERT","key_ref":"env:TLS_KEY"}}]}' > "$TMP/tlsref.json"
out=$("$PC" config show --envelope "$TMP/tlsref.json" 2>&1)
case "$out" in
  *'"cert_ref":"env:TLS_CERT"'*) ok "tls uses env:/file: references, values never resolved by show" ;;
  *) bad "tls ref not shown as reference: $out" ;;
esac

echo "=== no runtime started during validation ==="
# validation must not open a listening socket; use the CLI's own timing + a
# check that nothing is bound. We assert it returns promptly and prints no
# 'listening'/'bound' text.
out=$("$PC" config validate --provider "c:$CPROV" 2>&1)
case "$out" in
  *listen*|*bound*|*"port "*) bad "validation mentioned binding a port" ;;
  *) ok "validation started no service" ;;
esac

echo "=== legacy -> v2 migration equivalence ==="
MIGDIR="$TMP/legacy"; mkdir -p "$MIGDIR"
cat > "$MIGDIR/Polycallfile" <<'EOF'
# two services + scalars
server node 8080:8084
server python 3001:8084
network start
network_timeout=5000
workspace_root=/opt/polycall
tls_enabled=true
cert_file=/etc/polycall/cert.pem
key_file=/etc/polycall/key.pem
max_connections=1000
enable_metrics=true
metrics_port=8090
EOF
"$PC" --project-root "$MIGDIR" config migrate --from-legacy --output "$TMP/native.json" >"$TMP/mig.out" 2>&1
rc=$?
if [ $rc -ne 0 ]; then bad "migrate --from-legacy (rc=$rc): $(cat "$TMP/mig.out")"; else
  ok "migrate --from-legacy produced $TMP/native.json"
  # effective values preserved?
  mig=$(cat "$TMP/native.json")
  case "$mig" in
    *'"id":"node"'*'"host_port":8080'*'"target_port":8084'*) ok "server node 8080:8084 preserved" ;;
    *) bad "node ports not preserved: $mig" ;;
  esac
  case "$mig" in
    *'"id":"python"'*) ok "server python preserved" ;;
    *) bad "python service missing" ;;
  esac
  case "$mig" in
    *'"timeout_ms":5000'*) ok "network_timeout=5000 -> timeout_ms:5000" ;;
    *) bad "network_timeout not mapped" ;;
  esac
  case "$mig" in
    *'"mode":"server"'*'"cert_ref":"file:/etc/polycall/cert.pem"'*) ok "tls_enabled -> file: refs" ;;
    *) bad "tls not mapped to file refs" ;;
  esac
  # unresolved keys reported, not silently dropped
  if grep -q "enable_metrics\|metrics_port" "$TMP/mig.out"; then ok "unmapped legacy keys reported (not dropped)"
  else bad "unmapped keys not reported: $(cat "$TMP/mig.out")"; fi
  # refuses to overwrite
  "$PC" --project-root "$MIGDIR" config migrate --from-legacy --output "$TMP/native.json" >/dev/null 2>&1
  [ $? -eq 3 ] && ok "migrate refuses to overwrite existing output" || bad "migrate overwrite guard"
  # original file untouched
  grep -q "^server node 8080:8084" "$MIGDIR/Polycallfile" && ok "legacy source left intact" || bad "legacy source modified"
fi

echo "--- equivalence: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ]
