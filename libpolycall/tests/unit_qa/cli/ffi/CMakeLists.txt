# Unit QA tests for cli/ffi
add_executable(test_cli_ffi_qa test_ffi_qa.c)
target_link_libraries(test_cli_ffi_qa ${TEST_LIBRARIES})
