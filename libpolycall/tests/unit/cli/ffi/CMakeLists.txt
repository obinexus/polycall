# Unit tests for cli/ffi
add_executable(test_cli_ffi test_ffi.c)
target_link_libraries(test_cli_ffi ${TEST_LIBRARIES})
