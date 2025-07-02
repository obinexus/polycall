#!/bin/bash
# Master Module-Waterfall Orchestrator
# Phase: Full Module Lifecycle

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

# Core modules
MODULES=(auth edge ffi micro network protocol telemetry accessibility config)
PHASES=(01-requirements 02-design 03-implementation 04-testing 05-deployment 06-maintenance)

# Execute phase for single module
execute_module_phase() {
    local module="$1"
    local phase="$2"
    local script="$ADHOC_DIR/modules/$module/$phase/main.sh"
    
    if [ -f "$script" ]; then
        echo "Executing: $module/$phase"
        bash "$script"
    else
        echo "Not implemented: $module/$phase"
    fi
}

# Execute phase for all modules
execute_phase_all_modules() {
    local phase="$1"
    echo "=== Executing $phase for all modules ==="
    
    for module in "${MODULES[@]}"; do
        execute_module_phase "$module" "$phase"
    done
}

# Execute all phases for single module
execute_module_lifecycle() {
    local module="$1"
    echo "=== Executing full lifecycle for module: $module ==="
    
    for phase in "${PHASES[@]}"; do
        execute_module_phase "$module" "$phase"
    done
}

# Main command dispatcher
case "${1:-help}" in
    module)
        # Execute specific module through all phases
        if [ -n "$2" ]; then
            execute_module_lifecycle "$2"
        else
            echo "Usage: $0 module <module-name>"
            exit 1
        fi
        ;;
    phase)
        # Execute specific phase for all modules
        if [ -n "$2" ]; then
            execute_phase_all_modules "$2"
        else
            echo "Usage: $0 phase <phase-name>"
            exit 1
        fi
        ;;
    matrix)
        # Show module-phase matrix
        echo "=== Module-Phase Matrix ==="
        printf "%-15s" "Module"
        for phase in "${PHASES[@]}"; do
            printf "%-20s" "${phase#*-}"
        done
        echo ""
        
        for module in "${MODULES[@]}"; do
            printf "%-15s" "$module"
            for phase in "${PHASES[@]}"; do
                if [ -f "$ADHOC_DIR/modules/$module/$phase/main.sh" ]; then
                    printf "%-20s" "✓"
                else
                    printf "%-20s" "○"
                fi
            done
            echo ""
        done
        ;;
    test)
        # Run TDD for specific module
        if [ -n "$2" ]; then
            echo "Running TDD for module: $2"
            execute_module_phase "$2" "04-testing"
        else
            echo "Usage: $0 test <module-name>"
            exit 1
        fi
        ;;
    help|*)
        echo "Usage: $0 {module|phase|matrix|test} [args]"
        echo ""
        echo "Commands:"
        echo "  module <name>  - Execute full lifecycle for module"
        echo "  phase <name>   - Execute phase for all modules"
        echo "  matrix         - Show module-phase implementation matrix"
        echo "  test <module>  - Run TDD tests for module"
        echo ""
        echo "Modules: ${MODULES[*]}"
        echo "Phases: ${PHASES[*]}"
        ;;
esac
