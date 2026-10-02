"""
NSIGII Python Node — Beta Layer
Peer-to-peer with Go Alpha node via FFI (ctypes) + HTTP fallback.
If Go node fails → Python continues serving independently.
Topology: X—X (peer-to-peer, no central broker)
"""

import ctypes
import json
import logging
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Optional
import urllib.request
import urllib.error

logging.basicConfig(
    level=logging.INFO,
    format="[PYTHON NODE] %(asctime)s %(levelname)s: %(message)s"
)
log = logging.getLogger(__name__)

NODE_ID = "python-node-beta"
PORT = 9002
GO_NODE_ADDR = "localhost:9001"
GO_LIB_PATH = os.environ.get("NSIGII_GO_LIB", "./nsigii_go.so")

# ─── FFI BRIDGE TO GO ────────────────────────────────────────────────────────

class GoFFIBridge:
    """
    Loads the compiled Go shared library and exposes its FFI exports.
    Falls back gracefully to HTTP if library not found.
    """
    def __init__(self, lib_path: str):
        self._lib: Optional[ctypes.CDLL] = None
        self._available = False
        try:
            self._lib = ctypes.CDLL(lib_path)
            self._lib.NSIGIISendMessage.restype = ctypes.c_char_p
            self._lib.NSIGIISendMessage.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
            self._lib.NSIGIIGetPeers.restype = ctypes.c_char_p
            self._lib.NSIGIIRegisterPeer.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
            self._available = True
            log.info("Go FFI bridge loaded from %s", lib_path)
        except OSError as e:
            log.warning("Go FFI lib not found (%s) — using HTTP fallback", e)

    @property
    def available(self) -> bool:
        return self._available

    def send_message(self, target: str, payload: str) -> dict:
        if self._available:
            result = self._lib.NSIGIISendMessage(
                target.encode(), payload.encode()
            )
            return json.loads(result.decode())
        return self._http_send(target, payload)

    def get_peers(self) -> dict:
        if self._available:
            result = self._lib.NSIGIIGetPeers()
            return json.loads(result.decode())
        return self._http_peers()

    def register_peer(self, peer_id: str, addr: str):
        if self._available:
            self._lib.NSIGIIRegisterPeer(peer_id.encode(), addr.encode())
        else:
            log.info("FFI unavailable — peer %s registered locally only", peer_id)

    # ── HTTP fallback (runs when Go .so not compiled yet) ──────────────────

    def _http_send(self, target: str, payload: str) -> dict:
        msg = {
            "node_id": NODE_ID,
            "payload": payload,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ"),
            "layer": "beta"
        }
        data = json.dumps(msg).encode()
        try:
            req = urllib.request.Request(
                f"http://{target}/receive",
                data=data,
                headers={"Content-Type": "application/json"},
                method="POST"
            )
            with urllib.request.urlopen(req, timeout=3) as resp:
                return json.loads(resp.read())
        except (urllib.error.URLError, OSError) as e:
            log.warning("HTTP send to %s failed: %s", target, e)
            return {"status": "failed", "reason": str(e)}

    def _http_peers(self) -> dict:
        try:
            with urllib.request.urlopen(
                f"http://{GO_NODE_ADDR}/peers", timeout=3
            ) as resp:
                return json.loads(resp.read())
        except Exception:
            return {}


# ─── LOCAL PEER REGISTRY (decentralised — no broker) ─────────────────────────

class PeerRegistry:
    def __init__(self):
        self._peers: dict[str, str] = {}  # id → address
        self._lock = threading.RLock()

    def register(self, peer_id: str, addr: str):
        with self._lock:
            self._peers[peer_id] = addr
            log.info("Registered peer %s @ %s", peer_id, addr)

    def remove(self, peer_id: str):
        with self._lock:
            self._peers.pop(peer_id, None)

    def all(self) -> dict:
        with self._lock:
            return dict(self._peers)

    def alive(self) -> dict:
        """Returns only peers that respond to health checks."""
        live = {}
        for pid, addr in self.all().items():
            try:
                with urllib.request.urlopen(
                    f"http://{addr}/health", timeout=2
                ) as r:
                    if r.status == 200:
                        live[pid] = addr
            except Exception:
                log.warning("Peer %s @ %s unreachable — excluded from active set", pid, addr)
        return live


