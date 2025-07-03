#!/bin/bash
# Ad-hoc Script 8: Build Debug Executable

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

CC=${CC:-gcc}
CFLAGS="-g -O0 -DDEBUG -Wall -Wextra"
EXE_NAME="polycall"
[[ "$(uname -s)" == *"NT"* ]] && EXE_NAME="polycall.exe"

echo "=== Building Debug Executable ==="

if [[ ! -f "build/obj/cli/main.o" ]]; then
    echo "❌ main.o not found. Run compile scripts first."
    exit 1
fi

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Would build: build/bin/debug/$EXE_NAME"
    echo "[DRY-RUN] Linking with: build/lib/polycall.a"
else
    $CC $CFLAGS build/obj/cli/main.o build/lib/polycall.a \
        -o "build/bin/debug/$EXE_NAME" && \
        echo "✓ Created: build/bin/debug/$EXE_NAME" || \
        echo "❌ Failed to build executable"
fi
