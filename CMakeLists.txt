cmake_minimum_required(VERSION 3.20)

project(polycall
  VERSION 1.0.1
  DESCRIPTION "Program-first cross-language runtime"
  LANGUAGES C)

# This CMake build is independent of the GNU Make build. It never invokes the
# root Makefile, and `make` never invokes CMake.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# GCC/Clang: gnu11, so POSIX APIs the code needs (usleep, ssize_t, nanosleep,
# poll, posix_spawn, ...) are visible without scattering feature-test macros.
# MSVC: strict /std:c11 (it has no "gnu" dialect; ssize_t is shimmed in
# network.h). This is the one justified use of compiler extensions.
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_C_EXTENSIONS OFF)
else()
  set(CMAKE_C_EXTENSIONS ON)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Keep artefacts where tests/run_all.sh expects them: <build>/bin and <build>/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------- sources
set(POLYCALL_CORE_SOURCES
  src/network.c
  src/polycall.c
  src/polycall_config.c
  src/polycall_micro.c
  src/polycall_parser.c
  src/polycall_protocol.c
  src/polycall_state_machine.c
  src/polycall_token.c
  src/polycall_tokenizer.c
  src/config/config2.c
  src/config/json.c
  src/config/legacy_import.c
  src/config/provider_host.c
  src/runtime/rpc_wire.c
  src/runtime/runtime.c)

set(POLYCALL_CLI_SOURCES
  src/cli/cli.c
  src/cli/cli_commands.c
  src/cli/cli_json.c
  src/cli/cmd_config2.c
  src/cli/cmd_runtime.c
  src/cli/repl.c)

set(POLYCALL_PUBLIC_HEADERS
  include/polycall.h
  include/polycall_export.h
  include/polycall_cli.h
  include/polycall_config2.h
  include/polycall_provider.h
  include/polycall_runtime.h)

