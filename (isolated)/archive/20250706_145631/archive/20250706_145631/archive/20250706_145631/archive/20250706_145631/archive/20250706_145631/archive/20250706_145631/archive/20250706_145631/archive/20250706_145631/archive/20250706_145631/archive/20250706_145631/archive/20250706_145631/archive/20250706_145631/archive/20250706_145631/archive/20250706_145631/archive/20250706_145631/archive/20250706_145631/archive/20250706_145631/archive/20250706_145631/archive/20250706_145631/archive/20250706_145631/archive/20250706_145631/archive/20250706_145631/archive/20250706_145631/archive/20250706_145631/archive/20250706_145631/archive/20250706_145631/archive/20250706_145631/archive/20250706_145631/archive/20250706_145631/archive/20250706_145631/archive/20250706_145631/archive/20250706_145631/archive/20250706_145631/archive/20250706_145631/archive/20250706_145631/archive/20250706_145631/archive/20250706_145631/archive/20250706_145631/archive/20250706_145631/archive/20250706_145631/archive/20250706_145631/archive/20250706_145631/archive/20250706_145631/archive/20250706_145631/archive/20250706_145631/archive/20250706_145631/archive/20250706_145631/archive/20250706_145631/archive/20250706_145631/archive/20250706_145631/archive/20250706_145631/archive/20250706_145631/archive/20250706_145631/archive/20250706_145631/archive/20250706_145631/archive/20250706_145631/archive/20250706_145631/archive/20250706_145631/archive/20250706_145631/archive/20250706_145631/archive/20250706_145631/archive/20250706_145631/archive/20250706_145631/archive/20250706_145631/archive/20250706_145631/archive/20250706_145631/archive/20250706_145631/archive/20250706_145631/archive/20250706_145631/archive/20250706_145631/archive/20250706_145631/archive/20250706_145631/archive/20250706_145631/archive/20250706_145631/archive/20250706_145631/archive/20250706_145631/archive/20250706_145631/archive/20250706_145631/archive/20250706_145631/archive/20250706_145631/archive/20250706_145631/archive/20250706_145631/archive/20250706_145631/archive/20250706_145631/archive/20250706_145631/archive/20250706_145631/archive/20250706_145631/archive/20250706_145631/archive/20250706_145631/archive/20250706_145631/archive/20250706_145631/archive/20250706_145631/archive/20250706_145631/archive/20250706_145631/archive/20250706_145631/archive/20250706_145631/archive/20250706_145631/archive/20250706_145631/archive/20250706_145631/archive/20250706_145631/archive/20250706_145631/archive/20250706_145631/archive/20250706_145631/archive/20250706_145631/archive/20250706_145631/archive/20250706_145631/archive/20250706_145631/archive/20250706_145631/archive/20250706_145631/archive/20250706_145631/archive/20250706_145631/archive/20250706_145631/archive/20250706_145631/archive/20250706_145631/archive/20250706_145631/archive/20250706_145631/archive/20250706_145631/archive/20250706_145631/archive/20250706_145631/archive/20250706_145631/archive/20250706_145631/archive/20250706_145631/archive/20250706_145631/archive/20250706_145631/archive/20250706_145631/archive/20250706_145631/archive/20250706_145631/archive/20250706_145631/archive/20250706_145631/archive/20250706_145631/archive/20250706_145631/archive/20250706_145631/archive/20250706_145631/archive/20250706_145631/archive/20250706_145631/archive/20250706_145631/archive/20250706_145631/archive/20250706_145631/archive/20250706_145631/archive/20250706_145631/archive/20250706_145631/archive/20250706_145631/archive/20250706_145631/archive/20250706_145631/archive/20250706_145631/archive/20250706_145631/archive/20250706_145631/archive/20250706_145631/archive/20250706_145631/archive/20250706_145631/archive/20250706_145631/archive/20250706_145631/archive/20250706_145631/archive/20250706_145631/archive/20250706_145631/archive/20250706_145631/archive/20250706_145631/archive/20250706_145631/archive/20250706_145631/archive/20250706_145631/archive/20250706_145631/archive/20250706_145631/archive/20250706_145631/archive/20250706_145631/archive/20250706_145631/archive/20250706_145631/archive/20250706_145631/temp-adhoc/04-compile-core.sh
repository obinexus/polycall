#!/bin/bash
# Ad-hoc Script 4: Compile Core Modules

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

CC=${CC:-gcc}
CFLAGS="-Wall -Wextra -O2 -fPIC -Ibuild/include -Iinclude -Isrc"

echo "=== Compiling Core Modules ==="

MODULES=(auth accessibility config edge ffi micro network protocol telemetry polycall)

for module in "${MODULES[@]}"; do
    SRC_DIR="src/core/$module"
    OBJ_DIR="build/obj/core/$module"
    
    if [[ -d "$SRC_DIR" ]]; then
        echo "Compiling module: $module"
        for src in "$SRC_DIR"/*.c; do
            [[ -f "$src" ]] || continue
            obj="$OBJ_DIR/$(basename "${src%.c}.o")"
            
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Would compile: $src -> $obj"
            else
                $CC $CFLAGS -c "$src" -o "$obj" 2>/dev/null && \
                    echo "  ✓ $(basename "$src")" || \
                    echo "  ❌ $(basename "$src")"
            fi
        done
    fi
done
