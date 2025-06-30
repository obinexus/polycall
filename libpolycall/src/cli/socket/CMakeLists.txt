# Socket CLI module CMakeLists.txt

# Set component name
set(COMPONENT_NAME socket)

# Source files
set(COMPONENT_SOURCES
    socket_commands.c
)

# Create component library
add_library(cli_${COMPONENT_NAME} STATIC ${COMPONENT_SOURCES})

# Add include directories
target_include_directories(cli_${COMPONENT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(cli_${COMPONENT_NAME}
    polycall_core
    polycall_socket
    polycall_auth
    cli_common
)

# Install component library
install(TARGETS cli_${COMPONENT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
