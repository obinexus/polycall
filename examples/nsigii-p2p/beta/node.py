"""NSIGII Beta node (Python), adapted onto Polycall.

An independent peer process listening on 127.0.0.1:9002 by default. It
loads libpolycall with ctypes and runs ONE Polycall peer node in-process
(its own registry and inbox). If the library is unavailable, older than
binding ABI v1, or reports another ABI, it says why and falls back to a
pure-Python implementation of the same wire protocol (polycall-peer/1) --
the NSIGII "HTTP fallback": the network keeps working.

Reference: ../reference/node.py; every change is listed in
docs/NSIGII_ADAPTATION.md. Control interface identical to alpha/main.go:

    register <id> <host:port> | unregister <id> | peers | health
    send <peer> <message-id> <base64-payload> | quit

Events: one JSON object per line on stdout.
"""
from __future__ import annotations

import argparse
import base64
import binascii
import collections
import ctypes
import hmac
import http.client
import json
import os
import re
import signal
import socket
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

_out = threading.Lock()


def emit(**ev) -> None:
    with _out:
        sys.stdout.write(json.dumps(ev, separators=(",", ":")) + "\n")
        sys.stdout.flush()


# ============================================================ native node
SIG = {
    "polycall_ffi_abi_version": (ctypes.c_int, []),
    "polycall_ffi_version": (ctypes.c_int, [ctypes.c_char_p, ctypes.c_int]),
    "polycall_strerror": (ctypes.c_char_p, [ctypes.c_int]),
    "polycall_last_error": (ctypes.c_int, [ctypes.c_char_p, ctypes.c_size_t]),
    "polycall_peer_open": (ctypes.c_int, [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,
                                          ctypes.POINTER(ctypes.c_int32)]),
    "polycall_peer_close": (ctypes.c_int, [ctypes.c_int32]),
    "polycall_peer_endpoint": (ctypes.c_int, [ctypes.c_int32, ctypes.c_char_p, ctypes.c_size_t]),
    "polycall_peer_register": (ctypes.c_int, [ctypes.c_int32, ctypes.c_char_p, ctypes.c_char_p]),
    "polycall_peer_unregister": (ctypes.c_int, [ctypes.c_int32, ctypes.c_char_p]),
    "polycall_peer_list": (ctypes.c_int, [ctypes.c_int32, ctypes.c_char_p, ctypes.c_size_t,
                                          ctypes.POINTER(ctypes.c_size_t)]),
    "polycall_peer_health": (ctypes.c_int, [ctypes.c_int32, ctypes.c_char_p, ctypes.c_size_t,
                                            ctypes.POINTER(ctypes.c_size_t)]),
    "polycall_peer_send": (ctypes.c_int, [ctypes.c_int32, ctypes.c_char_p, ctypes.c_void_p,
                                          ctypes.c_size_t, ctypes.c_char_p, ctypes.c_uint32]),
    "polycall_peer_recv": (ctypes.c_int, [ctypes.c_int32, ctypes.c_uint32, ctypes.c_char_p,
                                          ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t,
                                          ctypes.c_void_p, ctypes.c_size_t,
                                          ctypes.POINTER(ctypes.c_size_t)]),
}
E_TOO_LARGE, E_CLOSED, E_INVALID_HANDLE, E_CANCELLED = -10, -17, -3, -12


def load_library() -> tuple[ctypes.CDLL | None, str]:
    """(library, reason). The library is used only if EVERY symbol exists
    and the ABI matches; otherwise (None, why)."""
    explicit = os.environ.get("POLYCALL_LIBRARY")
    if explicit:
        names = [explicit]
    elif sys.platform == "win32":
        names = [os.path.join(d, n) for d in os.environ.get("PATH", "").split(os.pathsep)
                 for n in ("polycall.dll", "libpolycall.dll") if d and os.path.isfile(os.path.join(d, n))]
    else:
        names = ["libpolycall.so.1"]
    reasons = []
    for name in names:
        try:
            lib = ctypes.CDLL(name)
        except OSError as exc:
            reasons.append(f"{name}: {exc}")
            continue
        missing = [s for s in SIG if not hasattr(lib, s)]
        if missing:
            return None, f"{name} lacks {', '.join(missing[:3])} (not libpolycall >= 1.1)"
        for s, (res, args) in SIG.items():
            getattr(lib, s).restype, getattr(lib, s).argtypes = res, args
        if lib.polycall_ffi_abi_version() != 1:
            return None, f"{name} implements binding ABI {lib.polycall_ffi_abi_version()}, need 1"
        return lib, name
    return None, "libpolycall not found (" + "; ".join(reasons or ["no candidates"]) + ")"


