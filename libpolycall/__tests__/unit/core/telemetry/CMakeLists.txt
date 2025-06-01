# Auto-generated CMakeLists.txt for core/telemetry tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME telemetry)

# Target for telemetry_test_telemetry_core
add_executable(telemetry_test_telemetry_core test_telemetry_core.c)
target_include_directories(telemetry_test_telemetry_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(telemetry_test_telemetry_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests telemetry_test_telemetry_core)

# Custom command to run the test
add_custom_command(TARGET telemetry_test_telemetry_core POST_BUILD
    COMMAND echo "Running telemetry_test_telemetry_core..."
    COMMAND telemetry_test_telemetry_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
