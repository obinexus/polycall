// NSIGII Alpha node (Go), adapted onto Polycall.
//
// An independent peer process: it owns ONE Polycall peer node (libpolycall,
// via cgo) -- its own registry and inbox -- listening on 127.0.0.1:9001 by
// default. Reference: ../reference/main.go; every change is listed in
// docs/NSIGII_ADAPTATION.md.
//
// Control interface (stdin, one command per line) and events (stdout, one
// JSON object per line) are identical to beta/node.py:
//
//	register <id> <host:port> | unregister <id> | peers | health
//	send <peer> <message-id> <base64-payload> | quit
//
// Build (cgo; point the flags at an installed or build-tree polycall):
//
//	CGO_CFLAGS="-I<prefix>/include/polycall" CGO_LDFLAGS="-L<prefix>/lib" go build -o alpha .
package main

/*
#cgo LDFLAGS: -lpolycall
#include <polycall.h>
#include <stdlib.h>
*/
import "C"

import (
	"bufio"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"sync"
	"syscall"
	"unsafe"
)

var outMu sync.Mutex

func emit(v map[string]any) {
	b, _ := json.Marshal(v)
	outMu.Lock()
	defer outMu.Unlock()
	os.Stdout.Write(append(b, '\n'))
}

// every C string we allocate is freed by us (the reference leaked C.CString)
func cstr(s string) *C.char { return C.CString(s) }

func lastError() string {
	buf := (*C.char)(C.malloc(512))
	defer C.free(unsafe.Pointer(buf))
	C.polycall_last_error(buf, 512)
	return C.GoString(buf)
}

func statusName(rc C.int) string {
	n := C.GoString(C.polycall_strerror(rc)) // static string: never freed
	if i := strings.IndexByte(n, ':'); i > 0 {
		return n[:i]
	}
	return n
}

// text calls fn(handle, buf, cap, &len) with a buffer we own, growing it on
// POLYCALL_E_TOO_LARGE.
func text(h C.polycall_peer_t, fn func(C.polycall_peer_t, *C.char, C.size_t, *C.size_t) C.int) (string, C.int) {
	capacity := C.size_t(4096)
	for {
		buf := (*C.char)(C.malloc(capacity))
		var n C.size_t
		rc := fn(h, buf, capacity, &n)
		s := C.GoString(buf)
		C.free(unsafe.Pointer(buf))
		if rc == C.POLYCALL_E_TOO_LARGE && n+1 > capacity {
			capacity = n + 1
			continue
		}
		return s, rc
	}
}

func main() {
	id := flag.String("id", "go-node-alpha", "this node's id")
	listen := flag.String("listen", "127.0.0.1:9001", "bind address (port 0 = ephemeral)")
	tokenEnv := flag.String("token-env", "POLYCALL_DEV_TOKEN", "environment variable holding the shared token")
	var peers []string
	flag.Func("peer", "register a peer: ID=HOST:PORT (repeatable)", func(s string) error {
		peers = append(peers, s)
		return nil
	})
	flag.Parse()

	if C.polycall_ffi_abi_version() != 1 {
		fmt.Fprintln(os.Stderr, "alpha: libpolycall binding ABI is not 1")
		os.Exit(4)
	}
	cid, clisten := cstr(*id), cstr(*listen)
	var ctok *C.char
	if t := os.Getenv(*tokenEnv); t != "" {
		ctok = cstr(t)
	}
	var h C.polycall_peer_t
	rc := C.polycall_peer_open(cid, clisten, ctok, &h)
	C.free(unsafe.Pointer(cid))
	C.free(unsafe.Pointer(clisten))
	if ctok != nil {
		C.free(unsafe.Pointer(ctok))
	}
	if rc != C.POLYCALL_OK {
		emit(map[string]any{"event": "error", "op": "open", "status": statusName(rc), "detail": lastError()})
		os.Exit(5)
	}
	ep, _ := text(h, func(h C.polycall_peer_t, b *C.char, c C.size_t, _ *C.size_t) C.int {
		return C.polycall_peer_endpoint(h, b, c)
	})
	for _, p := range peers {
		k, v, _ := strings.Cut(p, "=")
		register(h, k, v)
	}
	ver := (*C.char)(C.malloc(32))
	C.polycall_ffi_version(ver, 32)
	emit(map[string]any{"event": "listening", "node_id": *id, "endpoint": ep,
		"backend": "libpolycall", "library": C.GoString(ver), "language": "go"})
	C.free(unsafe.Pointer(ver))

	// receiver: the only consumer of this node's inbox
	done := make(chan struct{})
	go func() {
		defer close(done)
		payload := (*C.char)(C.malloc(C.POLYCALL_PEER_MAX_PAYLOAD))
		from := (*C.char)(C.malloc(C.POLYCALL_PEER_ID_MAX))
		mid := (*C.char)(C.malloc(C.POLYCALL_MESSAGE_ID_MAX))
		defer C.free(unsafe.Pointer(payload))
		defer C.free(unsafe.Pointer(from))
		defer C.free(unsafe.Pointer(mid))
		for {
			var n C.size_t
			rc := C.polycall_peer_recv(h, C.uint32_t(0xFFFFFFFF), from, C.POLYCALL_PEER_ID_MAX,
				mid, C.POLYCALL_MESSAGE_ID_MAX, unsafe.Pointer(payload), C.POLYCALL_PEER_MAX_PAYLOAD, &n)
			if rc == C.POLYCALL_OK {
				emit(map[string]any{"event": "message", "from": C.GoString(from), "id": C.GoString(mid),
					"payload_b64": base64.StdEncoding.EncodeToString(C.GoBytes(unsafe.Pointer(payload), C.int(n)))})
				continue
			}
			if rc == C.POLYCALL_E_CLOSED || rc == C.POLYCALL_E_INVALID_HANDLE || rc == C.POLYCALL_E_CANCELLED {
				return
			}
			emit(map[string]any{"event": "error", "op": "recv", "status": statusName(rc), "detail": lastError()})
		}
	}()

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, os.Interrupt, syscall.SIGTERM)
	lines := make(chan string)
	go func() {
		sc := bufio.NewScanner(os.Stdin)
		sc.Buffer(make([]byte, 64*1024), 4*1024*1024)
		for sc.Scan() {
			lines <- sc.Text()
		}
		close(lines)
	}()

