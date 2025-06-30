# CMakeLists.txt for LibPolyCall component
# This is a template for component build configuration
# Replace {COMPONENT_NAME} with the actual component name

# Include common build system modules
include(${CMAKE_SOURCE_DIR}/cmake/ComponentBuild.cmake)

# Initialize component build system
polycall_component_build_init(VERBOSE)

# Get all source files in this component
file(GLOB COMPONENT_SOURCES "*.c")
file(GLOB COMPONENT_HEADERS "*.h")

# Collect subdirectory sources if they exist
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")
foreach(SUBDIR ${SUBDIRS})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
    file(GLOB SUBDIR_SOURCES "${SUBDIR}/*.c")
    list(APPEND COMPONENT_SOURCES ${SUBDIR_SOURCES})
    
    file(GLOB SUBDIR_HEADERS "${SUBDIR}/*.h")
    list(APPEND COMPONENT_HEADERS ${SUBDIR_HEADERS})
  endif()
endforeach()

# Define component include directories
set(COMPONENT_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/polycall
  ${CMAKE_SOURCE_DIR}/include/polycall/core
  ${CMAKE_SOURCE_DIR}/src
)

# Define component dependencies
set(COMPONENT_DEPENDENCIES
  # Add dependencies to other components here
  # For example: polycall_core_static
  pthread
)

# Build the component
polycall_build_component(
  # NAME should match the directory name for auto-detection
  # TYPE is typically "core" for core components, or "cli" for CLI components
  TYPE core
  SOURCES ${COMPONENT_SOURCES}
  HEADERS ${COMPONENT_HEADERS}
  INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
  DEPENDENCIES ${COMPONENT_DEPENDENCIES}
  STATIC   # Build static library
  SHARED   # Build shared library
  VERBOSE  # Output build information
)

# Add tests if testing is enabled
if(BUILD_TESTING)
  # Get test sources
  file(GLOB TEST_SOURCES "tests/*.c")
  
  if(TEST_SOURCES)
    polycall_build_component_tests(
      SOURCES ${TEST_SOURCES}
      DEPENDENCIES ${COMPONENT_DEPENDENCIES}
      VERBOSE
    )
  endif()
endif()

# Finalize component build
polycall_finalize_component_build()