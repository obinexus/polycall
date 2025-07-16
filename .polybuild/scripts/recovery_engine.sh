#!/bin/bash
# Recovery Engine Script for PolyBuild

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="${SCRIPT_DIR}/../logs/recovery_engine.log"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

attempt_recovery() {
    local failed_system=$1
    log "Attempting recovery for failed system: $failed_system"
    
    case $failed_system in
        make)
            log "Attempting CMake fallback..."
            if command -v cmake >/dev/null 2>&1; then
                make build-cmake
                return $?
            fi
            log "Attempting Meson fallback..."
            if command -v meson >/dev/null 2>&1; then
                make build-meson
                return $?
            fi
            ;;
        cmake)
            log "Attempting Make fallback..."
            if command -v make >/dev/null 2>&1; then
                make build-make
                return $?
            fi
            log "Attempting Meson fallback..."
            if command -v meson >/dev/null 2>&1; then
                make build-meson
                return $?
            fi
            ;;
        meson)
            log "Attempting Make fallback..."
            if command -v make >/dev/null 2>&1; then
                make build-make
                return $?
            fi
            log "Attempting CMake fallback..."
            if command -v cmake >/dev/null 2>&1; then
                make build-cmake
                return $?
            fi
            ;;
        *)
            log "Unknown system: $failed_system"
            return 1
            ;;
    esac
    
    log "All recovery attempts failed"
    return 1
}

# Main execution
if [ "$1" = "recover" ] && [ -n "$2" ]; then
    attempt_recovery "$2"
    exit $?
else
    echo "Usage: $0 recover <failed_system>"
    exit 1
fi
