# OBINexus Polycall Test Suite CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Test configuration
set(POLYCALL_TEST_VERSION "0.7.0")

# Find all test source files
file(GLOB_RECURSE TEST_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Test subdirectories to process
set(TEST_CATEGORIES
    unit
    integration
    performance
    functional
)

# Common test include directories
set(TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Common test libraries
set(TEST_LIBRARIES
    polycall_core
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Process test categories
foreach(CATEGORY ${TEST_CATEGORIES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY})
        message(STATUS "Processing test category: ${CATEGORY}")
        
        # Find test files in this category
        file(GLOB_RECURSE CATEGORY_TESTS
            "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/*test*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/*test*.cpp"
        )
        
        # Create test executable for each test file
        foreach(TEST_FILE ${CATEGORY_TESTS})
            get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
            set(TEST_TARGET "test_${CATEGORY}_${TEST_NAME}")
            
            add_executable(${TEST_TARGET} ${TEST_FILE})
            
            target_include_directories(${TEST_TARGET}
                PRIVATE ${TEST_INCLUDE_DIRS}
            )
            
            target_link_libraries(${TEST_TARGET}
                PRIVATE ${TEST_LIBRARIES}
            )
            
            # Add to CTest
            add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
            
            # Set test properties
            set_tests_properties(${TEST_TARGET} PROPERTIES
                TIMEOUT 60
                LABELS ${CATEGORY}
            )
            
            message(STATUS "  Added test: ${TEST_TARGET}")
        endforeach()
        
        # Add subdirectory if it has CMakeLists.txt
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${CATEGORY}/CMakeLists.txt)
            add_subdirectory(${CATEGORY})
        endif()
    endif()
endforeach()

# Create test runner if no specific tests found
if(NOT CATEGORY_TESTS)
    message(STATUS "No test files found, creating test runner stub")
    
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_runner_stub.c
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "\n"
        "int main() {\n"
        "    printf(\"OBINexus Polycall Test Suite v${POLYCALL_TEST_VERSION}\\n\");\n"
        "    printf(\"No tests implemented yet\\n\");\n"
        "    return 0;\n"
        "}\n"
    )
    
    add_executable(test_runner ${CMAKE_CURRENT_BINARY_DIR}/test_runner_stub.c)
    
    target_link_libraries(test_runner
        PRIVATE ${TEST_LIBRARIES}
    )
    
    add_test(NAME test_runner COMMAND test_runner)
endif()

# Test utilities
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_all_tests.sh
    "#!/bin/bash\n"
    "echo \"Running OBINexus Polycall Test Suite\"\n"
    "cd ${CMAKE_CURRENT_BINARY_DIR}\n"
    "ctest --output-on-failure --verbose\n"
)

# Test summary
message(STATUS "Test suite configuration complete:")
message(STATUS "  Test categories: ${TEST_CATEGORIES}")
message(STATUS "  Total test files: ${list_length:TEST_SOURCES}")
