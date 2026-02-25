# NDCAFMakerDeps.cmake
#
# Discovers all external dependencies and creates deps::* INTERFACE IMPORTED
# targets. Included by the root CMakeLists.txt via include(NDCAFMakerDeps).
#
# All packages are located through environment variables set by ndcaf_setup.sh.
# No system paths are searched except for curl (system library).

include(FindUPSPackage)

# ROOT — has native CMake support via ROOTConfig.cmake
find_package(ROOT REQUIRED)

# Capture root-config --glibs exactly as the Makefile does, including
# -lMathMore -lGeom -lEGPythia6 -lGenVector which have no CMake targets.
execute_process(
  COMMAND root-config --glibs
  OUTPUT_VARIABLE _root_glibs
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Append the extra flags that the Makefile adds explicitly after --glibs
string(APPEND _root_glibs " -lMathMore -lGeom -lEGPythia6 -lGenVector")

execute_process(
  COMMAND root-config --incdir
  OUTPUT_VARIABLE _root_incdir
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_library(deps::root_glibs INTERFACE IMPORTED)
set_target_properties(deps::root_glibs PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_root_incdir}"
  INTERFACE_LINK_LIBRARIES      "${_root_glibs}"
)

# GENIE — locate genie-config, then capture --libs flags
find_program(GENIE_CONFIG genie-config
  HINTS "$ENV{GENIE_INC}/../bin"
  REQUIRED
)

execute_process(
  COMMAND ${GENIE_CONFIG} --libs
  OUTPUT_VARIABLE _genie_libs
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_library(deps::genie INTERFACE IMPORTED)
set_target_properties(deps::genie PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{GENIE_INC}/GENIE"
  INTERFACE_LINK_LIBRARIES      "${_genie_libs}"
)

# HDF5 — library name differs between debug and prof flavours
if("$ENV{HDF5_FQ_DIR}" MATCHES "debug")
  set(_hdf5_lib hdf5_cpp_debug)
else()
  set(_hdf5_lib hdf5_cpp)
endif()

find_ups_package(
  TARGET_NAME deps::hdf5
  INC_VAR     HDF5_INC
  LIB_VAR     HDF5_LIB
  LIBS        ${_hdf5_lib}
  REQUIRED
)

# Standard UPS packages via find_ups_package
find_ups_package(
  TARGET_NAME deps::log4cpp
  INC_VAR     LOG4CPP_INC
  LIB_VAR     LOG4CPP_LIB
  LIBS        log4cpp
  REQUIRED
)

find_ups_package(
  TARGET_NAME deps::edepsim
  INC_VAR     EDEPSIM_INC
  INC_SUFFIX  EDepSim
  LIB_VAR     EDEPSIM_LIB
  LIBS        edepsim
  REQUIRED
)

find_ups_package(
  TARGET_NAME deps::boost_po
  INC_VAR     BOOST_INC
  LIB_VAR     BOOST_LIB
  LIBS        boost_program_options
  REQUIRED
)

find_ups_package(
  TARGET_NAME deps::fhiclcpp
  INC_VAR     FHICLCPP_INC
  LIB_VAR     FHICLCPP_LIB
  LIBS        fhiclcpp fhiclcpp_types
  REQUIRED
)

find_ups_package(
  TARGET_NAME deps::duneanaobj
  INC_VAR     DUNEANAOBJ_INC
  LIB_VAR     DUNEANAOBJ_LIB
  LIBS        duneanaobj_StandardRecord duneanaobj_StandardRecordFlat
  REQUIRED
)

# Header-only packages (no LIB_VAR / LIBS)
find_ups_package(
  TARGET_NAME deps::nlohmann
  INC_VAR     NLOHMANN_JSON_INC
)

find_ups_package(
  TARGET_NAME deps::srproxy
  INC_VAR     SRPROXY_INC
)

# cetlib — two separate env var pairs merged into one target
find_library(CETLIB_LIB_PATH
  NAMES cetlib
  PATHS "$ENV{CETLIB_LIB}"
  NO_DEFAULT_PATH
)

find_library(CETLIB_EXCEPT_LIB_PATH
  NAMES cetlib_except
  PATHS "$ENV{CETLIB_EXCEPT_LIB}"
  NO_DEFAULT_PATH
)

add_library(deps::cetlib INTERFACE IMPORTED)
set_target_properties(deps::cetlib PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{CETLIB_INC};$ENV{CETLIB_EXCEPT_INC}"
  INTERFACE_LINK_LIBRARIES      "${CETLIB_LIB_PATH};${CETLIB_EXCEPT_LIB_PATH}"
)

# Lib-only packages (no include dir needed)
find_ups_package(
  TARGET_NAME deps::tbb
  LIB_VAR     TBB_LIB
  LIBS        tbb
  REQUIRED
)

# libxml2: env var points to the product root, lib subdir is fixed
find_library(LIBXML2_LIB_PATH
  NAMES xml2
  PATHS "$ENV{LIBXML2_FQ_DIR}/lib"
  NO_DEFAULT_PATH
)
add_library(deps::libxml2 INTERFACE IMPORTED)
set_target_properties(deps::libxml2 PROPERTIES
  INTERFACE_LINK_LIBRARIES "${LIBXML2_LIB_PATH}"
)

# Pythia6: UPS exposes PYTHIA6_LIBRARY pointing directly to the lib dir
find_library(PYTHIA6_LIB_PATH
  NAMES Pythia6
  PATHS "$ENV{PYTHIA6_LIBRARY}"
  NO_DEFAULT_PATH
)
add_library(deps::pythia6 INTERFACE IMPORTED)
set_target_properties(deps::pythia6 PROPERTIES
  INTERFACE_LINK_LIBRARIES "${PYTHIA6_LIB_PATH}"
)

find_ups_package(
  TARGET_NAME deps::gsl
  LIB_VAR     GSL_LIB
  LIBS        gsl gslcblas
  REQUIRED
)

find_ups_package(
  TARGET_NAME deps::lhapdf
  LIB_VAR     LHAPDF_LIB
  LIBS        LHAPDF
  REQUIRED
)

# curl — system library
find_library(CURL_LIB_PATH
  NAMES curl
  PATHS /usr/lib64
  NO_DEFAULT_PATH
)
add_library(deps::curl INTERFACE IMPORTED)
set_target_properties(deps::curl PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include/curl"
  INTERFACE_LINK_LIBRARIES      "${CURL_LIB_PATH}"
)

# SANDReco — auto-detected exactly as in src/Makefile
if(DEFINED ENV{SANDRECO_INC} OR DEFINED ENV{SANDRECO_LIB})
  set(SAND_ENABLED TRUE)
  find_library(SANDRECO_LIB_PATH
    NAMES Struct
    PATHS "$ENV{SANDRECO_LIB}"
    NO_DEFAULT_PATH
  )
  add_library(deps::sand INTERFACE IMPORTED)
  set_target_properties(deps::sand PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$ENV{SANDRECO_INC}"
    INTERFACE_LINK_LIBRARIES      "${SANDRECO_LIB_PATH}"
  )
  message(STATUS "SANDReco: enabled (SANDRECO_INC=$ENV{SANDRECO_INC})")
else()
  set(SAND_ENABLED FALSE)
  # No-op target so src/CMakeLists.txt can link deps::sand unconditionally
  add_library(deps::sand INTERFACE IMPORTED)
  message(STATUS "SANDReco: disabled (SANDRECO_INC/LIB not set)")
endif()
