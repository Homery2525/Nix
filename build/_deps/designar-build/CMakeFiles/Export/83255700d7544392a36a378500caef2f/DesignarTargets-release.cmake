#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Designar::Designar" for configuration "Release"
set_property(TARGET Designar::Designar APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Designar::Designar PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libDesignar.a"
  )

list(APPEND _cmake_import_check_targets Designar::Designar )
list(APPEND _cmake_import_check_files_for_Designar::Designar "${_IMPORT_PREFIX}/lib/libDesignar.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
