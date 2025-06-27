#!/bin/bash
# Performance profiling using perf

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TESTS_DIR}/../build"

find "${BUILD_DIR}/tests" -name "test_*qa*" -type f -executable | while read -r test_exe; do
    echo "Profiling performance of $(basename "${test_exe}")..."
    perf record -g "${test_exe}"
    perf report > "${TESTS_DIR}/reports/$(basename "${test_exe}").perf"
done
