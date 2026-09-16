#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "polycall::polycall_static" for configuration "MinSizeRel"
set_property(TARGET polycall::polycall_static APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(polycall::polycall_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/polycall-static.lib"
  )

list(APPEND _cmake_import_check_targets polycall::polycall_static )
list(APPEND _cmake_import_check_files_for_polycall::polycall_static "${_IMPORT_PREFIX}/lib/polycall-static.lib" )

# Import target "polycall::polycall_shared" for configuration "MinSizeRel"
set_property(TARGET polycall::polycall_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(polycall::polycall_shared PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/polycall.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/polycall.dll"
  )

list(APPEND _cmake_import_check_targets polycall::polycall_shared )
list(APPEND _cmake_import_check_files_for_polycall::polycall_shared "${_IMPORT_PREFIX}/lib/polycall.lib" "${_IMPORT_PREFIX}/bin/polycall.dll" )

# Import target "polycall::polycall_cli" for configuration "MinSizeRel"
set_property(TARGET polycall::polycall_cli APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(polycall::polycall_cli PROPERTIES
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/polycall.exe"
  )

list(APPEND _cmake_import_check_targets polycall::polycall_cli )
list(APPEND _cmake_import_check_files_for_polycall::polycall_cli "${_IMPORT_PREFIX}/bin/polycall.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
