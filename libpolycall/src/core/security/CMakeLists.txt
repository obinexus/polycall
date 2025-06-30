# Security Validation Component CMake configuration

# Add library target for security validation
add_library(polycall_security_validation
	security_validation.c
)

# Set include directories for this component
target_include_directories(polycall_security_validation
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against required dependencies
target_link_libraries(polycall_security_validation
	PRIVATE
		polycall_core
)

# Set compile options
target_compile_options(polycall_security_validation
	PRIVATE
		$<$<C_COMPILER_ID:GNU>:-Wall -Wextra>
		$<$<C_COMPILER_ID:Clang>:-Wall -Wextra>
)

# Install rules
install(TARGETS polycall_security_validation
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
