# CMakeLists.txt - OBINexus PolyCall with Command Purity Enforcement
cmake_minimum_required(VERSION 3.16)
project(polycall VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable strict compilation flags
add_compile_options(
    -Wall -Wextra -Werror
    -Wno-unused-parameter
    -fvisibility=hidden
    -fstack-protector-strong
)

# ==============================================================================
# Base Infrastructure Library (No Dependencies)
# ==============================================================================
set(BASE_SOURCES
    src/core/base/memory.c
    src/core/base/error.c
    src/core/base/context.c
    src/core/base/type_system.c
)

add_library(polycall_base STATIC ${BASE_SOURCES})
target_include_directories(polycall_base PUBLIC src/core)

# ==============================================================================
# Protocol Layer (Depends on Base Only)
# ==============================================================================
set(PROTOCOL_SOURCES
    src/core/protocol/protocol_bridge.c
    src/core/protocol/protocol_config.c
    src/core/protocol/protocol_container.c
    src/core/protocol/protocol_error.c
    src/core/protocol/protocol_registry.c
    src/core/protocol/protocol_state_machine.c
    src/core/protocol/protocol_enhacements_config.c
)

add_library(polycall_protocol STATIC ${PROTOCOL_SOURCES})
target_link_libraries(polycall_protocol PUBLIC polycall_base)
target_include_directories(polycall_protocol PUBLIC src/core)

# ==============================================================================
# Network Layer (Depends on Base + Protocol)
# ==============================================================================
set(NETWORK_SOURCES
    src/core/network/network.c
    src/core/network/network_client.c
    src/core/network/network_server.c
    src/core/network/network_config.c
    src/core/network/network_container.c
    src/core/network/network_endpoint.c
    src/core/network/network_error.c
    src/core/network/network_packet.c
    src/core/network/network_registry.c
    src/core/network/connection_pool.c
)

add_library(polycall_network STATIC ${NETWORK_SOURCES})
target_link_libraries(polycall_network PUBLIC polycall_base polycall_protocol)

# ==============================================================================
# Authentication Layer (Depends on Base + Protocol + Network)
# ==============================================================================
set(AUTH_SOURCES
    src/core/auth/auth_audit.c
    src/core/auth/auth_config.c
    src/core/auth/auth_container.c
    src/core/auth/auth_context.c
    src/core/auth/auth_error.c
    src/core/auth/auth_identity.c
    src/core/auth/auth_integration.c
    src/core/auth/auth_policy.c
    src/core/auth/auth_registry.c
    src/core/auth/auth_token.c
    src/core/auth/advanced_security.c
)

add_library(polycall_auth STATIC ${AUTH_SOURCES})
target_link_libraries(polycall_auth PUBLIC 
    polycall_base 
    polycall_protocol 
    polycall_network
)

# ==============================================================================
# Command Libraries (Isolated - No Cross-Dependencies)
# ==============================================================================
set(COMMANDS micro telemetry guid edge crypto topo)

foreach(CMD ${COMMANDS})
    file(GLOB CMD_SOURCES src/core/commands/${CMD}/*.c)
    add_library(polycall_cmd_${CMD} STATIC ${CMD_SOURCES})
    
    # Commands can ONLY depend on infrastructure layers
    target_link_libraries(polycall_cmd_${CMD} PUBLIC
        polycall_base
        polycall_protocol
        polycall_network
        polycall_auth
    )
    
    # Enforce isolation: Commands cannot link to other commands
    foreach(OTHER_CMD ${COMMANDS})
        if(NOT ${CMD} STREQUAL ${OTHER_CMD})
            set_target_properties(polycall_cmd_${CMD} PROPERTIES
                LINK_LIBRARIES_CHECK_EXCLUDES "polycall_cmd_${OTHER_CMD}"
            )
        endif()
    endforeach()
endforeach()

# ==============================================================================
# Hot-wire Subsystem
# ==============================================================================
set(HOTWIRE_SOURCES
    src/core/hotwire/hotwire_core.c
    src/core/hotwire/hotwire_adapter.c
    src/core/hotwire/hotwire_routing.c
)

add_library(polycall_hotwire STATIC ${HOTWIRE_SOURCES})
target_link_libraries(polycall_hotwire PUBLIC
    polycall_base
    polycall_network
)

# ==============================================================================
# Language Bridges (Depend on Core Infrastructure Only)
# ==============================================================================
set(BRIDGE_SOURCES
    src/core/bridges/cobol_bridge.c
    src/core/bridges/js_bridge.c
    src/core/bridges/jvm_bridge.c
    src/core/bridges/python_bridge_dop.c
    src/core/bridges/memory_bridge.c
)

add_library(polycall_bridges STATIC ${BRIDGE_SOURCES})
target_link_libraries(polycall_bridges PUBLIC
    polycall_base
    polycall_protocol
    polycall_network
)

# ==============================================================================
# PolyCall Runtime
# ==============================================================================
set(POLYCALL_SOURCES
    src/core/polycall/polycall.c
    src/core/polycall/polycall_binding.c
    src/core/polycall/polycall_config.c
    src/core/polycall/polycall_config_tools.c
    src/core/polycall/polycall_container.c
    src/core/polycall/polycall_error.c
    src/core/polycall/polycall_guid.c
    src/core/polycall/polycall_hierachical_error.c
    src/core/polycall/polycall_protocol_context.c
    src/core/polycall/polycall_registry.c
    src/core/polycall/polycall_token.c
    src/core/polycall/polycall_auth_adapter.c
)

add_library(polycall_runtime STATIC ${POLYCALL_SOURCES})
target_link_libraries(polycall_runtime PUBLIC
    polycall_base
    polycall_protocol
    polycall_network
    polycall_auth
    polycall_hotwire
)

# ==============================================================================
# CLI Executable
# ==============================================================================
add_executable(polycall
    src/cli/main.c
    src/cli/commands/cli_micro.c
    src/cli/commands/cli_telemetry.c
    src/cli/commands/cli_guid.c
    src/cli/commands/cli_edge.c
    src/cli/commands/cli_crypto.c
    src/cli/commands/cli_topo.c
)

# Link all command libraries
foreach(CMD ${COMMANDS})
    target_link_libraries(polycall PRIVATE polycall_cmd_${CMD})
endforeach()

target_link_libraries(polycall PRIVATE
    polycall_runtime
    polycall_bridges
)

# ==============================================================================
# Command Purity Validation Target
# ==============================================================================
add_custom_target(validate_command_purity
    COMMAND ${CMAKE_COMMAND} -E echo "Validating command purity..."
    COMMAND ${CMAKE_SOURCE_DIR}/tools/validate_purity.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking for command cross-dependencies"
)

# ==============================================================================
# Installation Rules
# ==============================================================================
install(TARGETS polycall
    RUNTIME DESTINATION bin
)

install(DIRECTORY src/core/
    DESTINATION include/polycall
    FILES_MATCHING PATTERN "*.h"
)

# ==============================================================================
# Testing Configuration
# ==============================================================================
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# Documentation
# ==============================================================================
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)
    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/src
        COMMENT "Generating API documentation"
    )
endif()
# ==============================================================================
# Include Root CMake Directory
# ==============================================================================
# Use the root-level cmake/ directory for additional CMake modules and helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Helpers OPTIONAL)
include(FindDependencies OPTIONAL)
