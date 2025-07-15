#!/bin/bash
# LibPolyCall FFI System Setup
# This script configures the FFI (Foreign Function Interface) components for LibPolyCall
# Supports modular language bridge integration for multiple programming languages

set -e  # Exit immediately if a command exits with non-zero status

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Print section header
section() {
    echo -e "\n${BLUE}==== $1 ====${NC}"
}

# Print success message
success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Print warning message
warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Print error message
error() {
    echo -e "${RED}✗ $1${NC}"
}

# Print info message
info() {
    echo -e "${CYAN}ℹ $1${NC}"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Display banner
show_banner() {
    echo -e "${CYAN}"
    echo "  ██▓     ██▓ ▄▄▄▄    ██▓███   ▒█████   ██▓     ▓██   ██▓ ▄████▄   ▄▄▄       ██▓     ██▓    "
    echo " ▓██▒    ▓██▒▓█████▄ ▓██░  ██▒▒██▒  ██▒▓██▒      ▒██  ██▒▒██▀ ▀█  ▒████▄    ▓██▒    ▓██▒    "
    echo " ▒██░    ▒██▒▒██▒ ▄██▓██░ ██▓▒▒██░  ██▒▒██░       ▒██ ██░▒▓█    ▄ ▒██  ▀█▄  ▒██░    ▒██░    "
    echo " ▒██░    ░██░▒██░█▀  ▒██▄█▓▒ ▒▒██   ██░▒██░       ░ ▐██▓░▒▓▓▄ ▄██▒░██▄▄▄▄██ ▒██░    ▒██░    "
    echo " ░██████▒░██░░▓█  ▀█▓▒██▒ ░  ░░ ████▓▒░░██████▒   ░ ██▒▓░▒ ▓███▀ ░ ▓█   ▓██▒░██████▒░██████▒"
    echo " ░ ▒░▓  ░░▓  ░▒▓███▀▒▒▓▒░ ░  ░░ ▒░▒░▒░ ░ ▒░▓  ░    ██▒▒▒ ░ ░▒ ▒  ░ ▒▒   ▓▒█░░ ▒░▓  ░░ ▒░▓  ░"
    echo " ░ ░ ▒  ░ ▒ ░▒░▒   ░ ░▒ ░       ░ ▒ ▒░ ░ ░ ▒  ░  ▓██ ░▒░   ░  ▒     ▒   ▒▒ ░░ ░ ▒  ░░ ░ ▒  ░"
    echo "   ░ ░    ▒ ░ ░    ░ ░░       ░ ░ ░ ▒    ░ ░     ▒ ▒ ░░  ░          ░   ▒     ░ ░     ░ ░   "
    echo "     ░  ░ ░   ░                  ░ ░      ░  ░   ░ ░     ░ ░            ░  ░    ░  ░    ░  ░"
    echo "                 ░                               ░ ░     ░                                    "
    echo -e "${NC}"
    echo -e "${BLUE}LibPolyCall: A Polymorphic Function Call Library${NC}"
    echo -e "${MAGENTA}FFI System Setup v1.0.0${NC}"
    echo -e "${MAGENTA}Copyright © 2025 OBINexus Computing${NC}"
    echo ""
}

# Detect operating system
detect_os() {
    section "Detecting Operating System"
    
    if [ "$(uname)" == "Darwin" ]; then
        OS="macos"
        info "macOS detected"
    elif [ "$(uname)" == "Linux" ]; then
        OS="linux"
        if [ -f /etc/debian_version ]; then
            DISTRO="debian"
            info "Debian-based Linux detected (Debian/Ubuntu/Mint)"
        elif [ -f /etc/redhat-release ]; then
            DISTRO="redhat"
            info "RedHat-based Linux detected (RHEL/CentOS/Fedora)"
        elif [ -f /etc/arch-release ]; then
            DISTRO="arch"
            info "Arch Linux detected"
        else
            DISTRO="unknown"
            warning "Unknown Linux distribution. You might need to install dependencies manually."
        fi
    elif [[ "$(uname -s)" == MINGW* || "$(uname -s)" == MSYS* ]]; then
        OS="windows"
        info "Windows detected (MinGW/MSYS2)"
    else
        OS="unknown"
        error "Unsupported operating system. This script is designed for Linux, macOS, and Windows (with MinGW/MSYS2)."
        exit 1
    fi
}

# Setup FFI core components
setup_ffi_core() {
    section "Setting Up FFI Core Components"
    
    info "Configuring FFI core adapter system..."
    
    # Ensure directories exist
    mkdir -p src/core/ffi
    mkdir -p include/polycall/core/ffi
    
    # Check if FFI core adapter is already implemented
    if [ -f "src/core/ffi/ffi_core.c" ] && [ -f "include/polycall/core/ffi/ffi_core.h" ]; then
        info "FFI core adapter already implemented"
    else
        warning "FFI core adapter implementation not found"
        info "Setting up basic FFI core implementation..."
        
        # Create directory structure if needed
        mkdir -p "src/core/ffi"
        mkdir -p "include/polycall/core/ffi"
        
        # Copy template implementations if available
        if [ -f "templates/ffi/ffi_core.c.template" ]; then
            cp "templates/ffi/ffi_core.c.template" "src/core/ffi/ffi_core.c"
            success "Created src/core/ffi/ffi_core.c from template"
        fi
        
        if [ -f "templates/ffi/ffi_core.h.template" ]; then
            cp "templates/ffi/ffi_core.h.template" "include/polycall/core/ffi/ffi_core.h"
            success "Created include/polycall/core/ffi/ffi_core.h from template"
        fi
    fi
    
    # Link FFI core with hierarchical error handling system
    info "Linking FFI core with hierarchical error handling system..."
    
    # Update CMakeLists.txt to include FFI components
    if [ -f "src/core/ffi/CMakeLists.txt" ]; then
        info "Ensuring CMakeLists.txt includes hierarchical error handling integration..."
        
        # Check if hierarchical error handling is already included
        if ! grep -q "polycall_hierarchical_error" "src/core/ffi/CMakeLists.txt"; then
            # Add hierarchical error handling integration
            echo "" >> "src/core/ffi/CMakeLists.txt"
            echo "# Link with hierarchical error handling" >> "src/core/ffi/CMakeLists.txt"
            echo "target_link_libraries(polycall_ffi" >> "src/core/ffi/CMakeLists.txt"
            echo "  PRIVATE" >> "src/core/ffi/CMakeLists.txt"
            echo "    polycall_core" >> "src/core/ffi/CMakeLists.txt"
            echo ")" >> "src/core/ffi/CMakeLists.txt"
            success "Added hierarchical error handling to CMakeLists.txt"
        else
            info "Hierarchical error handling already integrated"
        fi
    else
        warning "CMakeLists.txt not found in src/core/ffi directory"
        info "Creating new CMakeLists.txt with hierarchical error handling integration..."
        
        # Create a new CMakeLists.txt
        cat > "src/core/ffi/CMakeLists.txt" << EOF
# CMakeLists.txt for the FFI component
cmake_minimum_required(VERSION 3.10)

# Source files for the FFI library
set(POLYCALL_FFI_SRCS
    ffi_core.c
    ffi_adapter.c
    memory_bridge.c
    type_system.c
    security.c
    performance.c
)

# Build the polycall FFI library
add_library(polycall_ffi \${POLYCALL_FFI_SRCS})

# Include directories
target_include_directories(polycall_ffi
    PUBLIC
        \${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

# Link with hierarchical error handling
target_link_libraries(polycall_ffi
    PRIVATE
        polycall_core
)

# Install the library
install(TARGETS polycall_ffi
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(FILES
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/ffi_core.h
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/ffi_adapter.h
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/memory_bridge.h
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/type_system.h
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/security.h
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/performance.h
    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/language_bridge.h
    DESTINATION include/polycall/core/ffi
)
EOF
        success "Created new CMakeLists.txt with hierarchical error handling integration"
    fi
    
    success "FFI core components setup complete"
}

# Setup language bridge for specific language
setup_language_bridge() {
    local language=$1
    local language_cap="$(tr '[:lower:]' '[:upper:]' <<< ${language:0:1})${language:1}"
    
    section "Setting Up $language_cap Bridge"
    
    # Check if language bridge is already implemented
    if [ -f "src/core/ffi/${language}_bridge.c" ] && [ -f "include/polycall/core/ffi/${language}_bridge.h" ]; then
        info "$language_cap bridge already implemented"
        return 0
    fi
    
    # Define language-specific dependencies and tests
    case $language in
        python)
            deps_function="install_python_deps"
            ;;
        js)
            deps_function="install_js_deps"
            ;;
        jvm)
            deps_function="install_jvm_deps"
            ;;
        c)
            deps_function="install_c_deps"
            ;;
        cobol)
            deps_function="install_cobol_deps"
            ;;
        *)
            warning "Unknown language: $language"
            deps_function=""
            ;;
    esac
    
    # Install language-specific dependencies if needed
    if [ -n "$deps_function" ]; then
        info "Installing $language_cap bridge dependencies..."
        $deps_function
    fi
    
    # Create bridge implementation files
    info "Creating $language_cap bridge implementation..."
    
    # Create header file
    cat > "include/polycall/core/ffi/${language}_bridge.h" << EOF
