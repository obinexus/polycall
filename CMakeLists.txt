cmake_minimum_required(VERSION 3.16)
project(libpolycall 
    VERSION 0.1.0
    DESCRIPTION "OBINexus LibPolyCall - Hybrid C/Python polyglot call framework"
    LANGUAGES C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection and configuration
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    message(STATUS "Configuring for Windows platform")
elseif(UNIX AND NOT APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Configuring for Linux platform")
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    message(STATUS "Configuring for macOS platform")
endif()

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find Python for hybrid builds
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
message(STATUS "Python3 version: ${Python3_VERSION}")
message(STATUS "Python3 include dirs: ${Python3_INCLUDE_DIRS}")

# Options for build configuration
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_SINPHASE "Enable Sinphasé governance checks" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

if(MSVC)
    add_compile_options(
        /W4
        /wd4996  # Disable deprecation warnings
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
    )
endif()

# Include helpers if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Helpers.cmake")
    include(Helpers)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindDependencies.cmake")
    include(FindDependencies)
endif()

# Add subdirectories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libpolycall/CMakeLists.txt")
    add_subdirectory(libpolycall)
else()
    message(WARNING "libpolycall/CMakeLists.txt not found, skipping C library build")
endif()

# Isolated builds for FFI/Protocol (not bundled into Python package)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/root-dynamic-c/CMakeLists.txt")
    add_subdirectory(root-dynamic-c EXCLUDE_FROM_ALL)
    message(STATUS "Added root-dynamic-c as isolated build target")
endif()

# Testing
if(BUILD_TESTING)
    enable_testing()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# Examples
if(BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# Installation rules
include(GNUInstallDirs)

# Export targets
install(EXPORT libpolycallTargets
    FILE libpolycallTargets.cmake
    NAMESPACE libpolycall::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpolycall
)

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libpolycallConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libpolycallConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpolycall
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libpolycallConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libpolycallConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libpolycallConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpolycall
)

# Summary
message(STATUS "")
message(STATUS "=== LibPolyCall Configuration Summary ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "Build tests: ${BUILD_TESTING}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Enable Sinphasé: ${ENABLE_SINPHASE}")
message(STATUS "Python bindings: ${BUILD_PYTHON_BINDINGS}")
message(STATUS "=========================================")