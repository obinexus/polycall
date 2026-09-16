#!/usr/bin/env python3
"""A generic polycall_rpc v1 CLI: one round trip, no retry.

    python polycall_call.py <host:port> <service> <operation> <json-input> [deadline_ms]

Prints the RESPONSE payload exactly as received (for byte-identical
cross-language comparison in tests/conformance/) and exits with the same
code contract as the C CLI's `polycall call` (docs/RPC.md). Sibling of the
inventory-specific inventory_client.py (kept as the Stage 3 first-proof).
"""

import json
import socket
import struct
import sys

MAGIC = b"PCR1"
HEADER = struct.Struct(">4sBBHII")

EXIT_FOR = {
    "request.malformed": 3, "input.invalid": 3,
    "operation.unknown": 4, "item.unknown": 4,
    "deadline.exceeded": 6, "auth.denied": 7,
}


def recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("peer closed")
        buf += chunk
    return buf


def main() -> int:
    if len(sys.argv) < 4 or len(sys.argv) > 6:
        sys.stderr.write(
            "usage: polycall_call.py <host:port> <service> <operation> <json-input> [deadline_ms]\n")
        return 2
    endpoint, service, operation = sys.argv[1], sys.argv[2], sys.argv[3]
    raw_input = sys.argv[4] if len(sys.argv) >= 5 else "null"
    deadline_ms = int(sys.argv[5]) if len(sys.argv) == 6 else 5000

    try:
        input_value = json.loads(raw_input)
    except json.JSONDecodeError as e:
        sys.stderr.write(f"polycall-call: invalid JSON input: {e}\n")
        return 3

    host, _, port = endpoint.rpartition(":")
    host = host or "127.0.0.1"
    req = json.dumps(
        {"service": service, "operation": operation, "deadline_ms": deadline_ms,
         "input": input_value},
        separators=(",", ":"),
    ).encode("utf-8")

    try:
        with socket.create_connection((host, int(port)), timeout=(deadline_ms + 1500) / 1000) as sock:
            sock.sendall(HEADER.pack(MAGIC, 1, 0, 0, 1, len(req)) + req)
            magic, mtype, _f, _r, _c, length = HEADER.unpack(recv_exact(sock, HEADER.size))
            if magic != MAGIC:
                sys.stderr.write("polycall-call: bad frame magic\n")
                return 5
            payload = recv_exact(sock, length).decode("utf-8") if length else ""
    except socket.timeout:
        sys.stderr.write("polycall-call: no reply before the deadline\n")
        return 6
    except OSError as e:
        sys.stderr.write(f"polycall-call: transport: {e}\n")
        return 5

    if mtype != 2:
        sys.stderr.write(f"polycall-call: unexpected frame type {mtype}\n")
        return 5

    sys.stdout.write(payload + "\n")
    try:
        obj = json.loads(payload)
    except json.JSONDecodeError:
        return 1
    if obj.get("ok") is True:
        return 0
    code = (obj.get("error") or {}).get("code")
    return EXIT_FOR.get(code, 1)


if __name__ == "__main__":
    raise SystemExit(main())
