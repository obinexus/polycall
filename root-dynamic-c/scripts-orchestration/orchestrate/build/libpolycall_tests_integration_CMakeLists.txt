# Integration Tests CMake Configuration

# Protocol + Network integration
add_executable(test_protocol_network_integration test_protocol_network_integration.c)
target_link_libraries(test_protocol_network_integration ${TEST_LIBRARIES})

# FFI + Protocol integration
add_executable(test_ffi_protocol_integration test_ffi_protocol_integration.c)
target_link_libraries(test_ffi_protocol_integration ${TEST_LIBRARIES})

# REPL + Protocol + Telemetry integration
add_executable(test_repl_protocol_telemetry test_repl_protocol_telemetry.c)
target_link_libraries(test_repl_protocol_telemetry ${TEST_LIBRARIES})

# Edge + Network + Micro integration
add_executable(test_edge_network_micro test_edge_network_micro.c)
target_link_libraries(test_edge_network_micro ${TEST_LIBRARIES})

# Auth + Security + Protocol integration
add_executable(test_auth_security_protocol test_auth_security_protocol.c)
target_link_libraries(test_auth_security_protocol ${TEST_LIBRARIES})
