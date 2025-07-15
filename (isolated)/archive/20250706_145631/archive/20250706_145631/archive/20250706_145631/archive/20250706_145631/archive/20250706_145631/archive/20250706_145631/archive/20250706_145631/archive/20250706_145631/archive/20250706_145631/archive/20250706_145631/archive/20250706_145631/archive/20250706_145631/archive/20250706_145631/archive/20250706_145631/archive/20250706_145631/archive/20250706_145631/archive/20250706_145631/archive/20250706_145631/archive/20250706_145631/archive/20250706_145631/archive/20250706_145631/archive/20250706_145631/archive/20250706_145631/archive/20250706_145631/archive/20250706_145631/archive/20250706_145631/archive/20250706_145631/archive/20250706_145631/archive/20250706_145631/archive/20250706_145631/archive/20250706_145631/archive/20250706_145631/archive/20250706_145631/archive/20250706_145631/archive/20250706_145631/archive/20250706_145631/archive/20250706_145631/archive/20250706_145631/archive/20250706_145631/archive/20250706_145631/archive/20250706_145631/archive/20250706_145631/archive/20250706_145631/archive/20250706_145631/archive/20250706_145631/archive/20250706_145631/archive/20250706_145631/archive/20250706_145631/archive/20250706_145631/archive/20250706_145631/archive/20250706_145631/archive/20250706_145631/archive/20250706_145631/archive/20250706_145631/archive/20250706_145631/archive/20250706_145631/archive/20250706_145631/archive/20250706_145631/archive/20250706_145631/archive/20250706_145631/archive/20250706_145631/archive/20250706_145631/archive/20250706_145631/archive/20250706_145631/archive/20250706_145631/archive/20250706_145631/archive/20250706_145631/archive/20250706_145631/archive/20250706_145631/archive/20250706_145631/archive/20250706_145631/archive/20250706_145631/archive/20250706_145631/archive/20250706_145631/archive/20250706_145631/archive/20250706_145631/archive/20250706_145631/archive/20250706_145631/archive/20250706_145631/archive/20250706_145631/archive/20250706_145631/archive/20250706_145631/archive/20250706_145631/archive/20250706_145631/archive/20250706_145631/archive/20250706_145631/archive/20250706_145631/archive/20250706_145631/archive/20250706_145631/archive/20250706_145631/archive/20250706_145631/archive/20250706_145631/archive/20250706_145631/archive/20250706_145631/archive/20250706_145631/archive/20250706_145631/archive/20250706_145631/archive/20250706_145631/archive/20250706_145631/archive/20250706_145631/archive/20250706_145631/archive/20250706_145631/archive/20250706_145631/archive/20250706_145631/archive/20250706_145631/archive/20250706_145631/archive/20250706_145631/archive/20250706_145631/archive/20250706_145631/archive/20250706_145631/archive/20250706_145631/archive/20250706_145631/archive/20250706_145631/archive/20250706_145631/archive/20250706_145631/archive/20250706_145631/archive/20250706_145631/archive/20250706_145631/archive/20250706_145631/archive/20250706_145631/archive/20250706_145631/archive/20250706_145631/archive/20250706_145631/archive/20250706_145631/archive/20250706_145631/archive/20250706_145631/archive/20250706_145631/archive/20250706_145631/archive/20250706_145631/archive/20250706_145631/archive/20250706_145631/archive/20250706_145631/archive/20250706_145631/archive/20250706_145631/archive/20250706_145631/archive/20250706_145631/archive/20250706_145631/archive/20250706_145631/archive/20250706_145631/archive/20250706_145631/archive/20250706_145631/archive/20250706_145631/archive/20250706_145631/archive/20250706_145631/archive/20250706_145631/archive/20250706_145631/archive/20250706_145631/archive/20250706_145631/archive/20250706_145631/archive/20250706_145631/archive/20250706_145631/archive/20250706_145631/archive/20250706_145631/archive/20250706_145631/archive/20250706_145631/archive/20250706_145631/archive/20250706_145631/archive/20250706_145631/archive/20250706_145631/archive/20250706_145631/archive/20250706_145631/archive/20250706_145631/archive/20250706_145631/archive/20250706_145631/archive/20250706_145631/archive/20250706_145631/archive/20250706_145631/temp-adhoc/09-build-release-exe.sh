#!/bin/bash
# Ad-hoc Script 9: Build Release Executable

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

CC=${CC:-gcc}
CFLAGS="-O3 -DNDEBUG -Wall -Wextra"
EXE_NAME="polycall"
[[ "$(uname -s)" == *"NT"* ]] && EXE_NAME="polycall.exe"

echo "=== Building Release Executable ==="

if [[ ! -f "build/obj/cli/main.o" ]]; then
    echo "❌ main.o not found. Run compile scripts first."
    exit 1
fi

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Would build: build/bin/prod/$EXE_NAME"
    echo "[DRY-RUN] Linking with: build/lib/polycall.a"
else
    # Recompile main.c with release flags
    $CC $CFLAGS -Ibuild/include -Iinclude -c src/cli/main.c \
        -o build/obj/cli/main_release.o && \
    $CC $CFLAGS build/obj/cli/main_release.o build/lib/polycall.a \
        -o "build/bin/prod/$EXE_NAME" && \
        echo "✓ Created: build/bin/prod/$EXE_NAME" || \
        echo "❌ Failed to build release executable"
fi
