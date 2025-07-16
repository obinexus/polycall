#!/bin/bash
# OBINexus Component Build Configuration Generator
# Addresses missing CMakeLists.txt and Makefile integration issues

set -euo pipefail

# Configuration
PROJECT_ROOT="$(pwd)"
SRC_DIR="${PROJECT_ROOT}/src"
TESTS_DIR="${PROJECT_ROOT}/tests"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== OBINexus Component Build Configuration Generator ===${NC}"

# Step 1: Create src/Makefile for coordinating component builds
create_src_makefile() {
    echo -e "${YELLOW}Creating src/Makefile for component coordination...${NC}"
    
    cat > "${SRC_DIR}/Makefile" << 'EOF'
# OBINexus Polycall Source Component Coordination Makefile
# Coordinates builds across core, cli, and ffi components

# Build configuration
BUILD_MODE ?= release
BUILD_DIR ?= ../build
PARALLEL_JOBS ?= $(shell nproc 2>/dev/null || echo 4)

# Component directories
CORE_COMPONENTS = $(wildcard core/*/.)
CLI_COMPONENTS = $(wildcard cli/*/.)
FFI_COMPONENTS = $(wildcard ffi/*/.)

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -std=c11 -I../include
LDFLAGS = -L$(BUILD_DIR)

ifeq ($(BUILD_MODE),debug)
    CFLAGS += -g -O0 -DDEBUG -DPOLYCALL_DEBUG=1
    BUILD_SUFFIX = _debug
else
    CFLAGS += -O3 -DNDEBUG
    BUILD_SUFFIX = 
endif

# Export build configuration to child makes
export CC CFLAGS LDFLAGS BUILD_MODE BUILD_DIR

# Main targets
.PHONY: all core cli ffi clean help

all: core cli

# Build core components
core:
	@echo "[SRC] Building core components..."
	@if [ -d core ]; then \
		for comp in core/*; do \
			if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
				echo "[CORE] Building $$comp..."; \
				$(MAKE) -C "$$comp" BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE) || exit 1; \
			fi; \
		done; \
	else \
		echo "[WARN] No core directory found"; \
	fi

# Build CLI components  
cli: core
	@echo "[SRC] Building CLI components..."
	@if [ -d cli ]; then \
		for comp in cli/*; do \
			if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
				echo "[CLI] Building $$comp..."; \
				$(MAKE) -C "$$comp" BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE) || exit 1; \
			fi; \
		done; \
	else \
		echo "[WARN] No cli directory found"; \
	fi

# Build FFI components
ffi:
	@echo "[SRC] Building FFI components..."
	@if [ -d ffi ]; then \
		for comp in ffi/*; do \
			if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
				echo "[FFI] Building $$comp..."; \
				$(MAKE) -C "$$comp" BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE) || exit 1; \
			fi; \
		done; \
	else \
		echo "[WARN] No ffi directory found"; \
	fi

# Legacy component support
legacy-build:
	@echo "[SRC] Building legacy components with CMake fallback..."
	@mkdir -p $(BUILD_DIR)
	@find . -name "*.c" -exec basename {} .c \; | sort -u > $(BUILD_DIR)/source_inventory.txt
	@echo "[INFO] Found $(shell wc -l < $(BUILD_DIR)/source_inventory.txt) unique source files"

# Clean all components
clean:
	@echo "[SRC] Cleaning all components..."
	@for comp in core/* cli/* ffi/*; do \
		if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
			$(MAKE) -C "$$comp" clean 2>/dev/null || true; \
		fi; \
	done
	@find . -name "*.o" -delete 2>/dev/null || true
	@find . -name "*.so" -delete 2>/dev/null || true

help:
	@echo "OBINexus Source Component Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build core and CLI components (default)"
	@echo "  core       - Build only core components"
	@echo "  cli        - Build only CLI components"
	@echo "  ffi        - Build only FFI components"
	@echo "  clean      - Clean all build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build modes: debug, release (default: release)"
	@echo "Example: make core BUILD_MODE=debug"
EOF
    
    echo -e "${GREEN}✓ src/Makefile created${NC}"
}

# Step 2: Create src/core/CMakeLists.txt
create_core_cmake() {
    echo -e "${YELLOW}Creating src/core/CMakeLists.txt...${NC}"
    
    mkdir -p "${SRC_DIR}/core"
    cat > "${SRC_DIR}/core/CMakeLists.txt" << 'EOF'
# OBINexus Polycall Core Components CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Core component configuration
set(POLYCALL_CORE_VERSION "0.7.0")

# Find all core component subdirectories
file(GLOB CORE_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")

# Core component source discovery
set(CORE_SOURCES "")
set(CORE_HEADERS "")

# Component directories to process
set(CORE_COMPONENTS
    auth
    config
    network
    protocol
    polycall
    schema
    security
    telemetry
    accessibility
    edge
    ffi
    micro
    repl
    socket
)

# Collect sources from each component
foreach(COMPONENT ${CORE_COMPONENTS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT})
        message(STATUS "Processing core component: ${COMPONENT}")
        
        # Find source files
        file(GLOB_RECURSE COMPONENT_SOURCES 
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.cpp"
        )
        
        # Find header files
        file(GLOB_RECURSE COMPONENT_HEADERS
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.hpp"
        )
        
        list(APPEND CORE_SOURCES ${COMPONENT_SOURCES})
        list(APPEND CORE_HEADERS ${COMPONENT_HEADERS})
        
        # Add component subdirectory if it has CMakeLists.txt
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/CMakeLists.txt)
            add_subdirectory(${COMPONENT})
        endif()
    endif()
endforeach()

# Filter out sources that might cause conflicts
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*test.*")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.c.*")

# Create core library if we have sources
if(CORE_SOURCES)
    message(STATUS "Creating polycall_core library with ${list_length:CORE_SOURCES} sources")
    
    # Static library
    add_library(polycall_core_static STATIC ${CORE_SOURCES})
    target_include_directories(polycall_core_static
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(polycall_core_static
        PRIVATE
            Threads::Threads
            ${CMAKE_DL_LIBS}
    )
    
    set_target_properties(polycall_core_static PROPERTIES
        OUTPUT_NAME "polycall_core"
        VERSION ${POLYCALL_CORE_VERSION}
    )
    
    # Shared library
    add_library(polycall_core_shared SHARED ${CORE_SOURCES})
    target_include_directories(polycall_core_shared
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(polycall_core_shared
        PRIVATE
            Threads::Threads
            ${CMAKE_DL_LIBS}
    )
    
    set_target_properties(polycall_core_shared PROPERTIES
        OUTPUT_NAME "polycall_core"
        VERSION ${POLYCALL_CORE_VERSION}
        SOVERSION 0
    )
    
    # Create alias for unified targeting
    add_library(polycall_core ALIAS polycall_core_static)
    
else()
    message(WARNING "No core sources found, creating empty library")
    
    # Create minimal library to satisfy dependencies
    add_library(polycall_core INTERFACE)
    target_include_directories(polycall_core
        INTERFACE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Installation
if(TARGET polycall_core_static)
    install(TARGETS polycall_core_static polycall_core_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# Component summary
message(STATUS "Core component configuration complete:")
message(STATUS "  Components processed: ${CORE_COMPONENTS}")
message(STATUS "  Total sources: ${list_length:CORE_SOURCES}")
message(STATUS "  Total headers: ${list_length:CORE_HEADERS}")
EOF
    
    echo -e "${GREEN}✓ src/core/CMakeLists.txt created${NC}"
}

# Step 3: Create src/cli/CMakeLists.txt
create_cli_cmake() {
    echo -e "${YELLOW}Creating src/cli/CMakeLists.txt...${NC}"
    
    mkdir -p "${SRC_DIR}/cli"
    cat > "${SRC_DIR}/cli/CMakeLists.txt" << 'EOF'
# OBINexus Polycall CLI Components CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# CLI component configuration
set(POLYCALL_CLI_VERSION "0.7.0")

# Find CLI source files
file(GLOB_RECURSE CLI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Find CLI header files
file(GLOB_RECURSE CLI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Filter out test files
list(FILTER CLI_SOURCES EXCLUDE REGEX ".*test.*")

# Check for CLI subdirectories
file(GLOB CLI_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")
foreach(SUBDIR ${CLI_SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt)
            message(STATUS "Adding CLI subdirectory: ${SUBDIR}")
            add_subdirectory(${SUBDIR})
        endif()
    endif()
endforeach()

# Create CLI executable if we have sources
if(CLI_SOURCES)
    message(STATUS "Creating polycall CLI executable with ${list_length:CLI_SOURCES} sources")
    
    add_executable(polycall_cli ${CLI_SOURCES})
    
    target_include_directories(polycall_cli
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/src/core
    )
    
    # Link with core library
    target_link_libraries(polycall_cli
        PRIVATE
            polycall_core
            Threads::Threads
            ${CMAKE_DL_LIBS}
    )
    
    set_target_properties(polycall_cli PROPERTIES
        OUTPUT_NAME "polycall"
        VERSION ${POLYCALL_CLI_VERSION}
    )
    
    # Installation
    install(TARGETS polycall_cli
        RUNTIME DESTINATION bin
    )
    
else()
    message(WARNING "No CLI sources found")
    
    # Create a minimal CLI target that just prints version
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/polycall_cli_stub.c
        "#include <stdio.h>\n"
        "int main() {\n"
        "    printf(\"polycall v${POLYCALL_CLI_VERSION}\\n\");\n"
        "    return 0;\n"
        "}\n"
    )
    
    add_executable(polycall_cli ${CMAKE_CURRENT_BINARY_DIR}/polycall_cli_stub.c)
    
    target_link_libraries(polycall_cli
        PRIVATE
            polycall_core
    )
    
    set_target_properties(polycall_cli PROPERTIES
        OUTPUT_NAME "polycall"
    )
    
    install(TARGETS polycall_cli
        RUNTIME DESTINATION bin
    )
    
    message(STATUS "Created stub CLI executable")
endif()

# CLI component summary
message(STATUS "CLI component configuration complete:")
message(STATUS "  Sources found: ${list_length:CLI_SOURCES}")
message(STATUS "  Headers found: ${list_length:CLI_HEADERS}")
EOF
    
    echo -e "${GREEN}✓ src/cli/CMakeLists.txt created${NC}"
}

# Step 4: Create tests/CMakeLists.txt
create_tests_cmake() {
    echo -e "${YELLOW}Creating tests/CMakeLists.txt...${NC}"
    
    mkdir -p "${TESTS_DIR}"
    cat > "${TESTS_DIR}/CMakeLists.txt" << 'EOF'
# OBINexus Polycall Test Suite CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Test configuration
set(POLYCALL_TEST_VERSION "0.7.0")

# Find all test source files
file(GLOB_RECURSE TEST_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Test subdirectories to process
set(TEST_CATEGORIES
    unit
    integration
    performance
    functional
)

# Common test include directories
set(TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Common test libraries
set(TEST_LIBRARIES
    polycall_core
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Process test categories
foreach(CATEGORY ${TEST_CATEGORIES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY})
        message(STATUS "Processing test category: ${CATEGORY}")
        
        # Find test files in this category
        file(GLOB_RECURSE CATEGORY_TESTS
            "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/*test*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/*test*.cpp"
        )
        
        # Create test executable for each test file
        foreach(TEST_FILE ${CATEGORY_TESTS})
            get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
            set(TEST_TARGET "test_${CATEGORY}_${TEST_NAME}")
            
            add_executable(${TEST_TARGET} ${TEST_FILE})
            
            target_include_directories(${TEST_TARGET}
                PRIVATE ${TEST_INCLUDE_DIRS}
            )
            
            target_link_libraries(${TEST_TARGET}
                PRIVATE ${TEST_LIBRARIES}
            )
            
            # Add to CTest
            add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
            
            # Set test properties
            set_tests_properties(${TEST_TARGET} PROPERTIES
                TIMEOUT 60
                LABELS ${CATEGORY}
            )
            
            message(STATUS "  Added test: ${TEST_TARGET}")
        endforeach()
        
        # Add subdirectory if it has CMakeLists.txt
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/CMakeLists.txt)
            add_subdirectory(${CATEGORY})
        endif()
    endif()
endforeach()

# Create test runner if no specific tests found
if(NOT CATEGORY_TESTS)
    message(STATUS "No test files found, creating test runner stub")
    
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_runner_stub.c
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "\n"
        "int main() {\n"
        "    printf(\"OBINexus Polycall Test Suite v${POLYCALL_TEST_VERSION}\\n\");\n"
        "    printf(\"No tests implemented yet\\n\");\n"
        "    return 0;\n"
        "}\n"
    )
    
    add_executable(test_runner ${CMAKE_CURRENT_BINARY_DIR}/test_runner_stub.c)
    
    target_link_libraries(test_runner
        PRIVATE ${TEST_LIBRARIES}
    )
    
    add_test(NAME test_runner COMMAND test_runner)
endif()

# Test utilities
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_all_tests.sh
    "#!/bin/bash\n"
    "echo \"Running OBINexus Polycall Test Suite\"\n"
    "cd ${CMAKE_CURRENT_BINARY_DIR}\n"
    "ctest --output-on-failure --verbose\n"
)

# Test summary
message(STATUS "Test suite configuration complete:")
message(STATUS "  Test categories: ${TEST_CATEGORIES}")
message(STATUS "  Total test files: ${list_length:TEST_SOURCES}")
EOF
    
    echo -e "${GREEN}✓ tests/CMakeLists.txt created${NC}"
}

# Step 5: Create component template generator
create_component_templates() {
    echo -e "${YELLOW}Creating component template system...${NC}"
    
    mkdir -p "${PROJECT_ROOT}/scripts/templates"
    
    cat > "${PROJECT_ROOT}/scripts/templates/component_cmake.template" << 'EOF'
# Component CMakeLists.txt Template
# Usage: Replace @COMPONENT_NAME@ with actual component name

cmake_minimum_required(VERSION 3.16)

# Component configuration
set(@COMPONENT_NAME@_VERSION "0.7.0")

# Find component sources
file(GLOB @COMPONENT_NAME@_SOURCES "*.c" "*.cpp")
file(GLOB @COMPONENT_NAME@_HEADERS "*.h" "*.hpp")

# Create component library
if(@COMPONENT_NAME@_SOURCES)
    add_library(@COMPONENT_NAME@_static STATIC ${@COMPONENT_NAME@_SOURCES})
    
    target_include_directories(@COMPONENT_NAME@_static
        PUBLIC ${CMAKE_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(@COMPONENT_NAME@_static
        PRIVATE Threads::Threads
    )
    
    set_target_properties(@COMPONENT_NAME@_static PROPERTIES
        OUTPUT_NAME "@COMPONENT_NAME@"
    )
endif()
EOF
    
    cat > "${PROJECT_ROOT}/scripts/create_component.sh" << 'EOF'
#!/bin/bash
# Component creation script

if [ $# -ne 2 ]; then
    echo "Usage: $0 <component_type> <component_name>"
    echo "Types: core, cli, ffi"
    exit 1
fi

COMPONENT_TYPE=$1
COMPONENT_NAME=$2
COMPONENT_DIR="src/${COMPONENT_TYPE}/${COMPONENT_NAME}"

mkdir -p "${COMPONENT_DIR}"

# Create CMakeLists.txt from template
sed "s/@COMPONENT_NAME@/${COMPONENT_NAME}/g" \
    scripts/templates/component_cmake.template > "${COMPONENT_DIR}/CMakeLists.txt"

# Create basic source files
echo "#include \"${COMPONENT_NAME}.h\"" > "${COMPONENT_DIR}/${COMPONENT_NAME}.c"
echo "#ifndef ${COMPONENT_NAME^^}_H" > "${COMPONENT_DIR}/${COMPONENT_NAME}.h"
echo "#define ${COMPONENT_NAME^^}_H" >> "${COMPONENT_DIR}/${COMPONENT_NAME}.h"
echo "#endif" >> "${COMPONENT_DIR}/${COMPONENT_NAME}.h"

echo "Component created: ${COMPONENT_DIR}"
EOF
    
    chmod +x "${PROJECT_ROOT}/scripts/create_component.sh"
    
    echo -e "${GREEN}✓ Component template system created${NC}"
}

# Main execution function
main() {
    echo -e "${BLUE}Starting component build configuration generation...${NC}"
    
    # Ensure directories exist
    mkdir -p "${SRC_DIR}" "${TESTS_DIR}"
    
    # Create build configurations
    create_src_makefile
    create_core_cmake
    create_cli_cmake  
    create_tests_cmake
    create_component_templates
    
    echo -e "${GREEN}=== Component Build Configuration Complete ===${NC}"
    echo -e "${YELLOW}Validation Steps:${NC}"
    echo "1. Test Make build: make build"
    echo "2. Test CMake build: make cmake-build"
    echo "3. Test individual components: make core BUILD_MODE=debug"
    echo "4. Run tests: make test"
    echo "5. Create new components: scripts/create_component.sh core mycomponent"
}

# Execute main function
main "$@"
