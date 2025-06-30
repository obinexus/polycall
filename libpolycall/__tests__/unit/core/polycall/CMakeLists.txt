# Auto-generated CMakeLists.txt for core/polycall tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME polycall)

# Target for polycall_test_config
add_executable(polycall_test_config test_config.c)
target_include_directories(polycall_test_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(polycall_test_config ${MODULE_LIB_NAME})
add_dependencies(unit_tests polycall_test_config)

# Custom command to run the test
add_custom_command(TARGET polycall_test_config POST_BUILD
    COMMAND echo "Running polycall_test_config..."
    COMMAND polycall_test_config
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for polycall_test_memory
add_executable(polycall_test_memory test_memory.c)
target_include_directories(polycall_test_memory PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(polycall_test_memory ${MODULE_LIB_NAME})
add_dependencies(unit_tests polycall_test_memory)

# Custom command to run the test
add_custom_command(TARGET polycall_test_memory POST_BUILD
    COMMAND echo "Running polycall_test_memory..."
    COMMAND polycall_test_memory
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for polycall_test_polycall_core
add_executable(polycall_test_polycall_core test_polycall_core.c)
target_include_directories(polycall_test_polycall_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(polycall_test_polycall_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests polycall_test_polycall_core)

# Custom command to run the test
add_custom_command(TARGET polycall_test_polycall_core POST_BUILD
    COMMAND echo "Running polycall_test_polycall_core..."
    COMMAND polycall_test_polycall_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for polycall_test_context
add_executable(polycall_test_context test_context.c)
target_include_directories(polycall_test_context PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(polycall_test_context ${MODULE_LIB_NAME})
add_dependencies(unit_tests polycall_test_context)

# Custom command to run the test
add_custom_command(TARGET polycall_test_context POST_BUILD
    COMMAND echo "Running polycall_test_context..."
    COMMAND polycall_test_context
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for polycall_test_hierachical_error
add_executable(polycall_test_hierachical_error test_hierachical_error.c)
target_include_directories(polycall_test_hierachical_error PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(polycall_test_hierachical_error ${MODULE_LIB_NAME})
add_dependencies(unit_tests polycall_test_hierachical_error)

# Custom command to run the test
add_custom_command(TARGET polycall_test_hierachical_error POST_BUILD
    COMMAND echo "Running polycall_test_hierachical_error..."
    COMMAND polycall_test_hierachical_error
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for polycall_test_error
add_executable(polycall_test_error test_error.c)
target_include_directories(polycall_test_error PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(polycall_test_error ${MODULE_LIB_NAME})
add_dependencies(unit_tests polycall_test_error)

# Custom command to run the test
add_custom_command(TARGET polycall_test_error POST_BUILD
    COMMAND echo "Running polycall_test_error..."
    COMMAND polycall_test_error
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

