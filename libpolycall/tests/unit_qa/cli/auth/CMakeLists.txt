# Unit QA tests for cli/auth
add_executable(test_cli_auth_qa test_auth_qa.c)
target_link_libraries(test_cli_auth_qa ${TEST_LIBRARIES})
