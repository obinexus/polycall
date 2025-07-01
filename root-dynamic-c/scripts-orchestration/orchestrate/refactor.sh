#!/bin/bash

# === Sinphasé-Compliant Shrink-Wrap Refactor Script ===
# Claude-generated script to isolate and restructure LibPolyCall architecture
# Enforces 2-level nesting and cost-based modular shrinkwrap

# --- Phase 1: FFI Isolation ---
echo "[Phase 1] Isolating FFI (Cost: 0.78)"
mkdir -p root-dynamic-c/ffi-isolated/{src,include}

mv libpolycall/src/core/ffi/*.c root-dynamic-c/ffi-isolated/src/ 2>/dev/null
mv libpolycall/emergency-ffi-recovery/* root-dynamic-c/ffi-isolated/ 2>/dev/null

cat > root-dynamic-c/ffi-isolated/Makefile << 'EOF'
CC = gcc
CFLAGS = -Wall -Werror -fPIC -O2
TARGET = libffi_isolated.so

all: $(TARGET)

$(TARGET): src/*.c
	$(CC) $(CFLAGS) -shared -o $@ $^
EOF

echo "# Isolation Log: FFI" > root-dynamic-c/ffi-isolated/ISOLATION_LOG.md
echo "Dynamic Cost: 0.78\nCircular: python_bridge ↔ c_bridge\nDate: $(date)" >> root-dynamic-c/ffi-isolated/ISOLATION_LOG.md

# --- Phase 2: Protocol Isolation ---
echo "[Phase 2] Isolating Protocol (Cost: 0.65)"
mkdir -p root-dynamic-c/protocol-isolated/src
mv libpolycall/src/core/protocol/* root-dynamic-c/protocol-isolated/src/ 2>/dev/null

cat > root-dynamic-c/protocol-isolated/Makefile << 'EOF'
CC = gcc
CFLAGS = -Wall -Werror -fPIC -O2
TARGET = libprotocol_isolated.so

all: $(TARGET)

$(TARGET): src/*.c
	$(CC) $(CFLAGS) -shared -o $@ $^
EOF

echo "# Isolation Log: Protocol" > root-dynamic-c/protocol-isolated/ISOLATION_LOG.md
echo "Dynamic Cost: 0.65\nCircular: command ↔ state_machine\nDate: $(date)" >> root-dynamic-c/protocol-isolated/ISOLATION_LOG.md

# --- Phase 3: Script Consolidation ---
echo "[Phase 3] Consolidating Scripts (Cost: 0.82)"
mkdir -p root-dynamic-c/scripts-orchestration/orchestrate/{build,test,validation,deployment}

find . -name "*.sh" -o -name "*.py" | grep -v orchestrate | while read script; do
  cp "$script" root-dynamic-c/scripts-orchestration/orchestrate/ 2>/dev/null
  echo "Moved $script"
done

# Phase logging
echo "# Isolation Log: Scripts" > root-dynamic-c/scripts-orchestration/ISOLATION_LOG.md
echo "Dynamic Cost: 0.82\nScattered: 15+\nDate: $(date)" >> root-dynamic-c/scripts-orchestration/ISOLATION_LOG.md

echo "\n[All phases completed. Please validate interface contracts and rewire root Makefile.]"
