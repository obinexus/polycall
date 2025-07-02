#!/bin/bash
# Directory Mapping Generator
# Maps include/polycall/{core,cli} to src/{core,cli}
# OBINexus Aegis Project - Build System Enhancement

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INCLUDE_DIR="$PROJECT_ROOT/include/polycall"
SRC_DIR="$PROJECT_ROOT/src"
MAPPING_FILE="$PROJECT_ROOT/.dirmappings"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Validate directory structure
validate_structure() {
    log_info "Validating directory structure..."
    
    local errors=0
    
    # Check required directories
    for dir in "$INCLUDE_DIR" "$SRC_DIR"; do
        if [ ! -d "$dir" ]; then
            log_error "Required directory not found: $dir"
            ((errors++))
        fi
    done
    
    # Check core modules
    for module in auth accessibility config edge ffi micro network protocol telemetry; do
        if [ ! -d "$SRC_DIR/core/$module" ]; then
            log_warn "Core module directory missing: $SRC_DIR/core/$module"
        fi
        if [ ! -d "$INCLUDE_DIR/core/$module" ]; then
            log_warn "Include directory missing: $INCLUDE_DIR/core/$module"
        fi
    done
    
    # Check CLI structure
    for component in commands providers repl; do
        if [ ! -d "$SRC_DIR/cli/$component" ]; then
            log_warn "CLI component missing: $SRC_DIR/cli/$component"
        fi
    done
    
    return $errors
}

# Generate mapping database
generate_mappings() {
    log_info "Generating directory mappings..."
    
    # Initialize mapping file
    cat > "$MAPPING_FILE" << EOF
# LibPolyCall Directory Mappings
# Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
# Format: INCLUDE_PATH:SOURCE_PATH:MODULE:STATUS

EOF
    
    # Map core modules
    log_info "Mapping core modules..."
    for module in auth accessibility config edge ffi micro network protocol telemetry polycall; do
        map_module "core" "$module"
    done
    
    # Map CLI components
    log_info "Mapping CLI components..."
    for component in commands providers repl common; do
        map_module "cli" "$component"
    done
    
    # Map special directories
    map_special_dirs
    
    log_info "Mapping complete: $MAPPING_FILE"
}

# Map individual module
map_module() {
    local category="$1"
    local module="$2"
    
    local include_path="$INCLUDE_DIR/$category/$module"
    local src_path="$SRC_DIR/$category/$module"
    
    # Check if paths exist
    local status="OK"
    if [ ! -d "$include_path" ] && [ ! -d "$src_path" ]; then
        status="MISSING"
    elif [ ! -d "$include_path" ]; then
        status="NO_INCLUDE"
    elif [ ! -d "$src_path" ]; then
        status="NO_SRC"
    fi
    
    # Record mapping
    echo "$include_path:$src_path:$category.$module:$status" >> "$MAPPING_FILE"
    
    # Generate detailed file mappings
    if [ -d "$include_path" ] && [ -d "$src_path" ]; then
        map_files "$include_path" "$src_path" "$category.$module"
    fi
}

# Map files within modules
map_files() {
    local include_dir="$1"
    local src_dir="$2"
    local module="$3"
    
    # Find all header files
    find "$include_dir" -name "*.h" -type f | while read -r header; do
        local rel_path="${header#$include_dir/}"
        local base_name="${rel_path%.h}"
        local src_file="$src_dir/${base_name}.c"
        
        # Check if corresponding source exists
        if [ -f "$src_file" ]; then
            echo "  $header:$src_file:$module:MAPPED" >> "$MAPPING_FILE"
        else
            echo "  $header::$module:NO_SOURCE" >> "$MAPPING_FILE"
        fi
    done
    
    # Find orphaned source files
    find "$src_dir" -name "*.c" -type f | while read -r source; do
        local rel_path="${source#$src_dir/}"
        local base_name="${rel_path%.c}"
        local header_file="$include_dir/${base_name}.h"
        
        if [ ! -f "$header_file" ]; then
            echo "  :$source:$module:NO_HEADER" >> "$MAPPING_FILE"
        fi
    done
}

