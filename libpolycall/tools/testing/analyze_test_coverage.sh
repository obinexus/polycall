#!/bin/bash
# analyze_test_coverage.sh - Generate and analyze test coverage

# Build directory
BUILD_DIR="$1"

if [ -z "$BUILD_DIR" ]; then
    echo "Usage: $0 <build_directory>"
    exit 1
fi

# Run tests with coverage enabled
cd "$BUILD_DIR"
cmake .. -DENABLE_TEST_COVERAGE=ON
make clean
make
make run_all_tests

# Generate coverage report
mkdir -p coverage-report
gcovr -r .. --html --html-details -o coverage-report/index.html
gcovr -r .. -s

echo "Coverage report generated in $BUILD_DIR/coverage-report/index.html"