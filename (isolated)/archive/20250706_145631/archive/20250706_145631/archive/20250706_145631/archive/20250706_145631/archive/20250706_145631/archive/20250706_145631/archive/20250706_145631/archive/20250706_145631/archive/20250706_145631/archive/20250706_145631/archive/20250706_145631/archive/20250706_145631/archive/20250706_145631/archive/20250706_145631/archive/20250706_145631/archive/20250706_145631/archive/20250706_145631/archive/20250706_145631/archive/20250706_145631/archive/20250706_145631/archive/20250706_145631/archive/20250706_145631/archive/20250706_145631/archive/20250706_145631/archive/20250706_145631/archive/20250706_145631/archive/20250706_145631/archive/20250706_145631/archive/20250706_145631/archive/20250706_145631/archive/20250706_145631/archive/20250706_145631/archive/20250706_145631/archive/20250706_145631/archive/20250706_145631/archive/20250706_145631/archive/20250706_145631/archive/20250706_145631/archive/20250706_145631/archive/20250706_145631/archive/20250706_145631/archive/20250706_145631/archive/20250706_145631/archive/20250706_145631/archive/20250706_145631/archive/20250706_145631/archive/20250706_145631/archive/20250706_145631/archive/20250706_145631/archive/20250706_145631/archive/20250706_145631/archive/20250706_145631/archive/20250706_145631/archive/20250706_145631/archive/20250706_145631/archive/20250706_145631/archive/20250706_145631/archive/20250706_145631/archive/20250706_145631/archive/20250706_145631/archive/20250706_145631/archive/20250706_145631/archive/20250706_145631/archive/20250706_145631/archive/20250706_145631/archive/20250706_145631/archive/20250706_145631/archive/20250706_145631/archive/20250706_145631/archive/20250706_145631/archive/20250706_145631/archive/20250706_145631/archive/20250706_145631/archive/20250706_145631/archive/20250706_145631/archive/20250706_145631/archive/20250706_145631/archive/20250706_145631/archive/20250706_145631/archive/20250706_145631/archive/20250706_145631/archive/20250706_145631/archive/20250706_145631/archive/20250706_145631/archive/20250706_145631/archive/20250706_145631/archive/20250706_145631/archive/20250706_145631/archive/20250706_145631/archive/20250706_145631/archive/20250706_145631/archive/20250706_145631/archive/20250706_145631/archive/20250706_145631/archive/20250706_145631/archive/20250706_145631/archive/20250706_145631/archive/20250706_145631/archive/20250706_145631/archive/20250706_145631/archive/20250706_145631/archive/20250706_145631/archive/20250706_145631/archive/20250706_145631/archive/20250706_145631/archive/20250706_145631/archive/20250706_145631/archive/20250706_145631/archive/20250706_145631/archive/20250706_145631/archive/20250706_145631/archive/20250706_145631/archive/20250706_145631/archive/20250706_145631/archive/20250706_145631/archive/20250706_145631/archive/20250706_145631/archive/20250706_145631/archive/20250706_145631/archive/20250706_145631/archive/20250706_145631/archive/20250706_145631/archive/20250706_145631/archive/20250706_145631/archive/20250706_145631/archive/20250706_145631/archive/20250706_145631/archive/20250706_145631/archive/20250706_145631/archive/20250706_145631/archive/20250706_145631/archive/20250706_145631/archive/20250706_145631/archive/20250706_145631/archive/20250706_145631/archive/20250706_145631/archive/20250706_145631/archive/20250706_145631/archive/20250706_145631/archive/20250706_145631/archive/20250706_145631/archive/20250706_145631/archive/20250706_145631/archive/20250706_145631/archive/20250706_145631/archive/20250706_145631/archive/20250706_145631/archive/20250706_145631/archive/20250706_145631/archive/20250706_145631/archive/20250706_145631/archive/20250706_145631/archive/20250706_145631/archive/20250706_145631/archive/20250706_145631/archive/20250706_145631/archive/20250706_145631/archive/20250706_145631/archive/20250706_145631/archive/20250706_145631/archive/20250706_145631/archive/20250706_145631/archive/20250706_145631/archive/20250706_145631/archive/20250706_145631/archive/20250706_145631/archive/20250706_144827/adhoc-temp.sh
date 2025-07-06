#!/bin/bash
# LibPolyCall v2 Temporary Ad-hoc Build Scripts
# OBINexus Aegis Project - Build System Migration
# Scripts 1-10 with --dry-run support

