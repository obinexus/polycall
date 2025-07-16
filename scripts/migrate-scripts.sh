#!/bin/bash
# scripts/migrate-scripts.sh

mkdir -p ../build-tools
find ../scripts -type f \( -name "*.sh" -o -name "*.py" \) -exec cp {} ../build-tools/ \;