loop:
	for {
		select {
		case <-stop:
			break loop
		case line, ok := <-lines:
			if !ok {
				break loop
			}
			f := strings.Fields(line)
			if len(f) == 0 {
				continue
			}
			switch f[0] {
			case "register":
				if len(f) == 3 {
					register(h, f[1], f[2])
				}
			case "unregister":
				if len(f) == 2 {
					c := cstr(f[1])
					rc := C.polycall_peer_unregister(h, c)
					C.free(unsafe.Pointer(c))
					emit(map[string]any{"event": "unregistered", "peer": f[1], "status": statusName(rc)})
				}
			case "peers":
				s, rc := text(h, func(h C.polycall_peer_t, b *C.char, c C.size_t, n *C.size_t) C.int {
					return C.polycall_peer_list(h, b, c, n)
				})
				var m map[string]string
				_ = json.Unmarshal([]byte(s), &m)
				emit(map[string]any{"event": "peers", "peers": m, "status": statusName(rc)})
			case "health":
				s, rc := text(h, func(h C.polycall_peer_t, b *C.char, c C.size_t, n *C.size_t) C.int {
					return C.polycall_peer_health(h, b, c, n)
				})
				var m map[string]any
				_ = json.Unmarshal([]byte(s), &m)
				emit(map[string]any{"event": "health", "health": m, "status": statusName(rc)})
			case "send":
				if len(f) != 4 && len(f) != 3 {
					emit(map[string]any{"event": "error", "op": "send", "detail": "usage: send <peer> <id> [b64]"})
					continue
				}
				b64 := ""
				if len(f) == 4 {
					b64 = f[3]
				}
				data, err := base64.StdEncoding.DecodeString(b64)
				if err != nil {
					emit(map[string]any{"event": "error", "op": "send", "detail": "payload is not base64"})
					continue
				}
				send(h, f[1], f[2], data)
			case "quit":
				break loop
			}
		}
	}
	C.polycall_peer_close(h) // wakes the receiver with POLYCALL_E_CLOSED
	<-done
	emit(map[string]any{"event": "stopped", "node_id": *id})
}

func register(h C.polycall_peer_t, id, ep string) {
	ci, ce := cstr(id), cstr(ep)
	rc := C.polycall_peer_register(h, ci, ce)
	C.free(unsafe.Pointer(ci))
	C.free(unsafe.Pointer(ce))
	ev := map[string]any{"event": "registered", "peer": id, "endpoint": ep, "status": statusName(rc)}
	if rc != C.POLYCALL_OK {
		ev["detail"] = lastError()
	}
	emit(ev)
}

func send(h C.polycall_peer_t, peer, mid string, data []byte) {
	cp, cm := cstr(peer), cstr(mid)
	defer C.free(unsafe.Pointer(cp))
	defer C.free(unsafe.Pointer(cm))
	var ptr unsafe.Pointer
	if len(data) > 0 {
		ptr = C.CBytes(data) // copied into C memory, freed below
		defer C.free(ptr)
	}
	rc := C.polycall_peer_send(h, cp, ptr, C.size_t(len(data)), cm, 5000)
	ev := map[string]any{"event": "sent", "to": peer, "id": mid, "status": statusName(rc)}
	if rc != C.POLYCALL_OK {
		ev["detail"] = lastError()
	}
	emit(ev)
}
