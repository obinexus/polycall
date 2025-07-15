#!/bin/bash
# 04-prune-dotfiles.sh - Removes stale dotfiles
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

declare -a PRUNE_PATTERNS=(
    ".DS_Store"
    "Thumbs.db"
    ".swp"
    ".swo"
    ".tmp"
    ".*.tmp"
    ".#*"
    "*~"
    ".vscode/settings.json.bak"
)

log() { [[ "$VERBOSE" == "true" ]] && echo "[PRUNE-DOTFILES] $*" >&2 || true; }

prune_gitkeep() {
    log "Checking .gitkeep files in non-empty directories..."
    
    find . -name ".gitkeep" -type f | while IFS= read -r gitkeep; do
        local dir
        dir=$(dirname "$gitkeep")
        
        # Count non-hidden files in directory (excluding .gitkeep itself)
        local file_count
        file_count=$(find "$dir" -maxdepth 1 -type f ! -name ".gitkeep" ! -name ".*" | wc -l)
        
        if [[ $file_count -gt 0 ]]; then
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Would remove unnecessary .gitkeep: $gitkeep (directory has $file_count files)"
            else
                log "Removing unnecessary .gitkeep: $gitkeep (directory has $file_count files)"
                rm -f "$gitkeep"
            fi
        else
            log "Keeping .gitkeep in empty directory: $dir"
        fi
    done
}

prune_stale_dotfiles() {
    for pattern in "${PRUNE_PATTERNS[@]}"; do
        local files
        files=$(find . -name "$pattern" -type f 2>/dev/null || true)
        
        if [[ -n "$files" ]]; then
            log "Found stale dotfiles matching: $pattern"
            while IFS= read -r file; do
                if [[ "$DRY_RUN" == "true" ]]; then
                    echo "[DRY-RUN] Would delete stale dotfile: $file"
                else
                    log "Deleting stale dotfile: $file"
                    rm -f "$file"
                fi
            done <<< "$files"
        fi
    done
}

main() {
    log "Starting dotfile pruning..."
    prune_gitkeep
    prune_stale_dotfiles
    log "Dotfile pruning completed"
}

main "$@"
