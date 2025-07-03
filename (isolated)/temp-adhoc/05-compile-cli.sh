#!/bin/bash
# Ad-hoc Script 5: Compile CLI Modules

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

CC=${CC:-gcc}
CFLAGS="-Wall -Wextra -O2 -fPIC -Ibuild/include -Iinclude -Isrc"

echo "=== Compiling CLI Modules ==="

MODULES=(commands providers repl common)

for module in "${MODULES[@]}"; do
    SRC_DIR="src/cli/$module"
    OBJ_DIR="build/obj/cli/$module"
    
    if [[ -d "$SRC_DIR" ]]; then
        echo "Compiling CLI module: $module"
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

# Compile main.c separately
if [[ -f "src/cli/main.c" ]]; then
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY-RUN] Would compile: src/cli/main.c -> build/obj/cli/main.o"
    else
        $CC $CFLAGS -c src/cli/main.c -o build/obj/cli/main.o && \
            echo "✓ Compiled main.c" || \
            echo "❌ Failed to compile main.c"
    fi
fi
