# Template CMakeLists.txt for LibPolyCall components
# Replace @COMPONENT_NAME@ with the actual component name

# Get component name from directory
get_filename_component(COMPONENT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Gather all source files in this component directory
file(GLOB COMPONENT_SOURCES "*.c")
file(GLOB COMPONENT_HEADERS "*.h")

# Check for subdirectories with additional sources
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")
foreach(SUBDIR ${SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        # Find sources in subdirectory
        file(GLOB SUBDIR_SOURCES "${SUBDIR}/*.c")
        list(APPEND COMPONENT_SOURCES ${SUBDIR_SOURCES})
        
        file(GLOB SUBDIR_HEADERS "${SUBDIR}/*.h")
        list(APPEND COMPONENT_HEADERS ${SUBDIR_HEADERS})
        
        # Add subdirectory for nested building if it has a CMakeLists.txt
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/CMakeLists.txt")
            add_subdirectory(${SUBDIR})
        endif()
    endif()
endforeach()

# Define component library
polycall_define_component_library(
    COMPONENT "${COMPONENT_NAME}"
    STATIC
    SOURCES ${COMPONENT_SOURCES}
    DEPENDENCIES pthread
)

# Also define shared library if requested
if(BUILD_SHARED_LIBS)
    polycall_define_component_library(
        COMPONENT "${COMPONENT_NAME}"
        SHARED
        SOURCES ${COMPONENT_SOURCES}
        DEPENDENCIES pthread
    )
endif()

# Set include directories
target_include_directories(polycall_${COMPONENT_NAME}_static PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(BUILD_SHARED_LIBS)
    target_include_directories(polycall_${COMPONENT_NAME}_shared PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Add tests if testing is enabled
if(BUILD_TESTING)
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/unit/core/${COMPONENT_NAME}")
        polycall_setup_component_test_dir(${COMPONENT_NAME})
    endif()
endif()

# Component installation
install(
    TARGETS polycall_${COMPONENT_NAME}_static
    EXPORT polycall-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT devel
)

if(BUILD_SHARED_LIBS)
    install(
        TARGETS polycall_${COMPONENT_NAME}_shared
        EXPORT polycall-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT runtime
    )
endif()

# Install headers
install(
    FILES ${COMPONENT_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/polycall/core/${COMPONENT_NAME}
    COMPONENT devel
)

# Add documentation if enabled
if(BUILD_DOCUMENTATION)
    file(GLOB DOC_FILES "*.md" "*.dox")
    if(DOC_FILES)
        install(
            FILES ${DOC_FILES}
            DESTINATION ${CMAKE_INSTALL_DOCDIR}/core/${COMPONENT_NAME}
            COMPONENT doc
        )
    endif()
endif()