# NSIGII P2P Network — Dual FFI Entry Points

## Architecture

```
┌─────────────────────┐     FFI (.so)      ┌─────────────────────┐
│   Go Node (Alpha)   │◄──────────────────►│  Python Node (Beta) │
│   Port :9001        │                    │  Port :9002         │
│   Layer: α          │◄── HTTP fallback ──│  Layer: β           │
└─────────────────────┘                    └─────────────────────┘
         │                                          │
         └──────────── Peer-to-Peer ────────────────┘
                    No central broker
                    If Go fails → Python still runs
                    If Python fails → Go still runs
```

## Topology (from Nnamdi's drawings)

```
Static:  O───O      (direct peer, 2 nodes)
              │
              O

Dynamic star: centralised failover detection
Ring:         circular redundancy
Out-relay:    message forwarding when direct path down
```

## Three Governance Clauses (NSIGII)

| Mode | State | Rule |
|------|-------|------|
| Human IN loop | Active | Never a problem |
| Human OUT loop | Passive | Never a weapon |
| Human ON loop | Observer | Never a toy |

## Endpoints

### Go Node (:9001)
- `GET  /health` — node status
- `GET  /peers` — known peers
- `POST /receive` — accept inbound message

### Python Node (:9002)
- `GET  /health` — node status + FFI bridge status
- `GET  /peers` — local peer registry
- `GET  /peers/alive` — liveness-checked peers
- `POST /receive` — accept inbound message
- `POST /send` — route message to peer
- `POST /register` — add peer to registry

## Run

```bash
chmod +x run.sh
./run.sh
```

## FFI Notes

The Go node compiles to `nsigii_go.so`. Python loads it via `ctypes`.
If the `.so` is not present, Python falls back to HTTP — **the network
still functions**. This is the fault-tolerance guarantee.

## Next Steps (from task board)

- [ ] 720° Electronic Magnetic Drone Gyroscopic Orbital Camera module
- [ ] Trident GPS MMUKO Single NoLock Buffon Calibration
- [ ] Gamma (γ) node — third peer point completes the tricopter topology
