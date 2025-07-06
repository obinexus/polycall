#!/bin/bash
# run_unit_tests.sh - Execute unit tests with result tracking

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Default values
BUILD_MODE="debug"
RESULTS_FILE=""
PARALLEL=0
TIMEOUT=5000

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --mode)
            BUILD_MODE="$2"
            shift 2
            ;;
        --results)
            RESULTS_FILE="$2"
            shift 2
            ;;
        --parallel)
            PARALLEL=1
            shift
            ;;
        --timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

# Test binary directory
TEST_BIN_DIR="${PROJECT_ROOT}/build/${BUILD_MODE}/bin/tests"

# Find all test executables
TEST_BINS=$(find "${TEST_BIN_DIR}" -name "test_*" -type f -executable 2>/dev/null || true)

if [[ -z "${TEST_BINS}" ]]; then
    echo "No test binaries found in ${TEST_BIN_DIR}"
    exit 1
fi

# Run tests and collect results
run_test() {
    local test_bin=$1
    local test_name=$(basename "${test_bin}")
    local test_output=$(mktemp)
    local test_result=0
    
    echo "Running: ${test_name}"
    
    # Set timeout and run test
    if timeout "${TIMEOUT}ms" "${test_bin}" > "${test_output}" 2>&1; then
        test_result=0
    else
        test_result=$?
    fi
    
    # Parse output for TP/TN/FP/FN
    local tp_count=$(grep -c "\[TP " "${test_output}" || true)
    local tn_count=$(grep -c "\[TN " "${test_output}" || true)
    local fp_count=$(grep -c "\[FP " "${test_output}" || true)
    local fn_count=$(grep -c "\[FN " "${test_output}" || true)
    
    # Update results file if provided
    if [[ -n "${RESULTS_FILE}" ]]; then
        python3 << EOF
import json

with open('${RESULTS_FILE}', 'r') as f:
    data = json.load(f)

test_data = {
    'name': '${test_name}',
    'result': ${test_result},
    'metrics': {
        'TP': ${tp_count},
        'TN': ${tn_count},
        'FP': ${fp_count},
        'FN': ${fn_count}
    }
}

# Categorize test result
if ${test_result} == 0:
    if ${fp_count} == 0 and ${fn_count} == 0:
        data['results']['TP'].append(test_data)
    else:
        if ${fp_count} > 0:
            data['results']['FP'].append(test_data)
        if ${fn_count} > 0:
            data['results']['FN'].append(test_data)
else:
    data['results']['TN'].append(test_data)

with open('${RESULTS_FILE}', 'w') as f:
    json.dump(data, f, indent=2)
EOF
    fi
    
    # Clean up
    rm -f "${test_output}"
    
    return ${test_result}
}

# Execute tests
FAILED_TESTS=0

if [[ ${PARALLEL} -eq 1 ]]; then
    # Run tests in parallel
    export -f run_test
    echo "${TEST_BINS}" | xargs -P $(nproc) -I {} bash -c 'run_test "$@"' _ {} || FAILED_TESTS=$?
else
    # Run tests sequentially
    while IFS= read -r test_bin; do
        if ! run_test "${test_bin}"; then
            ((FAILED_TESTS++))
        fi
    done <<< "${TEST_BINS}"
fi

# Summary
echo "========================================="
echo "Unit Test Summary"
echo "========================================="
echo "Total tests run: $(echo "${TEST_BINS}" | wc -l)"
echo "Failed tests: ${FAILED_TESTS}"

exit ${FAILED_TESTS}
