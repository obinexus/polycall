#!/bin/bash
# 03-reset-cache.sh - Clears build cache, temp objects, .polycache
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

declare -a CACHE_DIRS=(
    ".polycache"
    ".cache"
    "cache"
    "tmp"
    "temp"
    ".tmp"
    "build/cache"
    "obj"
    ".ccache"
    ".ninja_deps"
    ".ninja_log"
)

declare -a TEMP_PATTERNS=(
    "*.o"
    "*.obj"
    "*.tmp"
    "*.temp"
    "*.bak"
    "*.orig"
    "*~"
    ".#*"
    "core.*"
    "*.core"
)

log() { [[ "$VERBOSE" == "true" ]] && echo "[RESET-CACHE] $*" >&2 || true; }

clean_cache_dirs() {
    for dir in "${CACHE_DIRS[@]}"; do
        if [[ -d "$dir" ]]; then
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Would clean cache directory: $dir"
                echo "[DRY-RUN] Directory size: $(du -sh "$dir" 2>/dev/null | cut -f1)"
            else
                log "Cleaning cache directory: $dir ($(du -sh "$dir" 2>/dev/null | cut -f1))"
                rm -rf "$dir"
                mkdir -p "$dir" 2>/dev/null || true
            fi
        fi
    done
}

clean_temp_files() {
    log "Cleaning temporary files..."
    
    for pattern in "${TEMP_PATTERNS[@]}"; do
        local files
        files=$(find . -name "$pattern" -type f 2>/dev/null || true)
        
        if [[ -n "$files" ]]; then
            while IFS= read -r file; do
                if [[ "$DRY_RUN" == "true" ]]; then
                    echo "[DRY-RUN] Would delete temp file: $file"
                else
                    log "Deleting temp file: $file"
                    rm -f "$file"
                fi
            done <<< "$files"
        fi
    done
}

main() {
    log "Starting cache reset..."
    clean_cache_dirs
    clean_temp_files
    log "Cache reset completed"
}

main "$@"