/**
 * @file ${language}_bridge.h
 * @brief ${language_cap} language bridge interface for LibPolyCall
 * @author LibPolyCall Development Team (OBINexusComputing)
 *
 * This file defines the bridge interface for interoperating with ${language_cap}
 * through the FFI system, enabling polymorphic function calls between languages.
 */

#ifndef POLYCALL_${language^^}_BRIDGE_H
#define POLYCALL_${language^^}_BRIDGE_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/ffi/language_bridge.h"
#include "polycall/core/ffi/ffi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the ${language_cap} bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @return Error code
 */
polycall_core_error_t polycall_${language}_bridge_init(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx
);

/**
 * @brief Clean up the ${language_cap} bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 */
void polycall_${language}_bridge_cleanup(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx
);

/**
 * @brief Get the ${language_cap} bridge interface
 *
 * @param bridge Pointer to store the bridge interface
 * @return Error code
 */
polycall_core_error_t polycall_${language}_get_bridge(
    language_bridge_t* bridge
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_${language^^}_BRIDGE_H */
EOF
    
    # Create implementation file
    cat > "src/core/ffi/${language}_bridge.c" << EOF
/**
 * @file ${language}_bridge.c
 * @brief ${language_cap} language bridge implementation for LibPolyCall
 * @author LibPolyCall Development Team (OBINexusComputing)
 *
 * This file implements the bridge interface for interoperating with ${language_cap}
 * through the FFI system, enabling polymorphic function calls between languages.
 */

#include "polycall/core/ffi/${language}_bridge.h"
#include "polycall/core/ffi/type_system.h"
#include "polycall/core/ffi/memory_bridge.h"
#include "polycall/core/polycall/polycall_hierarchical_error.h"

// Forward declarations of internal functions
static polycall_core_error_t ${language}_call_function(
    polycall_core_context_t* ctx,
    const char* function_name,
    ffi_value_t* args,
    size_t arg_count,
    ffi_value_t* result
);

static polycall_core_error_t ${language}_register_function(
    polycall_core_context_t* ctx,
    const char* function_name,
    void* function_ptr,
    ffi_signature_t* signature
);

static polycall_core_error_t ${language}_initialize(
    polycall_core_context_t* ctx
);

static void ${language}_cleanup(
    polycall_core_context_t* ctx
);

// Language bridge implementation
static language_bridge_t ${language}_bridge = {
    .call_function = ${language}_call_function,
    .register_function = ${language}_register_function,
    .initialize = ${language}_initialize,
    .cleanup = ${language}_cleanup,
    .name = "${language}"
};

// Initialize the bridge
polycall_core_error_t polycall_${language}_bridge_init(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx
) {
    if (!ctx || !ffi_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Register with FFI system
    return polycall_ffi_register_language(
        ctx,
        ffi_ctx,
        "${language}",
        &${language}_bridge
    );
}

// Clean up the bridge
void polycall_${language}_bridge_cleanup(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx
) {
    // No specific cleanup needed at this level
    // The FFI system handles language unregistration
}

// Get the bridge interface
polycall_core_error_t polycall_${language}_get_bridge(
    language_bridge_t* bridge
) {
    if (!bridge) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    *bridge = ${language}_bridge;
    return POLYCALL_CORE_SUCCESS;
}

// Internal implementation of bridge functions

static polycall_core_error_t ${language}_call_function(
    polycall_core_context_t* ctx,
    const char* function_name,
    ffi_value_t* args,
    size_t arg_count,
    ffi_value_t* result
) {
    // TODO: Implement ${language}-specific function call logic
    POLYCALL_HIERARCHICAL_ERROR_SET(
        ctx, NULL,
        "ffi.${language}", POLYCALL_ERROR_SOURCE_FFI,
        POLYCALL_CORE_ERROR_NOT_IMPLEMENTED, POLYCALL_ERROR_SEVERITY_ERROR,
        "${language_cap} bridge call_function not implemented"
    );
    return POLYCALL_CORE_ERROR_NOT_IMPLEMENTED;
}

static polycall_core_error_t ${language}_register_function(
    polycall_core_context_t* ctx,
    const char* function_name,
    void* function_ptr,
    ffi_signature_t* signature
) {
    // TODO: Implement ${language}-specific function registration logic
    POLYCALL_HIERARCHICAL_ERROR_SET(
        ctx, NULL,
        "ffi.${language}", POLYCALL_ERROR_SOURCE_FFI,
        POLYCALL_CORE_ERROR_NOT_IMPLEMENTED, POLYCALL_ERROR_SEVERITY_ERROR,
        "${language_cap} bridge register_function not implemented"
    );
    return POLYCALL_CORE_ERROR_NOT_IMPLEMENTED;
}

static polycall_core_error_t ${language}_initialize(
    polycall_core_context_t* ctx
) {
    // TODO: Implement ${language}-specific initialization logic
    return POLYCALL_CORE_SUCCESS;
}

static void ${language}_cleanup(
    polycall_core_context_t* ctx
) {
    // TODO: Implement ${language}-specific cleanup logic
}
EOF
    
    success "Created $language_cap bridge implementation files"
    
    # Create test file for the bridge
    info "Creating test file for $language_cap bridge..."
    mkdir -p "tests/unit/core/ffi"
    
    cat > "tests/unit/core/ffi/test_${language}_bridge.c" << EOF
/**
 * @file test_${language}_bridge.c
 * @brief Test suite for ${language_cap} bridge
 */

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/ffi/${language}_bridge.h"
#include "unit_test_framework.h"

// Test fixture
typedef struct {
    polycall_core_context_t* core_ctx;
    polycall_ffi_context_t* ffi_ctx;
} test_fixture_t;

// Setup function
static void* setup() {
    test_fixture_t* fixture = malloc(sizeof(test_fixture_t));
    assert(fixture != NULL);
    
    polycall_core_init(&fixture->core_ctx, NULL);
    
    polycall_ffi_config_t config = {0};
    config.function_capacity = 32;
    config.type_capacity = 32;
    config.memory_pool_size = 1024 * 1024;  // 1MB
    
    polycall_ffi_init(fixture->core_ctx, &fixture->ffi_ctx, &config);
    
    return fixture;
}

// Teardown function
static void teardown(void* fixture_ptr) {
    test_fixture_t* fixture = (test_fixture_t*)fixture_ptr;
    
    polycall_ffi_cleanup(fixture->core_ctx, fixture->ffi_ctx);
    polycall_core_cleanup(fixture->core_ctx);
    
    free(fixture);
}

// Test ${language_cap} bridge initialization
static void test_${language}_bridge_init(void* fixture_ptr) {
    test_fixture_t* fixture = (test_fixture_t*)fixture_ptr;
    
    polycall_core_error_t result = polycall_${language}_bridge_init(
        fixture->core_ctx,
        fixture->ffi_ctx
    );
    
    ASSERT_EQUAL(POLYCALL_CORE_SUCCESS, result, "Bridge initialization should succeed");
    
    // Verify that the language is registered
    size_t language_count = 0;
    result = polycall_ffi_get_info(
        fixture->core_ctx,
        fixture->ffi_ctx,
        &language_count,
        NULL,
        NULL
    );
    
    ASSERT_EQUAL(POLYCALL_CORE_SUCCESS, result, "Getting info should succeed");
    ASSERT_TRUE(language_count > 0, "At least one language should be registered");
}

// Test ${language_cap} bridge interface retrieval
static void test_${language}_get_bridge(void* fixture_ptr) {
    language_bridge_t bridge = {0};
    
    polycall_core_error_t result = polycall_${language}_get_bridge(&bridge);
    
    ASSERT_EQUAL(POLYCALL_CORE_SUCCESS, result, "Getting bridge interface should succeed");
    ASSERT_STR_EQUAL("${language}", bridge.name, "Bridge name should match");
    ASSERT_TRUE(bridge.call_function != NULL, "call_function should be implemented");
    ASSERT_TRUE(bridge.register_function != NULL, "register_function should be implemented");
    ASSERT_TRUE(bridge.initialize != NULL, "initialize should be implemented");
    ASSERT_TRUE(bridge.cleanup != NULL, "cleanup should be implemented");
}

// Register tests
int main() {
    TEST_SUITE("${language_cap} Bridge Tests");
    
    REGISTER_TEST_FIXTURE(test_${language}_bridge_init, setup, teardown);
    REGISTER_TEST_FIXTURE(test_${language}_get_bridge, setup, teardown);
    
    RUN_TESTS();
    
    return TEST_RESULT();
}
EOF
    
    success "Created test file for $language_cap bridge"
    
    # Add to CMakeLists.txt
    info "Updating CMakeLists.txt to include $language_cap bridge..."
    
    # Add to src/core/ffi/CMakeLists.txt
    if [ -f "src/core/ffi/CMakeLists.txt" ]; then
        if ! grep -q "${language}_bridge.c" "src/core/ffi/CMakeLists.txt"; then
            # Add to source files
            sed -i.bak "/set(POLYCALL_FFI_SRCS/a\\    ${language}_bridge.c" "src/core/ffi/CMakeLists.txt"
            
            # Add to install headers
            sed -i.bak "/install(FILES/a\\    \${CMAKE_CURRENT_SOURCE_DIR}/../../../include/polycall/core/ffi/${language}_bridge.h" "src/core/ffi/CMakeLists.txt"
            
            # Remove backup file
            rm -f "src/core/ffi/CMakeLists.txt.bak"
            
            success "Updated CMakeLists.txt to include $language_cap bridge"
        else
            info "$language_cap bridge already included in CMakeLists.txt"
        fi
    else
        warning "CMakeLists.txt not found in src/core/ffi directory"
    fi
    
    # Add to tests/unit/core/ffi/CMakeLists.txt
    if [ -f "tests/unit/core/ffi/CMakeLists.txt" ]; then
        if ! grep -q "test_${language}_bridge" "tests/unit/core/ffi/CMakeLists.txt"; then
            # Add test executable
            cat >> "tests/unit/core/ffi/CMakeLists.txt" << EOF

# ${language_cap} bridge test
add_executable(test_${language}_bridge test_${language}_bridge.c)
target_link_libraries(test_${language}_bridge
    PRIVATE
        polycall_ffi
        polycall_core
        unit_test_framework
)
add_test(NAME test_${language}_bridge COMMAND test_${language}_bridge)
EOF
            success "Updated tests CMakeLists.txt to include $language_cap bridge test"
        else
            info "$language_cap bridge test already included in tests CMakeLists.txt"
        fi
    else
        warning "CMakeLists.txt not found in tests/unit/core/ffi directory"
        info "Creating new CMakeLists.txt for FFI tests..."
        
        mkdir -p "tests/unit/core/ffi"
        cat > "tests/unit/core/ffi/CMakeLists.txt" << EOF
# CMakeLists.txt for FFI unit tests

# FFI core test
add_executable(test_ffi_core test_ffi_core.c)
target_link_libraries(test_ffi_core
    PRIVATE
        polycall_ffi
        polycall_core
        unit_test_framework
)
add_test(NAME test_ffi_core COMMAND test_ffi_core)

# ${language_cap} bridge test
add_executable(test_${language}_bridge test_${language}_bridge.c)
target_link_libraries(test_${language}_bridge
    PRIVATE
        polycall_ffi
        polycall_core
        unit_test_framework
)
add_test(NAME test_${language}_bridge COMMAND test_${language}_bridge)
EOF
        success "Created new CMakeLists.txt for FFI tests including $language_cap bridge test"
    fi
    
    success "$language_cap bridge setup complete"
}

# Python dependencies installation
install_python_deps() {
    info "Installing Python bridge dependencies..."
    
    case $OS in
        linux)
            case $DISTRO in
                debian)
                    sudo apt-get install -y python3-dev
                    ;;
                redhat)
                    if command_exists dnf; then
                        sudo dnf install -y python3-devel
                    else
                        sudo yum install -y python3-devel
                    fi
                    ;;
                arch)
                    sudo pacman -Sy --noconfirm python
                    ;;
                *)
                    warning "Unknown Linux distribution. Please install Python development packages manually."
                    ;;
            esac
            ;;
        macos)
            brew install python
            ;;
        windows)
            if command_exists pacman; then
                pacman -Sy --noconfirm mingw-w64-x86_64-python
            else
                warning "MSYS2 package manager not found. Please install Python development packages manually."
            fi
            ;;
    esac
    
    success "Python bridge dependencies installed"
}

