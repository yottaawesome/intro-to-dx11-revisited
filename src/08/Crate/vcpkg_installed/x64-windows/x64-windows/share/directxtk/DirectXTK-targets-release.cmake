#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Microsoft::DirectXTK" for configuration "Release"
set_property(TARGET Microsoft::DirectXTK APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Microsoft::DirectXTK PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/DirectXTK.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/DirectXTK.dll"
  )

list(APPEND _cmake_import_check_targets Microsoft::DirectXTK )
list(APPEND _cmake_import_check_files_for_Microsoft::DirectXTK "${_IMPORT_PREFIX}/lib/DirectXTK.lib" "${_IMPORT_PREFIX}/bin/DirectXTK.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
