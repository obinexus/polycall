# Unit QA tests for cli/telemetry
add_executable(test_cli_telemetry_qa test_telemetry_qa.c)
target_link_libraries(test_cli_telemetry_qa ${TEST_LIBRARIES})