# JavaScript dependencies installation
install_js_deps() {
    info "Installing JavaScript bridge dependencies..."
    
    case $OS in
        linux)
            case $DISTRO in
                debian)
                    sudo apt-get install -y libv8-dev
                    ;;
                redhat)
                    if command_exists dnf; then
                        sudo dnf install -y v8-devel
                    else
                        warning "V8 development packages might not be available in standard repositories."
                        warning "You may need to install them manually from SCLs or EPEL."
                    fi
                    ;;
                arch)
                    sudo pacman -Sy --noconfirm v8
                    ;;
                *)
                    warning "Unknown Linux distribution. Please install V8 development packages manually."
                    ;;
            esac
            ;;
        macos)
            brew install v8
            ;;
        windows)
            if command_exists pacman; then
                pacman -Sy --noconfirm mingw-w64-x86_64-v8
            else
                warning "MSYS2 package manager not found. Please install V8 development packages manually."
            fi
            ;;
    esac
    
    success "JavaScript bridge dependencies installed"
}

# JVM dependencies installation
install_jvm_deps() {
    info "Installing JVM bridge dependencies..."
    
    case $OS in
        linux)
            case $DISTRO in
                debian)
                    sudo apt-get install -y default-jdk
                    ;;
                redhat)
                    if command_exists dnf; then
                        sudo dnf install -y java-devel
                    else
                        sudo yum install -y java-devel
                    fi
                    ;;
                arch)
                    sudo pacman -Sy --noconfirm jdk-openjdk
                    ;;
                *)
                    warning "Unknown Linux distribution. Please install JDK with JNI headers manually."
                    ;;
            esac
            ;;
        macos)
            brew install --cask temurin
            ;;
        windows)
            warning "Please install Java JDK manually and ensure JAVA_HOME is set correctly"
            ;;
    esac
    
    success "JVM bridge dependencies installed"
}

