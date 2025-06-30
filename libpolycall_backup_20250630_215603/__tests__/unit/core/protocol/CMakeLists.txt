# Auto-generated CMakeLists.txt for core/protocol tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME protocol)

# Target for protocol_test_protocol_message
add_executable(protocol_test_protocol_message test_protocol_message.c)
target_include_directories(protocol_test_protocol_message PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_protocol_message ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_protocol_message)

# Custom command to run the test
add_custom_command(TARGET protocol_test_protocol_message POST_BUILD
    COMMAND echo "Running protocol_test_protocol_message..."
    COMMAND protocol_test_protocol_message
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_message
add_executable(protocol_test_message test_message.c)
target_include_directories(protocol_test_message PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_message ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_message)

# Custom command to run the test
add_custom_command(TARGET protocol_test_message POST_BUILD
    COMMAND echo "Running protocol_test_message..."
    COMMAND protocol_test_message
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_command
add_executable(protocol_test_command test_command.c)
target_include_directories(protocol_test_command PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_command ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_command)

# Custom command to run the test
add_custom_command(TARGET protocol_test_command POST_BUILD
    COMMAND echo "Running protocol_test_command..."
    COMMAND protocol_test_command
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_protocol_core
add_executable(protocol_test_protocol_core test_protocol_core.c)
target_include_directories(protocol_test_protocol_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_protocol_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_protocol_core)

# Custom command to run the test
add_custom_command(TARGET protocol_test_protocol_core POST_BUILD
    COMMAND echo "Running protocol_test_protocol_core..."
    COMMAND protocol_test_protocol_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_crypto
add_executable(protocol_test_crypto test_crypto.c)
target_include_directories(protocol_test_crypto PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_crypto ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_crypto)

# Custom command to run the test
add_custom_command(TARGET protocol_test_crypto POST_BUILD
    COMMAND echo "Running protocol_test_crypto..."
    COMMAND protocol_test_crypto
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_protocol_enhancements
add_executable(protocol_test_protocol_enhancements test_protocol_enhancements.c)
target_include_directories(protocol_test_protocol_enhancements PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_protocol_enhancements ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_protocol_enhancements)

# Custom command to run the test
add_custom_command(TARGET protocol_test_protocol_enhancements POST_BUILD
    COMMAND echo "Running protocol_test_protocol_enhancements..."
    COMMAND protocol_test_protocol_enhancements
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_protocol_config
add_executable(protocol_test_protocol_config test_protocol_config.c)
target_include_directories(protocol_test_protocol_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_protocol_config ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_protocol_config)

# Custom command to run the test
add_custom_command(TARGET protocol_test_protocol_config POST_BUILD
    COMMAND echo "Running protocol_test_protocol_config..."
    COMMAND protocol_test_protocol_config
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for protocol_test_protocol_state_machine
add_executable(protocol_test_protocol_state_machine test_protocol_state_machine.c)
target_include_directories(protocol_test_protocol_state_machine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(protocol_test_protocol_state_machine ${MODULE_LIB_NAME})
add_dependencies(unit_tests protocol_test_protocol_state_machine)

# Custom command to run the test
add_custom_command(TARGET protocol_test_protocol_state_machine POST_BUILD
    COMMAND echo "Running protocol_test_protocol_state_machine..."
    COMMAND protocol_test_protocol_state_machine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# CMakeLists.txt for LibPolyCall unit/core/protocol tests
cmake_minimum_required(VERSION 3.13)

# Include the test framework module
include(TestFramework)

# Create component stubs (if needed)
polycall_create_component_stubs(protocol)

# Create target for protocol unit tests
add_custom_target(unit_core_protocol_tests
    COMMENT "protocol unit tests target"
)

# Get all test sources in this directory
file(GLOB PROTOCOL_TEST_SOURCES "*.c")

# Add each test file
foreach(TEST_SOURCE ${PROTOCOL_TEST_SOURCES})
    # Get test name from file name
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create full test name with component prefix
    set(FULL_TEST_NAME "test_core_protocol_${TEST_NAME}")
    
    # Add the test using the AAA pattern
    polycall_add_aaa_test(
        NAME ${TEST_NAME}
        COMPONENT "protocol"
        SOURCES ${TEST_SOURCE}
        MOCK_COMPONENTS "protocol"
    )
endforeach()

# Create a target that runs all protocol tests
add_custom_target(run_core_protocol_tests
    DEPENDS unit_core_protocol_tests
    COMMENT "Running all protocol tests"
)

# Add this component's tests to the unit_core_tests target
add_dependencies(unit_core_tests unit_core_protocol_tests)
