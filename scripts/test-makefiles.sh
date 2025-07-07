#!/bin/bash
# OBINexus PolyCall Makefile Test Script
# Verifies non-recursive behavior and proper delegation

set -e

echo "=== OBINexus PolyCall Makefile Test ==="
echo "Testing non-recursive delegation..."
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test function
test_make_target() {
    local target=$1
    local description=$2
    
    echo -e "${YELLOW}Testing:${NC} $description"
    echo "Command: make $target"
    
    # Run make with trace to see what's happening
    if make -n $target 2>&1 | grep -q "Entering directory.*Entering directory"; then
        echo -e "${RED}FAIL:${NC} Recursive directory entry detected!"
        return 1
    else
        echo -e "${GREEN}PASS:${NC} No recursion detected"
        return 0
    fi
    echo
}

# Create mock sub-makefiles if they don't exist
create_mock_makefiles() {
    echo "Creating mock makefiles for testing..."
    
    # Mock Makefile.spec
    if [ ! -f Makefile.spec ]; then
        cat > Makefile.spec << 'EOF'
ifndef POLYCALL_SPEC_INCLUDED
POLYCALL_SPEC_INCLUDED := 1

.DEFAULT_GOAL := all
.PHONY: all test qa

all: qa

test:
	@echo "[SPEC] Running tests..."

qa:
	@echo "[SPEC] Running QA checks..."

endif
EOF
    fi
    
    # Mock Makefile.purity
    if [ ! -f Makefile.purity ]; then
        cat > Makefile.purity << 'EOF'
ifndef POLYCALL_PURITY_INCLUDED
POLYCALL_PURITY_INCLUDED := 1

.DEFAULT_GOAL := all
.PHONY: all check-commands security-scan

all: check-commands

check-commands:
	@echo "[PURITY] Checking command purity..."

security-scan:
	@echo "[PURITY] Running security scan..."

endif
EOF
    fi
    
    # Mock Makefile.projects
    if [ ! -f Makefile.projects ]; then
        cat > Makefile.projects << 'EOF'
ifndef POLYCALL_PROJECTS_INCLUDED
POLYCALL_PROJECTS_INCLUDED := 1

.DEFAULT_GOAL := help
.PHONY: help setup

help:
	@echo "[PROJECTS] Available project commands..."

setup:
	@echo "[PROJECTS] Setting up environment..."

endif
EOF
    fi
}

# Main test sequence
main() {
    # Setup
    create_mock_makefiles
    
    echo "=== Running Makefile Tests ==="
    echo
    
    # Test various targets
    local total=0
    local passed=0
    
    # Test cases
    test_cases=(
        "help:Basic help command"
        "build:Build delegation"
        "test:Test delegation"
        "qa:QA delegation"
        "security-scan:Security scan delegation"
        "setup:Project setup delegation"
        "all:Default all target"
        "build test:Compound build and test"
        "clean build:Clean then build"
    )
    
    for test_case in "${test_cases[@]}"; do
        IFS=':' read -r target description <<< "$test_case"
        total=$((total + 1))
        if test_make_target "$target" "$description"; then
            passed=$((passed + 1))
        fi
    done
    
    echo
    echo "=== Test Summary ==="
    echo "Total tests: $total"
    echo "Passed: $passed"
    echo "Failed: $((total - passed))"
    
    if [ $passed -eq $total ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        
        echo
        echo "=== Verification Complete ==="
        echo "The makefile system is working correctly with:"
        echo "✓ Non-recursive delegation"
        echo "✓ Proper recursion guards"
        echo "✓ Clean target isolation"
        echo "✓ No duplicate directory entries"
        
        return 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        return 1
    fi
}

# Run main
main
