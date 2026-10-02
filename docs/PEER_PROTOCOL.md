# polycall-peer/1 — peer-to-peer wire protocol

Independent peer processes exchange payloads directly — no central broker —
over **HTTP/1.1 + JSON**. The C library implements both ends
(`polycall_peer_*`, `polycall peer serve`); any language with an HTTP
client and server can implement it natively (the Python binding's
fallback does, for when the shared library is unavailable).

Adapted from the NSIGII two-process reference; see
[NSIGII_ADAPTATION.md](NSIGII_ADAPTATION.md) for what changed and why.

## Transport rules

* One request per connection; every response carries `Connection: close`.
* Bodies require `Content-Length`. `Transfer-Encoding` is refused (501) —
  no request-smuggling ambiguity. Conflicting duplicate `Content-Length`
  headers → 400.
* Limits: request headers 16 KiB (431), body 2 MiB (413), path 255 bytes
  (414). Header read deadline 5 s, body 10 s (408).
* Unknown path → 404; wrong method → 405 with `Allow`.
* At most 32 concurrent connections per node; more → immediate 503
  `server.busy` with `Retry-After: 1`.
* Authentication: when a node has a shared token, every endpoint except
  `GET /health` requires `Authorization: Bearer <token>` (401 otherwise,
  `WWW-Authenticate: Bearer`). Tokens are compared in constant time and are
  never logged. Identity: a node's id is asserted by its sender; the shared
  token authenticates membership of the trusted group, not individual nodes.
* No TLS. Bind to loopback, or put a TLS-terminating proxy in front; a node
  refuses a non-loopback bind without a token.

Errors are JSON: `{"ok":false,"error":{"code":"<code>","message":"<text>"}}`.

## Endpoints

### `GET /health` (no token needed)

```json
{"ok":true,"protocol":"polycall-peer/1","node_id":"alpha","endpoint":"127.0.0.1:9001",
 "status":"ok","peers":1,"inbox":0,"inbox_capacity":256,"received":3,"duplicates":0,
 "rejected_busy":0,"rejected_malformed":0,"rejected_auth":0,"sent_ok":2,"sent_failed":0,
 "uptime_ms":1234,"auth":true,"implementation":"libpolycall 1.1.0"}
```

`status` is `"ok"` or `"stopping"`. A client that pings a registered id
must also check that `node_id` is the id it expected.

### `GET /peers` (token)

`{"ok":true,"node_id":"alpha","peers":{"beta":"127.0.0.1:9002"}}` — this
node's own registry.

### `POST /register` (token)

Body `{"node_id":"beta","endpoint":"127.0.0.1:9002"}` → `200
{"ok":true,"registered":"beta"}`; 400 on an invalid id/endpoint; 409
`registry.full` (64 entries).

### `POST /receive` (token)

```json
{"v":1,"id":"<message id>","from":"<sender node id>","payload_b64":"<standard base64>"}
```

* `v` must be `1`; `id` and `from` are 1–63 of `[A-Za-z0-9._-]`;
  `payload_b64` is canonical padded base64 of at most 1 MiB of bytes.
  Anything else → 400 (`request.malformed` / `protocol.version`), never a
  silent 200.
* Success → `200 {"ok":true,"status":"received","node_id":"<receiver>","id":"<id>","duplicate":false}`.
  A sender counts a delivery only when it gets 200, `ok:true`, the same
  `id`, and — when it addressed a registered id — the expected `node_id`.
* Duplicate `(from, id)` within the receiver's last 1024 messages → 200
  with `"duplicate":true`, stored once.
* Inbox full (256 messages or 64 MiB) → 503 `inbox.full`, `Retry-After: 1`.
* The receiver never adds the sender to its registry.

### `POST /inbox/next` (token)

Body `{"timeout_ms":N}` (0–30000). Removes and returns the oldest message:
`200 {"ok":true,"message":{"from":"alpha","id":"m1","payload_b64":"..."}}`, or
`{"ok":true,"message":null}` when none arrived in time. Used by
`polycall peer recv` and by out-of-process consumers.

## Delivery semantics

| Sender sees | Meaning | What to do |
| --- | --- | --- |
| 200 + matching ack | stored exactly once | done |
| connection refused / reset (`E_TRANSPORT`) | not delivered | retry later, same id |
| timeout (`E_TIMEOUT`) | unknown: may arrive late | retry with the **same id**; the duplicate is dropped |
| 503 (`E_BUSY`) | receiver overloaded | back off, retry same id |
| 401 / 413 / 400 | rejected | fix the request; retrying will not help |

Ordering: per (sender, receiver) pair, messages a single thread sends
sequentially are queued in send order; concurrent senders are not ordered.

## Topologies

Two-node peer-to-peer exchange is what is implemented and tested
(`tests/peer/p2p.sh`, `tests/unit/ffi_unit.c`, `examples/nsigii-p2p`).
Ring, star and relay topologies described in the NSIGII material are **not**
implemented by Polycall and are not claimed; they would need their own
multi-node tests.
