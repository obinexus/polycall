package main

import (
	"C"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"
)

// NSIGIIMessage is the canonical message format for the P2P network
type NSIGIIMessage struct {
	NodeID    string    `json:"node_id"`
	Payload   string    `json:"payload"`
	Timestamp time.Time `json:"timestamp"`
	Layer     string    `json:"layer"` // alpha / beta / gamma
}

// PeerRegistry holds known peers (decentralised - no central broker)
var (
	peers   = map[string]string{} // nodeID -> address
	peersMu sync.RWMutex
	nodeID  = "go-node-alpha"
)

// --- FFI EXPORTS (callable from Python via ctypes) ---

//export NSIGIISendMessage
func NSIGIISendMessage(target *C.char, payload *C.char) *C.char {
	addr := C.GoString(target)
	msg := NSIGIIMessage{
		NodeID:    nodeID,
		Payload:   C.GoString(payload),
		Timestamp: time.Now(),
		Layer:     "alpha",
	}
	data, _ := json.Marshal(msg)

	resp, err := http.Post(
		fmt.Sprintf("http://%s/receive", addr),
		"application/json",
		nil,
	)
	if err != nil || resp.StatusCode != 200 {
		result := fmt.Sprintf(`{"status":"failed","reason":"%v"}`, err)
		return C.CString(result)
	}
	return C.CString(fmt.Sprintf(`{"status":"ok","sent":%s}`, string(data)))
}

//export NSIGIIRegisterPeer
func NSIGIIRegisterPeer(id *C.char, addr *C.char) {
	peersMu.Lock()
	defer peersMu.Unlock()
	peers[C.GoString(id)] = C.GoString(addr)
	log.Printf("[GO NODE] Registered peer: %s @ %s", C.GoString(id), C.GoString(addr))
}

//export NSIGIIGetPeers
func NSIGIIGetPeers() *C.char {
	peersMu.RLock()
	defer peersMu.RUnlock()
	data, _ := json.Marshal(peers)
	return C.CString(string(data))
}

// --- P2P HTTP SERVER ---

func receiveHandler(w http.ResponseWriter, r *http.Request) {
	var msg NSIGIIMessage
	json.NewDecoder(r.Body).Decode(&msg)
	log.Printf("[GO NODE] Received from %s: %s", msg.NodeID, msg.Payload)
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "received",
		"node_id": nodeID,
	})
}

func healthHandler(w http.ResponseWriter, r *http.Request) {
	json.NewEncoder(w).Encode(map[string]interface{}{
		"node_id": nodeID,
		"layer":   "alpha",
		"peers":   peers,
		"time":    time.Now(),
	})
}

func peersHandler(w http.ResponseWriter, r *http.Request) {
	peersMu.RLock()
	defer peersMu.RUnlock()
	json.NewEncoder(w).Encode(peers)
}

func main() {
	mux := http.NewServeMux()
	mux.HandleFunc("/receive", receiveHandler)
	mux.HandleFunc("/health", healthHandler)
	mux.HandleFunc("/peers", peersHandler)

	port := ":9001"
	log.Printf("[GO NODE] NSIGII Alpha Node starting on %s", port)
	log.Fatal(http.ListenAndServe(port, mux))
}
