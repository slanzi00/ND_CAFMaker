cmake_minimum_required(VERSION 3.20)
project(ND_CAFMaker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(ENABLE_TMS     "Enable TMS reconstruction branch filler" ON)
option(ENABLE_TESTEXE "Build the testHDF test executable"       OFF)

if(NOT ENABLE_TMS)
  add_compile_definitions(DISABLE_TMS)
endif()

# Global compile flags — replicate Makefile CXXFLAGS
add_compile_options(-g -Wall -fPIC -O2)
add_compile_definitions(NO_ART)

# Dependency discovery
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(NDCAFMakerDeps)

add_subdirectory(src)