class NativeNode:
    backend = "libpolycall"

    def __init__(self, lib: ctypes.CDLL, node_id: str, bind: str, token: str | None) -> None:
        self.lib = lib
        h = ctypes.c_int32()
        rc = lib.polycall_peer_open(node_id.encode(), bind.encode(),
                                    token.encode() if token else None, ctypes.byref(h))
        if rc:
            raise RuntimeError(f"peer_open: {self._err(rc)}")
        self.h = h.value
        buf = ctypes.create_string_buffer(128)       # buffer owned by Python
        lib.polycall_peer_endpoint(self.h, buf, 128)
        self.endpoint = buf.value.decode()
        v = ctypes.create_string_buffer(32)
        lib.polycall_ffi_version(v, 32)
        self.library = v.value.decode()

    def _err(self, rc: int) -> str:
        name = self.lib.polycall_strerror(rc).decode().split(":")[0]   # static string
        d = ctypes.create_string_buffer(512)
        self.lib.polycall_last_error(d, 512)
        return f"{name}: {d.value.decode('utf-8', 'replace')}"

    def _status(self, rc: int) -> tuple[str, str]:
        if rc == 0:
            return "POLYCALL_OK", ""
        name, _, detail = self._err(rc).partition(": ")
        return name, detail

    def register(self, pid: str, ep: str):
        return self._status(self.lib.polycall_peer_register(self.h, pid.encode(), ep.encode()))

    def unregister(self, pid: str):
        return self._status(self.lib.polycall_peer_unregister(self.h, pid.encode()))

    def _json(self, fn) -> dict:
        cap = 4096
        while True:
            buf, n = ctypes.create_string_buffer(cap), ctypes.c_size_t()
            rc = fn(self.h, buf, cap, ctypes.byref(n))
            if rc == E_TOO_LARGE and n.value + 1 > cap:
                cap = n.value + 1
                continue
            return json.loads(buf.value.decode()) if rc == 0 else {}

    def peers(self) -> dict:
        return self._json(self.lib.polycall_peer_list)

    def health(self) -> dict:
        return self._json(self.lib.polycall_peer_health)

    def send(self, peer: str, mid: str, data: bytes):
        return self._status(self.lib.polycall_peer_send(self.h, peer.encode(), data, len(data),
                                                        mid.encode(), 5000))

    def recv_loop(self) -> None:
        frm, mid = ctypes.create_string_buffer(64), ctypes.create_string_buffer(64)
        cap = 1 << 20
        buf, n = ctypes.create_string_buffer(cap), ctypes.c_size_t()
        while True:
            rc = self.lib.polycall_peer_recv(self.h, 0xFFFFFFFF, frm, 64, mid, 64, buf, cap,
                                             ctypes.byref(n))
            if rc == 0:
                emit(event="message", **{"from": frm.value.decode()}, id=mid.value.decode(),
                     payload_b64=base64.b64encode(ctypes.string_at(buf, n.value)).decode())
            elif rc in (E_CLOSED, E_INVALID_HANDLE, E_CANCELLED):
                return
            else:
                name, detail = self._status(rc)
                emit(event="error", op="recv", status=name, detail=detail)

    def close(self) -> None:
        self.lib.polycall_peer_close(self.h)


# ======================================================= HTTP fallback node
ID = re.compile(r"^[A-Za-z0-9._-]{1,63}$")
EP = re.compile(r"^[A-Za-z0-9._-]+:[0-9]{1,5}$")


