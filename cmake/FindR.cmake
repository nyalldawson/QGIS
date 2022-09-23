# CMake module to search for R
#
# Once done this will define
#
#  R_FOUND - system has the R library
#  R_INCLUDE_DIR - the R library include directories
#  R_LIB - the R library
#
# Copyright (c) 2022, Nyall Dawson, <nyall dot dawson at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

execute_process(COMMAND R CMD config --cppflags OUTPUT_VARIABLE R_INCLUDE_DIR_TMP)
string(REGEX REPLACE "^-I" "" R_INCLUDE_DIR_TMP "${R_INCLUDE_DIR_TMP}")
string(STRIP ${R_INCLUDE_DIR_TMP} R_INCLUDE_DIR_TMP)
set(R_INCLUDE_DIR "${R_INCLUDE_DIR_TMP}" CACHE STRING INTERNAL)

#message(STATUS "Found R include dirs: ${R_INCLUDE_DIR}")

execute_process(COMMAND R CMD config --ldflags OUTPUT_VARIABLE R_LDFLAGS)
if (${R_LDFLAGS} MATCHES "[-][L]([^ ;])+")
    string(SUBSTRING ${CMAKE_MATCH_0} 2 -1 R_LIB_DIR)
    string(STRIP ${R_LIB_DIR} R_LIB_DIR)
    find_library(R_LIB
      NAMES libR.so PATHS
      "${R_LIB_DIR}"
    )
endif()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(R DEFAULT_MSG
                                  R_LIB R_INCLUDE_DIR)

if(R_FOUND)
  message(STATUS "Found R library: ${R_LIB}")
endif()

mark_as_advanced(R_INCLUDE_DIR)
mark_as_advanced(R_LIB)
