# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.13)
project(OBINexusPolyCall 
    VERSION 0.7.0
    DESCRIPTION "OBINexus PolyCall - Unified Runtime Abstraction"
    LANGUAGES C CXX
)

# CMake module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Include enhanced modules
include(PolycallConfig)
include(PolycallDocs)
include(PolycallTests)

if(POLYCALL_ENABLE_NLINK)
    include(PolycallNLink)
endif()

# Create build directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Process source directories
add_subdirectory(src)

# Documentation
if(POLYCALL_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Tests
if(POLYCALL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Examples
if(POLYCALL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
include(PolycallPackaging)

# Generate configuration files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PolycallConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PolycallConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Polycall
)

# Install CMake modules
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/PolycallConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Polycall
)

# Summary
message(STATUS "") 
message(STATUS "OBINexus 
PolyCall Configuration 
Summary:") message(STATUS " 
Version: ${POLYCALL_VERSION}") 
message(STATUS " Build Type: 
${CMAKE_BUILD_TYPE}") 
message(STATUS " Security 
Level: 
${POLYCALL_SECURITY_LEVEL}") 
message(STATUS " 
Documentation: 
${POLYCALL_BUILD_DOCS}") 
message(STATUS " Tests: 
${POLYCALL_BUILD_TESTS}") 
message(STATUS " NLink: 
${POLYCALL_ENABLE_NLINK}") 
message(STATUS "")