# C bridge dependencies installation
install_c_deps() {
    info "C bridge dependencies are provided by the core system"
    success "No additional dependencies needed for C bridge"
}

# COBOL bridge dependencies installation
install_cobol_deps() {
    info "Installing COBOL bridge dependencies..."
    
    case $OS in
        linux)
            case $DISTRO in
                debian)
                    sudo apt-get install -y open-cobol
                    ;;
                redhat)
                    if command_exists dnf; then
                        sudo dnf install -y gnu-cobol
                    else
                        sudo yum install -y gnu-cobol
                    fi
                    ;;
                arch)
                    warning "COBOL packages may not be available in standard repositories."
                    warning "Please install GNU COBOL (GnuCOBOL) manually through AUR."
                    ;;
                *)
                    warning "Unknown Linux distribution. Please install GNU COBOL (GnuCOBOL) manually."
                    ;;
            esac
            ;;
        macos)
            if brew tap | grep -q "homebrew/core"; then
                brew install gnu-cobol
            else
                warning "GNU COBOL may not be available in standard repositories."
                warning "Please install GNU COBOL (GnuCOBOL) manually."
            fi
            ;;
        windows)
            warning "Please install GNU COBOL (GnuCOBOL) manually for Windows."
            ;;
    esac
    
    success "COBOL bridge dependencies installed"
}

