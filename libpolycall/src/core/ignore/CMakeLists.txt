# CMakeLists.txt for PolycallRC ignore system

# Define the target for the polycallrc ignore library
add_library(polycallrc_ignore_core OBJECT
    polycall_ignore.c
)

# Set include directories
target_include_directories(polycallrc_ignore_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/polycall/core/config/polycallrc/ignore
        ${CMAKE_CURRENT_SOURCE_DIR}
)


# Set compilation options
target_compile_options(polycallrc_ignore_core
    PRIVATE
        -Wall
        -Wextra
        -Werror
)

# Ensure we build with the correct C standard
set_target_properties(polycallrc_ignore_core PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Link dependencies if the targets exist
if(TARGET polycall_core AND TARGET polycall_ignore)
    target_link_libraries(polycallrc_ignore_core
        PRIVATE
            polycall_core
            polycall_ignore
    )
endif()

# Make this library available to parent scope
set(POLYCALLRC_IGNORE_LIB polycallrc_ignore_core PARENT_SCOPE)

