# LibPolyCall Examples

add_executable(example_basic basic.c)
target_link_libraries(example_basic PRIVATE polycall)

add_executable(example_micro micro_example.c)
target_link_libraries(example_micro PRIVATE polycall)

# Install examples
install(TARGETS example_basic example_micro
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)
