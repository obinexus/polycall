# CMakeLists.txt for the core polycall component
cmake_minimum_required(VERSION 3.10)

project(LibPolyCall VERSION 0.1.0 LANGUAGES C)

# Source files for the core polycall library
set(POLYCALL_CORE_SRCS
    polycall/context.c
    polycall/error.c
    polycall/memory.c
    polycall/polycall.c
    polycall/hierarchical_state.c
)

# Define component paths
set(POLYCALL_COMPONENTS
    polycall
    auth
    config
    edge
    ffi
    micro
    network
    protocol
    telemetry
)

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Build the polycall core library
add_library(polycall_core ${POLYCALL_CORE_SRCS})

# Generate object files for each component module
foreach(COMPONENT ${POLYCALL_COMPONENTS})
    file(GLOB COMPONENT_SRCS "src/core/${COMPONENT}/*.c")
    if(COMPONENT_SRCS)
        set(COMPONENT_LIB "polycall_${COMPONENT}")
        add_library(${COMPONENT_LIB} OBJECT ${COMPONENT_SRCS})
        target_include_directories(${COMPONENT_LIB}
            PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
    endif()
endforeach()

# Include directories
target_include_directories(polycall_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add hierarchical error handling to the PolyCall core library
target_sources(polycall_core
    PRIVATE
        polycall/hierarchical_error.c
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/polycall/core/polycall/polycall_hierarchical_error.h
)

# Set up version information
set(POLYCALL_VERSION_MAJOR 0)
set(POLYCALL_VERSION_MINOR 1)
set(POLYCALL_VERSION_PATCH 0)
set(POLYCALL_VERSION "${POLYCALL_VERSION_MAJOR}.${POLYCALL_VERSION_MINOR}.${POLYCALL_VERSION_PATCH}")

# Configure version information
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/polycall/core/polycall/polycall_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/polycall_version.h
)
# CMakeLists.txt for the polycall core component
cmake_minimum_required(VERSION 3.13)

# Gather all source files in this directory
file(GLOB COMPONENT_SOURCES "*.c")
  "/polycall_container.c"
  "/polycall_registry.c"

# Define component static library
polycall_define_component_library(
  COMPONENT "polycall"
  STATIC
  SOURCES ${COMPONENT_SOURCES}
  DEPENDENCIES pthread
)

# Optional: Define component shared library
polycall_define_component_library(
  COMPONENT "polycall"
  SHARED
  OUTPUT_NAME "polycall_core"
  SOURCES ${COMPONENT_SOURCES}
  DEPENDENCIES pthread
)

# Validate include paths
polycall_validate_target_includes(
  TARGET polycall_polycall_static
)

# Print component details
message(STATUS "Configured polycall core component")
message(STATUS "  Sources: ${COMPONENT_SOURCES}")
# Include the generated version header
target_include_directories(polycall_core
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Link with required libraries
target_link_libraries(polycall_core
    PRIVATE
        pthread
)

# Setup testing infrastructure
enable_testing()

# Generate test executables for unit tests
file(GLOB_RECURSE UNIT_TEST_SRCS "tests/unit/*/*.c")
foreach(TEST_SRC ${UNIT_TEST_SRCS})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} polycall_core)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Generate test executables for integration tests
file(GLOB_RECURSE INTEGRATION_TEST_SRCS "tests/integration/*/*.c")
foreach(TEST_SRC ${INTEGRATION_TEST_SRCS})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    string(PREPEND TEST_NAME "integration_")
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} polycall_core)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Install the library
install(TARGETS polycall_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/polycall_version.h
    DESTINATION include/polycall/core/polycall
)

# Export package information
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/polycall-config-version.cmake"
    VERSION ${POLYCALL_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Create package configuration
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/polycall-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/polycall-config.cmake
    INSTALL_DESTINATION lib/cmake/polycall
)

# Install package configuration files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/polycall-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/polycall-config-version.cmake
    DESTINATION lib/cmake/polycall
)

# Install targets file
install(EXPORT polycall-targets
    FILE polycall-targets.cmake
    DESTINATION lib/cmake/polycall
)