// PolyCall polycall_rpc v1 client (Node).
//
//   node inventory_client.mjs <host:port> <item_id>
//
// Frames one REQUEST for inventory.get, reads one RESPONSE, prints the reply
// payload (JSON) on stdout, exits 0. On a transport error: message on stderr,
// exit 5. It performs exactly one round trip and never retries.

import net from "node:net";

const [, , endpoint, itemId] = process.argv;
if (!endpoint || !itemId) {
  process.stderr.write("usage: inventory_client.mjs <host:port> <item_id>\n");
  process.exit(2);
}
const i = endpoint.lastIndexOf(":");
const host = endpoint.slice(0, i) || "127.0.0.1";
const port = Number(endpoint.slice(i + 1));

const MAGIC = Buffer.from("PCR1", "ascii");
const T_REQUEST = 1;
const T_RESPONSE = 2;

function frame(type, corr, payloadStr) {
  const payload = Buffer.from(payloadStr, "utf8");
  const hdr = Buffer.alloc(16);
  MAGIC.copy(hdr, 0);
  hdr.writeUInt8(type, 4);
  hdr.writeUInt8(0, 5);
  hdr.writeUInt16BE(0, 6);
  hdr.writeUInt32BE(corr >>> 0, 8);
  hdr.writeUInt32BE(payload.length, 12);
  return Buffer.concat([hdr, payload]);
}

const reqPayload = JSON.stringify({
  service: "inventory",
  operation: "get",
  deadline_ms: 2000,
  input: { item_id: itemId },
});

const sock = net.connect({ host, port }, () => {
  sock.write(frame(T_REQUEST, 1, reqPayload));
});

let buf = Buffer.alloc(0);
sock.on("data", (d) => {
  buf = Buffer.concat([buf, d]);
  if (buf.length < 16) return;
  if (buf.subarray(0, 4).toString("ascii") !== "PCR1") {
    process.stderr.write("bad frame magic\n");
    process.exit(5);
  }
  const type = buf.readUInt8(4);
  const len = buf.readUInt32BE(12);
  if (buf.length < 16 + len) return;
  const payload = buf.subarray(16, 16 + len).toString("utf8");
  sock.end();
  if (type !== T_RESPONSE) {
    process.stderr.write(`unexpected frame type ${type}\n`);
    process.exit(5);
  }
  process.stdout.write(payload + "\n");
  let ok = false;
  try { ok = JSON.parse(payload).ok === true; } catch { /* ignore */ }
  process.exit(ok ? 0 : 4);
});
sock.on("error", (e) => {
  process.stderr.write(`transport: ${e.message}\n`);
  process.exit(5);
});
