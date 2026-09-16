# CMake generated Testfile for 
# Source directory: C:/Users/Nnamdi/Projects/polycall
# Build directory: C:/Users/Nnamdi/Projects/polycall
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test("polycall_suite" "C:/Users/Nnamdi/AppData/Local/Microsoft/WindowsApps/bash.exe" "C:/Users/Nnamdi/Projects/polycall/tests/run_all.sh")
  set_tests_properties("polycall_suite" PROPERTIES  ENVIRONMENT "BUILD_DIR=C:/Users/Nnamdi/Projects/polycall;CC=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe;EXE_EXT=.exe;SHARED_EXT=dll;IS_WINDOWS=1" FAIL_REGULAR_EXPRESSION "FAIL |TESTS FAILED" WORKING_DIRECTORY "C:/Users/Nnamdi/Projects/polycall" _BACKTRACE_TRIPLES "C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;238;add_test;C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test("polycall_suite" "C:/Users/Nnamdi/AppData/Local/Microsoft/WindowsApps/bash.exe" "C:/Users/Nnamdi/Projects/polycall/tests/run_all.sh")
  set_tests_properties("polycall_suite" PROPERTIES  ENVIRONMENT "BUILD_DIR=C:/Users/Nnamdi/Projects/polycall;CC=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe;EXE_EXT=.exe;SHARED_EXT=dll;IS_WINDOWS=1" FAIL_REGULAR_EXPRESSION "FAIL |TESTS FAILED" WORKING_DIRECTORY "C:/Users/Nnamdi/Projects/polycall" _BACKTRACE_TRIPLES "C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;238;add_test;C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test("polycall_suite" "C:/Users/Nnamdi/AppData/Local/Microsoft/WindowsApps/bash.exe" "C:/Users/Nnamdi/Projects/polycall/tests/run_all.sh")
  set_tests_properties("polycall_suite" PROPERTIES  ENVIRONMENT "BUILD_DIR=C:/Users/Nnamdi/Projects/polycall;CC=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe;EXE_EXT=.exe;SHARED_EXT=dll;IS_WINDOWS=1" FAIL_REGULAR_EXPRESSION "FAIL |TESTS FAILED" WORKING_DIRECTORY "C:/Users/Nnamdi/Projects/polycall" _BACKTRACE_TRIPLES "C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;238;add_test;C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test("polycall_suite" "C:/Users/Nnamdi/AppData/Local/Microsoft/WindowsApps/bash.exe" "C:/Users/Nnamdi/Projects/polycall/tests/run_all.sh")
  set_tests_properties("polycall_suite" PROPERTIES  ENVIRONMENT "BUILD_DIR=C:/Users/Nnamdi/Projects/polycall;CC=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe;EXE_EXT=.exe;SHARED_EXT=dll;IS_WINDOWS=1" FAIL_REGULAR_EXPRESSION "FAIL |TESTS FAILED" WORKING_DIRECTORY "C:/Users/Nnamdi/Projects/polycall" _BACKTRACE_TRIPLES "C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;238;add_test;C:/Users/Nnamdi/Projects/polycall/CMakeLists.txt;0;")
else()
  add_test("polycall_suite" NOT_AVAILABLE)
endif()
