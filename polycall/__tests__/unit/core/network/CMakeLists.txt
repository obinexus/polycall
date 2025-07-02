# Auto-generated CMakeLists.txt for core/network tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME network)

# Target for network_test_network
add_executable(network_test_network test_network.c)
target_include_directories(network_test_network PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(network_test_network ${MODULE_LIB_NAME})
add_dependencies(unit_tests network_test_network)

# Custom command to run the test
add_custom_command(TARGET network_test_network POST_BUILD
    COMMAND echo "Running network_test_network..."
    COMMAND network_test_network
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for network_test_network_packet
add_executable(network_test_network_packet test_network_packet.c)
target_include_directories(network_test_network_packet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(network_test_network_packet ${MODULE_LIB_NAME})
add_dependencies(unit_tests network_test_network_packet)

# Custom command to run the test
add_custom_command(TARGET network_test_network_packet POST_BUILD
    COMMAND echo "Running network_test_network_packet..."
    COMMAND network_test_network_packet
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for network_test_network_config
add_executable(network_test_network_config test_network_config.c)
target_include_directories(network_test_network_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(network_test_network_config ${MODULE_LIB_NAME})
add_dependencies(unit_tests network_test_network_config)

# Custom command to run the test
add_custom_command(TARGET network_test_network_config POST_BUILD
    COMMAND echo "Running network_test_network_config..."
    COMMAND network_test_network_config
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for network_test_network_server
add_executable(network_test_network_server test_network_server.c)
target_include_directories(network_test_network_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(network_test_network_server ${MODULE_LIB_NAME})
add_dependencies(unit_tests network_test_network_server)

# Custom command to run the test
add_custom_command(TARGET network_test_network_server POST_BUILD
    COMMAND echo "Running network_test_network_server..."
    COMMAND network_test_network_server
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for network_test_network_client
add_executable(network_test_network_client test_network_client.c)
target_include_directories(network_test_network_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(network_test_network_client ${MODULE_LIB_NAME})
add_dependencies(unit_tests network_test_network_client)

# Custom command to run the test
add_custom_command(TARGET network_test_network_client POST_BUILD
    COMMAND echo "Running network_test_network_client..."
    COMMAND network_test_network_client
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
