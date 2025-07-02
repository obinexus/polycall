# Auto-generated CMakeLists.txt for core/micro tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME micro)

# Target for micro_test_micro_core
add_executable(micro_test_micro_core test_micro_core.c)
target_include_directories(micro_test_micro_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(micro_test_micro_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests micro_test_micro_core)

# Custom command to run the test
add_custom_command(TARGET micro_test_micro_core POST_BUILD
    COMMAND echo "Running micro_test_micro_core..."
    COMMAND micro_test_micro_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
