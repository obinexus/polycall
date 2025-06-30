# PolycallRC Module CMakeLists.txt

# Collect sources for the polycallrc module
set(POLYCALLRC_SOURCES
	binding_config.c
	ignore/polycallrc_ignore.c
)

# Create object library for polycallrc
add_library(polycall_rc OBJECT ${POLYCALLRC_SOURCES})

# Add include directories
target_include_directories(polycall_rc PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/include/polycall/core/config/polycallrc
)
