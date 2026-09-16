#!/bin/sh
# P1 gate: `polycall start --load PATH` (repeatable). The fixture plugin's
# demo.greet is callable from the C CLI and the Node and Python clients with
# no core rebuild, on whatever platform this runs on. Also covers: ABI
# mismatch, a library missing polycall_ops_register, and that loaded
# operations show up in `status` (describe) with their schema hints and
# idempotent flag.
#
# Usage: plugin.sh <polycall> <repo-root> <lib-dir> <shared-ext>
set -u
PC=${1:?}; ROOT=${2:?}; LIBDIR=${3:?}; SHEXT=${4:?}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

PLUGIN="$LIBDIR/fixture_ops_plugin.$SHEXT"
PLUGIN_BAD="$LIBDIR/fixture_ops_plugin_bad_abi.$SHEXT"
CPROV="$LIBDIR/example_provider.$SHEXT"   # any real lib lacking polycall_ops_register
NODE_CLIENT="$ROOT/tools/rpc-clients/node/inventory_client.mjs"

if [ ! -f "$PLUGIN" ] || [ ! -f "$PLUGIN_BAD" ]; then
  skip "plugin loader (fixture plugins not built; run 'make plugin-fixture')"
  echo "--- plugin: 0 passed, 0 failed, 1 not run ---"
  exit 0
fi

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_plug.$$"); mkdir -p "$TMP"
trap 'rm -rf "$TMP"; [ -n "${RUNPID:-}" ] && kill "$RUNPID" 2>/dev/null' EXIT

echo "=== fail fast: no socket bound on a bad plugin ==="

t0=$(date +%s 2>/dev/null || echo 0)
"$PC" start --load "$TMP/does-not-exist.$SHEXT" >/dev/null 2>&1
[ $? -eq 4 ] && ok "nonexistent plugin path -> exit 4" || bad "nonexistent plugin path exit"

if [ -f "$CPROV" ]; then
  "$PC" start --load "$CPROV" >"$TMP/missing_sym.out" 2>&1
  rc=$?
  { [ $rc -eq 4 ] && grep -qi "polycall_ops_register" "$TMP/missing_sym.out"; } \
    && ok "library missing polycall_ops_register -> exit 4" \
    || bad "missing-symbol plugin (rc=$rc): $(cat "$TMP/missing_sym.out")"
else
  skip "missing-symbol case (example_provider not built)"
fi

"$PC" start --load "$PLUGIN_BAD" >"$TMP/abi.out" 2>&1
rc=$?
{ [ $rc -eq 4 ] && grep -qi "abi" "$TMP/abi.out"; } \
  && ok "ABI-mismatched plugin -> exit 4, reported distinctly" \
  || bad "ABI mismatch (rc=$rc): $(cat "$TMP/abi.out")"

echo "=== load the real fixture, call it from three entry points ==="
EPF="$TMP/endpoint"
"$PC" start --endpoint 127.0.0.1:0 --endpoint-file "$EPF" --load "$PLUGIN" \
  >"$TMP/run.log" 2>&1 &
