# CMakeLists.txt for LibPolyCall tests
cmake_minimum_required(VERSION 3.13)

# Define test framework library
set(TEST_FRAMEWORK_SOURCES
    framework/polycall_test_framework.c
    /home/obinexus/projects/obinexus/pkg/libpolycall/libpolycall/include/__tests__/setup/test_stub_manager.c
)

# Add stub implementations
file(GLOB COMPONENT_STUB_SOURCES "/home/obinexus/projects/obinexus/pkg/libpolycall/libpolycall/include/__tests__/setup/*_test_stub.c")
list(APPEND TEST_FRAMEWORK_SOURCES ${COMPONENT_STUB_SOURCES})

add_library(polycall_test_framework STATIC ${TEST_FRAMEWORK_SOURCES})
target_include_directories(polycall_test_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/__tests__
)
target_link_libraries(polycall_test_framework PRIVATE
    polycall_static
)

# Function to add a test directory with all its tests
function(add_test_subdirectory DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/CMakeLists.txt")
        add_subdirectory(${DIR})
    else()
        # Auto-generate tests from .c files in directory
        file(GLOB_RECURSE TEST_SOURCES "${DIR}/*.c")
        
        foreach(TEST_SOURCE ${TEST_SOURCES})
            get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
            get_filename_component(TEST_PATH ${TEST_SOURCE} DIRECTORY)
            file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PATH})
            
            # Create namespace from relative path
            string(REPLACE "/" "_" TEST_NAMESPACE ${REL_PATH})
            
            # Final test executable name
            set(TEST_EXEC_NAME "${TEST_NAMESPACE}_${TEST_NAME}")
            
            # Add test executable
            add_executable(${TEST_EXEC_NAME} ${TEST_SOURCE})
            target_link_libraries(${TEST_EXEC_NAME} PRIVATE
                polycall_test_framework
                polycall_static
            )
            target_include_directories(${TEST_EXEC_NAME} PRIVATE
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_SOURCE_DIR}/include/__tests__
                ${CMAKE_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}
            )
            
            # Add as test
            add_test(NAME ${TEST_EXEC_NAME} COMMAND ${TEST_EXEC_NAME})
            
            # Create target to run this test individually
            add_custom_target(run_${TEST_EXEC_NAME}
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_EXEC_NAME}
                DEPENDS ${TEST_EXEC_NAME}
                COMMENT "Running test ${TEST_EXEC_NAME}"
                VERBATIM
            )
            
            # Add to test list for the directory
            list(APPEND ${DIR}_TESTS ${TEST_EXEC_NAME})
        endforeach()
        
        # Create target to run all tests in this directory
        add_custom_target(run_${DIR}_tests
            COMMENT "Running all tests in ${DIR}"
        )
        
        # Add dependencies for each test
        foreach(TEST ${_TESTS})
            add_dependencies(run_${DIR}_tests ${TEST})
        endforeach()
    endif()
endfunction()

# Add test directories
add_test_subdirectory(unit)
add_test_subdirectory(integration)
add_test_subdirectory(e2e)

# Create target to run all tests
add_custom_target(run_all_tests
    DEPENDS run_unit_tests run_integration_tests run_e2e_tests
    COMMENT "Running all tests"
)

# Create targets for each test level
add_custom_target(run_unit_tests
    DEPENDS unit_tests
    COMMENT "Running unit tests"
)

add_custom_target(run_integration_tests
    DEPENDS integration_tests
    COMMENT "Running integration tests"
)

add_custom_target(run_e2e_tests
    DEPENDS e2e_tests
    COMMENT "Running end-to-end tests"
)

# Create test collection targets
add_custom_target(unit_tests)
add_custom_target(integration_tests)
add_custom_target(e2e_tests)

# Add unit test subdirectories
add_custom_target(unit_core_tests)
add_dependencies(unit_tests unit_core_tests)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit/core")
    file(GLOB UNIT_CORE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/unit/core/*")
    foreach(DIR ${UNIT_CORE_DIRS})
        if(IS_DIRECTORY ${DIR})
            get_filename_component(COMPONENT_NAME ${DIR} NAME)
            add_test_subdirectory(unit/core/${COMPONENT_NAME})
        endif()
    endforeach()
endif()

# Generate coverage report if enabled
if(CMAKE_COMPILER_IS_GNUCXX AND ENABLE_COVERAGE)
    include(CodeCoverage)
    SETUP_TARGET_FOR_COVERAGE_LCOV(
        NAME coverage
        EXECUTABLE run_all_tests
        DEPENDENCIES run_all_tests
        BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
        EXCLUDE "/usr/*" "${CMAKE_SOURCE_DIR}/tests/*" "${CMAKE_SOURCE_DIR}/build/*"
        NO_DEMANGLE
    )
endif()

# CMakeLists.txt for LibPolyCall tests
cmake_minimum_required(VERSION 3.13)

# Include necessary modules
include(CTest)
include(TestFramework)
include(TestGeneration)

# Set test directories
set(UNIT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit)
set(INTEGRATION_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/integration)
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/tests)

# Make sure output directory exists
file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR})

# Create main test targets
add_custom_target(unit_tests
  COMMENT "Building all unit tests"
)

add_custom_target(integration_tests
  COMMENT "Building all integration tests"
)

# Add core unit tests
add_subdirectory(unit/core)

# Add integration tests if they exist
if(EXISTS ${INTEGRATION_TEST_DIR})
  add_subdirectory(integration)
endif()

# Setup component test directories for any missing ones
get_property(REGISTERED_COMPONENTS GLOBAL PROPERTY POLYCALL_REGISTERED_COMPONENTS)
foreach(COMPONENT ${REGISTERED_COMPONENTS})
  if(NOT EXISTS "${UNIT_TEST_DIR}/core/${COMPONENT}/CMakeLists.txt")
    polycall_setup_component_test_dir(${COMPONENT})
  endif()
  
  # Create test targets for each component
  add_custom_target(test:unit:${COMPONENT}
    COMMENT "Running unit tests for ${COMPONENT}"
    COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE} -R "test_core_${COMPONENT}_*"
  )
  
  # Add component test target as dependency to unit_tests
  add_dependencies(unit_tests test:unit:${COMPONENT})
endforeach()

# Create test stub generation target
add_custom_target(generate_test_stubs
  COMMAND ${CMAKE_COMMAND} -E echo "Generating test stubs..."
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/generation/generate-test-stubs.sh --setup-framework
  DEPENDS ${REGISTERED_COMPONENTS}
  COMMENT "Generating test stubs for all components"
)

# Add run targets
add_custom_target(run_unit_tests
  DEPENDS unit_tests
  COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE} -L unit
  COMMENT "Running all unit tests"
)

add_custom_target(run_integration_tests
  DEPENDS integration_tests
  COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE} -L integration
  COMMENT "Running all integration tests"
)

add_custom_target(run_all_tests
  DEPENDS unit_tests integration_tests
  COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE}
  COMMENT "Running all tests"
)