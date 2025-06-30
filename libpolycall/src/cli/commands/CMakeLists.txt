# CMakeLists.txt for LibPolyCall CLI Commands
cmake_minimum_required(VERSION 3.13)

# Get all command source files
file(GLOB COMMAND_SOURCES
    "/*.c"
)

# Define commands library
add_library(polycall_cli_commands STATIC )

# Add include directories
target_include_directories(polycall_cli_commands PRIVATE
    /include
    /src
)

# Link with core libraries
target_link_libraries(polycall_cli_commands PRIVATE
    polycall_core_static
)

# Add to CLI library
target_link_libraries(polycall_cli PRIVATE
    polycall_cli_commands
)
