#!/bin/bash
# Script to format all C/C++ code in the RIFT project

# Find all C/C++ files in include and src directories
find include src -name "*.c" -o -name "*.h" | while read -r file; do
    echo "Formatting $file"
    clang-format -i "$file"
done

echo "Formatting complete!"
