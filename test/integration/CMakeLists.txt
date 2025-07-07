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
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${GROUP})
        add_subdirectory(${GROUP})
    endif()
endforeach()

# Define integration test driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration_test_driver.c)
    add_executable(run_integration_tests integration_test_driver.c)
    target_link_libraries(run_integration_tests
        polycall_core
        ${CMAKE_THREAD_LIBS_INIT}
    )
endif()

# Add integration test target
add_custom_target(integration_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_integration_tests
    DEPENDS run_integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests"
)
