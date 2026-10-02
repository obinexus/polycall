# External consumer of an INSTALLED polycall: nothing from the source or
# build tree is visible here -- only CMAKE_PREFIX_PATH=<install prefix>.
cmake_minimum_required(VERSION 3.20)
project(polycall_consumer LANGUAGES C CXX)

find_package(polycall 1.1 REQUIRED COMPONENTS shared static)

# C, shared (Windows: polycall.lib import library -> polycall.dll at run time)
add_executable(consumer_c_shared consumer.c)
target_link_libraries(consumer_c_shared PRIVATE polycall::polycall)

# C, static (Windows MSVC: polycall_static.lib; GNU: libpolycall.a)
add_executable(consumer_c_static consumer.c)
target_link_libraries(consumer_c_static PRIVATE polycall::polycall_static)

# C++ against the C ABI, shared
add_executable(consumer_cxx consumer.cpp)
set_target_properties(consumer_cxx PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(consumer_cxx PRIVATE polycall::polycall)

# the deprecated forwarding header still compiles (with a notice)
add_executable(consumer_compat compat.c)
target_link_libraries(consumer_compat PRIVATE polycall::polycall_static)

if(WIN32)
  # make the DLL loadable next to the executables (the installed one, not
  # a build-tree copy)
  add_custom_command(TARGET consumer_c_shared POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:polycall::polycall_shared> $<TARGET_FILE_DIR:consumer_c_shared>)
endif()