# Setup FFI test framework
setup_ffi_tests() {
    section "Setting Up FFI Test Framework"
    
    mkdir -p "tests/unit/core/ffi"
    
    # Create/update CMakeLists.txt for tests
    if [ ! -f "tests/unit/core/ffi/CMakeLists.txt" ]; then
        info "Creating CMakeLists.txt for FFI tests..."
        
        cat > "tests/unit/core/ffi/CMakeLists.txt" << EOF
# CMakeLists.txt for FFI unit tests

# FFI core test
add_executable(test_ffi_core test_ffi_core.c)
target_link_libraries(test_ffi_core
    PRIVATE
        polycall_ffi
        polycall_core
        unit_test_framework
)
add_test(NAME test_ffi_core COMMAND test_ffi_core)

# Memory bridge test
add_executable(test_memory_bridge test_memory_bridge.c)
target_link_libraries(test_memory_bridge
    PRIVATE
        polycall_ffi
        polycall_core
        unit_test_framework
)
add_test(NAME test_memory_bridge COMMAND test_memory_bridge)

# Type system test
add_executable(test_type_system test_type_system.c)
target_link_libraries(test_type_system
    PRIVATE
        polycall_ffi
        polycall_core
        unit_test_framework
)
add_test(NAME test_type_system COMMAND test_type_system)
EOF
        success "Created CMakeLists.txt for FFI tests"
    else
        info "CMakeLists.txt for FFI tests already exists"
    fi
    
    success "FFI test framework setup complete"
}

