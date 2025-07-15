#!/bin/bash
# cleanup.sh - Master wrapper for running all cleanup scripts in sequence
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERBOSE=${1:-false}
DRY_RUN=${2:-false}

declare -a CLEANUP_SCRIPTS=(
    "01-clear-bin.sh"
    "02-clear-lib.sh" 
    "03-reset-cache.sh"
    "04-prune-dotfiles.sh"
    "05-validate-config.sh"
    "06-orphan-finder.sh"
    "07-empty-dir-audit.sh"
    "08-metadata-fixer.sh"
    "09-log-archive.sh"
    "10-clean-claude-hooks.sh"
)

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >&2
}

run_script() {
    local script="$1"
    local script_path="$SCRIPT_DIR/$script"
    
    if [[ ! -f "$script_path" ]]; then
        log "ERROR: Script $script not found at $script_path"
        return 1
    fi
    
    log "Running $script..."
    if [[ "$DRY_RUN" == "true" ]]; then
        "$script_path" --dry-run ${VERBOSE:+--verbose}
    else
        "$script_path" ${VERBOSE:+--verbose}
    fi
    
    local exit_code=$?
    if [[ $exit_code -eq 0 ]]; then
        log "✅ $script completed successfully"
    else
        log "❌ $script failed with exit code $exit_code"
        return $exit_code
    fi
}

# Add to cleanup.sh
update_cleanup_timestamp() {
    date +%s > ".last_cleanup"
    rm -f ".cleanup_required"
    log "Cleanup timestamp updated"
}
main() {
    log "Starting Polycall cleanup phase (${#CLEANUP_SCRIPTS[@]} scripts)"
    log "Mode: $([ "$DRY_RUN" == "true" ] && echo "DRY-RUN" || echo "EXECUTE")"
    
    local failed_scripts=()
    
    for script in "${CLEANUP_SCRIPTS[@]}"; do
        if ! run_script "$script"; then
            failed_scripts+=("$script")
        fi
    done
    
    if [[ ${#failed_scripts[@]} -eq 0 ]]; then
        log "🎉 All cleanup scripts completed successfully"
        return 0
    else
        log "💥 ${#failed_scripts[@]} scripts failed: ${failed_scripts[*]}"
        return 1
    fi
}

# Usage: ./cleanup.sh [--verbose] [--dry-run]
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [--verbose] [--dry-run]"
        echo "  --verbose    Enable verbose output"
        echo "  --dry-run    Show what would be done without executing"
        exit 0
        ;;
    --verbose)
        VERBOSE=true
        DRY_RUN="${2:-false}"
        ;;
    --dry-run)
        DRY_RUN=true
        VERBOSE="${2:-false}"
        ;;
esac

main "$@"
