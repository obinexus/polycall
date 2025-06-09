# Unit QA tests for cli/config
add_executable(test_cli_config_qa test_config_qa.c)
target_link_libraries(test_cli_config_qa ${TEST_LIBRARIES})