# Update main CMakeLists.txt to include FFI
update_main_cmake() {
    section "Updating Main CMakeLists.txt"
    
    if [ -f "CMakeLists.txt" ]; then
        info "Checking if FFI module is included in main CMakeLists.txt..."
        
        if ! grep -q "POLYCALL_ENABLE_FFI" "CMakeLists.txt"; then
            info "Adding FFI module option to CMakeLists.txt..."
            
            # Add FFI option
            if grep -q "option(" "CMakeLists.txt"; then
                # Find the last option and add after it
                sed -i.bak '/option(/a option(POLYCALL_ENABLE_FFI "Enable FFI (Foreign Function Interface) module" ON)' "CMakeLists.txt"
            else
                # Add at the beginning after cmake_minimum_required
                sed -i.bak '/cmake_minimum_required/a option(POLYCALL_ENABLE_FFI "Enable FFI (Foreign Function Interface) module" ON)' "CMakeLists.txt"
            fi
            
            # Add conditional inclusion of FFI directory
            if grep -q "add_subdirectory" "CMakeLists.txt"; then
                # Find the last add_subdirectory and add after it
                sed -i.bak '/add_subdirectory/a if(POLYCALL_ENABLE_FFI)\n  add_subdirectory(src/core/ffi)\nendif()' "CMakeLists.txt"
            else
                # Add at the end
                echo "" >> "CMakeLists.txt"
                echo "# Include FFI module if enabled" >> "CMakeLists.txt"
                echo "if(POLYCALL_ENABLE_FFI)" >> "CMakeLists.txt"
                echo "  add_subdirectory(src/core/ffi)" >> "CMakeLists.txt"
                echo "endif()" >> "CMakeLists.txt"
            fi
            
            # Remove backup file
            rm -f "CMakeLists.txt.bak"
            
            success "Updated main CMakeLists.txt to include FFI module"
        else
            info "FFI module already included in main CMakeLists.txt"
        fi
    else
        warning "Main CMakeLists.txt not found"
    fi
    
    success "Main CMakeLists.txt update complete"
}