# Create temp-adhoc directory
mkdir -p temp-adhoc

# Script 1: Validate Include Paths
cat > temp-adhoc/01-validate-includes.sh << 'SCRIPT1'
#!/bin/bash
# Ad-hoc Script 1: Validate Include Path Structure
# Ensures all includes use polycall/ prefix, not libpolycall/

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

echo "=== Include Path Validation ==="
echo "Scanning for incorrect include patterns..."

ERRORS=0
while IFS= read -r file; do
    if grep -E '#include.*[<"]libpolycall/' "$file" > /dev/null; then
        echo "❌ Found libpolycall/ prefix in: $file"
        if [[ "$DRY_RUN" == "false" ]]; then
            sed -i 's/libpolycall\//polycall\//g' "$file"
            echo "   ✓ Fixed: libpolycall/ -> polycall/"
        else
            echo "   [DRY-RUN] Would fix: libpolycall/ -> polycall/"
        fi
        ((ERRORS++))
    fi
done < <(find src include -name "*.c" -o -name "*.h" 2>/dev/null)

echo "Total issues found: $ERRORS"
SCRIPT1

# Script 2: Create Build Directory Structure
cat > temp-adhoc/02-create-build-dirs.sh << 'SCRIPT2'
#!/bin/bash
# Ad-hoc Script 2: Create Centralized Build Structure

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

echo "=== Creating Build Directory Structure ==="

DIRS=(
    "build/obj/core/auth"
    "build/obj/core/accessibility"
    "build/obj/core/config"
    "build/obj/core/edge"
    "build/obj/core/ffi"
    "build/obj/core/micro"
    "build/obj/core/network"
    "build/obj/core/protocol"
    "build/obj/core/telemetry"
    "build/obj/core/polycall"
    "build/obj/cli/commands"
    "build/obj/cli/providers"
    "build/obj/cli/repl"
    "build/lib"
    "build/bin/debug"
    "build/bin/prod"
    "build/include/polycall"
)

for dir in "${DIRS[@]}"; do
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY-RUN] Would create: $dir"
    else
        mkdir -p "$dir"
        echo "✓ Created: $dir"
    fi
done
SCRIPT2

# Script 3: Stage Headers
cat > temp-adhoc/03-stage-headers.sh << 'SCRIPT3'
#!/bin/bash
# Ad-hoc Script 3: Stage Headers to Build Directory

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

echo "=== Staging Headers ==="

if [[ -d "include/polycall" ]]; then
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY-RUN] Would copy: include/polycall -> build/include/polycall"
        find include/polycall -name "*.h" | head -10
    else
        cp -r include/polycall build/include/
        echo "✓ Staged headers to build/include/polycall"
        echo "  Total headers: $(find build/include/polycall -name "*.h" | wc -l)"
    fi
else
    echo "❌ Source include/polycall not found"
fi
SCRIPT3

# Script 4: Compile Core Modules
cat > temp-adhoc/04-compile-core.sh << 'SCRIPT4'
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
SCRIPT4

# Script 5: Compile CLI Modules
cat > temp-adhoc/05-compile-cli.sh << 'SCRIPT5'
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
SCRIPT5

# Script 6: Create Static Library
cat > temp-adhoc/06-create-static-lib.sh << 'SCRIPT6'
#!/bin/bash
# Ad-hoc Script 6: Create Static Library (polycall.a)

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

AR=${AR:-ar}
LIB_NAME="polycall.a"

echo "=== Creating Static Library ==="

# Find all object files
OBJS=$(find build/obj -name "*.o" 2>/dev/null | grep -v main.o)
OBJ_COUNT=$(echo "$OBJS" | wc -w)

if [[ $OBJ_COUNT -eq 0 ]]; then
    echo "❌ No object files found"
    exit 1
