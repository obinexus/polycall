#!/bin/bash
# claude-observer.sh - Automated project monitoring
set -euo pipefail

LOCK_FILE="/tmp/claude-observer.lock"
LOG_FILE="logs/claude-observer.log"
LAST_CLEANUP_FILE=".last_cleanup"

log_event() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOG_FILE"
}

check_singleton() {
    if [[ -f "$LOCK_FILE" ]]; then
        local pid
        pid=$(cat "$LOCK_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "Observer already running (PID: $pid)"
            exit 1
        else
            rm -f "$LOCK_FILE"
        fi
    fi
    echo $$ > "$LOCK_FILE"
    trap 'rm -f "$LOCK_FILE"' EXIT
}

observe_directory_changes() {
    log_event "Starting directory change monitoring"
    
    # Monitor src/ and config/ for changes
    inotifywait -m -r --format '%w%f %e' \
                -e modify,create,delete,move \
                src/ config/ 2>/dev/null | \
    while read file event; do
        log_event "File change detected: $file ($event)"
        
        case "$file" in
            *.cfg|*.json|*.yaml)
                log_event "Configuration change detected, running validation"
                ./05-validate-config.sh || log_event "Config validation failed for $file"
                ;;
            *.c|*.h)
                log_event "Source code change detected, running enexus-search"
                ./enexus-search.sh || log_event "Health check failed after change to $file"
                ;;
        esac
    done &
}

enforce_cleanup_policy() {
    if [[ ! -f "$LAST_CLEANUP_FILE" ]]; then
        log_event "No cleanup history found, skipping cleanup enforcement"
        return 0
    fi
    
    local last_cleanup_time
    last_cleanup_time=$(cat "$LAST_CLEANUP_FILE")
    local current_time
    current_time=$(date +%s)
    local hours_since_cleanup
    hours_since_cleanup=$(( (current_time - last_cleanup_time) / 3600 ))
    
    if [[ $hours_since_cleanup -gt 24 ]]; then
        log_event "WARNING: Cleanup not run in $hours_since_cleanup hours"
        log_event "Blocking refactor operations until cleanup is performed"
        
        # Create blocking sentinel
        touch ".cleanup_required"
        return 1
    fi
    
    return 0
}

auto_run_enexus_search() {
    local hour
    hour=$(date +%H)
    
    # Run nightly at 2 AM
    if [[ "$hour" == "02" ]]; then
        log_event "Running nightly enexus-search"
        if ./enexus-search.sh; then
            log_event "Nightly health check passed"
        else
            log_event "ALERT: Nightly health check failed"
            # Could send notification here
        fi
    fi
}

detect_orphan_files() {
    log_event "Running orphan file detection"
    
    if ./06-orphan-finder.sh; then
        log_event "No orphan files detected"
    else
        log_event "WARNING: Orphan files detected, manual review required"
    fi
}

block_refactor_if_needed() {
    if [[ -f ".cleanup_required" ]]; then
        log_event "BLOCKING: Refactor blocked due to required cleanup"
        echo "ERROR: Cleanup required before refactor operations" >&2
        echo "Run ./cleanup.sh to clear the block" >&2
        exit 1
    fi
}

main() {
    check_singleton
    
    log_event "Claude Observer started"
    
    case "${1:-monitor}" in
        monitor)
            observe_directory_changes
            while true; do
                auto_run_enexus_search
                detect_orphan_files
                enforce_cleanup_policy
                sleep 3600  # Check every hour
            done
            ;;
        check-cleanup)
            enforce_cleanup_policy
            ;;
        block-refactor)
            block_refactor_if_needed
            ;;
        *)
            echo "Usage: $0 [monitor|check-cleanup|block-refactor]"
            exit 1
            ;;
    esac
}

main "$@"
