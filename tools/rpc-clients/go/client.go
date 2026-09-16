// Package rpcv1 is a polycall_rpc v1 client (see docs/RPC.md), added
// alongside the existing (legacy-protocol) bindings/go-polycall/pkg/client.go
// per docs/TODO.md P2. Standard library only.
//
// Frame: 16-byte header (magic "PCR1", type, flags, reserved, big-endian
// correlation id, big-endian length) followed by a UTF-8 JSON payload. No Go
// struct is ever placed on the wire -- payloads are JSON in both directions.
package rpcv1

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"time"
)

const (
	TypeRequest  = 1
	TypeResponse = 2
	TypeControl  = 3
	TypeReply    = 4

	headerLen  = 16
	magic      = "PCR1"
	maxPayload = 1 << 20 // 1 MiB, matches src/runtime/rpc_wire.h
)

// Frame is one polycall_rpc v1 message.
type Frame struct {
	Type    uint8
	Corr    uint32
	Payload []byte
}

func encode(f Frame) []byte {
	hdr := make([]byte, headerLen)
	copy(hdr[0:4], magic)
	hdr[4] = f.Type
	binary.BigEndian.PutUint32(hdr[8:12], f.Corr)
	binary.BigEndian.PutUint32(hdr[12:16], uint32(len(f.Payload)))
	return append(hdr, f.Payload...)
}

func decode(r io.Reader) (Frame, error) {
	hdr := make([]byte, headerLen)
	if _, err := io.ReadFull(r, hdr); err != nil {
		return Frame{}, fmt.Errorf("read header: %w", err)
	}
	if !bytes.Equal(hdr[0:4], []byte(magic)) {
		return Frame{}, fmt.Errorf("bad frame magic")
	}
	length := binary.BigEndian.Uint32(hdr[12:16])
	if length > maxPayload {
		return Frame{}, fmt.Errorf("payload too large: %d", length)
	}
	payload := make([]byte, length)
	if length > 0 {
		if _, err := io.ReadFull(r, payload); err != nil {
			return Frame{}, fmt.Errorf("read payload: %w", err)
		}
	}
	return Frame{Type: hdr[4], Corr: binary.BigEndian.Uint32(hdr[8:12]), Payload: payload}, nil
}

// Request is a polycall_rpc v1 REQUEST payload.
type Request struct {
	Service    string      `json:"service"`
	Operation  string      `json:"operation"`
	DeadlineMs int         `json:"deadline_ms"`
	Input      interface{} `json:"input"`
}

// Response is a polycall_rpc v1 RESPONSE payload.
type Response struct {
	OK     bool            `json:"ok"`
	Output json.RawMessage `json:"output,omitempty"`
	Error  *ResponseError  `json:"error,omitempty"`
}

type ResponseError struct {
	Code    string `json:"code"`
	Message string `json:"message"`
}

// CallRaw performs exactly ONE round trip against endpoint ("host:port") and
// never retries -- callers must not loop this for non-idempotent operations.
// It returns the RESPONSE payload exactly as the wire delivered it (no
// re-marshalling), so byte-for-byte comparison across language clients is
// meaningful.
func CallRaw(endpoint, service, operation string, input interface{}, deadlineMs int) ([]byte, error) {
	if deadlineMs <= 0 {
		deadlineMs = 5000
	}
	body, err := json.Marshal(Request{Service: service, Operation: operation,
		DeadlineMs: deadlineMs, Input: input})
	if err != nil {
		return nil, fmt.Errorf("encode request: %w", err)
	}

	timeout := time.Duration(deadlineMs+1000) * time.Millisecond
	d := net.Dialer{Timeout: timeout}
	conn, err := d.Dial("tcp", endpoint)
	if err != nil {
		return nil, fmt.Errorf("transport: %w", err)
	}
	defer conn.Close()
	_ = conn.SetDeadline(time.Now().Add(timeout))

	if _, err := conn.Write(encode(Frame{Type: TypeRequest, Corr: 1, Payload: body})); err != nil {
		return nil, fmt.Errorf("transport: %w", err)
	}
	frame, err := decode(conn)
	if err != nil {
		if ne, ok := err.(net.Error); ok && ne.Timeout() {
			return nil, fmt.Errorf("deadline: %w", err)
		}
		return nil, fmt.Errorf("transport: %w", err)
	}
	if frame.Type != TypeResponse {
		return nil, fmt.Errorf("unexpected frame type %d", frame.Type)
	}
	return frame.Payload, nil
}

// Call is CallRaw plus typed decoding, for library callers that want a
// structured Response rather than raw bytes.
func Call(endpoint, service, operation string, input interface{}, deadlineMs int) (*Response, error) {
	payload, err := CallRaw(endpoint, service, operation, input, deadlineMs)
	if err != nil {
		return nil, err
	}
	var resp Response
	if err := json.Unmarshal(payload, &resp); err != nil {
		return nil, fmt.Errorf("decode response: %w", err)
	}
	return &resp, nil
}

// Control sends one CONTROL frame (ping / describe / shutdown) and returns
// the raw CONTROL_REPLY payload.
func Control(endpoint string, action string, authToken string, timeout time.Duration) (json.RawMessage, error) {
	req := map[string]string{"action": action}
	if authToken != "" {
		req["auth_token"] = authToken
	}
	body, err := json.Marshal(req)
	if err != nil {
		return nil, err
	}
	if timeout <= 0 {
		timeout = 3 * time.Second
	}
	d := net.Dialer{Timeout: timeout}
	conn, err := d.Dial("tcp", endpoint)
	if err != nil {
		return nil, fmt.Errorf("transport: %w", err)
	}
	defer conn.Close()
	_ = conn.SetDeadline(time.Now().Add(timeout))

	if _, err := conn.Write(encode(Frame{Type: TypeControl, Corr: 1, Payload: body})); err != nil {
		return nil, fmt.Errorf("transport: %w", err)
	}
	frame, err := decode(conn)
	if err != nil {
		return nil, fmt.Errorf("transport: %w", err)
	}
	if frame.Type != TypeReply {
		return nil, fmt.Errorf("unexpected frame type %d", frame.Type)
	}
	return frame.Payload, nil
}
