#!/bin/bash
# scripts/integrate-build-system.sh
# OBINexus Build System Integration Script
# Integrates enhanced CMake modules with existing Makefile system

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMAKE_DIR="${PROJECT_ROOT}/cmake"
SCRIPTS_DIR="${PROJECT_ROOT}/scripts"
BUILD_DIR="${PROJECT_ROOT}/build"

echo -e "${BLUE}=== OBINexus Build System Integration ===${NC}"

# Step 1: Verify prerequisites
check_prerequisites() {
    echo -e "${YELLOW}Checking prerequisites...${NC}"
    
    local missing=0
    
    # Check for required tools
    for tool in cmake make gcc g++ doxygen; do
        if ! command -v $tool &> /dev/null; then
            echo -e "${RED}✗ $tool not found${NC}"
            ((missing++))
        else
            echo -e "${GREEN}✓ $tool found${NC}"
        fi
    done
    
    if [ $missing -gt 0 ]; then
        echo -e "${RED}Please install missing tools before continuing${NC}"
        exit 1
    fi
}

# Step 2: Create CMake module structure
setup_cmake_modules() {
    echo -e "${YELLOW}Setting up CMake modules...${NC}"
    
    mkdir -p "${CMAKE_DIR}/modules"
    
    # Create module configuration files
    cat > "${CMAKE_DIR}/modules/CoreModules.cmake" << 'EOF'
# Core modules configuration
set(POLYCALL_CORE_INFRASTRUCTURE
    base common polycall
    config parser schema factory
)

set(POLYCALL_CORE_COMMUNICATION
    protocol network auth
)

set(POLYCALL_CORE_SERVICES
    accessibility repl ffi bridges hotwire
)

# Module source mapping
foreach(module ${POLYCALL_CORE_INFRASTRUCTURE} ${POLYCALL_CORE_COMMUNICATION} ${POLYCALL_CORE_SERVICES})
    set(POLYCALL_MODULE_${module}_DIR ${POLYCALL_SOURCE_DIR}/core/${module})
    set(POLYCALL_MODULE_${module}_TYPE "core")
endforeach()
EOF

    cat > "${CMAKE_DIR}/modules/CLIModules.cmake" << 'EOF'
# CLI modules configuration
set(POLYCALL_CLI_MODULES
    cli
)

# CLI source mapping
set(POLYCALL_MODULE_cli_DIR ${POLYCALL_SOURCE_DIR}/cli)
set(POLYCALL_MODULE_cli_TYPE "cli")
EOF

    cat > "${CMAKE_DIR}/modules/CommandModules.cmake" << 'EOF'
# Command modules configuration
set(POLYCALL_COMMAND_MODULES
    micro telemetry guid edge
    crypto topo doctor ignore
)

# Command source mapping
foreach(module ${POLYCALL_COMMAND_MODULES})
    set(POLYCALL_MODULE_${module}_DIR ${POLYCALL_SOURCE_DIR}/command/${module})
    set(POLYCALL_MODULE_${module}_TYPE "command")
endforeach()
EOF
    
    echo -e "${GREEN}✓ CMake modules created${NC}"
}

