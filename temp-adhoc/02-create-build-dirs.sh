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
