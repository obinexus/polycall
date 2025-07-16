#!/bin/bash
# build.sh

set -e

# Setup directories
./build-tools/setup-deps.sh

# Check for --qa-mode flag
if [ "$1" = "--qa-mode=true" ]; then
    export BUILD_MODE="qa"
fi

# Enable debug output if requested
if [ "$2" = "--debug" ]; then
    export BUILD_MODE="debug"
fi

# Run error isolation process
./build-tools/error-isolation.sh
ERROR_ISOLATION_EXIT=$?

# Check if we should continue
if [ $ERROR_ISOLATION_EXIT -ne 0 ]; then
    echo "❌ Build failed: Error threshold exceeded"
    exit 1
fi

# Generate build manifest
./build-tools/generate-manifest.sh

# Setup Zero-Trust Layer
./build-tools/setup-zero-trust.sh

# Final status check
./build-tools/status-check.sh
STATUS_CHECK_EXIT=$?

if [ $STATUS_CHECK_EXIT -eq 0 ]; then
    echo "✅ Build completed successfully"
    echo "🔒 Zero-Trust Layer activated"
    echo "Run '.polycall config -c' to access the configuration interface"
else
    echo "❌ Build status check failed"
    exit 1
fi

exit 0
