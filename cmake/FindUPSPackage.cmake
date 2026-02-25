# FindUPSPackage.cmake
#
# Reusable helper to create CMake INTERFACE IMPORTED targets from UPS packages
# (DUNE/Fermilab CVMFS). Dependencies are located via environment variables
# set by ndcaf_setup.sh, NOT from standard system paths.
#
# Usage:
#   find_ups_package(
#     TARGET_NAME  <target-name>     # e.g. "deps::hdf5"
#     INC_VAR      <env-var-include> # e.g. "HDF5_INC"
#     [INC_SUFFIX  <subdir>]         # e.g. "EDepSim"
#     LIB_VAR      <env-var-lib>     # e.g. "HDF5_LIB"
#     LIBS         <lib1> [<lib2>…]  # names without "lib" prefix and extension
#     [REQUIRED]
#   )
#
# The target is always created (even for header-only or missing packages) so
# that src/CMakeLists.txt can link against it unconditionally.

function(find_ups_package)

  # Argument parsing
  cmake_parse_arguments(
    UPS                                           # prefix for result variables
    "REQUIRED"                                    # boolean flags (presence = TRUE)
    "TARGET_NAME;INC_VAR;INC_SUFFIX;LIB_VAR"      # single-value arguments
    "LIBS"                                        # list arguments
    ${ARGN}
  )

  # TARGET_NAME is the only truly mandatory argument
  if(NOT UPS_TARGET_NAME)
    message(FATAL_ERROR "find_ups_package: TARGET_NAME is required")
  endif()

  # Include path resolution
  # INC_VAR is optional: header-only or lib-only packages may omit it
  if(UPS_INC_VAR)
    set(_inc_dir "$ENV{${UPS_INC_VAR}}")   # double indirection: var name -> env value
    if(UPS_INC_SUFFIX)
      set(_inc_dir "${_inc_dir}/${UPS_INC_SUFFIX}")
    endif()
  endif()

  # Library search
  # NO_DEFAULT_PATH prevents CMake from picking up libraries from /usr/lib or
  # similar system paths; search is restricted to the UPS env var directory.
  set(_libs "")
  if(UPS_LIB_VAR AND UPS_LIBS)
    foreach(_lib IN LISTS UPS_LIBS)
      find_library(_found_${_lib}
        NAMES  ${_lib}
        PATHS  "$ENV{${UPS_LIB_VAR}}"
        NO_DEFAULT_PATH
      )
      if(_found_${_lib})
        list(APPEND _libs "${_found_${_lib}}")
      elseif(UPS_REQUIRED)
        message(FATAL_ERROR
          "find_ups_package: library '${_lib}' not found in $ENV{${UPS_LIB_VAR}}")
      endif()
    endforeach()
  endif()

  # INTERFACE IMPORTED target creation
  # INTERFACE = no build artifact, only properties propagated to consumers
  # IMPORTED  = the library is pre-built externally (UPS), not by this build
  add_library(${UPS_TARGET_NAME} INTERFACE IMPORTED)

  if(_inc_dir)
    set_target_properties(${UPS_TARGET_NAME} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${_inc_dir}"
    )
  endif()

  if(_libs)
    set_target_properties(${UPS_TARGET_NAME} PROPERTIES
      INTERFACE_LINK_LIBRARIES "${_libs}"
    )
  endif()

endfunction()
