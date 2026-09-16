// MicroVM <-> PolyCall adapter, STRICT mode (Node).
//
// Bounded, optional follow-up (design milestone 5). The reference
// microvm/polycall_adapter.py `invoke` returns an *accepted echo* without ever
// touching the native runtime -- that must NOT count as a passed integration.
//
// Here:
//   - strict:true  -> a real polycall_rpc v1 round trip to a running runtime.
//                     No runtime  => throws PolyCallRuntimeUnavailable. A
//                     missing runtime is a FAILURE, never a pass.
//   - strict:false -> an explicitly labelled demo fallback. The result carries
//                     {"_demo_fallback":true} so no caller can mistake it for a
//                     native call.
//
// This adapter adds no VM/container requirement and no telephony surface.

import net from "node:net";

export class PolyCallRuntimeUnavailable extends Error {}

const MAGIC = Buffer.from("PCR1", "ascii");
const T_REQUEST = 1;
const T_RESPONSE = 2;

function frame(type, corr, payloadStr) {
  const payload = Buffer.from(payloadStr, "utf8");
  const hdr = Buffer.alloc(16);
  MAGIC.copy(hdr, 0);
  hdr.writeUInt8(type, 4);
  hdr.writeUInt32BE(corr >>> 0, 8);
  hdr.writeUInt32BE(payload.length, 12);
  return Buffer.concat([hdr, payload]);
}

function roundtrip(host, port, reqObj, deadlineMs) {
  return new Promise((resolve, reject) => {
    const sock = net.connect({ host, port });
    let buf = Buffer.alloc(0);
    const timer = setTimeout(() => {
      sock.destroy();
      reject(new PolyCallRuntimeUnavailable("no reply before the deadline"));
    }, deadlineMs + 500);
    sock.on("connect", () => sock.write(frame(T_REQUEST, 1, JSON.stringify(reqObj))));
    sock.on("data", (d) => {
      buf = Buffer.concat([buf, d]);
      if (buf.length < 16) return;
      const len = buf.readUInt32BE(12);
      if (buf.length < 16 + len) return;
      clearTimeout(timer);
      const type = buf.readUInt8(4);
      const payload = buf.subarray(16, 16 + len).toString("utf8");
      sock.end();
      if (type !== T_RESPONSE) return reject(new Error(`unexpected frame ${type}`));
      resolve(JSON.parse(payload));
    });
    sock.on("error", (e) =>
      reject(new PolyCallRuntimeUnavailable(`cannot reach the native runtime: ${e.message}`)));
  });
}

/**
 * @param {{service:string, operation:string, input?:object}} call
 * @param {{strict?:boolean, endpoint?:string, deadlineMs?:number}} [opts]
 */
export async function invoke(call, opts = {}) {
  const { service, operation, input = {} } = call;
  const strict = opts.strict === true;
  const deadlineMs = opts.deadlineMs ?? 2000;

  if (!strict) {
    return {
      _demo_fallback: true,
      note: "NOT a native call; pass {strict:true} with a running `polycall run`",
      service,
      operation,
      input,
    };
  }

  const ep = opts.endpoint || process.env.POLYCALL_ENDPOINT;
  if (!ep) {
    throw new PolyCallRuntimeUnavailable(
      "strict mode needs an endpoint (opts.endpoint or POLYCALL_ENDPOINT)");
  }
  const i = ep.lastIndexOf(":");
  const host = ep.slice(0, i) || "127.0.0.1";
  const port = Number(ep.slice(i + 1));

  const reply = await roundtrip(
    host, port,
    { service, operation, deadline_ms: deadlineMs, input },
    deadlineMs);

  if (reply.ok !== true) {
    const err = new Error(reply.error?.message || "operation failed");
    err.code = reply.error?.code;
    throw err;
  }
  return reply.output;   // a real value from the real C operation
}

// CLI: node strict_adapter.mjs <service> <operation> <json-input> [--strict]
if (import.meta.url === `file://${process.argv[1]}` ||
    process.argv[1]?.endsWith("strict_adapter.mjs")) {
  const [, , svc, op, rawInput, flag] = process.argv;
  if (!svc || !op) {
    process.stderr.write("usage: strict_adapter.mjs <service> <operation> <json> [--strict]\n");
    process.exit(2);
  }
  const call = { service: svc, operation: op, input: rawInput ? JSON.parse(rawInput) : {} };
  invoke(call, { strict: flag === "--strict" })
    .then((out) => { process.stdout.write(JSON.stringify(out) + "\n"); })
    .catch((e) => {
      process.stderr.write(
        `${e instanceof PolyCallRuntimeUnavailable ? "RUNTIME-UNAVAILABLE" : "ERROR"}: ${e.message}\n`);
      process.exit(e instanceof PolyCallRuntimeUnavailable ? 5 : 1);
    });
}
