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
