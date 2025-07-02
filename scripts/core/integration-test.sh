#!/bin/bash
# Integration Test Framework
# Phase: Testing - Integration

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

# Run integration tests between modules
run_integration_tests() {
    echo "=== Running Integration Tests ==="
    
    # Test module interactions
    local test_pairs=(
        "auth:network"     # Auth + Network integration
        "edge:protocol"    # Edge + Protocol integration
        "ffi:micro"        # FFI + Microservices integration
        "telemetry:config" # Telemetry + Config integration
    )
    
    for pair in "${test_pairs[@]}"; do
        IFS=':' read -r module1 module2 <<< "$pair"
        echo "Testing integration: $module1 <-> $module2"
        
        # Run integration test if exists
        local test_script="$PROJECT_ROOT/tests/integration/test_${module1}_${module2}.py"
        if [ -f "$test_script" ]; then
            python3 "$test_script" --project-root "$PROJECT_ROOT"
        else
            echo "  - Integration test pending"
        fi
    done
}

# Validate module dependencies
validate_dependencies() {
    echo "=== Validating Module Dependencies ==="
    
    # Check include dependencies
    for module_dir in "$PROJECT_ROOT/include/polycall"/*; do
        if [ -d "$module_dir" ]; then
            module=$(basename "$module_dir")
            echo "Checking dependencies for: $module"
            
            # Analyze includes
            grep -h "^#include" "$module_dir"/*.h 2>/dev/null | \
                grep -v "polycall/$module" | \
                sort | uniq | \
                while read -r include; do
                    echo "  - Depends on: $include"
                done
        fi
    done
}

# Main execution
case "${1:-all}" in
    all)
        run_integration_tests
        validate_dependencies
        ;;
    test)
        run_integration_tests
        ;;
    deps)
        validate_dependencies
        ;;
    *)
        echo "Usage: $0 {all|test|deps}"
        ;;
esac
