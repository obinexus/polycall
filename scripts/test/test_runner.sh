#!/bin/bash

# CBLPolyCall Test Runner v1.0
# OBINexus Aegis Engineering - Automated Testing Framework

set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$TEST_DIR")"
TARGET_DIR="$PROJECT_ROOT/target"

# Test configuration
TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=0

log_test() { echo "[TEST] $1"; }
log_pass() { echo "[PASS] $1"; ((TESTS_PASSED++)); }
log_fail() { echo "[FAIL] $1"; ((TESTS_FAILED++)); }

# Test 1: Executable exists
test_executable_exists() {
    log_test "Checking if CBLPolyCall executable exists..."
    if [ -f "$TARGET_DIR/cblpolycall" ] || [ -f "$TARGET_DIR/cblpolycall.exe" ]; then
        log_pass "Executable found"
    else
        log_fail "Executable not found"
    fi
    ((TOTAL_TESTS++))
}

# Test 2: Executable runs
test_executable_runs() {
    log_test "Testing if CBLPolyCall executable runs..."
    local exe_path=""
    if [ -f "$TARGET_DIR/cblpolycall" ]; then
        exe_path="$TARGET_DIR/cblpolycall"
    elif [ -f "$TARGET_DIR/cblpolycall.exe" ]; then
        exe_path="$TARGET_DIR/cblpolycall.exe"
    fi
    
    if [ -n "$exe_path" ]; then
        # Test with timeout to prevent hanging
        if timeout 5s echo "5" | "$exe_path" >/dev/null 2>&1; then
            log_pass "Executable runs successfully"
        else
            log_fail "Executable failed to run or hanged"
        fi
    else
        log_fail "No executable found to test"
    fi
    ((TOTAL_TESTS++))
}

# Main test execution
main() {
    echo "CBLPolyCall Test Runner v1.0"
    echo "============================"
    echo ""
    
    test_executable_exists
    test_executable_runs
    
    echo ""
    echo "Test Results:"
    echo "  Total Tests: $TOTAL_TESTS"
    echo "  Passed: $TESTS_PASSED"
    echo "  Failed: $TESTS_FAILED"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo "All tests passed!"
        exit 0
    else
        echo "Some tests failed!"
        exit 1
    fi
}

main "$@"
