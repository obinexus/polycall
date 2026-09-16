package org.obinexus.polycall.rpcv1;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;

/**
 * polycall_rpc v1 client (see docs/RPC.md), added alongside the existing
 * (legacy-protocol) bindings/java-polycall JNI binding per docs/TODO.md P2.
 * JDK standard library only -- no external dependency.
 *
 * Frame: 16-byte header (magic "PCR1", type, flags, reserved, big-endian
 * correlation id, big-endian length) followed by a UTF-8 JSON payload. No
 * Java object is ever placed on the wire -- payloads are JSON only.
 */
public final class PolyCallRpcClient {

    public static final int TYPE_REQUEST = 1;
    public static final int TYPE_RESPONSE = 2;
    public static final int TYPE_CONTROL = 3;
    public static final int TYPE_REPLY = 4;

    private static final byte[] MAGIC = {'P', 'C', 'R', '1'};
    private static final int HEADER_LEN = 16;
    private static final int MAX_PAYLOAD = 1 << 20; // 1 MiB

    public static class TransportException extends Exception {
        public TransportException(String m, Throwable c) { super(m, c); }
        public TransportException(String m) { super(m); }
    }

    public static final class DeadlineException extends TransportException {
        public DeadlineException(String m, Throwable c) { super(m, c); }
    }

    public static final class RpcResult {
        public final boolean ok;
        public final String rawPayload;   // exactly as received on the wire
        public final String errorCode;    // null when ok

        RpcResult(boolean ok, String rawPayload, String errorCode) {
            this.ok = ok;
            this.rawPayload = rawPayload;
            this.errorCode = errorCode;
        }
    }

    private PolyCallRpcClient() { }

    private static String escape(String s) {
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': b.append("\\\""); break;
                case '\\': b.append("\\\\"); break;
                case '\n': b.append("\\n"); break;
                case '\r': b.append("\\r"); break;
                case '\t': b.append("\\t"); break;
                default:
                    if (c < 0x20) b.append(String.format("\\u%04x", (int) c));
                    else b.append(c);
            }
        }
        return b.toString();
    }

    private static void writeFrame(DataOutputStream out, int type, int corr, byte[] payload)
            throws IOException {
        out.write(MAGIC);
        out.writeByte(type);
        out.writeByte(0);           // flags
        out.writeShort(0);          // reserved
        out.writeInt(corr);
        out.writeInt(payload.length);
        out.write(payload);
        out.flush();
    }

    private static final class Frame {
        int type;
        byte[] payload;
    }

    private static Frame readFrame(DataInputStream in) throws IOException {
        byte[] hdr = new byte[HEADER_LEN];
        in.readFully(hdr);
        if (hdr[0] != 'P' || hdr[1] != 'C' || hdr[2] != 'R' || hdr[3] != '1') {
            throw new IOException("bad frame magic");
        }
        Frame f = new Frame();
        f.type = hdr[4] & 0xFF;
        long length = ((long) (hdr[12] & 0xFF) << 24) | ((hdr[13] & 0xFF) << 16)
                | ((hdr[14] & 0xFF) << 8) | (hdr[15] & 0xFF);
        if (length > MAX_PAYLOAD) throw new IOException("payload too large: " + length);
        ByteArrayOutputStream buf = new ByteArrayOutputStream((int) length);
        byte[] chunk = new byte[8192];
        long remaining = length;
        while (remaining > 0) {
            int want = (int) Math.min(chunk.length, remaining);
            int n = in.read(chunk, 0, want);
            if (n < 0) throw new IOException("peer closed mid-payload");
            buf.write(chunk, 0, n);
            remaining -= n;
        }
        f.payload = buf.toByteArray();
        return f;
    }

    /**
     * Performs exactly ONE round trip and never retries -- callers must not
     * loop this for non-idempotent operations. {@code rawInputJson} is
     * spliced verbatim as the request's "input" value (validated as JSON
     * first, but not re-serialised, so an arbitrary caller-supplied literal
     * survives byte-for-byte).
     */
    public static RpcResult call(String host, int port, String service, String operation,
                                  String rawInputJson, int deadlineMs) throws TransportException {
        if (deadlineMs <= 0) deadlineMs = 5000;
        if (rawInputJson == null || rawInputJson.isEmpty()) rawInputJson = "null";
        try {
            MiniJson.parse(rawInputJson); // validate only
        } catch (RuntimeException e) {
            throw new TransportException("invalid JSON input: " + e.getMessage(), e);
        }

        String body = "{\"service\":\"" + escape(service) + "\",\"operation\":\""
                + escape(operation) + "\",\"deadline_ms\":" + deadlineMs
                + ",\"input\":" + rawInputJson + "}";
        byte[] payload = body.getBytes(StandardCharsets.UTF_8);

        int timeoutMs = deadlineMs + 1000;
        try (Socket sock = new Socket()) {
            sock.connect(new InetSocketAddress(host, port), timeoutMs);
            sock.setSoTimeout(timeoutMs);
            DataOutputStream out = new DataOutputStream(sock.getOutputStream());
            DataInputStream in = new DataInputStream(sock.getInputStream());

            writeFrame(out, TYPE_REQUEST, 1, payload);
            Frame reply = readFrame(in);
            if (reply.type != TYPE_RESPONSE) {
                throw new TransportException("unexpected frame type " + reply.type);
            }
            String raw = new String(reply.payload, StandardCharsets.UTF_8);
            Object parsed = MiniJson.parse(raw);
            boolean ok = MiniJson.asBool(MiniJson.get(parsed, "ok"), false);
            String errorCode = null;
            if (!ok) {
                Object err = MiniJson.get(parsed, "error");
                errorCode = MiniJson.asString(MiniJson.get(err, "code"), "");
            }
            return new RpcResult(ok, raw, errorCode);
        } catch (SocketTimeoutException e) {
            throw new DeadlineException("no reply before the deadline", e);
        } catch (IOException e) {
            throw new TransportException("cannot reach the runtime: " + e.getMessage(), e);
        }
    }

    /** One CONTROL round trip (ping / describe / shutdown); returns the raw
     * CONTROL_REPLY payload exactly as received. */
    public static String control(String host, int port, String action, String authToken,
                                  int timeoutMs) throws TransportException {
        if (timeoutMs <= 0) timeoutMs = 3000;
        String body = authToken == null || authToken.isEmpty()
                ? "{\"action\":\"" + escape(action) + "\"}"
                : "{\"action\":\"" + escape(action) + "\",\"auth_token\":\""
                  + escape(authToken) + "\"}";
        byte[] payload = body.getBytes(StandardCharsets.UTF_8);
        try (Socket sock = new Socket()) {
            sock.connect(new InetSocketAddress(host, port), timeoutMs);
            sock.setSoTimeout(timeoutMs);
            DataOutputStream out = new DataOutputStream(sock.getOutputStream());
            DataInputStream in = new DataInputStream(sock.getInputStream());
            writeFrame(out, TYPE_CONTROL, 1, payload);
            Frame reply = readFrame(in);
            if (reply.type != TYPE_REPLY) {
                throw new TransportException("unexpected frame type " + reply.type);
            }
            return new String(reply.payload, StandardCharsets.UTF_8);
        } catch (SocketTimeoutException e) {
            throw new DeadlineException("no reply before the deadline", e);
        } catch (IOException e) {
            throw new TransportException("cannot reach the runtime: " + e.getMessage(), e);
        }
    }
}
