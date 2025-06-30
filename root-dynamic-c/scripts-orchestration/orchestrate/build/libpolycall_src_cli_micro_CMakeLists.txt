# CMakeLists.txt for LibPolyCall CLI micro commands
cmake_minimum_required(VERSION 3.13)

# Define command source files
file(GLOB micro_COMMAND_SOURCES
    "*.c"
)

# Create library target
add_library(polycall_cli_micro STATIC ${micro_COMMAND_SOURCES})

# Add include directories
target_include_directories(polycall_cli_micro PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(polycall_cli_micro PRIVATE
    polycall_core_micro
    polycall_cli_common
)

# Add to CLI library
target_link_libraries(polycall_cli PRIVATE
    polycall_cli_micro
)
