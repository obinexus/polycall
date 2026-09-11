// polycall-call: a generic polycall_rpc v1 CLI, one round trip, no retry.
//
//	polycall-call <host:port> <service> <operation> <json-input> [deadline_ms]
//
// Prints the RESPONSE payload exactly as received (for byte-identical
// cross-language comparison in tests/conformance/) and exits with the same
// code contract as the C CLI's `polycall call` (docs/RPC.md).
package main

import (
	"encoding/json"
	"fmt"
	"os"
	"strconv"

	rpcv1 "github.com/obinexus/polycall/bindings/go-polycall/rpcv1"
)

const (
	exitOK          = 0
	exitOther       = 1
	exitUsage       = 2
	exitConfig      = 3
	exitUnsupported = 4
	exitTransport   = 5
	exitDeadline    = 6
	exitAuth        = 7
)

func exitFor(code string) int {
	switch code {
	case "request.malformed", "input.invalid":
		return exitConfig
	case "operation.unknown", "item.unknown":
		return exitUnsupported
	case "deadline.exceeded":
		return exitDeadline
	case "auth.denied":
		return exitAuth
	default:
		return exitOther
	}
}

func main() {
	if len(os.Args) < 4 || len(os.Args) > 6 {
		fmt.Fprintln(os.Stderr, "usage: polycall-call <host:port> <service> <operation> <json-input> [deadline_ms]")
		os.Exit(exitUsage)
	}
	endpoint, service, operation, rawInput := os.Args[1], os.Args[2], os.Args[3], "null"
	if len(os.Args) >= 5 {
		rawInput = os.Args[4]
	}
	deadlineMs := 5000
	if len(os.Args) == 6 {
		if v, err := strconv.Atoi(os.Args[5]); err == nil {
			deadlineMs = v
		}
	}

	var input interface{}
	if err := json.Unmarshal([]byte(rawInput), &input); err != nil {
		fmt.Fprintf(os.Stderr, "polycall-call: invalid JSON input: %v\n", err)
		os.Exit(exitConfig)
	}

	payload, err := rpcv1.CallRaw(endpoint, service, operation, input, deadlineMs)
	if err != nil {
		fmt.Fprintf(os.Stderr, "polycall-call: %v\n", err)
		os.Exit(exitTransport)
	}
	fmt.Println(string(payload))

	var resp rpcv1.Response
	if err := json.Unmarshal(payload, &resp); err != nil {
		os.Exit(exitOther)
	}
	if resp.OK {
		os.Exit(exitOK)
	}
	code := ""
	if resp.Error != nil {
		code = resp.Error.Code
	}
	os.Exit(exitFor(code))
}
