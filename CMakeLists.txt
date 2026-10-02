cmake_minimum_required(VERSION 3.20)

# One version source: read POLYCALL_ABI_VERSION_{MAJOR,MINOR,PATCH} out of
# the public header instead of a second hardcoded literal here.
# src/core/status.c does the equivalent for polycall_ffi_version().
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/polycall_export.h"
  _pc_ver_lines REGEX "^#define POLYCALL_ABI_VERSION_(MAJOR|MINOR|PATCH) ")
foreach(_line IN LISTS _pc_ver_lines)
  if(_line MATCHES "^#define POLYCALL_ABI_VERSION_MAJOR ([0-9]+)")
    set(_pc_ver_major "${CMAKE_MATCH_1}")
  elseif(_line MATCHES "^#define POLYCALL_ABI_VERSION_MINOR ([0-9]+)")
    set(_pc_ver_minor "${CMAKE_MATCH_1}")
  elseif(_line MATCHES "^#define POLYCALL_ABI_VERSION_PATCH ([0-9]+)")
    set(_pc_ver_patch "${CMAKE_MATCH_1}")
  endif()
endforeach()
if(NOT _pc_ver_major OR NOT DEFINED _pc_ver_minor OR NOT DEFINED _pc_ver_patch)
  message(FATAL_ERROR "could not read POLYCALL_ABI_VERSION_* from include/polycall_export.h")
endif()

project(polycall
  VERSION "${_pc_ver_major}.${_pc_ver_minor}.${_pc_ver_patch}"
  DESCRIPTION "Program-first cross-language runtime"
  HOMEPAGE_URL "https://github.com/obinexus/polycall"
  LANGUAGES C)

# Out-of-source builds only: an in-source configure once committed
# CMakeCache.txt / *.vcxproj with absolute build-machine paths (460b2b7).
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported. Use a separate build directory:\n"
    "  cmake -S . -B build/<name> [--preset <preset>]\n"
    "and remove the CMakeCache.txt / CMakeFiles just created in the source tree.")
endif()

# This CMake build is independent of the GNU Make build. It never invokes the
# root Makefile, and `make` never invokes CMake.

# ---------------------------------------------------------------- options
option(POLYCALL_BUILD_SHARED "Build the shared library (polycall.dll / libpolycall.so)" ON)
option(POLYCALL_BUILD_STATIC "Build the static library (polycall_static.lib / libpolycall.a)" ON)
option(POLYCALL_BUILD_CLI "Build the polycall command-line program" ON)
option(POLYCALL_INSTALL "Generate install rules (headers, libraries, CMake + pkg-config metadata)" ON)
option(POLYCALL_INSTALL_SYSTEMD_UNIT "Install the polycall.service systemd unit (Linux packaging)" OFF)
option(POLYCALL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
set(POLYCALL_SANITIZERS "" CACHE STRING
  "GCC/Clang sanitizers for the whole build, e.g. 'address;undefined' or 'thread'")

include(CTest)   # defines BUILD_TESTING (default ON)

# Supported combinations are validated here; anything else fails at
# configure time with a reason instead of silently building less.
if(NOT POLYCALL_BUILD_SHARED AND NOT POLYCALL_BUILD_STATIC)
  message(FATAL_ERROR
    "POLYCALL_BUILD_SHARED and POLYCALL_BUILD_STATIC are both OFF: nothing to build.")
endif()
if(BUILD_TESTING AND NOT (POLYCALL_BUILD_SHARED AND POLYCALL_BUILD_STATIC AND POLYCALL_BUILD_CLI))
  message(FATAL_ERROR
    "BUILD_TESTING=ON exercises the CLI and both libraries (static, import-lib "
    "and dynamically-loaded consumers). Either keep POLYCALL_BUILD_SHARED, "
    "POLYCALL_BUILD_STATIC and POLYCALL_BUILD_CLI ON, or configure with "
    "-DBUILD_TESTING=OFF.")
endif()
if(POLYCALL_SANITIZERS AND MSVC)
  if(NOT POLYCALL_SANITIZERS STREQUAL "address")
    message(FATAL_ERROR "MSVC supports only POLYCALL_SANITIZERS=address")
  endif()
endif()
if(POLYCALL_SANITIZERS MATCHES "thread" AND POLYCALL_SANITIZERS MATCHES "address")
  message(FATAL_ERROR "ThreadSanitizer cannot be combined with AddressSanitizer")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# GCC/Clang: gnu11, so POSIX APIs the code needs (nanosleep, poll,
# clock_gettime, ...) are visible without scattering feature-test macros.
# MSVC: strict /std:c11 (it has no "gnu" dialect).
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_C_EXTENSIONS OFF)
else()
  set(CMAKE_C_EXTENSIONS ON)