# Interactive setup wizard
run_wizard() {
    section "FFI Setup Wizard"
    
    echo "This wizard will guide you through setting up the LibPolyCall FFI system."
    echo "FFI (Foreign Function Interface) enables polymorphic function calls between different programming languages."
    echo ""
    
    # Select languages to enable
    echo "Select languages to enable:"
    
    read -p "Enable Python bridge? (y/n) " enable_python
    read -p "Enable JavaScript bridge? (y/n) " enable_js
    read -p "Enable JVM bridge? (y/n) " enable_jvm
    read -p "Enable C bridge? (y/n) " enable_c
    read -p "Enable COBOL bridge? (y/n) " enable_cobol
    
    # Setup core components
    setup_ffi_core
    
    # Setup selected language bridges
    if [[ "$enable_python" =~ ^[Yy]$ ]]; then
        setup_language_bridge "python"
    fi
    
    if [[ "$enable_js" =~ ^[Yy]$ ]]; then
        setup_language_bridge "js"
    fi
    
    if [[ "$enable_jvm" =~ ^[Yy]$ ]]; then
        setup_language_bridge "jvm"
    fi
    
    if [[ "$enable_c" =~ ^[Yy]$ ]]; then
        setup_language_bridge "c"
    fi
    
    if [[ "$enable_cobol" =~ ^[Yy]$ ]]; then
        setup_language_bridge "cobol"
    fi
    
    # Setup tests
    setup_ffi_tests
    
    # Update main CMakeLists.txt
    update_main_cmake
    
    success "FFI Setup Wizard complete!"
}

