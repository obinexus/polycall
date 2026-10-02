#!/usr/bin/env python3
"""NSIGII two-process test: Go Alpha (cgo -> libpolycall) and Python Beta
(ctypes -> libpolycall, or its HTTP fallback) as separate OS processes.

Every delivery is checked at the RECEIVING process: the exact payload bytes,
the sender's node id and the message id it printed -- never the sender's
status alone.

usage: nsigii_test.py --root SRC --include DIR --libdir DIR --shared-lib FILE
                      [--import-lib FILE] [--proxy EXE]
Exit: 0 all passed, 1 a check failed, 77 a prerequisite (go/cgo) missing.
"""
from __future__ import annotations

import argparse
import base64
import json
import os
import queue
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time

PASS = FAIL = 0


def ok(msg: str) -> None:
    global PASS
    PASS += 1
    print(f"PASS  {msg}", flush=True)


def bad(msg: str) -> None:
    global FAIL
    FAIL += 1
    print(f"FAIL  {msg}", flush=True)


def check(cond: bool, msg: str) -> bool:
    (ok if cond else bad)(msg)
    return cond


class Node:
    """A peer process driven over stdin; its stdout JSON events are queued."""

    def __init__(self, argv: list[str], env: dict, name: str) -> None:
        self.name = name
        self.p = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, env=env)
        self.events: queue.Queue = queue.Queue()
        self.stderr: list[str] = []
        threading.Thread(target=self._read, daemon=True).start()
        threading.Thread(target=self._read_err, daemon=True).start()
        ev = self.wait(lambda e: e["event"] in ("listening", "error"), 30)
        if not ev or ev["event"] != "listening":
            raise RuntimeError(f"{name} did not start: {ev} {''.join(self.stderr)}")
        self.info = ev
        self.endpoint = ev["endpoint"]

    def _read(self) -> None:
        for line in self.p.stdout:
            try:
                self.events.put(json.loads(line))
            except ValueError:
                pass

    def _read_err(self) -> None:
        for line in self.p.stderr:
            self.stderr.append(line.decode("utf-8", "replace"))

    def cmd(self, line: str) -> None:
        self.p.stdin.write((line + "\n").encode())
        self.p.stdin.flush()

    def wait(self, pred, timeout: float):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            try:
                e = self.events.get(timeout=max(0.01, end - time.monotonic()))
            except queue.Empty:
                break
            if pred(e):
                return e
        return None

    def ask(self, line: str, event: str, timeout: float = 10):
        self.cmd(line)
        return self.wait(lambda e: e["event"] == event, timeout)

    def send(self, peer: str, mid: str, data: bytes, timeout: float = 15):
        b64 = base64.b64encode(data).decode()
        self.cmd(f"send {peer} {mid} {b64}" if b64 else f"send {peer} {mid}")
        return self.wait(lambda e: e["event"] == "sent" and e.get("id") == mid, timeout)

    def message(self, timeout: float = 10):
        return self.wait(lambda e: e["event"] == "message", timeout)

    def kill(self) -> None:
        self.p.kill()
        self.p.wait(10)

    def stop(self) -> None:
        if self.p.poll() is None:
            try:
                self.cmd("quit")
                self.p.wait(10)
            except (OSError, subprocess.TimeoutExpired):
                self.kill()


