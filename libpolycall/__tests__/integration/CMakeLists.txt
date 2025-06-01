# Integration tests CMakeLists.txt
cmake_minimum_required(VERSION 3.13)

# Define integration test groups
set(INTEGRATION_TEST_GROUPS
    core_protocol
    core_polycall_micro
    micro_edge_command
    polycall_telemetry
    full_stack
)

# Add each integration test group
foreach(GROUP ${INTEGRATION_TEST_GROUPS})
    add_subdirectory(${GROUP})
endforeach()

# Define integration test driver
add_executable(run_integration_tests integration_test_driver.c)
target_link_libraries(run_integration_tests
    polycall_core
    polycall_shared
    ${UNIT_TEST_FRAMEWORK}
)

# Add integration test target
add_custom_target(integration_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_integration_tests
    DEPENDS run_integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests"
)