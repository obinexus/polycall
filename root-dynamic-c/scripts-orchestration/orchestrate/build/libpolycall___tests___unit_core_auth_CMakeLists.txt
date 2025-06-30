# Auto-generated CMakeLists.txt for core/auth tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME auth)

# Target for auth_test_auth_core
add_executable(auth_test_auth_core test_auth_core.c)
target_include_directories(auth_test_auth_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(auth_test_auth_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests auth_test_auth_core)

# Custom command to run the test
add_custom_command(TARGET auth_test_auth_core POST_BUILD
    COMMAND echo "Running auth_test_auth_core..."
    COMMAND auth_test_auth_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
