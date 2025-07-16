# OBINexus Polycall Core Components CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Core component configuration
set(POLYCALL_CORE_VERSION "0.7.0")

# Find all core component subdirectories
file(GLOB CORE_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")

# Core component source discovery
set(CORE_SOURCES "")
set(CORE_HEADERS "")

# Component directories to process
set(CORE_COMPONENTS
    auth
    config
    network
    protocol
    polycall
    schema
    security
    telemetry
    accessibility
    edge
    ffi
    micro
    repl
    socket
)

# Add to CMakeLists.txt in src/core
add_custom_target(quality_check
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../build-tools/status-check.sh 
          ${CMAKE_BINARY_DIR}/build/logs/warning_count.txt "cmake"
  COMMENT "Verifying build quality metrics"
  DEPENDS ${PROJECT_NAME}
)

add_dependencies(${PROJECT_NAME} quality_check)
# Collect sources from each component
foreach(COMPONENT ${CORE_COMPONENTS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT})
        message(STATUS "Processing core component: ${COMPONENT}")
        
        # Find source files
        file(GLOB_RECURSE COMPONENT_SOURCES 
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.cpp"
        )
        
        # Find header files
        file(GLOB_RECURSE COMPONENT_HEADERS
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/*.hpp"
        )
        
        list(APPEND CORE_SOURCES ${COMPONENT_SOURCES})
        list(APPEND CORE_HEADERS ${COMPONENT_HEADERS})
        
        # Add component subdirectory if it has CMakeLists.txt
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}/CMakeLists.txt)
            add_subdirectory(${COMPONENT})
        endif()
    endif()
endforeach()

# Filter out sources that might cause conflicts
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*test.*")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.c.*")

# Create core library if we have sources
if(CORE_SOURCES)
    message(STATUS "Creating polycall_core library with ${list_length:CORE_SOURCES} sources")
    
    # Static library
    add_library(polycall_core_static STATIC ${CORE_SOURCES})
    target_include_directories(polycall_core_static
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(polycall_core_static
        PRIVATE
            Threads::Threads
            ${CMAKE_DL_LIBS}
    )
    
    set_target_properties(polycall_core_static PROPERTIES
        OUTPUT_NAME "polycall_core"
        VERSION ${POLYCALL_CORE_VERSION}
    )
    
    # Shared library
    add_library(polycall_core_shared SHARED ${CORE_SOURCES})
    target_include_directories(polycall_core_shared
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(polycall_core_shared
        PRIVATE
            Threads::Threads
            ${CMAKE_DL_LIBS}
    )
    
    set_target_properties(polycall_core_shared PROPERTIES
        OUTPUT_NAME "polycall_core"
        VERSION ${POLYCALL_CORE_VERSION}
        SOVERSION 0
    )
    
    # Create alias for unified targeting
    add_library(polycall_core ALIAS polycall_core_static)
    
else()
    message(WARNING "No core sources found, creating empty library")
    
    # Create minimal library to satisfy dependencies
    add_library(polycall_core INTERFACE)
    target_include_directories(polycall_core
        INTERFACE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Installation
if(TARGET polycall_core_static)
    install(TARGETS polycall_core_static polycall_core_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# Component summary
message(STATUS "Core component configuration complete:")
message(STATUS "  Components processed: ${CORE_COMPONENTS}")
message(STATUS "  Total sources: ${list_length:CORE_SOURCES}")
message(STATUS "  Total headers: ${list_length:CORE_HEADERS}")
