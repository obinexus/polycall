#!/bin/bash
# ==================================================================
# LibPolyCall Test Execution Script
# OBINexus Framework - Comprehensive Test Runner
# ==================================================================

set -euo pipefail

# Configuration
TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TESTS_DIR}/../build"
REPORTS_DIR="${TESTS_DIR}/reports"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging
log() { echo -e "${BLUE}[$(date +'%H:%M:%S')]${NC} $1"; }
success() { echo -e "${GREEN}✅ $1${NC}"; }
error() { echo -e "${RED}❌ $1${NC}"; }
warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a single test
run_test() {
    local test_executable="$1"
    local test_name="$(basename "${test_executable}" .out)"
    
    log "Running ${test_name}..."
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if "${test_executable}" > "${REPORTS_DIR}/${test_name}.log" 2>&1; then
        success "${test_name} PASSED"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        error "${test_name} FAILED"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        warning "Log: ${REPORTS_DIR}/${test_name}.log"
        return 1
    fi
}

# Function to build tests
build_tests() {
    log "Building tests..."
    
    if [ ! -d "${BUILD_DIR}" ]; then
        mkdir -p "${BUILD_DIR}"
    fi
    
    cd "${BUILD_DIR}"
    
    if ! cmake .. -DBUILD_TESTS=ON; then
        error "CMake configuration failed"
        exit 1
    fi
    
    if ! make -j$(nproc); then
        error "Build failed"
        exit 1
    fi
    
    success "Tests built successfully"
}

# Function to run test category
run_test_category() {
    local category="$1"
    local test_dir="${BUILD_DIR}/tests/${category}"
    
    if [ ! -d "${test_dir}" ]; then
        warning "Test directory ${test_dir} not found, skipping ${category}"
        return
    fi
    
    log "Running ${category} tests..."
    
    # Find and run all test executables
    find "${test_dir}" -name "test_*" -type f -executable | while read -r test_exe; do
        run_test "${test_exe}"
    done
}

# Main execution
main() {
    echo "========================================"
    echo "LibPolyCall Comprehensive Test Suite"
    echo "OBINexus Framework Testing"
    echo "========================================"
    
    # Prepare reports directory
    mkdir -p "${REPORTS_DIR}"
    rm -f "${REPORTS_DIR}"/*.log
    
    # Build tests
    build_tests
    
    # Run test categories in order
    log "Starting test execution..."
    
    # 1. Unit tests (basic functionality)
    run_test_category "unit"
    
    # 2. Unit QA tests (resilience and error handling)
    run_test_category "unit_qa"
    
    # 3. Integration tests (cross-module functionality)
    run_test_category "integration"
    
    # 4. Integration QA tests (system-wide resilience)
    run_test_category "integration_qa"
    
    # Generate summary report
    echo "========================================"
    echo "Test Execution Summary"
    echo "========================================"
    echo "Total Tests: ${TOTAL_TESTS}"
    echo "Passed: ${PASSED_TESTS}"
    echo "Failed: ${FAILED_TESTS}"
    
    if [ ${FAILED_TESTS} -eq 0 ]; then
        success "All tests passed! 🎉"
        echo "LibPolyCall test suite completed successfully."
    else
        error "${FAILED_TESTS} test(s) failed"
        echo "Check individual test logs in ${REPORTS_DIR}/"
        exit 1
    fi
}

# Execute main function
main "$@"
