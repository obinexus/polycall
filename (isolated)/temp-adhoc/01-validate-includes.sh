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