# Map special directories
map_special_dirs() {
    log_info "Mapping special directories..."
    
    # Map root includes
    echo "$INCLUDE_DIR:$SRC_DIR:root:OK" >> "$MAPPING_FILE"
    
    # Map utility directories
    local special_dirs="common factory parser schema security socket"
    for dir in $special_dirs; do
        if [ -d "$INCLUDE_DIR/core/$dir" ] || [ -d "$SRC_DIR/core/$dir" ]; then
            map_module "core" "$dir"
        fi
    done
}

# Generate CMake include file
generate_cmake_mappings() {
    log_info "Generating CMake directory mappings..."
    
    local cmake_file="$PROJECT_ROOT/cmake/DirectoryMappings.cmake"
    mkdir -p "$(dirname "$cmake_file")"
    
    cat > "$cmake_file" << 'EOF'
# LibPolyCall Directory Mappings for CMake
# Auto-generated - Do not edit manually

# Function to map include directories to source directories
function(map_module_directories MODULE_NAME)
    set(MODULE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/polycall/${MODULE_NAME}")
    set(MODULE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/${MODULE_NAME}")
    
    if(EXISTS "${MODULE_INCLUDE_DIR}" AND EXISTS "${MODULE_SOURCE_DIR}")
        message(STATUS "Mapping ${MODULE_NAME}: ${MODULE_INCLUDE_DIR} -> ${MODULE_SOURCE_DIR}")
        
        # Collect source and header files
        file(GLOB_RECURSE MODULE_HEADERS "${MODULE_INCLUDE_DIR}/*.h")
        file(GLOB_RECURSE MODULE_SOURCES "${MODULE_SOURCE_DIR}/*.c")
        
        # Create module library target name
        string(REPLACE "/" "_" MODULE_TARGET "polycall_${MODULE_NAME}")
        
        # Store in parent scope
        set(${MODULE_TARGET}_HEADERS ${MODULE_HEADERS} PARENT_SCOPE)
        set(${MODULE_TARGET}_SOURCES ${MODULE_SOURCES} PARENT_SCOPE)
        set(${MODULE_TARGET}_INCLUDE_DIR ${MODULE_INCLUDE_DIR} PARENT_SCOPE)
    else()
        message(WARNING "Module ${MODULE_NAME} directory structure incomplete")
    endif()
endfunction()

# Map all core modules
set(CORE_MODULES
    core/auth
    core/accessibility
    core/config
    core/edge
    core/ffi
    core/micro
    core/network
    core/protocol
    core/telemetry
    core/polycall
)

# Map all CLI components
set(CLI_MODULES
    cli/commands
    cli/providers
    cli/repl
    cli/common
)

# Process all modules
foreach(MODULE ${CORE_MODULES} ${CLI_MODULES})
    map_module_directories(${MODULE})
endforeach()

# Global include directories
set(LIBPOLYCALL_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/polycall"
)

# Export for use in other CMake files
set(LIBPOLYCALL_INCLUDE_DIRS ${LIBPOLYCALL_INCLUDE_DIRS} PARENT_SCOPE)
EOF
    
    log_info "CMake mappings generated: $cmake_file"
}

# Generate validation script
generate_validator() {
    log_info "Generating mapping validator..."
    
    cat > "$PROJECT_ROOT/scripts/validate-mappings.sh" << 'EOF'
#!/bin/bash
# Directory Mapping Validator
# Validates include/src synchronization

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAPPING_FILE="$PROJECT_ROOT/.dirmappings"
ERRORS=0
WARNINGS=0

if [ ! -f "$MAPPING_FILE" ]; then
    echo "Error: Mapping file not found: $MAPPING_FILE"
    exit 1
fi

echo "=== Directory Mapping Validation ==="

# Check for missing mappings
grep ":MISSING" "$MAPPING_FILE" | while read -r line; do
    module=$(echo "$line" | cut -d: -f3)
    echo "❌ Missing module: $module"
    ((ERRORS++))
done

# Check for missing headers
grep ":NO_HEADER" "$MAPPING_FILE" | while read -r line; do
    source=$(echo "$line" | cut -d: -f2)
    echo "⚠️  Source without header: $source"
    ((WARNINGS++))
done

# Check for missing sources
grep ":NO_SOURCE" "$MAPPING_FILE" | while read -r line; do
    header=$(echo "$line" | cut -d: -f1 | sed 's/^  //')
    echo "⚠️  Header without source: $header"
    ((WARNINGS++))
done

echo
echo "=== Validation Summary ==="
echo "Errors: $ERRORS"
echo "Warnings: $WARNINGS"

if [ $ERRORS -gt 0 ]; then
    echo "❌ Validation failed"
    exit 1
else
    echo "✅ Validation passed"
fi
EOF
    chmod +x "$PROJECT_ROOT/scripts/validate-mappings.sh"
}

# Generate synchronization script
generate_sync_script() {
    log_info "Generating include/src synchronization script..."
    
    cat > "$PROJECT_ROOT/scripts/sync-directories.sh" << 'EOF'
#!/bin/bash
# Directory Synchronization Script
# Ensures include and src directories are properly aligned

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
INCLUDE_DIR="$PROJECT_ROOT/include/polycall"
SRC_DIR="$PROJECT_ROOT/src"

sync_module() {
    local module_path="$1"
    local include_path="$INCLUDE_DIR/$module_path"
    local src_path="$SRC_DIR/$module_path"
    
    echo "Syncing $module_path..."
    
    # Ensure directories exist
    mkdir -p "$include_path" "$src_path"
    
    # Sync headers to match sources
    find "$src_path" -name "*.c" -type f | while read -r source; do
        local rel_path="${source#$src_path/}"
        local base_name="${rel_path%.c}"
        local header_path="$include_path/${base_name}.h"
        
        if [ ! -f "$header_path" ]; then
            echo "  Creating header stub: $header_path"
            mkdir -p "$(dirname "$header_path")"
            generate_header_stub "$source" "$header_path" "$module_path"
        fi
    done
}

generate_header_stub() {
    local source_file="$1"
    local header_file="$2"
    local module="$3"
    
    local guard_name="POLYCALL_$(echo "$header_file" | sed 's/.*include\/polycall\///' | tr 'a-z/.\\-' 'A-Z___')_"
    
    cat > "$header_file" << HEADER
/*
 * LibPolyCall - $(basename "$header_file")
 * Auto-generated header stub for $(basename "$source_file")
 * Module: $module
 * OBINexus Aegis Project
 */

#ifndef ${guard_name}
#define ${guard_name}

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: Add function declarations from $source_file */

#ifdef __cplusplus
}
#endif

#endif /* ${guard_name} */
HEADER
}

# Sync all modules
for module in core/auth core/accessibility core/config core/edge core/ffi \
              core/micro core/network core/protocol core/telemetry \
              cli/commands cli/providers cli/repl; do
    if [ -d "$SRC_DIR/$module" ]; then
        sync_module "$module"
    fi
done

echo "✅ Directory synchronization complete"
EOF
    chmod +x "$PROJECT_ROOT/scripts/sync-directories.sh"
}

# Main execution
main() {
    echo "=== LibPolyCall Directory Mapping Generator ==="
    echo "Project root: $PROJECT_ROOT"
    echo
    
    # Validate structure
    if ! validate_structure; then
        log_error "Directory structure validation failed"
        exit 1
    fi
    
    # Generate mappings
    generate_mappings
    
    # Generate supporting files
    generate_cmake_mappings
    generate_validator
    generate_sync_script
    
    # Show summary
    echo
    echo "=== Mapping Summary ==="
    echo "Total mappings: $(grep -c "^[^#]" "$MAPPING_FILE" 2>/dev/null || echo 0)"
    echo "Missing modules: $(grep -c ":MISSING" "$MAPPING_FILE" 2>/dev/null || echo 0)"
    echo "Headers without source: $(grep -c ":NO_SOURCE" "$MAPPING_FILE" 2>/dev/null || echo 0)"
    echo "Sources without header: $(grep -c ":NO_HEADER" "$MAPPING_FILE" 2>/dev/null || echo 0)"
    echo
    echo "✅ Directory mapping generation complete"
    echo "   Mapping file: $MAPPING_FILE"
    echo "   CMake config: $PROJECT_ROOT/cmake/DirectoryMappings.cmake"
    echo "   Validator: $PROJECT_ROOT/scripts/validate-mappings.sh"
    echo "   Sync script: $PROJECT_ROOT/scripts/sync-directories.sh"
}

main
