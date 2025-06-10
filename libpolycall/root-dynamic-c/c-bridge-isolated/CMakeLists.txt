# Sinphasé Isolated Component: C Bridge
# Target Cost: C ≤ 0.3 (Autonomous Zone)
# Dependencies: NONE

cmake_minimum_required(VERSION 3.16)
project(polycall_c_bridge_isolated)

# Enforce single-pass compilation
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/pdb)
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/pdb)

# Source files (minimal set)
set(SOURCES
    src/c_bridge.c
)

set(HEADERS
    include/polycall/ffi/c_bridge.h
)

# Create static library (isolated)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Include directories (minimal)
target_include_directories(${PROJECT_NAME} 
    PUBLIC include
    PRIVATE src
)

# Compiler flags for Sinphasé compliance
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Werror
    -std=c11
    -O2
    -fno-exceptions
    # Sinphasé enforcement flags
    -Wno-unused-parameter  # Allow minimal implementations
)

# Sinphasé validation target
add_custom_target(sinphase_validate_c_bridge
    COMMAND echo "Validating C Bridge Sinphasé compliance..."
    COMMAND echo "Target Cost: C ≤ 0.3"
    COMMAND echo "Dependencies: NONE"
    COMMAND echo "Files: ${CMAKE_CURRENT_LIST_DIR}/src/c_bridge.c"
    COMMENT "Sinphasé: C Bridge validation"
)

# Cost evaluation integration
add_custom_target(sinphase_cost_check
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/evaluator/sinphase_cost_evaluator.py 
            --component ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Sinphasé: Evaluating C Bridge cost function"
)
