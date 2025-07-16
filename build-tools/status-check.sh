#!/bin/bash
# build-tools/status-check.sh

assess_distress_state() {
    local warnings=$1
    case $warnings in
        0|1|2|3) echo "STATE_OK :: Artifact stable ✅" ;;
        4|5|6) echo "STATE_WARNING :: Degraded but buildable ⚠️" ;;
        7|8|9) echo "STATE_CRITICAL :: Major faults 🚨" ;;
        10|11|12) echo "STATE_PANIC :: Kill node to protect ring ❌" ;;
        *) echo "STATE_UNKNOWN :: Inconclusive" ;;
    esac
}

log_topology_state() {
    local topology=$1
    local state=$2
    
    echo "$(date +"%Y-%m-%d %H:%M:%S") - TOPOLOGY: $topology - STATE: $state" >> build/metadata/topology-health.log
}

# Main function
main() {
    local warnings=$1
    local topology=${2:-"unknown"}
    
    mkdir -p build/metadata
    
    state=$(assess_distress_state $warnings)
    log_topology_state "$topology" "$state"
    
    echo $state
    
    # Return exit code based on state
    case $state in
        *"STATE_OK"*) return 0 ;;
        *"STATE_WARNING"*) return 1 ;;
        *"STATE_CRITICAL"*) return 2 ;;
        *"STATE_PANIC"*) return 3 ;;
        *) return 4 ;;
    esac
}

main "$@"
