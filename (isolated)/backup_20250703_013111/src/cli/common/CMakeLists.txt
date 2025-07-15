# CMakeLists.txt for LibPolyCall CLI common functionality
cmake_minimum_required(VERSION 3.13)

# Define command source files
file(GLOB COMMON_SOURCES
    "*.c"
)

# Create library target
add_library(polycall_cli_common STATIC ${COMMON_SOURCES})

# Add include directories
target_include_directories(polycall_cli_common PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(polycall_cli_common PRIVATE
    polycall_core_static
)

# Add to CLI library
target_link_libraries(polycall_cli PRIVATE
    polycall_cli_common
)
