# Auto-generated CMakeLists.txt for core/ffi tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME ffi)

# Target for ffi_test_ffi_core
add_executable(ffi_test_ffi_core test_ffi_core.c)
target_include_directories(ffi_test_ffi_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(ffi_test_ffi_core ${MODULE_LIB_NAME})
add_dependencies(unit_tests ffi_test_ffi_core)

# Custom command to run the test
add_custom_command(TARGET ffi_test_ffi_core POST_BUILD
    COMMAND echo "Running ffi_test_ffi_core..."
    COMMAND ffi_test_ffi_core
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for ffi_test_memory_bridge
add_executable(ffi_test_memory_bridge test_memory_bridge.c)
target_include_directories(ffi_test_memory_bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(ffi_test_memory_bridge ${MODULE_LIB_NAME})
add_dependencies(unit_tests ffi_test_memory_bridge)

# Custom command to run the test
add_custom_command(TARGET ffi_test_memory_bridge POST_BUILD
    COMMAND echo "Running ffi_test_memory_bridge..."
    COMMAND ffi_test_memory_bridge
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Target for ffi_test_type_system
add_executable(ffi_test_type_system test_type_system.c)
target_include_directories(ffi_test_type_system PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../framework
)
target_link_libraries(ffi_test_type_system ${MODULE_LIB_NAME})
add_dependencies(unit_tests ffi_test_type_system)

# Custom command to run the test
add_custom_command(TARGET ffi_test_type_system POST_BUILD
    COMMAND echo "Running ffi_test_type_system..."
    COMMAND ffi_test_type_system
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