endif()

get_property(_pc_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT CMAKE_BUILD_TYPE AND NOT _pc_multi)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CheckCCompilerFlag)

# One output layout for single- AND multi-config generators (Visual Studio
# would otherwise add bin/Release/, which the test suite does not expect):
# <build>/bin for executables, <build>/lib for libraries and modules.
foreach(_cfg "" ${CMAKE_CONFIGURATION_TYPES})
  if(_cfg)
    string(TOUPPER "_${_cfg}" _sfx)
  else()
    set(_sfx "")
  endif()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY${_sfx} "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY${_sfx} "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY${_sfx} "${CMAKE_BINARY_DIR}/lib")
endforeach()

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
  src/core/ffi.c
  src/core/pc_buf.c
  src/core/pc_sys.c
  src/core/status.c
  src/peer/http.c
  src/peer/peer.c
  src/runtime/rpc_wire.c
  src/runtime/runtime.c
  src/runtime/telemetry.c)

set(POLYCALL_CLI_SOURCES
  src/cli/cli.c
  src/cli/cli_commands.c
  src/cli/cli_json.c
  src/cli/cli_proc.c
  src/cli/cmd_config2.c
  src/cli/cmd_daemon.c
  src/cli/cmd_peer.c
  src/cli/cmd_runtime.c
  src/cli/cmd_telemetry.c
  src/cli/repl.c)

set(POLYCALL_PUBLIC_HEADERS
  include/polycall.h
  include/polycall_export.h
  include/polycall_cli.h
  include/polycall_config2.h
  include/polycall_provider.h
  include/polycall_runtime.h
  include/polycall_telemetry.h
  include/libpolycall.h)