# ─── HTTP SERVER (Python P2P node) ───────────────────────────────────────────

registry = PeerRegistry()
bridge = GoFFIBridge(GO_LIB_PATH)


class NSIGIIHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass  # suppress default httpd noise

    def _json(self, code: int, payload: dict):
        body = json.dumps(payload).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)
        if self.path == "/receive":
            try:
                msg = json.loads(body)
                log.info("Received from %s: %s", msg.get("node_id"), msg.get("payload"))
                # Auto-register sender
                if "node_id" in msg:
                    sender_addr = self.headers.get("X-Peer-Addr", "")
                    if sender_addr:
                        registry.register(msg["node_id"], sender_addr)
                self._json(200, {"status": "received", "node_id": NODE_ID})
            except Exception as e:
                self._json(400, {"error": str(e)})

        elif self.path == "/send":
            try:
                req = json.loads(body)
                target = req["target"]   # "node_id" or "address"
                payload = req["payload"]
                # Resolve address
                peers = registry.all()
                addr = peers.get(target, target)  # fallback: treat target as address
                result = bridge.send_message(addr, payload)
                self._json(200, result)
            except Exception as e:
                self._json(400, {"error": str(e)})

        elif self.path == "/register":
            try:
                data = json.loads(body)
                registry.register(data["node_id"], data["address"])
                bridge.register_peer(data["node_id"], data["address"])
                self._json(200, {"status": "registered"})
            except Exception as e:
                self._json(400, {"error": str(e)})

    def do_GET(self):
        if self.path == "/health":
            self._json(200, {
                "node_id": NODE_ID,
                "layer": "beta",
                "ffi_bridge": bridge.available,
                "peers": registry.all(),
                "time": time.strftime("%Y-%m-%dT%H:%M:%SZ")
            })
        elif self.path == "/peers":
            self._json(200, registry.all())
        elif self.path == "/peers/alive":
            self._json(200, registry.alive())
        else:
            self._json(404, {"error": "not found"})


# ─── HEARTBEAT (keeps peer mesh alive) ───────────────────────────────────────

def heartbeat_loop():
    """Periodically announce self to known peers — distributed discovery."""
    while True:
        time.sleep(10)
        for peer_id, addr in registry.all().items():
            try:
                msg = json.dumps({
                    "node_id": NODE_ID,
                    "payload": "HEARTBEAT",
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ"),
                    "layer": "beta"
                }).encode()
                req = urllib.request.Request(
                    f"http://{addr}/receive",
                    data=msg,
                    headers={
                        "Content-Type": "application/json",
                        "X-Peer-Addr": f"localhost:{PORT}"
                    },
                    method="POST"
                )
                urllib.request.urlopen(req, timeout=3)
                log.debug("Heartbeat sent to %s", peer_id)
            except Exception:
                log.warning("Heartbeat to %s failed — peer may be down", peer_id)


# ─── ENTRY POINT ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    # Auto-register Go node as a peer on startup
    registry.register("go-node-alpha", GO_NODE_ADDR)

    # Heartbeat thread (daemon — dies with main)
    hb = threading.Thread(target=heartbeat_loop, daemon=True)
    hb.start()

    server = HTTPServer(("0.0.0.0", PORT), NSIGIIHandler)
    log.info("NSIGII Beta Node starting on port %d", PORT)
    log.info("FFI bridge to Go: %s", "ACTIVE" if bridge.available else "HTTP FALLBACK")
    log.info("Known peers: %s", registry.all())

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        log.info("Beta node shutting down.")
        server.server_close()