class FallbackNode:
    """polycall-peer/1 in pure Python (docs/PEER_PROTOCOL.md)."""

    backend = "python-fallback"
    library = None

    def __init__(self, node_id: str, bind: str, token: str | None) -> None:
        self.node_id, self.token = node_id, token or ""
        self.mu = threading.Condition()
        self.registry: dict[str, str] = {}
        self.inbox: collections.deque = collections.deque()
        self.seen: collections.OrderedDict = collections.OrderedDict()
        self.closed = False
        node = self

        class H(BaseHTTPRequestHandler):
            protocol_version, timeout = "HTTP/1.1", 10

            def log_message(self, *a):         # never log bodies or tokens
                pass

            def reply(self, code: int, obj: dict) -> None:
                body = json.dumps(obj).encode()
                self.send_response(code)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.send_header("Connection", "close")
                self.end_headers()
                self.wfile.write(body)

            def fail(self, code: int, err: str, msg: str) -> None:
                self.reply(code, {"ok": False, "error": {"code": err, "message": msg}})

            def auth(self) -> bool:
                got = self.headers.get("Authorization", "")
                return not node.token or (got.startswith("Bearer ") and
                                          hmac.compare_digest(got[7:].encode(), node.token.encode()))

            def do_GET(self):  # noqa: N802
                if self.path == "/health":
                    with node.mu:
                        self.reply(200, {"ok": True, "protocol": "polycall-peer/1", "node_id": node.node_id,
                                         "status": "ok", "peers": len(node.registry), "inbox": len(node.inbox)})
                elif self.path == "/peers":
                    if not self.auth():
                        return self.fail(401, "auth.required", "token required")
                    with node.mu:
                        self.reply(200, {"ok": True, "node_id": node.node_id, "peers": dict(node.registry)})
                elif self.path in ("/receive", "/register", "/inbox/next"):
                    self.fail(405, "http.method", "use POST")
                else:
                    self.fail(404, "http.not_found", "unknown path")

            def do_POST(self):  # noqa: N802
                if self.path not in ("/receive", "/register", "/inbox/next"):
                    return self.fail(404 if self.path not in ("/health", "/peers") else 405,
                                     "http.rejected", "unknown path or method")
                if not self.auth():
                    return self.fail(401, "auth.required", "token required")
                if self.headers.get("Transfer-Encoding"):
                    return self.fail(501, "http.rejected", "Transfer-Encoding not supported")
                cl = self.headers.get("Content-Length")
                if cl is None or not cl.isdigit():
                    return self.fail(411, "http.rejected", "Content-Length required")
                if int(cl) > (2 << 20):
                    return self.fail(413, "http.rejected", "body too large")
                try:
                    obj = json.loads(self.rfile.read(int(cl)))
                except ValueError:
                    obj = None                      # the reference ignored this
                if self.path != "/receive":
                    return self.fail(501, "http.rejected", "not offered by this fallback node")
                if not isinstance(obj, dict) or obj.get("v") != 1:
                    return self.fail(400, "request.malformed", "body must be {\"v\":1,...}")
                frm, mid, b64 = obj.get("from"), obj.get("id"), obj.get("payload_b64")
                if not (isinstance(frm, str) and ID.match(frm) and isinstance(mid, str) and ID.match(mid)
                        and isinstance(b64, str)):
                    return self.fail(400, "request.malformed", "need from, id, payload_b64")
                try:
                    data = base64.b64decode(b64, validate=True)
                except (binascii.Error, ValueError):
                    return self.fail(400, "request.malformed", "payload_b64 is not base64")
                if len(data) > (1 << 20):
                    return self.fail(413, "payload.too_large", "payload exceeds 1 MiB")
                with node.mu:
                    dup = (frm, mid) in node.seen
                    if not dup:
                        if len(node.inbox) >= 256:
                            return self.fail(503, "inbox.full", "retry later with the same id")
                        node.inbox.append((frm, mid, data))
                        node.seen[(frm, mid)] = True
                        while len(node.seen) > 1024:
                            node.seen.popitem(last=False)
                        node.mu.notify_all()
                self.reply(200, {"ok": True, "status": "received", "node_id": node.node_id,
                                 "id": mid, "duplicate": dup})

        host, _, port = bind.rpartition(":")
        self.server = ThreadingHTTPServer((host, int(port)), H)
        self.server.daemon_threads = True
        self.endpoint = f"{host}:{self.server.server_address[1]}"
        threading.Thread(target=self.server.serve_forever, kwargs={"poll_interval": 0.2},
                         daemon=True).start()

    def register(self, pid: str, ep: str):
        if not ID.match(pid) or not EP.match(ep):
            return "POLYCALL_E_INVALID_ARGUMENT", "bad id or endpoint"
        with self.mu:
            self.registry[pid] = ep
        return "POLYCALL_OK", ""

    def unregister(self, pid: str):
        with self.mu:
            return ("POLYCALL_OK", "") if self.registry.pop(pid, None) else ("POLYCALL_E_NOT_FOUND", pid)

    def peers(self) -> dict:
        with self.mu:
            return dict(self.registry)

    def health(self) -> dict:
        with self.mu:
            return {"node_id": self.node_id, "status": "ok", "peers": len(self.registry),
                    "inbox": len(self.inbox), "implementation": "nsigii beta python fallback"}

    def send(self, peer: str, mid: str, data: bytes):
        with self.mu:
            ep, expected = self.registry.get(peer), peer if peer in self.registry else None
        ep = ep or peer
        if ":" not in ep:
            return "POLYCALL_E_NOT_FOUND", f"peer {peer!r} not registered"
        host, _, port = ep.rpartition(":")
        body = json.dumps({"v": 1, "id": mid, "from": self.node_id,
                           "payload_b64": base64.b64encode(data).decode()}).encode()
        headers = {"Content-Type": "application/json", "Connection": "close"}
        if self.token:
            headers["Authorization"] = "Bearer " + self.token
        conn = http.client.HTTPConnection(host, int(port), timeout=5)
        try:                                        # the reference sent no body
            conn.request("POST", "/receive", body=body, headers=headers)
            resp = conn.getresponse()
            status, raw = resp.status, resp.read(65536)
        except socket.timeout:
            return "POLYCALL_E_TIMEOUT", f"{ep} did not answer"
        except OSError as exc:
            return "POLYCALL_E_TRANSPORT", str(exc)
        finally:
            conn.close()                            # the reference leaked responses
        try:
            ack = json.loads(raw)
        except ValueError:
            ack = {}
        if status != 200:
            return {401: "POLYCALL_E_AUTH", 503: "POLYCALL_E_BUSY"}.get(status, "POLYCALL_E_REMOTE"), f"HTTP {status}"
        if ack.get("ok") is not True or ack.get("id") != mid or (expected and ack.get("node_id") != expected):
            return "POLYCALL_E_PROTOCOL", "not acknowledged by the expected node"
        return "POLYCALL_OK", ""

    def recv_loop(self) -> None:
        while True:
            with self.mu:
                while not self.inbox and not self.closed:
                    self.mu.wait()
                if self.closed:
                    return
                frm, mid, data = self.inbox.popleft()
            emit(event="message", **{"from": frm}, id=mid, payload_b64=base64.b64encode(data).decode())

    def close(self) -> None:
        with self.mu:
            self.closed = True
            self.mu.notify_all()
        self.server.shutdown()
        self.server.server_close()


