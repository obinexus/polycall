#!/bin/bash
# Memory leak detection using valgrind

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TESTS_DIR}/../build"

find "${BUILD_DIR}/tests" -name "test_*" -type f -executable | while read -r test_exe; do
    echo "Checking memory leaks in $(basename "${test_exe}")..."
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
             --log-file="${TESTS_DIR}/reports/$(basename "${test_exe}").valgrind" \
             "${test_exe}"
done
