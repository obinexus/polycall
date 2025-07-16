#!/bin/bash
# Fault Detection Script for PolyBuild

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="${SCRIPT_DIR}/../logs/fault_detector.log"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

check_node_health() {
    local node=$1
    if command -v "$node" >/dev/null 2>&1; then
        log "Node $node: HEALTHY"
        return 0
    else
        log "Node $node: FAILED"
        return 1
    fi
}

check_build_system() {
    local build_system=$1
    case $build_system in
        make)
            check_node_health "make"
            ;;
        cmake)
            check_node_health "cmake"
            ;;
        meson)
            check_node_health "meson"
            ;;
        *)
            log "Unknown build system: $build_system"
            return 1
            ;;
    esac
}

detect_faults() {
    log "Starting fault detection..."
    
    local faults=0
    
    # Check build tools
    for tool in make cmake meson gcc clang; do
        if ! check_node_health "$tool"; then
            ((faults++))
        fi
    done
    
    # Check disk space
    local disk_usage=$(df . | tail -1 | awk '{print $5}' | sed 's/%//')
    if [ "$disk_usage" -gt 90 ]; then
        log "FAULT: Disk usage critical: ${disk_usage}%"
        ((faults++))
    fi
    
    # Check memory
    local mem_usage=$(free | grep Mem | awk '{printf "%.0f", $3/$2 * 100.0}')
    if [ "$mem_usage" -gt 90 ]; then
        log "FAULT: Memory usage critical: ${mem_usage}%"
        ((faults++))
    fi
    
    log "Fault detection completed: $faults faults detected"
    return $faults
}

# Main execution
if [ "$1" = "check" ]; then
    detect_faults
    exit $?
elif [ "$1" = "node" ] && [ -n "$2" ]; then
    check_build_system "$2"
    exit $?
else
    echo "Usage: $0 {check|node <build_system>}"
    exit 1
fi
