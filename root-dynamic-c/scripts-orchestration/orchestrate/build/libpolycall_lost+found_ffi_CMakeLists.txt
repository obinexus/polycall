# FFI Module CMakeLists.txt

# Check for COBOL runtime (GnuCOBOL, Micro Focus, etc.)
option(WITH_COBOL "Build with COBOL support" OFF)
if(WITH_COBOL)
    # Check for GnuCOBOL first (open source)
    find_program(GNUCOBOL_COMPILER "cobc")
    if(GNUCOBOL_COMPILER)
        set(HAVE_GNUCOBOL TRUE)
        include_directories(${GNUCOBOL_INCLUDE_DIRS})
        add_definitions(-DHAVE_GNUCOBOL)
        message(STATUS "GnuCOBOL found at ${GNUCOBOL_COMPILER}")
    else()
        # Check for Micro Focus COBOL
        find_program(MICROFOCUS_COMPILER "cob")
        if(MICROFOCUS_COMPILER)
            set(HAVE_MICROFOCUS_COBOL TRUE)
            include_directories(${MICROFOCUS_INCLUDE_DIRS})
            add_definitions(-DHAVE_MICROFOCUS_COBOL)
            message(STATUS "Micro Focus COBOL found at ${MICROFOCUS_COMPILER}")
        else()
            # Check for IBM Enterprise COBOL (more complex, likely requires manual setting)
            if(IBM_COBOL_PATH)
                set(HAVE_IBM_COBOL TRUE)
                include_directories(${IBM_COBOL_INCLUDE_DIRS})
                add_definitions(-DHAVE_IBM_COBOL)
                message(STATUS "IBM Enterprise COBOL found at ${IBM_COBOL_PATH}")
            else()
                message(WARNING "COBOL support requested but no COBOL compiler found")
                set(WITH_COBOL OFF)
            endif()
        endif()
    endif()
endif()

# Collect all sources
set(FFI_SOURCES
    ffi_core.c
    type_system.c
    memory_bridge.c
    security.c
    performance.c
    protocol_bridge.c
    c_bridge.c
)

# Conditionally add language bridges based on available dependencies
if(HAVE_JVM)
    list(APPEND FFI_SOURCES jvm_bridge.c)
endif()

# Add JavaScript bridge only if the appropriate JS engine (V8, JavaScriptCore, etc.) is available
if(HAVE_JS_ENGINE)
    list(APPEND FFI_SOURCES js_bridge.c)
endif()

# Add Python bridge only if Python dev is available
if(HAVE_PYTHON_DEV)
    list(APPEND FFI_SOURCES python_bridge.c)
endif()

# Add COBOL bridge if a COBOL compiler is available
if(WITH_COBOL AND (HAVE_GNUCOBOL OR HAVE_MICROFOCUS_COBOL OR HAVE_IBM_COBOL))
    list(APPEND FFI_SOURCES cobol_bridge.c)
endif()

# Add the FFI sources to the main library
target_sources(polycall_shared PRIVATE
    ${FFI_SOURCES}
)

if(BUILD_STATIC)
    target_sources(polycall_static PRIVATE
        ${FFI_SOURCES}
    )
endif()

# Add include directories specific to FFI module
target_include_directories(polycall_shared
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/core/ffi
)

if(BUILD_STATIC)
    target_include_directories(polycall_static
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/core/ffi
    )
endif()

# Add dependencies for specific bridges if needed
if(HAVE_JVM)
    target_include_directories(polycall_shared PRIVATE ${JNI_INCLUDE_DIRS})
    if(BUILD_STATIC)
        target_include_directories(polycall_static PRIVATE ${JNI_INCLUDE_DIRS})
    endif()
endif()

if(HAVE_JS_ENGINE)
    # Add JavaScript engine include directories
endif()

if(HAVE_PYTHON_DEV)
    # Add Python include directories
endif()

if(WITH_COBOL)
    if(HAVE_GNUCOBOL)
        # Add GnuCOBOL-specific flags and libraries
        target_link_libraries(polycall_shared PRIVATE ${GNUCOBOL_LIBRARIES})
        if(BUILD_STATIC)
            target_link_libraries(polycall_static PRIVATE ${GNUCOBOL_LIBRARIES})
        endif()
    elseif(HAVE_MICROFOCUS_COBOL)
        # Add Micro Focus COBOL-specific flags and libraries
        target_link_libraries(polycall_shared PRIVATE ${MICROFOCUS_LIBRARIES})
        if(BUILD_STATIC)
            target_link_libraries(polycall_static PRIVATE ${MICROFOCUS_LIBRARIES})
        endif()
    elseif(HAVE_IBM_COBOL)
        # Add IBM Enterprise COBOL-specific flags and libraries
        target_link_libraries(polycall_shared PRIVATE ${IBM_COBOL_LIBRARIES})
        if(BUILD_STATIC)
            target_link_libraries(polycall_static PRIVATE ${IBM_COBOL_LIBRARIES})
        endif()
    endif()
endif()