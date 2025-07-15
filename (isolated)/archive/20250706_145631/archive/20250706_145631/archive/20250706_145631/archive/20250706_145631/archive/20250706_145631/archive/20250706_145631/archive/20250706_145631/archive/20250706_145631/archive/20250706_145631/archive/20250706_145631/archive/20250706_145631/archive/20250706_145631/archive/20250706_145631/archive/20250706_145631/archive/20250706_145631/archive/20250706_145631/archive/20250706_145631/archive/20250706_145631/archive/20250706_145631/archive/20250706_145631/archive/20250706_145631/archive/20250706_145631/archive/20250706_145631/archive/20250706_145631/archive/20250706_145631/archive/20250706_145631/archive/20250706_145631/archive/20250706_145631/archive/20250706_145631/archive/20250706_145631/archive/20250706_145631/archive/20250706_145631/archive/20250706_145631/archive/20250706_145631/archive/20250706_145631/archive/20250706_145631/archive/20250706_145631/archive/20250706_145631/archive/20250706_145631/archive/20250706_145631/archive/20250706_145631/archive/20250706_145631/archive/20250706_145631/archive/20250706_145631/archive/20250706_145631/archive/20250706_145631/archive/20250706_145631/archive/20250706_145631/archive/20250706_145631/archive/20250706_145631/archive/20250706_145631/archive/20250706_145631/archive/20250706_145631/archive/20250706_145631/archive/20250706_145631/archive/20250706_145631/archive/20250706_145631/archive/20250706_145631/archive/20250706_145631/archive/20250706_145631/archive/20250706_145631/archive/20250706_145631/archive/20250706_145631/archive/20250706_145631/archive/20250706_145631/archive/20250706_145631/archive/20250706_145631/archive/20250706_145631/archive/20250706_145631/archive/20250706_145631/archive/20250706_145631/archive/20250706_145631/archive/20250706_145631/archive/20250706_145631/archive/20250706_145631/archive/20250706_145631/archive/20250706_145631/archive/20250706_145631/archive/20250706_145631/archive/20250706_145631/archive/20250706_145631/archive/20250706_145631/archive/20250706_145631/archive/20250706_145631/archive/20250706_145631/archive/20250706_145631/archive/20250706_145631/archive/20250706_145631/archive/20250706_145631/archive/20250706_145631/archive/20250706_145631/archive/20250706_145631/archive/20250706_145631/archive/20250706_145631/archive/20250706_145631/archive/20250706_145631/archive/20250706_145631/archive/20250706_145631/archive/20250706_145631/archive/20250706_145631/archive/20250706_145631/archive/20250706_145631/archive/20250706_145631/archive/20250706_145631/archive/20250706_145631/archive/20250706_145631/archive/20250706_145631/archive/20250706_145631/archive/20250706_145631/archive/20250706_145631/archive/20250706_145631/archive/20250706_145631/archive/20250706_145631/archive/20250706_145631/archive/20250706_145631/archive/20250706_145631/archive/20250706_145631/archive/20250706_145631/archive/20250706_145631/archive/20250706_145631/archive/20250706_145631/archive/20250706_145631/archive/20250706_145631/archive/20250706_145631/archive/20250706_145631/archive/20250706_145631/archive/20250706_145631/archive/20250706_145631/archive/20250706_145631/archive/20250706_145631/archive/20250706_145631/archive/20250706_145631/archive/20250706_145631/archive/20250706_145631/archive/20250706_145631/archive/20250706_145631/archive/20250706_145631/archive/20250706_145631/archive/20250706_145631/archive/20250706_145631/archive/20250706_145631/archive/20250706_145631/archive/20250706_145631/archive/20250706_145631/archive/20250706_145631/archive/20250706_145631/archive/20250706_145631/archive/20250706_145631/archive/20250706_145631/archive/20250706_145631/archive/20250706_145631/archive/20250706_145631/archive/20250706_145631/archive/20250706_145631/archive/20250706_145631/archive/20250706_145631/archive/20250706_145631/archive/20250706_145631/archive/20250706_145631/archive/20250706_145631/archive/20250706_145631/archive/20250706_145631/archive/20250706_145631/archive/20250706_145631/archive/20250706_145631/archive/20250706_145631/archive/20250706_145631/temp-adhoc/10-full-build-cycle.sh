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
