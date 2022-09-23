# CMake module to search for RCpp
#
# Once done this will define
#
#  RCpp_FOUND - system has the RCpp library
#  RCpp_INCLUDE_DIR - the RCpp library include directories
#  RCpp_LIB - the RCpp library
#
# Copyright (c) 2022, Nyall Dawson, <nyall dot dawson at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

execute_process(COMMAND Rscript -e "Rcpp:::CxxFlags()"
                OUTPUT_VARIABLE RCpp_INCLUDE_DIR_TMP)
string(REGEX REPLACE "^-I" "" RCpp_INCLUDE_DIR_TMP "${RCpp_INCLUDE_DIR_TMP}")
string(STRIP ${RCpp_INCLUDE_DIR_TMP} RCpp_INCLUDE_DIR_TMP )
string(REGEX REPLACE "^\"" "" RCpp_INCLUDE_DIR_TMP "${RCpp_INCLUDE_DIR_TMP}")
string(REGEX REPLACE "\"$" "" RCpp_INCLUDE_DIR_TMP "${RCpp_INCLUDE_DIR_TMP}")
set(RCpp_INCLUDE_DIR "${RCpp_INCLUDE_DIR_TMP}" CACHE STRING INTERNAL)

find_library(RCpp_LIB
  NAMES Rcpp.so PATHS
  "${RCpp_INCLUDE_DIR}/../libs"
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RCpp DEFAULT_MSG
                                  RCpp_LIB RCpp_INCLUDE_DIR)

if(RCpp_FOUND)
  message(STATUS "Found Rcpp library: ${RCpp_LIB}")
endif()

add_library(Rcpp UNKNOWN IMPORTED)
set_property(TARGET Rcpp PROPERTY IMPORTED_LOCATION "${RCpp_LIB}")

mark_as_advanced(RCpp_INCLUDE_DIR)
mark_as_advanced(RCpp_LIB)
