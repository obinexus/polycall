# CMakeLists.txt for LibPolyCall examples
cmake_minimum_required(VERSION 3.13)

project(LibPolyCallExamples C)

# Find LibPolyCall package
find_package(polycall 1.1 REQUIRED)

# List of example applications
set(EXAMPLE_APPLICATIONS
    basic_example
    network_example
    auth_example
    protocol_example
)

# Build each example
foreach(EXAMPLE ${EXAMPLE_APPLICATIONS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}.c")
        add_executable(${EXAMPLE} ${EXAMPLE}.c)
        target_link_libraries(${EXAMPLE} PRIVATE polycall::polycall)
        
        # Set C standard
        set_target_properties(${EXAMPLE} PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
        )
    endif()
endforeach()

# Add standalone example that shows how to compile and link manually
add_custom_target(standalone_example
    COMMAND ${CMAKE_C_COMPILER} -o standalone_example ${CMAKE_CURRENT_SOURCE_DIR}/basic_example.c -lpolycall
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building standalone example with manual linking"
    VERBATIM
)

# Add test target that runs the basic example
add_custom_target(run_example
    COMMAND ./basic_example
    DEPENDS basic_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running basic LibPolyCall example"
)