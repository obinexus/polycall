#!/bin/bash
# generate-test-stubs.sh - Generate Empty Test Object Files
# Author: Implementation Team
# Purpose: Create empty object files for LibPolyCall components to enable independent testing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"

# Configuration
SRC_DIR="${PROJECT_ROOT}/src"
INCLUDE_DIR="${PROJECT_ROOT}/include"
OBJ_DIR="${PROJECT_ROOT}/obj"

# Set up compiler
CC="${CC:-gcc}"
CFLAGS="-c -fPIC -Wall -Werror -I${INCLUDE_DIR}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print usage information
print_usage() {
    echo -e "${BLUE}LibPolyCall Test Stub Generator${NC}"
    echo -e "Usage: $0 [OPTIONS] [COMPONENTS...]"
    echo -e "Options:"
    echo -e "  --all             Generate stubs for all components"
    echo -e "  --clean           Clean existing stub files before generating"
    echo -e "  --verbose         Display detailed output"
    echo -e "  --help            Display this help message"
    echo -e ""
    echo -e "If no components are specified, generates stubs for core components only."
    echo -e "COMPONENTS can be one or more of: polycall, auth, config, edge, ffi, micro, network, protocol, telemetry, accessibility"
}

# Core components to process
ALL_COMPONENTS=(
    "polycall"
    "auth"
    "config"
    "edge"
    "ffi"
    "micro"
    "network"
    "protocol"
    "telemetry"
    "accessibility"
)

