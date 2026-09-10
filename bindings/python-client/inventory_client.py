#!/usr/bin/env python3
"""PolyCall polycall_rpc v1 client (Python, stdlib only).

    python inventory_client.py <host:port> <item_id>

Frames one REQUEST for inventory.get, reads one RESPONSE, prints the reply
payload (JSON) on stdout, exits 0. Transport error: message on stderr, exit 5.
Exactly one round trip; never retries.
"""

import json
import socket
import struct
import sys

MAGIC = b"PCR1"
T_REQUEST = 1
T_RESPONSE = 2
HEADER = struct.Struct(">4sBBHII")  # magic, type, flags, rsvd, corr, length


def frame(msg_type: int, corr: int, payload: bytes) -> bytes:
    return HEADER.pack(MAGIC, msg_type, 0, 0, corr, len(payload)) + payload


def recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("peer closed")
        buf += chunk
    return buf


def main() -> int:
    if len(sys.argv) != 3:
        sys.stderr.write("usage: inventory_client.py <host:port> <item_id>\n")
        return 2
    endpoint, item_id = sys.argv[1], sys.argv[2]
    host, _, port = endpoint.rpartition(":")
    host = host or "127.0.0.1"

    req = json.dumps(
        {
            "service": "inventory",
            "operation": "get",
            "deadline_ms": 2000,
            "input": {"item_id": item_id},
        },
        separators=(",", ":"),
    ).encode("utf-8")

    try:
        with socket.create_connection((host, int(port)), timeout=3) as sock:
            sock.sendall(frame(T_REQUEST, 1, req))
            magic, mtype, _flags, _rsvd, _corr, length = HEADER.unpack(
                recv_exact(sock, HEADER.size)
            )
            if magic != MAGIC:
                sys.stderr.write("bad frame magic\n")
                return 5
            payload = recv_exact(sock, length).decode("utf-8") if length else ""
    except (OSError, ConnectionError) as e:
        sys.stderr.write(f"transport: {e}\n")
        return 5

    if mtype != T_RESPONSE:
        sys.stderr.write(f"unexpected frame type {mtype}\n")
        return 5

    sys.stdout.write(payload + "\n")
    try:
        return 0 if json.loads(payload).get("ok") is True else 4
    except json.JSONDecodeError:
        return 5


if __name__ == "__main__":
    raise SystemExit(main())
