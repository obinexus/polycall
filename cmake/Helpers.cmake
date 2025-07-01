# LibPolyCall CMake Helpers
# Common utility functions for build configuration

function(polycall_add_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE polycall)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

function(polycall_add_component name)
    add_library(polycall_${name} STATIC ${ARGN})
    target_include_directories(polycall_${name}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_SOURCE_DIR}/libpolycall/include
    )
    set_target_properties(polycall_${name} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endfunction()
