# CMakeLists.txt for Biafran UI/UX Accessibility module unit tests
# Configures tests for the accessibility color system and interface components

# Find required packages
if(USE_CHECK_FRAMEWORK)
    find_package(Check REQUIRED)
endif()

# Define test source files
set(TEST_SOURCES
    accessibility_colors_test.c
    accessibility_interface_test.c
)

# Create targets for each test
foreach(TEST_SOURCE ${TEST_SOURCES})
    # Extract test name from file name (remove .c extension)
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create test executable
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Set include directories
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../framework
    )
    
    # Link with required libraries
    target_link_libraries(${TEST_NAME} PRIVATE
        polycall_core
        polycall_accessibility
    )
    
    # Link with test framework if available
    if(USE_CHECK_FRAMEWORK AND CHECK_FOUND)
        target_link_libraries(${TEST_NAME} PRIVATE Check::Check)
        target_compile_definitions(${TEST_NAME} PRIVATE USE_CHECK_FRAMEWORK)
    endif()
    
    # Add test to CTest
    add_test(
        NAME ${TEST_NAME}
        COMMAND ${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "unit;accessibility"
        TIMEOUT 30
    )
    
    # Add to global test target
    add_dependencies(build_all_tests ${TEST_NAME})
endforeach()

# Add message about configured tests
message(STATUS "Configured Biafran UI/UX Accessibility unit tests")