def raw_http(endpoint: str, req: bytes) -> bytes:
    host, port = endpoint.rsplit(":", 1)
    with socket.create_connection((host, int(port)), timeout=5) as s:
        s.sendall(req)
        out = b""
        while chunk := s.recv(4096):
            out += chunk
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--include", required=True)
    ap.add_argument("--libdir", required=True)
    ap.add_argument("--shared-lib", required=True)
    ap.add_argument("--import-lib")
    ap.add_argument("--proxy")
    a = ap.parse_args()

    go = shutil.which("go")
    if not go:
        print("NOT RUN  NSIGII two-process test (go not on PATH)")
        return 77
    work = tempfile.mkdtemp(prefix="nsigii-")
    win = sys.platform == "win32"
    alpha_exe = os.path.join(work, "alpha.exe" if win else "alpha")
    libdir = a.libdir
    if a.import_lib and win:                 # GNU ld reads MSVC import libraries
        libdir = os.path.join(work, "implib")
        os.makedirs(libdir)
        shutil.copy(a.import_lib, os.path.join(libdir, "libpolycall.dll.a"))
    shdir = os.path.dirname(os.path.abspath(a.shared_lib))
    env = dict(os.environ)
    if os.path.basename(env.get("CC", "")).lower() in ("cl", "cl.exe"):
        env.pop("CC")                        # cgo needs a GCC-compatible compiler
    if win:
        # cgo on Windows needs a current MinGW-w64 toolchain: with TDM-GCC 10's
        # binutils the Go linker emits a PE image Windows refuses to load
        # ("Exec format error"). MSYS2 UCRT64 GCC is the tested toolchain.
        cgo_cc = os.environ.get("NSIGII_CGO_CC") or r"C:\msys64\ucrt64\bin\gcc.exe"
        if os.path.exists(cgo_cc):
            env["CC"] = cgo_cc
            env["PATH"] = os.path.dirname(cgo_cc) + os.pathsep + env.get("PATH", "")
    env["CGO_ENABLED"] = "1"
    env["CGO_CFLAGS"] = f"-I{a.include}"
    env["CGO_LDFLAGS"] = f"-L{libdir}" + ("" if win else f" -Wl,-rpath,{shdir}")
    build = subprocess.run([go, "build", "-o", alpha_exe, "."], cwd=os.path.join(
        a.root, "examples", "nsigii-p2p", "alpha"), env=env, capture_output=True, text=True)
    if build.returncode != 0:
        if "cgo" in build.stderr and ("gcc" in build.stderr or "C compiler" in build.stderr):
            print(f"NOT RUN  NSIGII two-process test (cgo needs a C compiler): {build.stderr.strip()}")
            return 77
        bad(f"Go Alpha does not build: {build.stderr.strip()}")
        return 1
    ok("Go Alpha (cgo -> libpolycall) builds")

    token = base64.b16encode(os.urandom(12)).decode()
    nenv = dict(os.environ, POLYCALL_DEV_TOKEN=token)
    if win:
        nenv["PATH"] = shdir + os.pathsep + nenv.get("PATH", "")
    beta_py = os.path.join(a.root, "examples", "nsigii-p2p", "beta", "node.py")

    def alpha(listen: str = "127.0.0.1:0") -> Node:
        return Node([alpha_exe, "-listen", listen], nenv, "alpha")

    def beta(listen: str = "127.0.0.1:0", library: str | None = a.shared_lib) -> Node:
        e = dict(nenv)
        e["POLYCALL_LIBRARY"] = library or os.path.join(work, "no-such-libpolycall")
        return Node([sys.executable, beta_py, "--listen", listen], e, "beta")

    nodes: list[Node] = []
    try:
        A, B = alpha(), beta()
        nodes += [A, B]
        print(f"== Alpha pid {A.p.pid} at {A.endpoint} [{A.info['backend']}], "
              f"Beta pid {B.p.pid} at {B.endpoint} [{B.info['backend']}]")
        check(A.p.pid != B.p.pid and A.p.poll() is None and B.p.poll() is None,
              "two distinct, running OS processes")
        check(A.info["backend"] == "libpolycall" and B.info["backend"] == "libpolycall",
              "FFI available: both nodes run libpolycall (Go via cgo, Python via ctypes)")

        # ---- registry ownership: explicit, per process -----------------------
        A.ask(f"register python-node-beta {B.endpoint}", "registered")
        pb = B.ask("peers", "peers")
        check(pb and pb["peers"] == {}, "Beta's registry is its own: Alpha's registration did not appear there")
        B.ask(f"register go-node-alpha {A.endpoint}", "registered")
        pa, pb = A.ask("peers", "peers"), B.ask("peers", "peers")
        check(pa["peers"] == {"python-node-beta": B.endpoint} and pb["peers"] == {"go-node-alpha": A.endpoint},
              "each registry holds only what that process registered")

        # ---- payloads both ways, verified at the receiver --------------------
        cases = [("text", b"hello beta"), ("utf8", "héllo — 世界 🌍".encode()),
                 ("binary", bytes(range(256))), ("empty", b""),
                 ("max", bytes(i % 253 for i in range(1 << 20)))]
        for sender, receiver, sid, peer in ((A, B, "go-node-alpha", "python-node-beta"),
                                            (B, A, "python-node-beta", "go-node-alpha")):
            for label, data in cases:
                mid = f"{sender.name}-{label}"
                s = sender.send(peer, mid, data)
                m = receiver.message(15)
                got = base64.b64decode(m["payload_b64"]) if m else None
                check(s and s["status"] == "POLYCALL_OK" and m and m["from"] == sid and m["id"] == mid
                      and got == data,
                      f"{sender.name} -> {receiver.name} [{label}, {len(data)} B]: receiver printed "
                      f"sender={m and m['from']}, id={m and m['id']}, exact bytes={got == data}")

        # ---- duplicate id delivered once -------------------------------------
        A.send("python-node-beta", "dup-1", b"once")
        s2 = A.send("python-node-beta", "dup-1", b"once")
        first, second = B.message(5), B.message(1.5)
        check(s2 and s2["status"] == "POLYCALL_OK" and first and first["id"] == "dup-1" and second is None,
              "a retried message id is acknowledged and delivered exactly once")

        # ---- malformed input at both nodes -----------------------------------
        for n in (A, B):
            r1 = raw_http(n.endpoint, b"POST /receive HTTP/1.1\r\nAuthorization: Bearer " + token.encode()
                          + b"\r\nContent-Length: 9\r\n\r\n{\"v\":1,}x")
            r2 = raw_http(n.endpoint, b"POST /nowhere HTTP/1.1\r\nContent-Length: 0\r\n\r\n")
            r3 = raw_http(n.endpoint, b"POST /receive HTTP/1.1\r\nAuthorization: Bearer " + token.encode()
                          + b"\r\n\r\n")
            h = n.ask("health", "health")
            check(r1.startswith(b"HTTP/1.1 400") and r2.startswith(b"HTTP/1.1 404")
                  and r3.startswith(b"HTTP/1.1 411") and h and h["health"]["status"] == "ok",
                  f"{n.name}: malformed JSON -> 400, unknown path -> 404, no length -> 411; still healthy")

        # ---- a peer dies; the survivor stays healthy and reports failure -----
        b_ep = B.endpoint
        B.kill()
        h = A.ask("health", "health")
        s = A.send("python-node-beta", "to-dead", b"x")
        check(h and h["health"]["status"] == "ok", "Alpha stays healthy while Beta is dead")
        check(s and s["status"] == "POLYCALL_E_TRANSPORT",
              f"send to dead Beta reports failed delivery ({s and s['status']}), never success")
        B = beta(b_ep)
        nodes.append(B)
        pb = B.ask("peers", "peers")
        s = A.send("python-node-beta", "rejoin-1", b"back")
        m = B.message(10)
        check(pb["peers"] == {} and s["status"] == "POLYCALL_OK" and m and m["id"] == "rejoin-1",
              "Beta restarted on its endpoint rejoins (fresh registry; Alpha's delivery succeeds)")
        B.ask(f"register go-node-alpha {A.endpoint}", "registered")
        a_ep = A.endpoint
        A.kill()
        s = B.send("go-node-alpha", "to-dead-a", b"x")
        h = B.ask("health", "health")
        check(s and s["status"] == "POLYCALL_E_TRANSPORT" and h["health"]["status"] == "ok",
              "Alpha dead: Beta stays healthy and reports POLYCALL_E_TRANSPORT")
        A = alpha(a_ep)
        nodes.append(A)
        A.ask(f"register python-node-beta {B.endpoint}", "registered")
        s = B.send("go-node-alpha", "rejoin-2", b"back")
        m = A.message(10)
        check(s and s["status"] == "POLYCALL_OK" and m and m["from"] == "python-node-beta",
              "Alpha restarted rejoins; Beta's delivery succeeds")

        # ---- partition and delay through the fault proxy ---------------------
        if a.proxy and os.path.exists(a.proxy):
            def proxy(mode: str, *extra: str):
                epf = os.path.join(work, f"proxy-{mode}.ep")
                p = subprocess.Popen([a.proxy, epf, B.endpoint, mode, *extra],
                                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                for _ in range(100):
                    if os.path.exists(epf) and open(epf).read().strip():
                        return p, open(epf).read().strip()
                    time.sleep(0.05)
                raise RuntimeError("proxy did not start")

            px, pep = proxy("blackhole")
            A.ask(f"register python-node-beta {pep}", "registered")
            s = A.send("python-node-beta", "part-1", b"partitioned", 15)
            hA, hB = A.ask("health", "health"), B.ask("health", "health")
            check(s and s["status"] == "POLYCALL_E_TIMEOUT", f"partition: send times out ({s and s['status']})")
            check(hA["health"]["status"] == "ok" and hB["health"]["status"] == "ok",
                  "partition: both nodes remain independently healthy")
            px.kill()
            px, pep = proxy("forward")
            A.ask(f"register python-node-beta {pep}", "registered")
            s = A.send("python-node-beta", "part-1", b"partitioned")
            m = B.message(10)
            check(s["status"] == "POLYCALL_OK" and m and m["id"] == "part-1",
                  "after the partition heals the same message id is delivered")
            px.kill()

            px, pep = proxy("delay", "6500")
            A.ask(f"register python-node-beta {pep}", "registered")
            s = A.send("python-node-beta", "late-1", b"slow", 20)
            check(s and s["status"] == "POLYCALL_E_TIMEOUT", "delayed link: sender reports timeout (outcome unknown)")
            m = B.message(10)
            check(m and m["id"] == "late-1", "the delayed message still arrives")
            A.ask(f"register python-node-beta {B.endpoint}", "registered")
            s = A.send("python-node-beta", "late-1", b"slow")
            again = B.message(1.5)
            check(s["status"] == "POLYCALL_OK" and again is None,
                  "retrying the timed-out id is acknowledged and not delivered twice")
            px.kill()
        else:
            print("NOT RUN  partition/delay faults (no fault proxy given)")

        # ---- FFI unavailable / incompatible: HTTP fallback --------------------
        for label, lib in (("library missing", None), ("foreign library (missing symbols)",
                           os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", "kernel32.dll")
                           if win else next((p for p in ("/lib/x86_64-linux-gnu/libz.so.1",
                                                         "/usr/lib/x86_64-linux-gnu/libz.so.1",
                                                         "/lib/x86_64-linux-gnu/libc.so.6") if os.path.exists(p)), None))):
            if label != "library missing" and not lib:
                print(f"NOT RUN  {label} (no foreign library found)")
                continue
            F = beta(library=lib)
            nodes.append(F)
            reason = F.info.get("fallback_reason", "")
            check(F.info["backend"] == "python-fallback" and reason,
                  f"FFI unavailable ({label}): Beta uses the HTTP fallback -- {reason}")
            F.ask(f"register go-node-alpha {A.endpoint}", "registered")
            A.ask(f"register python-node-beta {F.endpoint}", "registered")
            s1 = A.send("python-node-beta", f"fb-{len(nodes)}-a", "native → fallback".encode())
            m1 = F.message(10)
            s2 = F.send("go-node-alpha", f"fb-{len(nodes)}-b", "fallback → native".encode())
            m2 = A.message(10)
            check(s1["status"] == "POLYCALL_OK" and m1 and base64.b64decode(m1["payload_b64"]) == "native → fallback".encode()
                  and s2["status"] == "POLYCALL_OK" and m2 and m2["from"] == "python-node-beta"
                  and base64.b64decode(m2["payload_b64"]) == "fallback → native".encode(),
                  f"native Alpha <-> fallback Beta exchange both ways ({label})")
            F.stop()
    except Exception as exc:  # noqa: BLE001
        bad(f"harness error: {exc!r}")
    finally:
        for n in nodes:
            n.stop()
        shutil.rmtree(work, ignore_errors=True)
    print(f"--- nsigii: {PASS} passed, {FAIL} failed ---")
    return 0 if FAIL == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
