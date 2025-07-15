#!/bin/bash
# 02-clear-lib.sh - Removes compiled libraries
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

declare -a LIB_PATTERNS=(
    "*.a"     # Static libraries
    "*.so"    # Shared libraries (Linux)
    "*.so.*"  # Versioned shared libraries
    "*.dll"   # Windows dynamic libraries
    "*.dylib" # macOS dynamic libraries
    "*.lib"   # Windows static libraries
    "*.pdb"   # Debug symbols
    "*.exp"   # Export files
    "*.ilk"   # Incremental link files
)

log() { [[ "$VERBOSE" == "true" ]] && echo "[CLEAR-LIB] $*" >&2 || true; }

clean_libraries() {
    log "Scanning for compiled libraries..."
    
    for pattern in "${LIB_PATTERNS[@]}"; do
        local files
        files=$(find . -name "$pattern" -type f 2>/dev/null || true)
        
        if [[ -n "$files" ]]; then
            log "Found libraries matching: $pattern"
            while IFS= read -r file; do
                # Skip system directories and protected paths
                if [[ "$file" =~ ^\./(usr|opt|system|windows)/ ]]; then
                    log "Skipping system library: $file"
                    continue
                fi
                
                if [[ "$DRY_RUN" == "true" ]]; then
                    echo "[DRY-RUN] Would delete library: $file"
                else
                    log "Deleting library: $file"
                    rm -f "$file"
                fi
            done <<< "$files"
        fi
    done
}

main() {
    log "Starting library cleanup..."
    clean_libraries
    log "Library cleanup completed"
}

main "$@"
