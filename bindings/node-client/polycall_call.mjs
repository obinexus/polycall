// A generic polycall_rpc v1 CLI: one round trip, no retry.
//
//   node polycall_call.mjs <host:port> <service> <operation> <json-input> [deadline_ms]
//
// Prints the RESPONSE payload exactly as received (for byte-identical
// cross-language comparison in tests/conformance/) and exits with the same
// code contract as the C CLI's `polycall call` (docs/RPC.md). Sibling of the
// inventory-specific inventory_client.mjs (kept as the Stage 3 first-proof).

import net from "node:net";

const [, , endpoint, service, operation, rawInput, rawDeadline] = process.argv;
if (!endpoint || !service || !operation) {
  process.stderr.write("usage: polycall_call.mjs <host:port> <service> <operation> <json-input> [deadline_ms]\n");
  process.exit(2);
}
const deadlineMs = rawDeadline ? Number(rawDeadline) : 5000;
let input;
try {
  input = JSON.parse(rawInput ?? "null");
} catch (e) {
  process.stderr.write(`polycall-call: invalid JSON input: ${e.message}\n`);
  process.exit(3);
}

const i = endpoint.lastIndexOf(":");
const host = endpoint.slice(0, i) || "127.0.0.1";
const port = Number(endpoint.slice(i + 1));

const EXIT = {
  "request.malformed": 3, "input.invalid": 3,
  "operation.unknown": 4, "item.unknown": 4,
  "deadline.exceeded": 6, "auth.denied": 7,
};

function frame(type, corr, payloadStr) {
  const payload = Buffer.from(payloadStr, "utf8");
  const hdr = Buffer.alloc(16);
  hdr.write("PCR1", 0, "ascii");
  hdr.writeUInt8(type, 4);
  hdr.writeUInt32BE(corr >>> 0, 8);
  hdr.writeUInt32BE(payload.length, 12);
  return Buffer.concat([hdr, payload]);
}

const reqPayload = JSON.stringify({ service, operation, deadline_ms: deadlineMs, input });
const sock = net.connect({ host, port }, () => sock.write(frame(1, 1, reqPayload)));
const timer = setTimeout(() => {
  process.stderr.write("polycall-call: no reply before the deadline\n");
  sock.destroy();
  process.exit(6);
}, deadlineMs + 1500);

let buf = Buffer.alloc(0);
sock.on("data", (d) => {
  buf = Buffer.concat([buf, d]);
  if (buf.length < 16) return;
  const len = buf.readUInt32BE(12);
  if (buf.length < 16 + len) return;
  clearTimeout(timer);
  const type = buf.readUInt8(4);
  const payload = buf.subarray(16, 16 + len).toString("utf8");
  sock.end();
  if (type !== 2) { process.stderr.write(`unexpected frame type ${type}\n`); process.exit(5); }
  process.stdout.write(payload + "\n");
  let ok = false, code = null;
  try { const o = JSON.parse(payload); ok = o.ok === true; code = o.error && o.error.code; } catch { /* ignore */ }
  process.exit(ok ? 0 : (EXIT[code] ?? 1));
});
sock.on("error", (e) => {
  clearTimeout(timer);
  process.stderr.write(`polycall-call: transport: ${e.message}\n`);
  process.exit(5);
});