# ================================================================== main
def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--id", default="python-node-beta")
    ap.add_argument("--listen", default="127.0.0.1:9002")
    ap.add_argument("--peer", action="append", default=[], help="ID=HOST:PORT")
    ap.add_argument("--token-env", default="POLYCALL_DEV_TOKEN")
    ap.add_argument("--backend", choices=["auto", "native", "python"], default="auto")
    a = ap.parse_args()
    token = os.environ.get(a.token_env) or None
    lib, why = (None, "python backend requested") if a.backend == "python" else load_library()
    if lib is None and a.backend == "native":
        emit(event="error", op="load", detail=why)
        return 4
    if lib is None:
        print(f"[beta] libpolycall unavailable -- HTTP fallback: {why}", file=sys.stderr, flush=True)
        node = FallbackNode(a.id, a.listen, token)
    else:
        node = NativeNode(lib, a.id, a.listen, token)
    for spec in a.peer:
        pid, _, ep = spec.partition("=")
        st, d = node.register(pid, ep)
        emit(event="registered", peer=pid, endpoint=ep, status=st, **({"detail": d} if d else {}))
    emit(event="listening", node_id=a.id, endpoint=node.endpoint, backend=node.backend,
         library=node.library, language="python", **({"fallback_reason": why} if lib is None else {}))
    rx = threading.Thread(target=node.recv_loop, daemon=True)
    rx.start()
    stop = threading.Event()
    signal.signal(signal.SIGTERM, lambda *_: stop.set())
    for line in sys.stdin:
        f = line.split()
        if not f:
            continue
        if f[0] == "quit" or stop.is_set():
            break
        if f[0] == "register" and len(f) == 3:
            st, d = node.register(f[1], f[2])
            emit(event="registered", peer=f[1], endpoint=f[2], status=st)
        elif f[0] == "unregister" and len(f) == 2:
            st, d = node.unregister(f[1])
            emit(event="unregistered", peer=f[1], status=st)
        elif f[0] == "peers":
            emit(event="peers", peers=node.peers(), status="POLYCALL_OK")
        elif f[0] == "health":
            emit(event="health", health=node.health(), status="POLYCALL_OK")
        elif f[0] == "send" and len(f) in (3, 4):
            try:
                data = base64.b64decode(f[3] if len(f) == 4 else "", validate=True)
            except (binascii.Error, ValueError):
                emit(event="error", op="send", detail="payload is not base64")
                continue
            st, d = node.send(f[1], f[2], data)
            emit(event="sent", to=f[1], id=f[2], status=st, **({"detail": d} if d else {}))
    node.close()
    rx.join(5)
    emit(event="stopped", node_id=a.id)
    return 0


if __name__ == "__main__":
    sys.exit(main())