fi

echo "Found $OBJ_COUNT object files"

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Would create: build/lib/$LIB_NAME"
    echo "[DRY-RUN] Using $OBJ_COUNT object files"
else
    $AR rcs "build/lib/$LIB_NAME" $OBJS && \
        echo "✓ Created: build/lib/$LIB_NAME" || \
        echo "❌ Failed to create static library"
fi
SCRIPT6

# Script 7: Create Shared Library
cat > temp-adhoc/07-create-shared-lib.sh << 'SCRIPT7'
#!/bin/bash
# Ad-hoc Script 7: Create Shared Library (polycall.so)

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

CC=${CC:-gcc}
LIB_NAME="polycall.so"

echo "=== Creating Shared Library ==="

# Find all object files
OBJS=$(find build/obj -name "*.o" 2>/dev/null | grep -v main.o)
OBJ_COUNT=$(echo "$OBJS" | wc -w)

if [[ $OBJ_COUNT -eq 0 ]]; then
    echo "❌ No object files found"
    exit 1
fi

echo "Found $OBJ_COUNT object files"

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Would create: build/lib/$LIB_NAME"
    echo "[DRY-RUN] Using $OBJ_COUNT object files"
else
    $CC -shared -o "build/lib/$LIB_NAME" $OBJS && \
        echo "✓ Created: build/lib/$LIB_NAME" || \
        echo "❌ Failed to create shared library"
fi
SCRIPT7

# Script 8: Build Debug Executable
cat > temp-adhoc/08-build-debug-exe.sh << 'SCRIPT8'
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
SCRIPT8

# Script 9: Build Release Executable
cat > temp-adhoc/09-build-release-exe.sh << 'SCRIPT9'
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
SCRIPT9

# Script 10: Full Build Cycle
cat > temp-adhoc/10-full-build-cycle.sh << 'SCRIPT10'
#!/bin/bash
# Ad-hoc Script 10: Full Build Cycle Orchestrator

DRY_RUN=false
[[ "$1" == "--dry-run" ]] && DRY_RUN=true

echo "=== LibPolyCall v2 Full Build Cycle ==="
echo "Mode: $([[ "$DRY_RUN" == "true" ]] && echo "DRY-RUN" || echo "EXECUTE")"
echo

# Execute all scripts in sequence
SCRIPTS=(
    "01-validate-includes.sh"
    "02-create-build-dirs.sh"
    "03-stage-headers.sh"
    "04-compile-core.sh"
    "05-compile-cli.sh"
    "06-create-static-lib.sh"
    "07-create-shared-lib.sh"
    "08-build-debug-exe.sh"
    "09-build-release-exe.sh"
)

for script in "${SCRIPTS[@]}"; do
    echo "─────────────────────────────────────"
    echo "Running: $script"
    echo "─────────────────────────────────────"
    
    if [[ -f "temp-adhoc/$script" ]]; then
        bash "temp-adhoc/$script" $1
        RESULT=$?
        if [[ $RESULT -ne 0 ]]; then
            echo "❌ Script failed: $script (exit code: $RESULT)"
            exit $RESULT
        fi
    else
        echo "❌ Script not found: temp-adhoc/$script"
    fi
    echo
done

echo "=== Build Cycle Complete ==="
if [[ "$DRY_RUN" == "false" ]]; then
    echo "Build artifacts:"
    echo "  Static lib: $(ls -lh build/lib/polycall.a 2>/dev/null || echo 'Not found')"
    echo "  Shared lib: $(ls -lh build/lib/polycall.so 2>/dev/null || echo 'Not found')"
    echo "  Debug exe:  $(ls -lh build/bin/debug/polycall* 2>/dev/null || echo 'Not found')"
    echo "  Release exe:$(ls -lh build/bin/prod/polycall* 2>/dev/null || echo 'Not found')"
fi
SCRIPT10

# Make all scripts executable
chmod +x temp-adhoc/*.sh

echo "Created 10 ad-hoc scripts in temp-adhoc/"
echo "All scripts support --dry-run flag for testing"
echo "Run: bash temp-adhoc/10-full-build-cycle.sh --dry-run"