# Parity guard: the Make build compiles the same core list. Keep in sync.
file(GLOB _disk_core RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.c" "src/config/*.c" "src/runtime/*.c")
list(REMOVE_ITEM _disk_core "src/main.c")
list(SORT _disk_core)
set(_want_core ${POLYCALL_CORE_SOURCES})
list(SORT _want_core)
if(NOT "${_disk_core}" STREQUAL "${_want_core}")
  message(WARNING
    "core source list drift:\n  on disk : ${_disk_core}\n  in build: ${_want_core}")
endif()

# ---------------------------------------------------------------- platform deps
set(POLYCALL_PLATFORM_LIBS "")
if(WIN32)
  list(APPEND POLYCALL_PLATFORM_LIBS ws2_32)
else()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  list(APPEND POLYCALL_PLATFORM_LIBS Threads::Threads)
  list(APPEND POLYCALL_PLATFORM_LIBS ${CMAKE_DL_LIBS})
endif()

if(WIN32)
  set(_os_defs _WIN32 __USE_MINGW_ANSI_STDIO=1)
else()
  set(_os_defs "")
endif()

# ---------------------------------------------------------------- static lib
add_library(polycall_static STATIC ${POLYCALL_CORE_SOURCES})
add_library(polycall::static ALIAS polycall_static)
target_include_directories(polycall_static
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/polycall>)
target_compile_definitions(polycall_static PRIVATE ${_os_defs})
set_target_properties(polycall_static PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  # avoid a name clash with the shared target's import library
  set_target_properties(polycall_static PROPERTIES OUTPUT_NAME polycall-static)
else()
  set_target_properties(polycall_static PROPERTIES OUTPUT_NAME polycall)
endif()
target_link_libraries(polycall_static PUBLIC ${POLYCALL_PLATFORM_LIBS})

# ---------------------------------------------------------------- shared lib
add_library(polycall_shared SHARED ${POLYCALL_CORE_SOURCES})
add_library(polycall::shared ALIAS polycall_shared)
target_include_directories(polycall_shared
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/polycall>)
target_compile_definitions(polycall_shared
  PRIVATE ${_os_defs} POLYCALL_BUILD_SHARED
  INTERFACE POLYCALL_USE_SHARED)
set_target_properties(polycall_shared PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  WINDOWS_EXPORT_ALL_SYMBOLS OFF
  OUTPUT_NAME polycall
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  # keep the loadable module beside the import lib so both build systems and
  # every consumer look in one place (<build>/lib)
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_link_libraries(polycall_shared PUBLIC ${POLYCALL_PLATFORM_LIBS})

# ---------------------------------------------------------------- CLI
add_executable(polycall_cli src/main.c ${POLYCALL_CLI_SOURCES})
set_target_properties(polycall_cli PROPERTIES OUTPUT_NAME polycall)
if(MSVC)
  # match the ~8 MiB main-thread stack GCC/Clang give by default, so the
  # legacy loader's large aggregate stack locals are safe.
  target_link_options(polycall_cli PRIVATE /STACK:8388608)
endif()
target_compile_definitions(polycall_cli PRIVATE
  ${_os_defs}
  POLYCALL_BUILD_SYSTEM="cmake"
  POLYCALL_BUILD_TYPE="$<LOWER_CASE:$<CONFIG>>"
  POLYCALL_BUILD_CC="${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
  POLYCALL_BUILD_TARGET="${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME}"
  POLYCALL_BUILD_CFLAGS="$<CONFIG>")
target_link_libraries(polycall_cli PRIVATE polycall_static)

# ---------------------------------------------------------------- example C provider
# A loadable module exporting polycall_config_provider_v1; self-contained
# (statically links the builder objects). Loaded by `config validate
# --provider c:<path>` via polycall_provider_load_c().
add_library(example_c_provider MODULE bindings/c-provider/example_provider.c)
target_link_libraries(example_c_provider PRIVATE polycall_static)
target_compile_definitions(example_c_provider PRIVATE ${_os_defs})
set_target_properties(example_c_provider PROPERTIES
  OUTPUT_NAME example_provider
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------- fixture operation plugin
# `polycall run --load`. Same source built twice: normal (matches the
# runtime's ABI major) and deliberately mismatched, to exercise both paths.
add_library(fixture_ops_plugin MODULE tests/fixtures/plugins/fixture_ops_plugin.c)
target_link_libraries(fixture_ops_plugin PRIVATE polycall_static)
target_compile_definitions(fixture_ops_plugin PRIVATE ${_os_defs})
set_target_properties(fixture_ops_plugin PROPERTIES
  OUTPUT_NAME fixture_ops_plugin
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_library(fixture_ops_plugin_bad_abi MODULE tests/fixtures/plugins/fixture_ops_plugin.c)
target_link_libraries(fixture_ops_plugin_bad_abi PRIVATE polycall_static)
target_compile_definitions(fixture_ops_plugin_bad_abi PRIVATE ${_os_defs} FIXTURE_ABI_MAJOR=999)
set_target_properties(fixture_ops_plugin_bad_abi PROPERTIES
  OUTPUT_NAME fixture_ops_plugin_bad_abi
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------- tests
include(CTest)
if(BUILD_TESTING)
  if(WIN32)
    set(_is_win 1)
    set(_exe ".exe")
    set(_shext "dll")
  elseif(APPLE)
    set(_is_win 0)
    set(_exe "")
    set(_shext "dylib")
  else()
    set(_is_win 0)
    set(_exe "")
    set(_shext "so")
  endif()

  find_program(SH_EXECUTABLE NAMES sh bash)
  if(SH_EXECUTABLE)
    add_test(NAME polycall_suite
      COMMAND ${SH_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all.sh"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(polycall_suite PROPERTIES
      ENVIRONMENT "BUILD_DIR=${CMAKE_BINARY_DIR};CC=${CMAKE_C_COMPILER};EXE_EXT=${_exe};SHARED_EXT=${_shext};IS_WINDOWS=${_is_win}"
      FAIL_REGULAR_EXPRESSION "FAIL |TESTS FAILED")
  else()
    message(STATUS "sh not found: the polycall_suite shell test will be skipped by ctest")
  endif()
endif()

# ---------------------------------------------------------------- install
install(TARGETS polycall_static polycall_shared polycall_cli
  EXPORT polycallTargets
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${POLYCALL_PUBLIC_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/polycall)

install(EXPORT polycallTargets
  FILE polycallTargets.cmake
  NAMESPACE polycall::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/polycall)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/polycallConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/polycallConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/polycall)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/polycallConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/polycallConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/polycallConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/polycall)

export(EXPORT polycallTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/polycallTargets.cmake"
  NAMESPACE polycall::)