# Parity guard: the Make build compiles the same core list. Keep in sync.
file(GLOB _disk_core RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "src/*.c" "src/config/*.c" "src/core/*.c" "src/peer/*.c" "src/runtime/*.c")
list(REMOVE_ITEM _disk_core "src/main.c")
list(SORT _disk_core)
set(_want_core ${POLYCALL_CORE_SOURCES})
list(SORT _want_core)
if(NOT "${_disk_core}" STREQUAL "${_want_core}")
  message(FATAL_ERROR
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
  # floor/fabs/isfinite (JSON numbers): an explicit libm dependency, not one
  # that only works when an optimised build happens to inline them
  find_library(POLYCALL_LIBM m)
  if(POLYCALL_LIBM)
    list(APPEND POLYCALL_PLATFORM_LIBS m)
  endif()
endif()

set(_os_defs "")
if(WIN32)
  # Vista+ APIs (InitOnce, condition variables, GetTickCount64,
  # PROC_THREAD_ATTRIBUTE_HANDLE_LIST); the floor we support is Windows 7.
  list(APPEND _os_defs _WIN32_WINNT=0x0601 WINVER=0x0601)
  if(MSVC)
    # portable C uses fopen/getenv/strerror; MSVC's *_s deprecation noise
    # (C4996) would bury the conversion warnings that matter
    list(APPEND _os_defs _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
  endif()
  if(MINGW)
    list(APPEND _os_defs __USE_MINGW_ANSI_STDIO=1)
  endif()
endif()

# ---------------------------------------------------------------- flags
set(_pc_warn "")
if(MSVC)
  list(APPEND _pc_warn /W3)
  if(POLYCALL_WARNINGS_AS_ERRORS)
    list(APPEND _pc_warn /WX)
  endif()
else()
  list(APPEND _pc_warn -Wall -Wextra)
  if(POLYCALL_WARNINGS_AS_ERRORS)
    list(APPEND _pc_warn -Werror)
  endif()
endif()

# No build-machine paths in artifacts: __FILE__ (assert messages) and debug
# info refer to source files relative to the project, never to this checkout.
set(_pc_prefix_map "")
if(NOT MSVC)
  check_c_compiler_flag("-ffile-prefix-map=a=b" _pc_has_file_prefix_map)
  if(_pc_has_file_prefix_map)
    list(APPEND _pc_prefix_map
      # no trailing slashes: the DWARF compilation directory is recorded
      # without one, and must be mapped too (GCC: the last match wins, so
      # the build tree -- possibly nested in the source tree -- comes last)
      "-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=polycall-src"
      "-ffile-prefix-map=${CMAKE_BINARY_DIR}=polycall-build")
  else()
    check_c_compiler_flag("-fmacro-prefix-map=a=b" _pc_has_macro_prefix_map)
    if(_pc_has_macro_prefix_map)
      list(APPEND _pc_prefix_map "-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=polycall-src"
                                 "-fdebug-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=polycall-src"
                                 "-fdebug-prefix-map=${CMAKE_BINARY_DIR}=polycall-build")
    endif()
  endif()
else()
  # Every MSVC object records its own absolute output path (CodeView
  # S_OBJNAME), and debug info records source paths: map both trees to
  # neutral names. MSVC Release links embed no PDB path; keep that explicit
  # for RelWithDebInfo too.
  file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" _pc_src_native)
  file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}" _pc_bin_native)
  # /pathmap alone does not cover the object-directory record; together with
  # /experimental:deterministic it does (verified: no build path survives in
  # polycall_static.lib), and the objects become bit-reproducible.
  list(APPEND _pc_prefix_map "/pathmap:${_pc_src_native}=polycall-src"
                             "/pathmap:${_pc_bin_native}=polycall-build"
                             "/experimental:deterministic")
  add_link_options($<$<CONFIG:RelWithDebInfo>:/PDBALTPATH:%_PDB%>)
endif()

if(POLYCALL_SANITIZERS)
  if(MSVC)
    add_compile_options(/fsanitize=address /Zi)
  else()
    string(REPLACE ";" "," _pc_san "${POLYCALL_SANITIZERS}")
    add_compile_options(-fsanitize=${_pc_san} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${_pc_san})
  endif()
endif()

function(polycall_common target)
  target_compile_options(${target} PRIVATE ${_pc_warn} ${_pc_prefix_map})
  target_compile_definitions(${target} PRIVATE ${_os_defs})
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endfunction()

set(_pc_lib_targets "")

# ---------------------------------------------------------------- static lib
if(POLYCALL_BUILD_STATIC)
  add_library(polycall_static STATIC ${POLYCALL_CORE_SOURCES})
  add_library(polycall::static ALIAS polycall_static)
  add_library(polycall::polycall_static ALIAS polycall_static)
  polycall_common(polycall_static)
  target_include_directories(polycall_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/polycall>)
  set_target_properties(polycall_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if(MSVC)
    # distinct from the shared target's import library polycall.lib
    set_target_properties(polycall_static PROPERTIES OUTPUT_NAME polycall_static)
    # no absolute build paths as archive member names (cmake/relative_archive.cmake)
    add_custom_command(TARGET polycall_static POST_BUILD
      COMMAND ${CMAKE_COMMAND} "-DAR=${CMAKE_AR}" "-DOUT=$<TARGET_FILE:polycall_static>"
              "-DOBJS=$<JOIN:$<TARGET_OBJECTS:polycall_static>,|>"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/relative_archive.cmake"
      VERBATIM)
  else()
    # libpolycall.a; distinct from libpolycall.so / libpolycall.dll.a
    set_target_properties(polycall_static PROPERTIES OUTPUT_NAME polycall)
  endif()
  target_link_libraries(polycall_static PUBLIC ${POLYCALL_PLATFORM_LIBS})
  list(APPEND _pc_lib_targets polycall_static)
endif()

# ---------------------------------------------------------------- shared lib
if(POLYCALL_BUILD_SHARED)
  add_library(polycall_shared SHARED ${POLYCALL_CORE_SOURCES})
  add_library(polycall::shared ALIAS polycall_shared)
  add_library(polycall::polycall_shared ALIAS polycall_shared)
  polycall_common(polycall_shared)
  target_include_directories(polycall_shared
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/polycall>)
  target_compile_definitions(polycall_shared
    PRIVATE POLYCALL_BUILD_SHARED
    INTERFACE POLYCALL_USE_SHARED)
  set_target_properties(polycall_shared PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
    OUTPUT_NAME polycall
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # every dependency must be explicit; hardened relocations
    target_link_options(polycall_shared PRIVATE
      -Wl,--no-undefined -Wl,-z,relro -Wl,-z,now)
  endif()
  if(APPLE)
    set_target_properties(polycall_shared PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  target_link_libraries(polycall_shared PUBLIC ${POLYCALL_PLATFORM_LIBS})
  list(APPEND _pc_lib_targets polycall_shared)
endif()

# ---------------------------------------------------------------- CLI
if(POLYCALL_BUILD_CLI)
  add_executable(polycall_cli src/main.c ${POLYCALL_CLI_SOURCES})
  polycall_common(polycall_cli)
  set_target_properties(polycall_cli PROPERTIES OUTPUT_NAME polycall)
  if(WIN32)
    # UTF-8 argv (CommandLineToArgvW) + an activeCodePage=UTF-8 manifest
    target_link_libraries(polycall_cli PRIVATE shell32)
    if(MSVC)
      target_sources(polycall_cli PRIVATE src/polycall.manifest)
    else()
      enable_language(RC)
      target_sources(polycall_cli PRIVATE src/polycall.rc)
    endif()
  endif()
  if(MSVC)
    # match the ~8 MiB main-thread stack GCC/Clang give by default, so the
    # legacy loader's large aggregate stack locals are safe.
    target_link_options(polycall_cli PRIVATE /STACK:8388608)
  endif()
  target_compile_definitions(polycall_cli PRIVATE
    POLYCALL_BUILD_SYSTEM="cmake"
    POLYCALL_BUILD_TYPE="$<LOWER_CASE:$<CONFIG>>"
    POLYCALL_BUILD_CC="${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
    POLYCALL_BUILD_TARGET="${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME}"
    POLYCALL_BUILD_CFLAGS="$<CONFIG>")
  # A self-contained tool: the static core when built, else the shared one.
  if(POLYCALL_BUILD_STATIC)
    target_link_libraries(polycall_cli PRIVATE polycall_static)
  else()
    target_link_libraries(polycall_cli PRIVATE polycall_shared)
    target_compile_definitions(polycall_cli PRIVATE POLYCALL_USE_SHARED)
    if(UNIX AND NOT APPLE)
      set_target_properties(polycall_cli PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    elseif(APPLE)
      set_target_properties(polycall_cli PROPERTIES INSTALL_RPATH "@executable_path/../${CMAKE_INSTALL_LIBDIR}")
    endif()
  endif()
endif()

# ---------------------------------------------------------------- test fixtures
if(BUILD_TESTING)
  # A loadable module exporting polycall_config_provider_v1; self-contained
  # (statically links the builder objects). Loaded by `config validate
  # --provider c:<path>` via polycall_provider_load_c().
  add_library(example_c_provider MODULE tests/fixtures/providers/c-provider/example_provider.c)
  # `polycall run --load`: the normal build and a deliberately mismatched
  # ABI build from the same source, to exercise both loader paths.
  add_library(fixture_ops_plugin MODULE tests/fixtures/plugins/fixture_ops_plugin.c)
  add_library(fixture_ops_plugin_bad_abi MODULE tests/fixtures/plugins/fixture_ops_plugin.c)
  target_compile_definitions(fixture_ops_plugin_bad_abi PRIVATE FIXTURE_ABI_MAJOR=999)
  # flagship demo (P3): ledger.balance / ledger.transfer via `--load`
  add_library(ledger_plugin MODULE examples/ledger/ledger_plugin.c)
  foreach(_m example_c_provider fixture_ops_plugin fixture_ops_plugin_bad_abi ledger_plugin)
    polycall_common(${_m})
    target_link_libraries(${_m} PRIVATE polycall_static)
    set_target_properties(${_m} PROPERTIES PREFIX "")
  endforeach()
  set_target_properties(example_c_provider PROPERTIES OUTPUT_NAME example_provider)

  # binding-ABI unit tests: null pointers, invalid/stale handles, double
  # close, calls after close, buffer limits, cancellation, concurrency
  add_executable(polycall_ffi_unit tests/unit/ffi_unit.c)
  polycall_common(polycall_ffi_unit)
  target_link_libraries(polycall_ffi_unit PRIVATE polycall_shared)
  # bounded, deterministic fuzzing of every parser that sees untrusted bytes
  add_executable(polycall_fuzz tests/unit/fuzz_parsers.c)
  polycall_common(polycall_fuzz)
  target_link_libraries(polycall_fuzz PRIVATE polycall_static)
  # The consumer programs and the config2 model test, built with this
  # toolchain (the shell suite only runs them): static, import-library and
  # dlopen C consumers, the deprecated-header consumer, and -- when a C++
  # compiler is available -- a C++ consumer of the C ABI.
  add_executable(model_test tests/config/model_test.c)
  target_link_libraries(model_test PRIVATE polycall_static)
  add_executable(static_consumer tests/consumers/static_consumer.c)
  target_link_libraries(static_consumer PRIVATE polycall_static)
  add_executable(link_shared_consumer tests/consumers/link_shared_consumer.c)
  target_link_libraries(link_shared_consumer PRIVATE polycall_shared)
  add_executable(dlopen_consumer tests/consumers/dlopen_consumer.c)
  target_link_libraries(dlopen_consumer PRIVATE ${CMAKE_DL_LIBS})
  target_include_directories(dlopen_consumer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  add_executable(compat_consumer tests/consumers/compat_consumer.c)
  target_link_libraries(compat_consumer PRIVATE polycall_static)
  foreach(_c model_test static_consumer link_shared_consumer dlopen_consumer compat_consumer)
    target_compile_definitions(${_c} PRIVATE ${_os_defs})
  endforeach()
  # The C++ consumer proves polycall.h compiles and links as C++. It needs a
  # C++ compiler; turn it off explicitly (the consumers suite then reports
  # the C++ check as NOT RUN) rather than having it vanish silently.
  option(POLYCALL_TEST_CXX "Build the C++ consumer test (needs a C++ compiler)" ON)
  if(POLYCALL_TEST_CXX)
    enable_language(CXX)
    add_executable(cxx_consumer tests/consumers/cxx_consumer.cpp)
    set_target_properties(cxx_consumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    target_link_libraries(cxx_consumer PRIVATE polycall_shared)
  endif()

  # TCP fault-injection proxy for the peer partition/delay/reset tests
  add_executable(polycall_fault_proxy tests/peer/fault_proxy.c)
  polycall_common(polycall_fault_proxy)
  target_link_libraries(polycall_fault_proxy PRIVATE polycall_static)

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
  set(_pc_env
    "BUILD_DIR=${CMAKE_BINARY_DIR}"
    "CC=${CMAKE_C_COMPILER}"
    "EXE_EXT=${_exe}"
    "SHARED_EXT=${_shext}"
    "IS_WINDOWS=${_is_win}")

  add_test(NAME polycall_ffi_unit COMMAND polycall_ffi_unit)
  add_test(NAME polycall_fuzz COMMAND polycall_fuzz 20000)
  set_tests_properties(polycall_ffi_unit polycall_fuzz PROPERTIES TIMEOUT 300)

  if(SH_EXECUTABLE)
    # One ctest entry per suite. A suite exits 77 when any of its checks
    # could not run (missing interpreter/toolchain), which ctest reports as
    # "Skipped" -- never as "Passed".
    set(_suites
      "cli_contract|tests/run_all.sh|cli"
      "config_model|tests/run_all.sh|model"
      "provider_equivalence|tests/run_all.sh|equivalence"
      "runtime_roundtrip|tests/run_all.sh|roundtrip"
      "plugins|tests/run_all.sh|plugins"
      "conformance_c_node_python|tests/run_all.sh|conformance:core"
      "conformance_go|tests/run_all.sh|conformance:go"
      "conformance_java|tests/run_all.sh|conformance:java"
      "microvm|tests/run_all.sh|microvm"
      "consumers_build_tree|tests/run_all.sh|consumers"
      "daemon|tests/run_all.sh|daemon"
      "peer_p2p|tests/run_all.sh|peer"
      "nsigii_two_process|tests/run_all.sh|nsigii")
    foreach(_s IN LISTS _suites)
      string(REPLACE "|" ";" _parts "${_s}")
      list(GET _parts 0 _name)
      list(GET _parts 1 _script)
      list(GET _parts 2 _section)
      add_test(NAME polycall_${_name}
        COMMAND ${SH_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/${_script}" --only "${_section}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set_tests_properties(polycall_${_name} PROPERTIES
        ENVIRONMENT "${_pc_env}"
        SKIP_RETURN_CODE 77
        TIMEOUT 600
        FAIL_REGULAR_EXPRESSION "(^|\n)FAIL ")
    endforeach()
    if(POLYCALL_INSTALL AND NOT POLYCALL_SANITIZERS)
      # (not in sanitizer builds: an instrumented library cannot be linked by
      # an uninstrumented external consumer -- that is not a product defect)
      # install -> relocate -> build external C/C++ consumers from the
      # installed files only (CMake package + pkg-config), run them
      add_test(NAME polycall_install_consumer
        COMMAND ${SH_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/install/consumer.sh"
                "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" "$<CONFIG>")
      set_tests_properties(polycall_install_consumer PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 900 FAIL_REGULAR_EXPRESSION "(^|\n)FAIL ")
    endif()
    if(POLYCALL_BUILD_SHARED AND UNIX AND NOT APPLE)
      add_test(NAME polycall_abi_exports
        COMMAND ${SH_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/abi/check_exports.sh"
                "$<TARGET_FILE:polycall_shared>" "${CMAKE_CURRENT_SOURCE_DIR}/abi/polycall.symbols")
      set_tests_properties(polycall_abi_exports PROPERTIES SKIP_RETURN_CODE 77)
    endif()
  else()
    message(WARNING "sh not found: the shell test suites cannot run; ctest will "
                    "only run the C unit tests")
    # visible in every ctest summary ("tests did not run ... (Disabled)"),
    # so the missing suites can never read as a full green run
    add_test(NAME polycall_shell_suites_not_run COMMAND ${CMAKE_COMMAND} -E true)
    set_tests_properties(polycall_shell_suites_not_run PROPERTIES DISABLED TRUE)
  endif()
endif()

# ---------------------------------------------------------------- install
if(POLYCALL_INSTALL)
  set(_pc_install_targets ${_pc_lib_targets})
  if(POLYCALL_BUILD_CLI)
    list(APPEND _pc_install_targets polycall_cli)
  endif()
  install(TARGETS ${_pc_install_targets}
    EXPORT polycallTargets
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_COMPONENT devel
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

  # pkg-config: relocatable (prefix derived from the .pc file's location)
  file(RELATIVE_PATH _pc_rel_prefix
    "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig" "${CMAKE_INSTALL_PREFIX}")
  string(REGEX REPLACE "/$" "" _pc_rel_prefix "${_pc_rel_prefix}")
  if(WIN32)
    set(PC_LIBS_PRIVATE "-lws2_32")
  else()
    set(PC_LIBS_PRIVATE "-lpthread")
    if(CMAKE_DL_LIBS)
      string(APPEND PC_LIBS_PRIVATE " -l${CMAKE_DL_LIBS}")
    endif()
    if(POLYCALL_LIBM)
      string(APPEND PC_LIBS_PRIVATE " -lm")
    endif()
  endif()
  if(MSVC AND NOT POLYCALL_BUILD_SHARED)
    set(PC_LIB_NAME "polycall_static")
  else()
    set(PC_LIB_NAME "polycall")
  endif()
  set(PC_REL_PREFIX "${_pc_rel_prefix}")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/polycall.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/polycall.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/polycall.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  if(POLYCALL_INSTALL_SYSTEMD_UNIT)
    set(POLYCALL_SYSTEMD_UNIT_DIR "lib/systemd/system" CACHE PATH
      "where polycall.service is installed (relative to the prefix)")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/systemd/polycall.service.in"
      "${CMAKE_CURRENT_BINARY_DIR}/polycall.service" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/polycall.service"
      DESTINATION ${POLYCALL_SYSTEMD_UNIT_DIR})
  endif()
endif()

if(POLYCALL_BUILD_STATIC OR POLYCALL_BUILD_SHARED)
  export(TARGETS ${_pc_lib_targets}
    NAMESPACE polycall::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/polycallTargets-build.cmake")
endif()