# Step 3: Create Makefile-CMake bridge
create_makefile_bridge() {
    echo -e "${YELLOW}Creating Makefile-CMake bridge...${NC}"
    
    # Update Makefile.build to integrate with CMake
    cat > "${PROJECT_ROOT}/Makefile.cmake" << 'EOF'
# OBINexus PolyCall CMake Integration
# Bridge between Makefile and CMake build systems

# Inherit from root
VERSION ?= 0.7.0
BUILD_MODE ?= release
ROOT_DIR ?= $(shell pwd)
BUILD_DIR ?= $(ROOT_DIR)/build

# CMake configuration
CMAKE_BUILD_DIR := $(BUILD_DIR)/cmake
CMAKE_FLAGS := \
    -DCMAKE_BUILD_TYPE=$(BUILD_MODE) \
    -DPOLYCALL_SECURITY_LEVEL=$(SECURITY_LEVEL) \
    -DPOLYCALL_BUILD_DOCS=$(BUILD_DOCS) \
    -DPOLYCALL_BUILD_TESTS=$(BUILD_TESTS) \
    -DPOLYCALL_ENABLE_NLINK=$(ENABLE_NLINK)

.PHONY: cmake-configure cmake-build cmake-test cmake-docs cmake-clean

cmake-configure:
	@echo "[CMAKE] Configuring build..."
	@mkdir -p $(CMAKE_BUILD_DIR)
	@cd $(CMAKE_BUILD_DIR) && cmake ../.. $(CMAKE_FLAGS)

cmake-build: cmake-configure
	@echo "[CMAKE] Building project..."
	@cd $(CMAKE_BUILD_DIR) && cmake --build . --parallel $(shell nproc)

cmake-test: cmake-build
	@echo "[CMAKE] Running tests..."
	@cd $(CMAKE_BUILD_DIR) && ctest --output-on-failure

cmake-docs: cmake-configure
	@echo "[CMAKE] Generating documentation..."
	@cd $(CMAKE_BUILD_DIR) && cmake --build . --target polycall_docs
	@cd $(CMAKE_BUILD_DIR) && cmake --build . --target organize_docs

cmake-install: cmake-build
	@echo "[CMAKE] Installing..."
	@cd $(CMAKE_BUILD_DIR) && cmake --install .

cmake-clean:
	@echo "[CMAKE] Cleaning..."
	@rm -rf $(CMAKE_BUILD_DIR)

# Integration targets for main Makefile
.PHONY: build-cmake test-cmake docs-cmake

build-cmake: cmake-build
test-cmake: cmake-test
docs-cmake: cmake-docs
EOF
    
    echo -e "${GREEN}✓ Makefile-CMake bridge created${NC}"
}

# Step 4: Create documentation structure
setup_documentation() {
    echo -e "${YELLOW}Setting up documentation structure...${NC}"
    
    # Run documentation refactoring
    if [ -f "${SCRIPTS_DIR}/refactor-docs.sh" ]; then
        bash "${SCRIPTS_DIR}/refactor-docs.sh"
    else
        # Create the refactor script if it doesn't exist
        cat > "${SCRIPTS_DIR}/refactor-docs.sh" << 'EOF'
#!/bin/bash
# Documentation refactoring script

set -e

echo "Creating documentation structure..."

# Create directories
mkdir -p docs/{architecture,api,guides,assets,specifications,internal}
mkdir -p docs/architecture/{core,cli,integration}
mkdir -p docs/api/{core,cli,commands}
mkdir -p docs/guides/{getting-started,development,deployment,troubleshooting}
mkdir -p docs/assets/images/{architecture,diagrams,screenshots,core,cli,commands}
mkdir -p docs/internal/{compliance,policies,milestones}

echo "Documentation structure created!"
EOF
        chmod +x "${SCRIPTS_DIR}/refactor-docs.sh"
        bash "${SCRIPTS_DIR}/refactor-docs.sh"
    fi
    
    echo -e "${GREEN}✓ Documentation structure setup${NC}"
}

# Step 5: Generate build configuration
generate_build_config() {
    echo -e "${YELLOW}Generating build configuration...${NC}"
    
    # Create polycall_config.h.in template
    cat > "${CMAKE_DIR}/polycall_config.h.in" << 'EOF'
#ifndef POLYCALL_CONFIG_H
#define POLYCALL_CONFIG_H

/* Version information */
#define POLYCALL_VERSION_MAJOR @POLYCALL_VERSION_MAJOR@
#define POLYCALL_VERSION_MINOR @POLYCALL_VERSION_MINOR@
#define POLYCALL_VERSION_PATCH @POLYCALL_VERSION_PATCH@
#define POLYCALL_VERSION "@POLYCALL_VERSION@"

/* Build configuration */
#cmakedefine POLYCALL_BUILD_SHARED
#cmakedefine POLYCALL_BUILD_STATIC
#cmakedefine POLYCALL_ENABLE_NLINK

/* Module configuration */
#cmakedefine POLYCALL_MODULE_EDGE
#cmakedefine POLYCALL_MODULE_MICRO
#cmakedefine POLYCALL_MODULE_TELEMETRY
#cmakedefine POLYCALL_MODULE_CRYPTO

/* Security configuration */
#define POLYCALL_SECURITY_LEVEL "@POLYCALL_SECURITY_LEVEL@"

#endif /* POLYCALL_CONFIG_H */
EOF
    
    echo -e "${GREEN}✓ Build configuration template created${NC}"
}

