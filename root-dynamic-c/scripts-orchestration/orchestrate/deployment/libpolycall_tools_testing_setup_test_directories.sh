#!/bin/bash
# setup_test_directories.sh - Create test directory structure mirroring source

SOURCE_DIR="$1"
TEST_DIR="$2"

if [ -z "$SOURCE_DIR" ] || [ -z "$TEST_DIR" ]; then
    echo "Usage: $0 <source_directory> <test_directory>"
    exit 1
fi

# Create test framework directory if it doesn't exist
mkdir -p "$TEST_DIR/framework"

# Process all directories in source
find "$SOURCE_DIR" -type d | while read -r dir; do
    # Get relative path
    rel_path="${dir#$SOURCE_DIR/}"
    
    # Skip if it's the root directory
    if [ -z "$rel_path" ]; then
        continue
    fi
    
    # Create corresponding test directory
    test_subdir="$TEST_DIR/$rel_path"
    mkdir -p "$test_subdir"
    
    # Create CMakeLists.txt if it doesn't exist
    if [ ! -f "$test_subdir/CMakeLists.txt" ]; then
        echo "# Tests for $rel_path" > "$test_subdir/CMakeLists.txt"
        echo "set(TEST_SOURCES)" >> "$test_subdir/CMakeLists.txt"
        echo "set(TEST_INCLUDES \${CMAKE_CURRENT_SOURCE_DIR})" >> "$test_subdir/CMakeLists.txt"
        echo "include_directories(\${TEST_INCLUDES})" >> "$test_subdir/CMakeLists.txt"
        echo "# Test executables will be added here" >> "$test_subdir/CMakeLists.txt"
    fi
    
    # Find source files and create corresponding test files if they don't exist
    find "$dir" -maxdepth 1 -name "*.c" | while read -r src_file; do
        src_file_name=$(basename "$src_file" .c)
        test_file="$test_subdir/test_${src_file_name}.c"
        
        # Create test file if it doesn't exist
        if [ ! -f "$test_file" ]; then
            echo "Creating test file: $test_file"
            create_test_file_template "$src_file" "$test_file" "$rel_path"
        fi
    done
done

echo "Test directory structure created successfully."