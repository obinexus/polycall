# Unit QA tests for cli/socket
add_executable(test_cli_socket_qa test_socket_qa.c)
target_link_libraries(test_cli_socket_qa ${TEST_LIBRARIES})
