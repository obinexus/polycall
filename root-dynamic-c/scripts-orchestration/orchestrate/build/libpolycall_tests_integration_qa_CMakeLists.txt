# Integration QA Tests CMake Configuration

# Protocol + Network QA integration
add_executable(test_protocol_network_qa test_protocol_network_qa.c)
target_link_libraries(test_protocol_network_qa ${TEST_LIBRARIES})

# FFI + Protocol QA integration
add_executable(test_ffi_protocol_qa test_ffi_protocol_qa.c)
target_link_libraries(test_ffi_protocol_qa ${TEST_LIBRARIES})

# REPL + Protocol + Telemetry QA integration
add_executable(test_repl_protocol_telemetry_qa test_repl_protocol_telemetry_qa.c)
target_link_libraries(test_repl_protocol_telemetry_qa ${TEST_LIBRARIES})

# Edge + Network + Micro QA integration
add_executable(test_edge_network_micro_qa test_edge_network_micro_qa.c)
target_link_libraries(test_edge_network_micro_qa ${TEST_LIBRARIES})

# Auth + Security + Protocol QA integration
add_executable(test_auth_security_protocol_qa test_auth_security_protocol_qa.c)
target_link_libraries(test_auth_security_protocol_qa ${TEST_LIBRARIES})
