# Unit QA tests for cli/repl
add_executable(test_cli_repl_qa test_repl_qa.c)
target_link_libraries(test_cli_repl_qa ${TEST_LIBRARIES})
