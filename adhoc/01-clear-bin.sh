#!/bin/bash
# 01-clear-bin.sh - Deletes all old compiled binaries
set -euo pipefail

VERBOSE=false
DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose) VERBOSE=true; shift ;;
        --dry-run) DRY_RUN=true; shift ;;
        *) shift ;;
    esac
done

declare -a BINARY_PATTERNS=(
    "*.exe"
    "*.out" 
    "*.bin"
    "polycall"
    "polycall.exe"
    "test_*"
    "bench_*"
    "*_test"
    "*_bench"
)

log() { [[ "$VERBOSE" == "true" ]] && echo "[CLEAR-BIN] $*" >&2 || true; }

find_and_clean() {
    local pattern="$1"
    local files
    
    # Find all matching files
    files=$(find . -name "$pattern" -type f 2>/dev/null || true)
    
    if [[ -n "$files" ]]; then
        log "Found files matching pattern: $pattern"
        while IFS= read -r file; do
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Would delete: $file"
            else
                log "Deleting: $file"
                rm -f "$file"
            fi
        done <<< "$files"
    fi
}

main() {
    log "Starting binary cleanup..."
    
    for pattern in "${BINARY_PATTERNS[@]}"; do
        find_and_clean "$pattern"
    done
    
    # Clean build output directories
    for dir in build bin out target; do
        if [[ -d "$dir" ]]; then
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Would clean directory: $dir"
            else
                log "Cleaning build directory: $dir"
                find "$dir" -name "*.exe" -o -name "*.out" -o -name "*.bin" | xargs -r rm -f
            fi
        fi
    done
    
    log "Binary cleanup completed"
}

main "$@"
