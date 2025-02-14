#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "clFFT" for configuration "Debug"
set_property(TARGET clFFT APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(clFFT PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libclFFT.2.14.0.dylib"
  IMPORTED_SONAME_DEBUG "libclFFT.2.dylib"
  )

list(APPEND _cmake_import_check_targets clFFT )
list(APPEND _cmake_import_check_files_for_clFFT "${_IMPORT_PREFIX}/lib/libclFFT.2.14.0.dylib" )

# Import target "StatTimer" for configuration "Debug"
set_property(TARGET StatTimer APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(StatTimer PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libStatTimer.2.14.0.dylib"
  IMPORTED_SONAME_DEBUG "libStatTimer.2.dylib"
  )

list(APPEND _cmake_import_check_targets StatTimer )
list(APPEND _cmake_import_check_files_for_StatTimer "${_IMPORT_PREFIX}/lib/libStatTimer.2.14.0.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