# Step 6: Update root Makefile
update_root_makefile() {
    echo -e "${YELLOW}Updating root Makefile...${NC}"
    
    # Add CMake integration to root Makefile
    if ! grep -q "Makefile.cmake" "${PROJECT_ROOT}/Makefile"; then
        cat >> "${PROJECT_ROOT}/Makefile" << 'EOF'

# ==============================================================================
# CMAKE INTEGRATION
# ==============================================================================
-include Makefile.cmake

.PHONY: cmake cmake-all

cmake: cmake-configure

cmake-all: cmake-build cmake-test cmake-docs

# Enhanced build targets
build-enhanced: build cmake-build

test-enhanced: test cmake-test

docs-enhanced: docs cmake-docs
EOF
    fi
    
    echo -e "${GREEN}✓ Root Makefile updated${NC}"
}

# Step 7: Create test build script
create_test_script() {
    echo -e "${YELLOW}Creating test build script...${NC}"
    
    cat > "${SCRIPTS_DIR}/test-integrated-build.sh" << 'EOF'
#!/bin/bash
# Test integrated build system

set -e

echo "=== Testing Integrated Build System ==="

# Test Make build
echo "Testing Makefile build..."
make clean
make build BUILD_MODE=debug

# Test CMake build  
echo "Testing CMake build..."
make cmake-clean
make cmake-build BUILD_MODE=debug

# Test documentation
echo "Testing documentation generation..."
make docs-enhanced

# Test combined targets
echo "Testing enhanced targets..."
make test-enhanced

echo "=== All tests passed! ==="
EOF
    
    chmod +x "${SCRIPTS_DIR}/test-integrated-build.sh"
    
    echo -e "${GREEN}✓ Test script created${NC}"
}

# Step 8: Final verification
verify_integration() {
    echo -e "${YELLOW}Verifying integration...${NC}"
    
    # Check for required files
    local files=(
        "${CMAKE_DIR}/PolycallConfig.cmake"
        "${CMAKE_DIR}/PolycallDocs.cmake"
        "${CMAKE_DIR}/PolycallModules.cmake"
        "${CMAKE_DIR}/OrganizeAssets.cmake"
        "${PROJECT_ROOT}/Makefile.cmake"
    )
    
    local missing=0
    for file in "${files[@]}"; do
        if [ -f "$file" ]; then
            echo -e "${GREEN}✓ $(basename "$file") present${NC}"
        else
            echo -e "${RED}✗ $(basename "$file") missing${NC}"
            ((missing++))
        fi
    done
    
    if [ $missing -eq 0 ]; then
        echo -e "${GREEN}✓ Integration verified successfully${NC}"
    else
        echo -e "${RED}✗ Integration incomplete${NC}"
        return 1
    fi
}

# Main execution
main() {
    check_prerequisites
    setup_cmake_modules
    create_makefile_bridge
    setup_documentation
    generate_build_config
    update_root_makefile
    create_test_script
    verify_integration
    
    echo -e "${GREEN}"
    echo "======================================"
    echo "Build System Integration Complete!"
    echo "======================================"
    echo -e "${NC}"
    echo "Next steps:"
    echo "1. Test the integrated build: ${SCRIPTS_DIR}/test-integrated-build.sh"
    echo "2. Build with CMake: make cmake-build"
    echo "3. Generate docs: make docs-enhanced"
    echo "4. Run full QA: make qa BUILD_MODE=debug"
}

# Run main function
main "$@"
