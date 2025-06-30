# CMakeLists.txt for LibPolyCall core components
# This file coordinates the build of all core components

# Include common build system modules
include(${CMAKE_SOURCE_DIR}/cmake/BuildSystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ComponentSystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Hooks.cmake)

# Initialize hook system
polycall_init_hook_system(
  OBJ_DIR "${CMAKE_BINARY_DIR}/obj"
  BIN_DIR "${CMAKE_BINARY_DIR}/bin"
  LIB_DIR "${CMAKE_BINARY_DIR}/lib"
  TEST_DIR "${CMAKE_BINARY_DIR}/tests"
  LOG_DIR "${CMAKE_BINARY_DIR}/logs"
  VERBOSE
)

# Execute pre-build hooks
polycall_execute_pre_build_hooks()

# Discover all core components
file(GLOB CORE_COMPONENTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")
set(CORE_COMPONENT_DIRS "")

foreach(COMPONENT ${CORE_COMPONENTS})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT})
    # Add to list of component directories
    list(APPEND CORE_COMPONENT_DIRS ${COMPONENT})
    
    # Register component with the build system
    if(NOT DEFINED POLYCALL_COMPONENT_SYSTEM_INCLUDED)
      # If component system isn't loaded yet, just add to our local list
      message(STATUS "Found core component: ${COMPONENT}")
    else()
      # Register with the component system
      polycall_register_component(${COMPONENT})
    endif()
  endif()
endforeach()

# Add each component subdirectory
foreach(COMPONENT_DIR ${CORE_COMPONENT_DIRS})
  add_subdirectory(${COMPONENT_DIR})
endforeach()

# Create unified core library
add_custom_target(polycall_unified_core ALL
  DEPENDS polycall_core_components
  COMMENT "Building unified core library"
)

# Create static unified library
get_property(STATIC_LIBS GLOBAL PROPERTY POLYCALL_STATIC_LIBRARIES)
if(STATIC_LIBS)
  add_library(polycall_core STATIC IMPORTED GLOBAL)
  set_property(TARGET polycall_core PROPERTY
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/libpolycall_core.a"
  )
  
  # Command to create the static unified library
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/lib/libpolycall_core.a"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_AR} crs "${CMAKE_BINARY_DIR}/lib/libpolycall_core.a"
            $<TARGET_OBJECTS:polycall_polycall_objects>
            $<$<TARGET_EXISTS:polycall_auth_objects>:$<TARGET_OBJECTS:polycall_auth_objects>>
            $<$<TARGET_EXISTS:polycall_config_objects>:$<TARGET_OBJECTS:polycall_config_objects>>
            $<$<TARGET_EXISTS:polycall_edge_objects>:$<TARGET_OBJECTS:polycall_edge_objects>>
            $<$<TARGET_EXISTS:polycall_ffi_objects>:$<TARGET_OBJECTS:polycall_ffi_objects>>
            $<$<TARGET_EXISTS:polycall_micro_objects>:$<TARGET_OBJECTS:polycall_micro_objects>>
            $<$<TARGET_EXISTS:polycall_network_objects>:$<TARGET_OBJECTS:polycall_network_objects>>
            $<$<TARGET_EXISTS:polycall_protocol_objects>:$<TARGET_OBJECTS:polycall_protocol_objects>>
            $<$<TARGET_EXISTS:polycall_telemetry_objects>:$<TARGET_OBJECTS:polycall_telemetry_objects>>
            $<$<TARGET_EXISTS:polycall_accessibility_objects>:$<TARGET_OBJECTS:polycall_accessibility_objects>>
    DEPENDS ${STATIC_LIBS}
    COMMENT "Creating unified static library"
  )
  
  add_custom_target(polycall_unified_static
    DEPENDS "${CMAKE_BINARY_DIR}/lib/libpolycall_core.a"
  )
  
  add_dependencies(polycall_unified_core polycall_unified_static)
endif()

