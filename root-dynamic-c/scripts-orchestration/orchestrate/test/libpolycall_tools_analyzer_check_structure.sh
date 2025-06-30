#!/bin/bash

# LibPolyCall Include Structure Repair
# This script systematically analyzes and fixes include organization

echo "Starting systematic include structure repair..."

# Create backup directory
backup_dir="backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$backup_dir"
echo "Creating backups in $backup_dir"
find . -name "*.c" -o -name "*.h" | xargs -I{} cp --parents {} "$backup_dir/"

# Phase 1: Fix include paths with consistent naming patterns
echo "Phase 1: Standardizing include paths..."
find . -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "polycall/core/|#include "core/polycall/|g'
find . -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "protocol/|#include "core/protocol/|g'
find . -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "network/|#include "core/network/|g'

# Phase 2: Fix header match patterns for implementation files
echo "Phase 2: Ensuring implementation files include their headers..."
find . -name "*.c" | while read -r src_file; do
    base_name=$(basename "$src_file" .c)
    module_path=$(dirname "$src_file" | grep -oP '(?<=\/)[^/]+\/[^/]+$' || echo "")
    expected_header="core/$module_path/$base_name.h"
    
    if ! grep -q "#include \"$expected_header\"" "$src_file"; then
        # Insert corresponding header as first include
        sed -i "1i#include \"$expected_header\"" "$src_file"
        echo "Added $expected_header to $src_file"
    fi
done

# Phase 3: Sort includes in proper order
echo "Phase 3: Standardizing include order..."
find . -name "*.c" -o -name "*.h" | while read -r file; do
    # Extract all includes
    includes=$(grep "^#include" "$file")
    
    # Remove all includes
    sed -i '/^#include/d' "$file"
    
    # Group includes by type
    # 1. Module's own header first
    base_name=$(basename "$file" | cut -d. -f1)
    own_header=$(echo "$includes" | grep -i "$base_name\.h")
    
    # 2. System headers
    system_headers=$(echo "$includes" | grep "#include <" | sort)
    
    # 3. Project headers
    project_headers=$(echo "$includes" | grep "#include \"" | grep -v "$own_header" | sort)
    
    # Write includes back in proper order
    if [ -n "$own_header" ]; then
        echo "$own_header" > temp_includes.txt
        echo "" >> temp_includes.txt
    fi
    
    if [ -n "$system_headers" ]; then
        echo "$system_headers" >> temp_includes.txt
        echo "" >> temp_includes.txt
    fi
    
    if [ -n "$project_headers" ]; then
        echo "$project_headers" >> temp_includes.txt
    fi
    
    # Insert at top of file
    sed -i '1r temp_includes.txt' "$file"
    rm -f temp_includes.txt
done

# Phase 4: Check for circular dependencies
echo "Phase 4: Analyzing dependency structure..."
modules=("core/polycall" "core/network" "core/protocol" "core/ffi")
for module in "${modules[@]}"; do
    echo "Analyzing dependencies for module: $module"
    module_files=$(find . -path "*/$module/*.h")
    
    for file in $module_files; do
        circular_deps=$(grep -l "#include \"$module/" $file)
        if [ -n "$circular_deps" ]; then
            echo "  Potential circular dependency in $file:"
            grep "#include \"$module/" $file | sed 's/^/    /'
        fi
    done
done

echo "Include structure repair complete."