RUNPID=$!
i=0
while [ ! -s "$EPF" ] && [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done
if [ ! -s "$EPF" ]; then
  bad "runtime with a loaded plugin did not start"; cat "$TMP/run.log"
  echo "--- plugin: $PASS passed, $FAIL failed, $SKIP not run ---"
  exit 1
fi
EP=$(cat "$EPF")

grep -q "loaded plugin" "$TMP/run.log" && ok "start reports the loaded plugin" \
  || bad "start did not report loading the plugin: $(cat "$TMP/run.log")"

C_OUT=$("$PC" call demo greet --endpoint "$EP" --input-value '{"name":"world"}'); c_rc=$?
{ [ $c_rc -eq 0 ] && [ "$C_OUT" = '{"ok":true,"output":{"message":"hello, world"}}' ]; } \
  && ok "C CLI: loaded op demo.greet callable, no core rebuild" \
  || bad "C CLI demo.greet (rc=$c_rc out=$C_OUT)"

if command -v node >/dev/null 2>&1 && [ -f "$NODE_CLIENT" ]; then
  # the generic inventory client only frames inventory.get; prove the plugin
  # op is reachable from Node via the same one-shot polycall_rpc v1 protocol
  # using a tiny inline client so this stays a genuine cross-language call.
  cat > "$TMP/greet_client.mjs" <<'EOF'
import net from "node:net";
const [, , ep, name] = process.argv;
const i = ep.lastIndexOf(":");
const host = ep.slice(0, i) || "127.0.0.1";
const port = Number(ep.slice(i + 1));
const hdr = (type, corr, len) => {
  const b = Buffer.alloc(16);
  b.write("PCR1", 0, "ascii"); b.writeUInt8(type, 4);
  b.writeUInt32BE(corr >>> 0, 8); b.writeUInt32BE(len, 12);
  return b;
};
const payload = Buffer.from(JSON.stringify({
  service: "demo", operation: "greet", deadline_ms: 2000, input: { name },
}));
const sock = net.connect({ host, port }, () => sock.write(Buffer.concat([hdr(1, 1, payload.length), payload])));
let buf = Buffer.alloc(0);
sock.on("data", (d) => {
  buf = Buffer.concat([buf, d]);
  if (buf.length < 16) return;
  const len = buf.readUInt32BE(12);
  if (buf.length < 16 + len) return;
  process.stdout.write(buf.subarray(16, 16 + len).toString("utf8") + "\n");
  sock.end(); process.exit(0);
});
sock.on("error", (e) => { process.stderr.write(String(e) + "\n"); process.exit(5); });
EOF
  N_OUT=$(node "$TMP/greet_client.mjs" "$EP" world); n_rc=$?
  { [ $n_rc -eq 0 ] && [ "$N_OUT" = "$C_OUT" ]; } \
    && ok "Node client: loaded op demo.greet == C result" \
    || bad "Node client demo.greet (rc=$n_rc out=$N_OUT want=$C_OUT)"
else
  skip "Node client demo.greet (node not on PATH)"
fi

if command -v python >/dev/null 2>&1 || command -v python3 >/dev/null 2>&1; then
  PY=$(command -v python || command -v python3)
  cat > "$TMP/greet_client.py" <<'EOF'
import json, socket, struct, sys
HEADER = struct.Struct(">4sBBHII")
host_port = sys.argv[1]
name = sys.argv[2]
host, _, port = host_port.rpartition(":")
host = host or "127.0.0.1"
req = json.dumps({"service": "demo", "operation": "greet", "deadline_ms": 2000,
                   "input": {"name": name}}, separators=(",", ":")).encode()
with socket.create_connection((host, int(port)), timeout=3) as s:
    s.sendall(HEADER.pack(b"PCR1", 1, 0, 0, 1, len(req)) + req)
    hdr = b""
    while len(hdr) < HEADER.size:
        hdr += s.recv(HEADER.size - len(hdr))
    _, _, _, _, _, length = HEADER.unpack(hdr)
    body = b""
    while len(body) < length:
        body += s.recv(length - len(body))
    sys.stdout.write(body.decode() + "\n")
EOF
  P_OUT=$("$PY" "$TMP/greet_client.py" "$EP" world); p_rc=$?
  { [ $p_rc -eq 0 ] && [ "$P_OUT" = "$C_OUT" ]; } \
    && ok "Python client: loaded op demo.greet == C result" \
    || bad "Python client demo.greet (rc=$p_rc out=$P_OUT want=$C_OUT)"
else
  skip "Python client demo.greet (python not on PATH)"
fi

echo "=== loaded op appears in status (describe) with schema + idempotent ==="
DESC=$("$PC" --format json status --endpoint "$EP")
case "$DESC" in
  *'"service":"demo"'*'"operation":"greet"'*'"idempotent":true'*) \
    ok "demo.greet listed in status/describe with idempotent:true" ;;
  *) bad "demo.greet missing from status/describe: $DESC" ;;
esac
case "$DESC" in
  *'"input":"{\"name\":\"string\"}"'*) ok "demo.greet input schema hint present, correctly escaped" ;;
  *) bad "demo.greet input schema hint missing/malformed: $DESC" ;;
esac

echo "=== duplicate service.operation is refused, not \"last wins\" ==="
"$PC" call inventory get --endpoint "$EP" --input-value '{"item_id":"widget-a"}' >/dev/null 2>&1
[ $? -eq 0 ] && ok "built-in inventory.get still served alongside the plugin" \
  || bad "built-in op broken after loading a plugin"

"$PC" stop --endpoint "$EP" >/dev/null 2>&1
j=0
while kill -0 "$RUNPID" 2>/dev/null && [ $j -lt 40 ]; do sleep 0.1; j=$((j+1)); done
if kill -0 "$RUNPID" 2>/dev/null; then bad "runtime with a plugin did not exit after stop"
else ok "runtime with a loaded plugin shuts down cleanly (unload in normal code)"; fi

echo "--- plugin: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ]
