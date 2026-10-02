# NSIGII two-process reference → Polycall

Source: `nsigii-p2p-dual-ffi-main (1).zip` (SHA-256
`a05d15eb7b11316fece4390b50658b5eac12a10af66f63bf5caf7fdb3779ae1e`; the
copy without "(1)" is byte-identical). Its text sources are preserved
unmodified in `examples/nsigii-p2p/reference/` (bytes protected by
`.gitattributes`; per-file hashes in `reference/SHA256SUMS`). The compiled
`nsigii_go.so` files and the blueprint media are not copied — they are
recorded by hash only.

## Architecture kept, and what changed

| Reference | Polycall adaptation |
| --- | --- |
| Go Alpha, `:9001`, standalone `net/http` server | `examples/nsigii-p2p/alpha` — Go process using **libpolycall through cgo**; one peer node, its own registry and inbox |
| Python Beta, `:9002`, `http.server` | `examples/nsigii-p2p/beta/node.py` — Python process using **libpolycall through ctypes**; one peer node |
| Python `ctypes` loads a *Go* C-shared library (`nsigii_go.so`) | Python `ctypes` loads the *Polycall* C shared library (`libpolycall.so.1` / `polycall.dll`), the shared core every binding uses |
| HTTP fallback when the `.so` is missing | Kept: Beta falls back to a pure-Python `polycall-peer/1` node when libpolycall is missing, lacks the ABI v1 symbols, or reports another ABI — and says which |
| `/health`, `/peers`, `/receive`, Python `/send` `/register` | `polycall-peer/1` (docs/PEER_PROTOCOL.md): `GET /health`, `GET /peers`, `POST /receive`, `POST /register`, `POST /inbox/next`; sending is a local operation of the node (stdin command / API), not an unauthenticated HTTP endpoint |
| No central broker | Unchanged: nodes talk directly; nothing is relayed |

**FFI vs transport.** An FFI call runs inside the process that loaded the
library. Python Beta's ctypes calls drive *Beta's own* node in the Python
process; Go Alpha's cgo calls drive *Alpha's own* node in the Go process.
The two processes are connected only by the network transport. In the
reference, the Go code loaded into Python kept a *separate* `peers` map
from the standalone Go Alpha process, although the README presented one
registry; Polycall makes this explicit: **each node's registry belongs to
that node**, changes only through that node's own `register` (local API
or authenticated `POST /register`), is never updated by receiving a
message, and is not synchronised with any other node. The test asserts it.

## Reference defects → fixes → evidence

| # | Reference defect (verified in source) | Fix in Polycall | Test |
| --- | --- | --- | --- |
| 1 | `NSIGIISendMessage` (`main.go:39-50`) marshals the message, then passes `nil` as the HTTP body: the receiver gets nothing yet answers 200, and the sender reports `"ok"` with the message it never sent | The body is the JSON envelope; a send succeeds only on 200 **and** an acknowledgement echoing the message id under the expected node id | `tests/peer/nsigii_test.py` (receiver prints exact bytes, sender, id); `ffi_unit` "acknowledged by 'beta', expected 'gamma'" |
| 2 | HTTP response bodies never closed (`main.go:41-50`; `node.py` heartbeat `urlopen` result dropped) | C client closes every socket on every path (`pc_http_exchange`); Python closes connections in `finally` | `ffi_unit` concurrency (400 sends), fuzz (thousands of connections) under ASan/TSan without leaks |
| 3 | `C.CString` results returned with no exported matching free → every call leaks C memory | The binding ABI never returns memory to free: outputs go to caller-owned buffers. Alpha frees every `C.CString`/`C.malloc` it makes | ASan+UBSan suite; `docs/BINDING_ABI.md` rule |
| 4 | Python declares `restype = c_char_p`, which converts to `bytes` and loses the pointer, so even a free function could not be called | Beta passes Python-owned `create_string_buffer`s; nothing is returned by pointer | `nsigii_test.py` 1 MiB both ways |
| 5 | `healthHandler` reads `peers` without `peersMu` (`main.go:82-88`) — a data race with `NSIGIIRegisterPeer` | Registry only under the node lock (health, list, register, resolve) | TSan: 0 races in `ffi_unit` (8 concurrent senders + concurrent register/recv) |
| 6 | `json.NewDecoder(r.Body).Decode(&msg)` error ignored (`main.go:73`) → garbage answered with 200 | Strict decoding: non-object, wrong `v`, bad id, bad base64 → 400 with a code | `nsigii_test.py` malformed JSON → 400 at both nodes; `fuzz_parsers` |
| 7 | No method checks (`/receive` accepts GET); Python `do_POST` has no `else`: unknown POST paths get **no response**; no body limit (`int(Content-Length)` read unbounded, negative hangs); no server timeouts | 404 unknown path, 405 wrong method (with `Allow`), 411 without Content-Length, 413 over 2 MiB, 431/414 header/path limits, 501 Transfer-Encoding, 5 s/10 s read deadlines, 32-connection cap with 503 | `ffi_unit` malformed requests; `test_fallback.py`; `nsigii_test.py` 404/411 |
| 8 | `run.sh`: no `set -e` (prints "✅ compiled" after a failed build); `go run main.go &` records the PID of `go run`, not of the server, so cleanup leaves the server running | `run.sh`: `set -eu`, builds the Alpha **binary** and runs it directly, waits for readiness, `trap` stops exactly the PIDs it started | `run.sh` review; same discipline in `nsigii_test.py` |
| 9 | `NSIGIISendMessage` hard-codes `nodeID = "go-node-alpha"`: messages Python Beta sent through the Go library claimed to come from **Alpha** | The sender id is the sending node's own id; receivers verify it | `nsigii_test.py`: receiver prints `from` = actual sender, both directions |
| 10 | Python `/receive` auto-registers the sender from an unauthenticated `X-Peer-Addr` header (registry poisoning) | Receiving never registers; registration requires the shared token | `nsigii_test.py` "registry ownership" block; `p2p.sh` unauthenticated `/peers` → 401 |
| 11 | Beta serves on a single-threaded `HTTPServer`, and `GET /peers/alive` calls every peer synchronously from that one thread: a slow or self-referencing peer stalls all other requests for up to 2 s per peer | Threaded servers with bounded workers; health checks are separate client calls with deadlines | `ffi_unit` ping, `nsigii_test.py` health during partition |
| 12 | Both nodes bind `0.0.0.0` with no authentication | Loopback by default; a non-loopback bind without a shared token is refused; Bearer token compared in constant time, never logged | `ffi_unit` "non-loopback bind without a token is refused" |
| 13 | Failure strings built with `%v` of a nil error (`"reason":"<nil>"`), and error text spliced into JSON unescaped | Errors are typed status codes plus an escaped detail | `polycall_strerror` / `polycall_last_error` |

## Delivery, retries, idempotency

`send` makes exactly one attempt and succeeds only on the receiver's
acknowledgement. `E_TRANSPORT` = not delivered; `E_TIMEOUT` = unknown (the
message may arrive late). Retrying with the **same message id** is safe:
the receiver de-duplicates `(sender, id)` over its last 1024 messages.
Tested: duplicate ids, a 6.5 s delayed link with a 5 s sender timeout
(late arrival + retry = exactly one copy), partition then heal.

## Not claimed

Ring, star and relay topologies from the NSIGII material are not
implemented; two-node tests do not prove them. The "720° camera", "GPS
calibration" and drone items of the reference's task list are out of
scope.
