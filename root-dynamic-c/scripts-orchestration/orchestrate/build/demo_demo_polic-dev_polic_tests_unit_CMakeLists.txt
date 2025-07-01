add_executable(test_polic_core test_core.c)
target_link_libraries(test_polic_core PRIVATE polic)
add_test(NAME test_polic_core COMMAND test_polic_core)
