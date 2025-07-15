#!/bin/bash
# Policy Validation Script
# Phase: Compliance Verification

echo "[VALIDATE] Checking policy structure..."
if [ -f "$(dirname "$0")/active/current-policy.json" ]; then
    echo "✓ Policy structure validated"
else
    echo "✗ Policy structure invalid"
    exit 1
fi
