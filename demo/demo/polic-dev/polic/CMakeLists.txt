cmake_minimum_required(VERSION 3.10)
project(polic VERSION 0.1.0
        DESCRIPTION "PoliC Security Framework"
        LANGUAGES C)

# Options
option(POLIC_BUILD_TESTS "Build tests" ON)
option(POLIC_BUILD_EXAMPLES "Build examples" ON)
option(POLIC_ENABLE_VM_HOOKS "Enable VM hooks" ON)
option(POLIC_ENABLE_STACK_PROTECTION "Enable stack protection" ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    
    # Security flags
    add_compile_options(-fstack-protector-strong)
    add_compile_options(-D_FORTIFY_SOURCE=2)
endif()

# Main library
add_library(polic
    src/core/polic.c
    # Add more source files here
)

target_include_directories(polic
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Define compile definitions based on options
if(POLIC_ENABLE_VM_HOOKS)
    target_compile_definitions(polic PRIVATE POLIC_ENABLE_VM_HOOKS)
endif()

if(POLIC_ENABLE_STACK_PROTECTION)
    target_compile_definitions(polic PRIVATE POLIC_ENABLE_STACK_PROTECTION)
endif()

# Examples
if(POLIC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(POLIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS polic
    EXPORT polic-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT polic-targets
    FILE polic-targets.cmake
    NAMESPACE polic::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/polic
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/polic-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/polic-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/polic
)

# Create version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/polic-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/polic-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/polic-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/polic
)
