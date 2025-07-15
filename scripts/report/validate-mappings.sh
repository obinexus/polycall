#!/bin/bash
# Directory Mapping Validator
# Phase: Structure Validation

echo "Validating directory mappings..."
if [ -d "src/core" ] && [ -d "include/polycall" ]; then
    echo "✓ Directory mappings valid"
else
    echo "✗ Directory mappings invalid"
    exit 1
fi
