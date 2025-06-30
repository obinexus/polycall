# Auto-generated CMakeLists.txt for core/edge tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME edge)

# Target for edge_test_edge_core
add_executable(edge_test_edge_core test_edge_core.c)
target_include_directories(edge_test_edge_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(edge_test_edge_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests edge_test_edge_core)

# Custom command to run the test
add_custom_command(TARGET edge_test_edge_core POST_BUILD
    COMMAND echo "Running edge_test_edge_core..."
    COMMAND edge_test_edge_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
