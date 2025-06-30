# CMakeLists.txt for LibPolyCall unit/core tests
cmake_minimum_required(VERSION 3.13)

# Include the test framework module
include(TestFramework)

# Create target for core unit tests
add_custom_target(unit_core_tests
  COMMENT "Core unit tests target"
)

# Get all component subdirectories
file(GLOB COMPONENT_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")
foreach(COMPONENT_DIR ${COMPONENT_DIRS})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT_DIR})
    # Add each component test directory
    add_subdirectory(${COMPONENT_DIR})
    
    # Create component stubs
    polycall_create_component_stubs(${COMPONENT_DIR})
  endif()
endforeach()

# Create a target that runs all core tests
add_custom_target(run_core_tests
  DEPENDS unit_core_tests
  COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE} -L "unit:core"
  COMMENT "Running all core unit tests"
)