# Function to generate stub header for a component
generate_stub_header() {
    component=$1
    output_dir=$2
    
    mkdir -p "${output_dir}"
    header_file="${output_dir}/polycall_${component}.h"
    
    if [ ! -f "${header_file}" ] || [ "${FORCE_REGENERATE}" = "true" ]; then
        echo -e "${BLUE}Generating stub header for ${component}...${NC}"
        
        cat > "${header_file}" << EOF
/**
 * @file polycall_${component}.h
 * @brief Stub header for ${component} component
 * @author LibPolyCall Implementation Team - Test Infrastructure
 */

#ifndef POLYCALL_${component^^}_H
#define POLYCALL_${component^^}_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core types
typedef struct polycall_${component}_context polycall_${component}_context_t;
typedef struct polycall_${component}_config polycall_${component}_config_t;

// Error codes
typedef enum {
    POLYCALL_${component^^}_SUCCESS = 0,
    POLYCALL_${component^^}_ERROR_GENERIC = -1,
    POLYCALL_${component^^}_ERROR_INVALID_PARAMETER = -2,
    POLYCALL_${component^^}_ERROR_NOT_INITIALIZED = -3,
    POLYCALL_${component^^}_ERROR_ALREADY_INITIALIZED = -4,
    POLYCALL_${component^^}_ERROR_OUT_OF_MEMORY = -5
} polycall_${component}_error_t;

// Initialization function
polycall_${component}_error_t polycall_${component}_init(
    polycall_${component}_context_t** ctx
);

// Cleanup function
void polycall_${component}_cleanup(
    polycall_${component}_context_t* ctx
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_${component^^}_H */
EOF
        echo -e "${GREEN}Generated ${header_file}${NC}"
    else
        [ "${VERBOSE}" = "true" ] && echo -e "${YELLOW}Skipping existing header: ${header_file}${NC}"
    fi
}

# Function to generate stub source for a component
generate_stub_source() {
    component=$1
    output_dir=$2
    
    mkdir -p "${output_dir}"
    source_file="${output_dir}/${component}.c"
    
    if [ ! -f "${source_file}" ] || [ "${FORCE_REGENERATE}" = "true" ]; then
        echo -e "${BLUE}Generating stub source for ${component}...${NC}"
        
        cat > "${source_file}" << EOF
/**
 * @file ${component}.c
 * @brief Stub implementation for ${component} component
 * @author LibPolyCall Implementation Team - Test Infrastructure
 */

#include "polycall/core/${component}/polycall_${component}.h"

/* Stub implementation for ${component} component */

// Context structure
struct polycall_${component}_context {
    bool initialized;
    void* placeholder;
};

// Config structure
struct polycall_${component}_config {
    char name[64];
    int placeholder;
};

// Initialization function
polycall_${component}_error_t polycall_${component}_init(
    polycall_${component}_context_t** ctx
) {
    if (!ctx) {
        return POLYCALL_${component^^}_ERROR_INVALID_PARAMETER;
    }
    
    *ctx = calloc(1, sizeof(polycall_${component}_context_t));
    if (!*ctx) {
        return POLYCALL_${component^^}_ERROR_OUT_OF_MEMORY;
    }
    
    (*ctx)->initialized = true;
    return POLYCALL_${component^^}_SUCCESS;
}

// Cleanup function
void polycall_${component}_cleanup(
    polycall_${component}_context_t* ctx
) {
    if (ctx) {
        free(ctx);
    }
}
EOF
        echo -e "${GREEN}Generated ${source_file}${NC}"
    else
        [ "${VERBOSE}" = "true" ] && echo -e "${YELLOW}Skipping existing source: ${source_file}${NC}"
    fi
}

# Function to compile stub object for a component
compile_stub_object() {
    component=$1
    source_file=$2
    output_dir="${OBJ_DIR}/core/${component}"
    
    mkdir -p "${output_dir}"
    object_file="${output_dir}/${component}.o"
    
    if [ ! -f "${object_file}" ] || [ "${FORCE_REGENERATE}" = "true" ]; then
        echo -e "${BLUE}Compiling stub object for ${component}...${NC}"
        
        ${CC} ${CFLAGS} -o "${object_file}" "${source_file}"
        
        echo -e "${GREEN}Compiled ${object_file}${NC}"
    else
        [ "${VERBOSE}" = "true" ] && echo -e "${YELLOW}Skipping existing object: ${object_file}${NC}"
    fi
}

# Function to generate all stubs for a component
generate_component_stubs() {
    component=$1
    
    # Create temp directory
    temp_dir=$(mktemp -d)
    trap 'rm -rf "${temp_dir}"' EXIT
    
    # Generate header
    component_include_dir="${INCLUDE_DIR}/polycall/core/${component}"
    mkdir -p "${component_include_dir}"
    generate_stub_header "${component}" "${component_include_dir}"
    
    # Generate source
    generate_stub_source "${component}" "${temp_dir}"
    
    # Compile object
    compile_stub_object "${component}" "${temp_dir}/${component}.c"
    
    # Generate standard object files required for testing
    for stub_type in "context" "error" "config"; do
        object_file="${OBJ_DIR}/core/${component}/${component}_${stub_type}.o"
        if [ ! -f "${object_file}" ] || [ "${FORCE_REGENERATE}" = "true" ]; then
            echo -e "${BLUE}Generating ${stub_type} stub for ${component}...${NC}"
            
            # Create simple source file
            cat > "${temp_dir}/${component}_${stub_type}.c" << EOF
/* Stub ${stub_type} implementation for ${component} */
void polycall_${component}_${stub_type}_stub(void) { }
EOF
            
            # Compile to object file
            ${CC} ${CFLAGS} -o "${object_file}" "${temp_dir}/${component}_${stub_type}.c"
            
            echo -e "${GREEN}Generated ${object_file}${NC}"
        else
            [ "${VERBOSE}" = "true" ] && echo -e "${YELLOW}Skipping existing object: ${object_file}${NC}"
        fi
    done
    
    # Generate mock object file
    mock_file="${OBJ_DIR}/core/${component}/mock_${component}.o"
    if [ ! -f "${mock_file}" ] || [ "${FORCE_REGENERATE}" = "true" ]; then
        echo -e "${BLUE}Generating mock object for ${component}...${NC}"
        
        # Create mock source file
        cat > "${temp_dir}/mock_${component}.c" << EOF
/**
 * @file mock_${component}.c
 * @brief Mock implementation for ${component} component testing
 * @author LibPolyCall Implementation Team - Test Infrastructure
 */

#include "polycall/core/${component}/polycall_${component}.h"

/* Mock implementation that can be used in tests */
void polycall_${component}_mock_init(void) { }
void polycall_${component}_mock_cleanup(void) { }
EOF
        
        # Compile to object file
        ${CC} ${CFLAGS} -o "${mock_file}" "${temp_dir}/mock_${component}.c"
        
        echo -e "${GREEN}Generated ${mock_file}${NC}"
    else
        [ "${VERBOSE}" = "true" ] && echo -e "${YELLOW}Skipping existing mock object: ${mock_file}${NC}"
    fi
}

# Function to clean component stubs
clean_component_stubs() {
    component=$1
    
    echo -e "${BLUE}Cleaning stub files for ${component}...${NC}"
    
    # Clean object files
    object_dir="${OBJ_DIR}/core/${component}"
    if [ -d "${object_dir}" ]; then
        rm -f "${object_dir}"/*.o
        echo -e "${GREEN}Cleaned object files in ${object_dir}${NC}"
    fi
    
    # Note: We don't clean include headers since they might be in use
}

# Parse command line arguments
COMPONENTS=()
CLEAN=false
VERBOSE=false
FORCE_REGENERATE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --all)
            COMPONENTS=("${ALL_COMPONENTS[@]}")
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --force)
            FORCE_REGENERATE=true
            shift
            ;;
        --help)
            print_usage
            exit 0
            ;;
        -*)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
        *)
            # Check if the component is valid
            valid=false
            for comp in "${ALL_COMPONENTS[@]}"; do
                if [ "$1" = "${comp}" ]; then
                    valid=true
                    break
                fi
            done
            
            if [ "${valid}" = "true" ]; then
                COMPONENTS+=("$1")
            else
                echo -e "${RED}Invalid component: $1${NC}"
                print_usage
                exit 1
            fi
            shift
            ;;
    esac
done

# If no components specified, use core components
if [ ${#COMPONENTS[@]} -eq 0 ]; then
    COMPONENTS=("polycall" "auth" "config")
fi

# Ensure needed directories exist
mkdir -p "${OBJ_DIR}/core"

# Process each component
for component in "${COMPONENTS[@]}"; do
    # Clean if requested
    if [ "${CLEAN}" = "true" ]; then
        clean_component_stubs "${component}"
    fi
    
    # Generate stubs
    generate_component_stubs "${component}"
done

echo -e "${GREEN}All stub files generated successfully.${NC}"