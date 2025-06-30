# CMakeLists.txt for CLI
cmake_minimum_required(VERSION 3.10)

# CLI library target
add_library(polycall_cli STATIC
    common/command_registry.c
    providers/cli_container.c
    repl.c
    # Add other common files
)

# Add include directories
target_include_directories(polycall_cli
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Create command module libraries
# Core commands
add_library(polycall_cli_core STATIC
    common/polycall_commands.c
)

# Protocol commands
add_library(polycall_cli_protocol STATIC
    protocol/protocol_commands.c
)

# Add other module libraries
# ...

# Link common dependencies
target_link_libraries(polycall_cli
    polycall_core
    readline  # For REPL
)

# Link module-specific dependencies
target_link_libraries(polycall_cli_core
    polycall_cli
    polycall_core
)

target_link_libraries(polycall_cli_protocol
    polycall_cli
    polycall_core
    polycall_protocol
)

# Main CLI executable
add_executable(polycall_cli_app
    main.c
)

# Link all modules
target_link_libraries(polycall_cli_app
    polycall_cli
    polycall_cli_core
    polycall_cli_protocol
    # Add other modules
)

# Unit tests
add_executable(test_command_registry
    tests/test_command_registry.c
)

target_link_libraries(test_command_registry
    polycall_cli
)

add_executable(test_commands
    tests/test_commands.c
)

target_link_libraries(test_commands
    polycall_cli
    polycall_cli_core
    polycall_cli_protocol
    # Add other modules for testing
)

# CLI components

# Collect all CLI component sources
file(GLOB_RECURSE POLYCALL_CLI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Exclude main.c as it's handled separately for the executable
list(FILTER POLYCALL_CLI_SOURCES EXCLUDE REGEX "main\\.c$")
list(FILTER POLYCALL_CLI_SOURCES EXCLUDE REGEX ".*test\\.c$")

# Create the CLI library
polycall_add_component(
    NAME polycall_cli
    TYPE STATIC
    SOURCES ${POLYCALL_CLI_SOURCES}
    DEPS polycall_core
    LIBS Threads::Threads
)

# Handle subdirectories with specific components
add_subdirectory(commands)
add_subdirectory(common)
add_subdirectory(accessibility)
add_subdirectory(auth)
add_subdirectory(config)
add_subdirectory(edge)
add_subdirectory(ffi)
add_subdirectory(micro)
add_subdirectory(network)
add_subdirectory(protocol)
add_subdirectory(repl)
add_subdirectory(socket)
add_subdirectory(telemetry)