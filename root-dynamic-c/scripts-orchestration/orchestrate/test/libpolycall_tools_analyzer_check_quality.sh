#!/bin/bash

# Comprehensive quality enforcement for LibRift
# This script identifies and fixes common code quality issues

echo "Initiating code quality enforcement..."

# Fix copyright notices
echo "Fixing missing copyright notices..."
find src include -name "*.c" -o -name "*.h" | xargs grep -L "Copyright.*LibRift" | while read -r file; do
    file_name=$(basename "$file")
    file_desc=$(grep -A 1 "@brief" "$file" | tail -n 1 | sed 's/\* //')
    
    if [ -z "$file_desc" ]; then
        file_desc="Implementation for the ${file_name%.*} module"
    fi
    
    # Create standard header
    copyright_notice="/**\n * @file ${file_name}\n * @brief ${file_desc}\n * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)\n *\n * @copyright Copyright (c) 2025 LibRift Project\n * @license MIT License\n */\n\n"
    
    # Insert at beginning of file
    temp_file=$(mktemp)
    echo -e "$copyright_notice" > "$temp_file"
    cat "$file" >> "$temp_file"
    mv "$temp_file" "$file"
    echo "Added copyright to $file"
done

# Standardize header guards
echo "Standardizing header guards..."
find include -name "*.h" | while read -r header; do
    base_name=$(basename "$header" .h)
    path_part=$(dirname "$header" | sed 's|^include/||')
    guard_name=$(echo "${path_part}_${base_name}_h" | tr '/' '_' | tr '[:lower:]' '[:upper:]')
    
    # Check if guard exists but is incorrect
    current_guard=$(grep -m 1 "#ifndef" "$header" | awk '{print $2}')
    if [ -n "$current_guard" ] && [ "$current_guard" != "$guard_name" ]; then
        # Replace with correct guard
        sed -i "s/#ifndef ${current_guard}/#ifndef ${guard_name}/g" "$header"
        sed -i "s/#define ${current_guard}/#define ${guard_name}/g" "$header"
        sed -i "s/#endif \/\* ${current_guard} \*\//#endif \/* ${guard_name} *\//g" "$header"
        sed -i "s/#endif.*/${guard_name}/#endif \/* ${guard_name} *\//g" "$header"
        echo "Fixed guard in $header from $current_guard to $guard_name"
    elif [ -z "$current_guard" ]; then
        # Guard missing, add it
        temp_file=$(mktemp)
        cat "$header" > "$temp_file"
        
        # Create new file with guard
        cat > "$header" << EOL
#ifndef ${guard_name}
#define ${guard_name}

EOL
        
        # Add C++ guard if not present
        if ! grep -q "__cplusplus" "$temp_file"; then
            cat >> "$header" << EOL
#ifdef __cplusplus
extern "C" {
#endif

EOL
            cat "$temp_file" >> "$header"
            cat >> "$header" << EOL

#ifdef __cplusplus
}
#endif

EOL
        else
            cat "$temp_file" >> "$header"
        fi
        
        # Add ending guard
        cat >> "$header" << EOL
#endif /* ${guard_name} */
EOL
        rm "$temp_file"
        echo "Added missing guard to $header"
    fi
done

# Fix include ordering
echo "Fixing include ordering..."
find src -name "*.c" | while read -r src_file; do
    base_name=$(basename "$src_file" .c)
    module_path=$(dirname "$src_file" | sed 's|^src/||')
    own_header="#include \"${module_path}/${base_name}.h\""
    
    # Get all includes
    all_includes=$(grep "^#include" "$src_file")
    
    # Remove all includes
    sed -i '/^#include/d' "$src_file"
    
    # Add own header first
    if echo "$all_includes" | grep -q "${base_name}.h"; then
        # Insert own header
        sed -i "1i${own_header}" "$src_file"
        
        # Add system headers
        system_headers=$(echo "$all_includes" | grep "#include <" | sort)
        if [ -n "$system_headers" ]; then
            echo "$system_headers" > temp_includes.txt
            sed -i "/^${own_header}/r temp_includes.txt" "$src_file"
        fi
        
        # Add other project headers
        project_headers=$(echo "$all_includes" | grep "#include \"" | grep -v "${base_name}.h" | sort)
        if [ -n "$project_headers" ]; then
            echo "$project_headers" > temp_includes.txt
            # Find where to insert them
            if [ -n "$system_headers" ]; then
                last_system=$(grep -n "#include <" "$src_file" | tail -1 | cut -d: -f1)
                sed -i "${last_system}r temp_includes.txt" "$src_file"
            else
                sed -i "/^${own_header}/r temp_includes.txt" "$src_file"
            fi
        fi
        
        rm -f temp_includes.txt
        echo "Fixed include ordering in $src_file"
    fi
done

# Document remaining TODOs
echo "Documenting TODOs..."
todo_file="TODO_ITEMS.md"
echo "# LibRift TODO Items" > "$todo_file"
echo "Generated on $(date)" >> "$todo_file"
echo "" >> "$todo_file"

find src include -name "*.c" -o -name "*.h" | xargs grep -n "TODO" | while read -r todo_line; do
    file=$(echo "$todo_line" | cut -d: -f1)
    line_no=$(echo "$todo_line" | cut -d: -f2)
    todo_text=$(echo "$todo_line" | cut -d: -f3-)
    
    echo "## $file (Line $line_no)" >> "$todo_file"
    echo "```c" >> "$todo_file"
    echo "$todo_text" >> "$todo_file"
    echo "```" >> "$todo_file"
    echo "" >> "$todo_file"
done

echo "Quality enforcement complete."
echo "TODO items documented in $todo_file"