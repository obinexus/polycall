# Schema component CMakeLists.txt

# Collect source files
set(SCHEMA_SOURCES
	config_schema.c
	schema_security_integration.c
)

# Create static library
add_library(polycall_schema STATIC ${SCHEMA_SOURCES})

# Set include directories
target_include_directories(polycall_schema
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/src
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

# Install rules
install(TARGETS polycall_schema
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

install(FILES
	${CMAKE_SOURCE_DIR}/include/polycall/core/config/schema/config_schema.h
	DESTINATION include/polycall/core/config/schema
)
