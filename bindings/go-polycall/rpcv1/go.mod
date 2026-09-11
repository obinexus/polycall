// Nested module, deliberately separate from the parent go-polycall module's
// heavy (and currently unresolved -- see ../pkg/client.go, whose
// "internal" import does not exist in this checkout) dependency graph.
// Standard library only: no `go mod download` is ever needed to build this.
module github.com/obinexus/polycall/bindings/go-polycall/rpcv1

go 1.21
