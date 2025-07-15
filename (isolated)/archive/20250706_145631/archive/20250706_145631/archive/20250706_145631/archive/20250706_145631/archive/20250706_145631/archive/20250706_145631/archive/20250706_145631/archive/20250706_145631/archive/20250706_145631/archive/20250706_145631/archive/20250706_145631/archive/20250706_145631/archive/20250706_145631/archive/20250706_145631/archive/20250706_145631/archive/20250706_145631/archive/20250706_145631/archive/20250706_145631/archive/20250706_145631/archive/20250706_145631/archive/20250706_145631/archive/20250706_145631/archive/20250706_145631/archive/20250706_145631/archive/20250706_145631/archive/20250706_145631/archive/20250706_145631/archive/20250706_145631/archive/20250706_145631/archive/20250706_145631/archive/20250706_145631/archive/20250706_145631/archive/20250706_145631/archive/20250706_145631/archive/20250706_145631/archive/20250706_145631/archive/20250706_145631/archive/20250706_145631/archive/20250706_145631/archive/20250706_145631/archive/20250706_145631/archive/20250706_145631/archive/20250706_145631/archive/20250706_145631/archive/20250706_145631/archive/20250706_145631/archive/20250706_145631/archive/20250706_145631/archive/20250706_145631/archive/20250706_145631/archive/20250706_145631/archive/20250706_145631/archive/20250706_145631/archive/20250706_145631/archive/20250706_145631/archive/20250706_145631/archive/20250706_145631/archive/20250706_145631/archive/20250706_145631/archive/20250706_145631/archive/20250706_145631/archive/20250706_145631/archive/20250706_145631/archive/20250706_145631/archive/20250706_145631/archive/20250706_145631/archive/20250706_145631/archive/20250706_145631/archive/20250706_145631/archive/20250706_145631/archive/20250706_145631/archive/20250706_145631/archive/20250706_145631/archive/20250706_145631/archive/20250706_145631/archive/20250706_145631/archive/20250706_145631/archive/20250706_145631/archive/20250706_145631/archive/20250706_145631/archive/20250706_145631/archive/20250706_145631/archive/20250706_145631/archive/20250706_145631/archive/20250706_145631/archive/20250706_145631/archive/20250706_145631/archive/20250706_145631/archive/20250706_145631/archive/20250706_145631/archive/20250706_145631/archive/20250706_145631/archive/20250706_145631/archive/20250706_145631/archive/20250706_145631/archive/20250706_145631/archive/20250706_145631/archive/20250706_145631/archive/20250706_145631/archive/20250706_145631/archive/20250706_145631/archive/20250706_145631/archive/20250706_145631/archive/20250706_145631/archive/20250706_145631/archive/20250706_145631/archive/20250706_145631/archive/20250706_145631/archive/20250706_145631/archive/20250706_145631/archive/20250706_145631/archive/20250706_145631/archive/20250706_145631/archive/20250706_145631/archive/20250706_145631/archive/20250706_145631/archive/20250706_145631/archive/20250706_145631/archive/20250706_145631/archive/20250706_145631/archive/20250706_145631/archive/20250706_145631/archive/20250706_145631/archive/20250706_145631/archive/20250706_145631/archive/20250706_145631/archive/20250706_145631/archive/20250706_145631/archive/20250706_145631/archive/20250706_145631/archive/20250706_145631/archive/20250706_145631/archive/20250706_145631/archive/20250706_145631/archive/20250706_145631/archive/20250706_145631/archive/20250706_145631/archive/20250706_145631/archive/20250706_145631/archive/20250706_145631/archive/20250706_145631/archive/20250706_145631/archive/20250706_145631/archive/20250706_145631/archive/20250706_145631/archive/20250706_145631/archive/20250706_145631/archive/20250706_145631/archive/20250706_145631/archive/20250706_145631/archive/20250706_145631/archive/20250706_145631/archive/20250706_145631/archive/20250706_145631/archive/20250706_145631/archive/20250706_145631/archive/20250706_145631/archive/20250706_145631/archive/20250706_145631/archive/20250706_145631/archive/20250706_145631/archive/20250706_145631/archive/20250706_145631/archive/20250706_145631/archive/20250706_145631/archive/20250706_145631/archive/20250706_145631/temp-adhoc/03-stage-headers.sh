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
