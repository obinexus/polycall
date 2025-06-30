cmake_minimum_required(VERSION 3.13)
project(LibPolyCallTests)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required dependencies
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Link directories
link_directories(${CMAKE_BINARY_DIR}/lib)

# Common test libraries
set(TEST_LIBRARIES polycall Threads::Threads)

# Add test subdirectories
add_subdirectory(unit)
add_subdirectory(unit_qa)
add_subdirectory(integration)
add_subdirectory(integration_qa)

# Custom target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running all libpolycall tests"
)
