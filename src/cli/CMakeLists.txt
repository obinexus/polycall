# OBINexus Polycall CLI Components CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# CLI component configuration
set(POLYCALL_CLI_VERSION "0.7.0")

# Find CLI source files
file(GLOB_RECURSE CLI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Find CLI header files
file(GLOB_RECURSE CLI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Filter out test files
list(FILTER CLI_SOURCES EXCLUDE REGEX ".*test.*")

# Check for CLI subdirectories
file(GLOB CLI_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")
foreach(SUBDIR ${CLI_SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt)
            message(STATUS "Adding CLI subdirectory: ${SUBDIR}")
            add_subdirectory(${SUBDIR})
        endif()
    endif()
endforeach()

# Create CLI executable if we have sources
if(CLI_SOURCES)
    message(STATUS "Creating polycall CLI executable with ${list_length:CLI_SOURCES} sources")
    
    add_executable(polycall_cli ${CLI_SOURCES})
    
    target_include_directories(polycall_cli
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/src/core
    )
    
    # Link with core library
    target_link_libraries(polycall_cli
        PRIVATE
            polycall_core
            Threads::Threads
            ${CMAKE_DL_LIBS}
    )
    
    set_target_properties(polycall_cli PROPERTIES
        OUTPUT_NAME "polycall"
        VERSION ${POLYCALL_CLI_VERSION}
    )
    
    # Installation
    install(TARGETS polycall_cli
        RUNTIME DESTINATION bin
    )
    
else()
    message(WARNING "No CLI sources found")
    
    # Create a minimal CLI target that just prints version
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/polycall_cli_stub.c
        "#include <stdio.h>\n"
        "int main() {\n"
        "    printf(\"polycall v${POLYCALL_CLI_VERSION}\\n\");\n"
        "    return 0;\n"
        "}\n"
    )
    
    add_executable(polycall_cli ${CMAKE_CURRENT_BINARY_DIR}/polycall_cli_stub.c)
    
    target_link_libraries(polycall_cli
        PRIVATE
            polycall_core
    )
    
    set_target_properties(polycall_cli PROPERTIES
        OUTPUT_NAME "polycall"
    )
    
    install(TARGETS polycall_cli
        RUNTIME DESTINATION bin
    )
    
    message(STATUS "Created stub CLI executable")
endif()

# CLI component summary
message(STATUS "CLI component configuration complete:")
message(STATUS "  Sources found: ${list_length:CLI_SOURCES}")
message(STATUS "  Headers found: ${list_length:CLI_HEADERS}")
