# Core library source files
set(CORE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/polycall.c
	${CMAKE_CURRENT_SOURCE_DIR}/polycall/polycall_config.c
	${CMAKE_CURRENT_SOURCE_DIR}/polycall/polycall_context.c
	${CMAKE_CURRENT_SOURCE_DIR}/polycall/polycall_error.c
	${CMAKE_CURRENT_SOURCE_DIR}/polycall/polycall_memory.c
)
# Create development workflow target
add_custom_target(dev_workflow
  DEPENDS validate-includes fix-includes standardize-includes build test:all
  COMMENT "Running complete development workflow"
)
# Create library from core sources
add_library(polycall_core_lib STATIC ${CORE_SOURCES})

# Set include directories
target_include_directories(polycall_core_lib
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../../../include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add subdirectories if they need their own build definitions
if(ENABLE_AUTH)
	add_subdirectory(auth)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config)
	add_subdirectory(config)
endif()

if(ENABLE_EDGE)
	add_subdirectory(edge)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ffi AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ffi)
	add_subdirectory(ffi)
endif()

if(ENABLE_MICRO)
	add_subdirectory(micro)
endif()

if(ENABLE_NETWORK)
	add_subdirectory(network)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/protocol AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protocol)
	add_subdirectory(protocol)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/telemetry AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/telemetry)
	add_subdirectory(telemetry)
endif()
# CMakeLists.txt for LibPolyCall core directory
# Coordinates the build of all core components

# Get the list of all core components
get_property(CORE_COMPONENTS GLOBAL PROPERTY POLYCALL_REGISTERED_COMPONENTS)

# Add each component subdirectory if it exists
foreach(COMPONENT ${CORE_COMPONENTS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${COMPONENT}")
        add_subdirectory(${COMPONENT})
    endif()
endforeach()

# Target for all core components
add_custom_target(core_components ALL
    DEPENDS ${CORE_COMPONENTS}
    COMMENT "Building all core components"
)

# Create directories for object files
foreach(COMPONENT ${CORE_COMPONENTS})
    polycall_ensure_directory(${POLYCALL_OBJ_DIR}/core/${COMPONENT})
endforeach()

# Generate component dependencies graph
if(BUILD_DOCUMENTATION)
    # Create a dot file showing component dependencies
    set(DEPENDENCIES_DOT_FILE "${CMAKE_BINARY_DIR}/docs/component_dependencies.dot")
    
    file(WRITE ${DEPENDENCIES_DOT_FILE} "digraph ComponentDependencies {\n")
    file(APPEND ${DEPENDENCIES_DOT_FILE} "  rankdir=LR;\n")
    file(APPEND ${DEPENDENCIES_DOT_FILE} "  node [shape=box, style=filled, fillcolor=lightblue];\n")
    
    foreach(COMPONENT ${CORE_COMPONENTS})
        # Get component dependencies if they exist
        get_property(COMPONENT_DEPS TARGET polycall_${COMPONENT} 
                               PROPERTY INTERFACE_LINK_LIBRARIES)
                               
        if(COMPONENT_DEPS)
            foreach(DEP ${COMPONENT_DEPS})
                if(DEP MATCHES "^polycall_.*")
                    string(REGEX REPLACE "^polycall_(.*)$" "\\1" DEP_NAME ${DEP})
                    file(APPEND ${DEPENDENCIES_DOT_FILE} "  \"${COMPONENT}\" -> \"${DEP_NAME}\";\n")
                endif()
            endforeach()
        endif()
    endforeach()
    
    file(APPEND ${DEPENDENCIES_DOT_FILE} "}\n")
    
    # If Graphviz is available, generate a PNG from the dot file
    find_program(DOT_EXECUTABLE dot)
    if(DOT_EXECUTABLE)
        add_custom_command(
            OUTPUT "${CMAKE_BINARY_DIR}/docs/component_dependencies.png"
            COMMAND ${DOT_EXECUTABLE} -Tpng 
                    -o "${CMAKE_BINARY_DIR}/docs/component_dependencies.png" 
                    "${DEPENDENCIES_DOT_FILE}"
            DEPENDS "${DEPENDENCIES_DOT_FILE}"
            COMMENT "Generating component dependencies graph"
        )
        
        add_custom_target(component_graph
            DEPENDS "${CMAKE_BINARY_DIR}/docs/component_dependencies.png"
        )
        
        add_dependencies(core_components component_graph)
    endif()
endif()
# Core components

# Collect all core component sources
file(GLOB_RECURSE POLYCALL_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Exclude specific files if needed
list(FILTER POLYCALL_CORE_SOURCES EXCLUDE REGEX ".*test\\.c$")

# Create the core library
polycall_add_component(
    NAME polycall_core
    TYPE SHARED
    SOURCES ${POLYCALL_CORE_SOURCES}
    DEPS 
    LIBS Threads::Threads
)

# Handle subdirectories with specific components
# Add them manually if they require special handling
add_subdirectory(polycall)
add_subdirectory(protocol)
add_subdirectory(config)
add_subdirectory(network)
add_subdirectory(ffi)
add_subdirectory(auth)
add_subdirectory(telemetry)
add_subdirectory(accessibility)
add_subdirectory(micro)
add_subdirectory(edge)
add_subdirectory(socket)
add_subdirectory(repl)
add_subdirectory(polycallfile)
add_subdirectory(polycallrc)