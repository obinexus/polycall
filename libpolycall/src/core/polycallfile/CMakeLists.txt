# CMakeLists.txt for the Polycallfile component

# Define the library target with explicit sources
add_library(polycallfile
    ${CMAKE_CURRENT_SOURCE_DIR}/ast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/expression_evaluator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/macro_expander.c
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/token.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/global_config.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ignore/polycallfile_ignore.c
)

# Expose public include directory and private module path
target_include_directories(polycallfile
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include            # Root include path
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/polycall/core/config/polycallfile  # Module headers
)

# Set C standard and warnings
set_target_properties(polycallfile PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
target_compile_options(polycallfile
    PRIVATE
        -Wall
        -Wextra
        -Werror
)

# Add include-path validation for this module
add_custom_target(validate_polycallfile
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/validate_includes.py
            --root ${PROJECT_SOURCE_DIR}
            --standard polycall
            --source-dir ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Validating include paths for polycallfile"
)

# Only add dependency if the global validation target exists
if(TARGET validate-includes)
    add_dependencies(polycallfile validate-includes)
endif()

# Add dependency on include validation for this target
add_dependencies(polycallfile validate_polycallfile)

# Link against core and math libraries
target_link_libraries(polycallfile
    PRIVATE
        m               # math
        polycall_core   # core framework
)

# Ensure unified-header generation runs after building this module
if(TARGET unified_header)
    add_dependencies(unified_header polycallfile)
endif()
