#!/bin/bash
# Compliance Check - Sinphasé Governance Verification
# Phase: Quality Assurance

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
COMPLIANCE_DIR="$PROJECT_ROOT/.compliance"

echo "=== Sinphasé Governance Compliance Check ==="
echo "Date: $(date)"
echo ""

# Check directory structure
echo "Checking directory structure..."
required_dirs=("src/core" "include/polycall" "scripts/adhoc" "tests")
missing=0

for dir in "${required_dirs[@]}"; do
    if [ -d "$PROJECT_ROOT/$dir" ]; then
        echo "  ✓ $dir"
    else
        echo "  ✗ $dir (missing)"
        ((missing++))
    fi
done

if [ $missing -eq 0 ]; then
    echo ""
    echo "✓ All compliance checks passed"
    exit 0
else
    echo ""
    echo "✗ Found $missing compliance issues"
    exit 1
fi
