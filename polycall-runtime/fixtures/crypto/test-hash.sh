#!/bin/bash
# Test crypto hashing functionality

echo "Testing PolyCall crypto hash..."
echo "test data" | polycall crypto hash --algorithm=sha256