# Create shared unified library
get_property(SHARED_LIBS GLOBAL PROPERTY POLYCALL_SHARED_LIBRARIES)
if(SHARED_LIBS)
  add_library(polycall_core_shared SHARED IMPORTED GLOBAL)
  set_property(TARGET polycall_core_shared PROPERTY
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/libpolycall_core.so"
  )
  
  # Command to create the shared unified library
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/lib/libpolycall_core.so"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND ${CMAKE_C_COMPILER} -shared -o "${CMAKE_BINARY_DIR}/lib/libpolycall_core.so"
            -Wl,--whole-archive
            $<TARGET_OBJECTS:polycall_polycall_objects>
            $<$<TARGET_EXISTS:polycall_auth_objects>:$<TARGET_OBJECTS:polycall_auth_objects>>
            $<$<TARGET_EXISTS:polycall_config_objects>:$<TARGET_OBJECTS:polycall_config_objects>>
            $<$<TARGET_EXISTS:polycall_edge_objects>:$<TARGET_OBJECTS:polycall_edge_objects>>
            $<$<TARGET_EXISTS:polycall_ffi_objects>:$<TARGET_OBJECTS:polycall_ffi_objects>>
            $<$<TARGET_EXISTS:polycall_micro_objects>:$<TARGET_OBJECTS:polycall_micro_objects>>
            $<$<TARGET_EXISTS:polycall_network_objects>:$<TARGET_OBJECTS:polycall_network_objects>>
            $<$<TARGET_EXISTS:polycall_protocol_objects>:$<TARGET_OBJECTS:polycall_protocol_objects>>
            $<$<TARGET_EXISTS:polycall_telemetry_objects>:$<TARGET_OBJECTS:polycall_telemetry_objects>>
            $<$<TARGET_EXISTS:polycall_accessibility_objects>:$<TARGET_OBJECTS:polycall_accessibility_objects>>
            -Wl,--no-whole-archive
            -lpthread
    DEPENDS ${SHARED_LIBS}
    COMMENT "Creating unified shared library"
  )
  
  add_custom_target(polycall_unified_shared
    DEPENDS "${CMAKE_BINARY_DIR}/lib/libpolycall_core.so"
  )
  
  add_dependencies(polycall_unified_core polycall_unified_shared)
endif()
# CMakeLists.txt for LibPolyCall src directory
# Coordinates the build of all source components

# Add core components directory
add_subdirectory(core)

# Add the main polycall source file
set(POLYCALL_MAIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/polycall.c
)

# Create the main polycall object library
add_library(polycall_main_objects OBJECT ${POLYCALL_MAIN_SOURCES})
target_include_directories(polycall_main_objects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Set position independent code for shared library compatibility
set_property(TARGET polycall_main_objects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Add the main source to both static and shared libraries
target_sources(polycall_core_static PRIVATE $<TARGET_OBJECTS:polycall_main_objects>)
target_sources(polycall_core_shared PRIVATE $<TARGET_OBJECTS:polycall_main_objects>)

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/polycall/core/polycall/polycall_version.h.in
    ${CMAKE_BINARY_DIR}/include/polycall/core/polycall/polycall_version.h
    @ONLY
)

# Install generated version header
install(
    FILES ${CMAKE_BINARY_DIR}/include/polycall/core/polycall/polycall_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/polycall/core/polycall
    COMPONENT devel
)

# Create symbolic link for the unified header
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/polycall.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include
    DEPENDS ${CMAKE_BINARY_DIR}/include/polycall.h
    COMMENT "Creating symbolic link for unified header"
)

# Add custom target for header generation
add_custom_target(generate_headers ALL
    DEPENDS ${CMAKE_BINARY_DIR}/include/polycall.h
)

# Execute post-build hooks
polycall_execute_post_build_hooks()

# Output build summary
message(STATUS "=== LibPolyCall Core Build Summary ===")
message(STATUS "Components: ${CORE_COMPONENT_DIRS}")
message(STATUS "Static libraries: ${STATIC_LIBS}")
message(STATUS "Shared libraries: ${SHARED_LIBS}")
message(STATUS "======================================")