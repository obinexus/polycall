# CMakeLists.txt for LibPolyCall unit/core tests
cmake_minimum_required(VERSION 3.13)

# Helper function to add a unit test
function(add_unit_test TEST_NAME TEST_SOURCE)
    # Create executable
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Link against test framework and polycall libraries
    target_link_libraries(${TEST_NAME} PRIVATE
        polycall_test_framework
        polycall_static
    )
    
    # Add include directories
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
    )
    
    # Add as test
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Add run target
    add_custom_target(run_${TEST_NAME}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}
        DEPENDS ${TEST_NAME}
        COMMENT "Running test ${TEST_NAME}"
    )
    
    # Add to core unit tests list
    add_dependencies(unit_core_tests ${TEST_NAME})
endfunction()

# Create target for core unit tests
add_custom_target(unit_core_tests
    COMMENT "Core unit tests target"
)

# Add component directories if they exist
set(CORE_COMPONENTS
    polycall
    auth
    config
    edge
    ffi
    micro
    network
    protocol
    telemetry
    accessibility
)

foreach(COMPONENT ${CORE_COMPONENTS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}")
        # Gather all test sources
        file(GLOB COMPONENT_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/test_*.c")
        
        foreach(TEST_SOURCE ${COMPONENT_TEST_SOURCES})
            # Get test name from file name
            get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
            
            # Create full test name with component prefix
            set(FULL_TEST_NAME "test_core_${COMPONENT}_${TEST_NAME}")
            
            # Add the test
            add_unit_test(${FULL_TEST_NAME} ${TEST_SOURCE})
        endforeach()
        
        # Create target to run all tests for this component
        add_custom_target(run_core_${COMPONENT}_tests
            COMMENT "Running all tests for core/${COMPONENT}"
        )
        
        # Add dependencies for each test in this component
        foreach(TEST_SOURCE ${COMPONENT_TEST_SOURCES})
            get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
            set(FULL_TEST_NAME "test_core_${COMPONENT}_${TEST_NAME}")
            add_dependencies(run_core_${COMPONENT}_tests run_${FULL_TEST_NAME})
        endforeach()
        
        # Add component tests to core tests
        add_dependencies(unit_core_tests run_core_${COMPONENT}_tests)
    endif()
endforeach()

# Add unit tests for hierarchical error handling
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/polycall/test_hierarchical_error.c")
    add_unit_test(test_core_polycall_hierarchical_error 
                 "${CMAKE_CURRENT_SOURCE_DIR}/polycall/test_hierarchical_error.c")
endif()

# Add this directory to unit tests target
add_dependencies(unit_tests unit_core_tests)