# Main function
main() {
    show_banner
    detect_os
    run_wizard
    
    section "Setup Complete"
    info "The LibPolyCall FFI system has been configured successfully!"
    info ""
    info "To build the FFI module:"
    info "  mkdir -p build && cd build"
    info "  cmake .. -DPOLYCALL_ENABLE_FFI=ON"
    info "  make"
    info ""
    info "To run FFI tests:"
    info "  cd build"
    info "  ctest -R 'test_.*_bridge|test_ffi_core'"
    info ""
    
    success "Setup completed successfully"
}

# Parse command line arguments
if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "LibPolyCall FFI Setup Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --help, -h     Show this help message"
    echo "  --python       Setup Python bridge only"
    echo "  --js           Setup JavaScript bridge only"
    echo "  --jvm          Setup JVM bridge only"
    echo "  --c            Setup C bridge only"
    echo "  --cobol        Setup COBOL bridge only"
    echo "  --all          Setup all language bridges"
    echo ""
    exit 0
fi

# Execute specific bridge setup based on arguments
if [ "$#" -gt 0 ]; then
    show_banner
    detect_os
    setup_ffi_core
    
    for arg in "$@"; do
        case $arg in
            --python)
                setup_language_bridge "python"
                ;;
            --js)
                setup_language_bridge "js"
                ;;
            --jvm)
                setup_language_bridge "jvm"
                ;;
            --c)
                setup_language_bridge "c"
                ;;
            --cobol)
                setup_language_bridge "cobol"
                ;;
            --all)
                setup_language_bridge "python"
                setup_language_bridge "js"
                setup_language_bridge "jvm"
                setup_language_bridge "c"
                setup_language_bridge "cobol"
                ;;
        esac
    done
    
    setup_ffi_tests
    update_main_cmake
    
    section "Setup Complete"
    info "The requested LibPolyCall FFI components have been configured successfully!"
    exit 0
fi

# Run main function if no arguments provided
main