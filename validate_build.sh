#!/bin/bash
# Build validation script

echo "=== PolyCall Build Validation ==="

# Check for required directories
echo -n "Checking directories... "
if [[ -d "src" && -d "include" ]]; then
    echo "✓"
else
    echo "✗"
    echo "ERROR: Missing required directories"
    exit 1
fi

# Check for header files
echo -n "Checking headers... "
HEADER_COUNT=$(find include -name "*.h" -type f | wc -l)
if [[ $HEADER_COUNT -gt 0 ]]; then
    echo "✓ ($HEADER_COUNT headers found)"
else
    echo "✗"
    echo "ERROR: No header files found"
    exit 1
fi

# Test compilation
echo "Testing compilation..."
make -f Makefile.build test-build

if [[ $? -eq 0 ]]; then
    echo ""
    echo "✅ Build validation PASSED"
else
    echo ""
    echo "❌ Build validation FAILED"
    exit 1
fi
