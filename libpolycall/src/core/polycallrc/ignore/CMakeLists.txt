# CMakeLists.txt for PolycallRC ignore system

# Define the target for the polycallrc ignore library
add_library(polycallrc_ignore OBJECT
    polycallrc_ignore.c
)

# Set include directories
target_include_directories(polycallrc_ignore
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include/polycall/core/config/polycallrc/ignore
)

# Set compilation options
target_compile_options(polycallrc_ignore
    PRIVATE
        -Wall
        -Wextra
        -Werror
)

# Ensure we build with the correct C standard
set_target_properties(polycallrc_ignore PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Link dependencies if the targets exist
if(TARGET polycall_core AND TARGET polycall_ignore)
    target_link_libraries(polycallrc_ignore
        PRIVATE
            polycall_core
            polycall_ignore
    )
endif()

# Make this library available to parent scope
set(POLYCALLRC_IGNORE_LIB polycallrc_ignore PARENT_SCOPE)