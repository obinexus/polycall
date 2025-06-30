cmake_minimum_required(VERSION 3.16)
project(minimal_polycall_emergency)

# EMERGENCY BUILD - ONLY SINPHASÉ COMPLIANT COMPONENTS
# All components in this build have C ≤ 0.5 (Autonomous Zone)

add_subdirectory(src/core/components)     # C = 0.1 - SAFE
add_subdirectory(src/core/config-tools)  # C = 0.1 - SAFE

# Generate minimal shared library
add_library(polycall_minimal SHARED
    $<TARGET_OBJECTS:components>
    $<TARGET_OBJECTS:config-tools>
)

target_include_directories(polycall_minimal PUBLIC include)
