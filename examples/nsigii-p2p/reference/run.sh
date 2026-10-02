#!/bin/bash
# NSIGII P2P Network — Fixed Run Script
# Works with flat structure: main.go, node.py, run.sh all in same directory

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🧬 NSIGII P2P Build — Go + Python FFI"
echo "======================================="
echo "📁 Working directory: $SCRIPT_DIR"

# ─── BUILD GO SHARED LIBRARY ──────────────────────────────────────────────────
echo "🐉 Building Go shared library..."

if [ ! -f go.mod ]; then
  go mod init nsigii_go
fi

CGO_ENABLED=1 go build -buildmode=c-shared -o nsigii_go.so .
echo "✅ Go .so compiled → nsigii_go.so"

# ─── START GO ALPHA NODE ──────────────────────────────────────────────────────
echo ""
echo "🐢 Starting Go Alpha Node on :9001..."
go run main.go &
GO_PID=$!

sleep 1

# ─── START PYTHON BETA NODE ───────────────────────────────────────────────────
echo "🫀 Starting Python Beta Node on :9002..."
NSIGII_GO_LIB="$SCRIPT_DIR/nsigii_go.so" python3 node.py &
PY_PID=$!

sleep 1

echo ""
echo "Both nodes running:"
echo "  Go Alpha    → http://localhost:9001/health"
echo "  Python Beta → http://localhost:9002/health"
echo ""
echo "Test peer registration:"
echo "  curl -X POST http://localhost:9002/register \\"
echo "    -H 'Content-Type: application/json' \\"
echo "    -d '{\"node_id\":\"go-node-alpha\",\"address\":\"localhost:9001\"}'"
echo ""
echo "Press Ctrl+C to stop both."

trap "echo ''; echo 'Stopping nodes...'; kill \$GO_PID \$PY_PID 2>/dev/null; echo 'Done.'" INT TERM
wait
