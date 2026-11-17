
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was DesignarConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# Designar links its own compile-option/warning interface targets
# PUBLICly (see the root CMakeLists.txt comment near
# designar-warning-interface/designar-compile-option-interface for why:
# changing that to PRIVATE would silently drop -Wall/-Werror coverage
# from every test/sample built inside this repo's own CMake project).
# A consequence of that choice a downstream consumer should know about:
# linking against Designar::Designar via find_package(Designar) pulls in
# this project's own warning flags (and, if the exporting build enabled
# them, hardening/sanitizer flags) into the *consumer's* build too. This
# is a deliberate, if opinionated, choice for a teaching-oriented
# library — flag it in your own CMakeLists.txt if you'd rather not
# inherit it, e.g. by not depending on the exported interface targets
# directly.
include("${CMAKE_CURRENT_LIST_DIR}/DesignarTargets.cmake")

check_required_components(Designar)
