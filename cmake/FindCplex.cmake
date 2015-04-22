# This module finds cplex.
#
# User can give CPLEX_ROOT_DIR as a hint stored in the cmake cache.
#
# It sets the following variables:
#  CPLEX_FOUND              - Set to false, or undefined, if cplex isn't found.
#  CPLEX_INCLUDE_DIRS       - include directory
#  CPLEX_LIBRARIES          - library files

message(STATUS "Cplex root dir: ${CPLEX_ROOT_DIR}")

find_path(CPLEX_INCLUDE_DIR
  ilcplex/cplex.h
  HINTS ${CPLEX_ROOT_DIR}/cplex/include
        ${CPLEX_ROOT_DIR}/include
  PATHS ENV C_INCLUDE_PATH
        ENV C_PLUS_INCLUDE_PATH
        ENV INCLUDE_PATH
)
message(STATUS "CPLEX Include: ${CPLEX_INCLUDE_DIR}")

find_path(CPLEX_CONCERT_INCLUDE_DIR
  ilconcert/iloenv.h
  HINTS ${CPLEX_ROOT_DIR}/concert/include
        ${CPLEX_ROOT_DIR}/include
  PATHS ENV C_INCLUDE_PATH
        ENV C_PLUS_INCLUDE_PATH
        ENV INCLUDE_PATH
)
message(STATUS "Concert Inlude: ${CPLEX_CONCERT_INCLUDE_DIR}")

find_library(CPLEX_LIBRARY
  NAMES cplex${CPLEX_WIN_VERSION} cplex
  HINTS ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic #linux
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_osx/static_pic #osx
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_darwin/static_pic #osx
  PATHS ENV LIBRARY_PATH #unix
        ENV LD_LIBRARY_PATH #unix
)
message(STATUS "CPLEX Library: ${CPLEX_LIBRARY}")

find_library(CPLEX_ILOCPLEX_LIBRARY
  ilocplex
  HINTS ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic #linux
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_osx/static_pic #osx
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_darwin/static_pic #osx
  PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
)
message(STATUS "ILOCPLEX Library: ${CPLEX_ILOCPLEX_LIBRARY}")

find_library(CPLEX_CONCERT_LIBRARY
  concert
  HINTS ${CPLEX_ROOT_DIR}/concert/lib/x86-64_linux/static_pic #linux
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_osx/static_pic #osx
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_darwin/static_pic #osx
  PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
)
message(STATUS "Concert Library: ${CPLEX_CONCERT_LIBRARY}")

find_path(CPLEX_BIN_DIR
  cplex
      HINTS ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_linux #linux
            ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_osx #osx
        ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_darwin #osx
  ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH
)
message(STATUS "CPLEX Bin Dir: ${CPLEX_BIN_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CPLEX DEFAULT_MSG
 CPLEX_LIBRARY CPLEX_INCLUDE_DIR CPLEX_ILOCPLEX_LIBRARY CPLEX_CONCERT_LIBRARY CPLEX_CONCERT_INCLUDE_DIR)

if(CPLEX_FOUND)
  set(CPLEX_INCLUDE_DIRS ${CPLEX_INCLUDE_DIR} ${CPLEX_CONCERT_INCLUDE_DIR})
  set(CPLEX_LIBRARIES ${CPLEX_CONCERT_LIBRARY} ${CPLEX_ILOCPLEX_LIBRARY} ${CPLEX_LIBRARY} )
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CPLEX_LIBRARIES "${CPLEX_LIBRARIES};m;pthread")
  endif()
endif()

mark_as_advanced(CPLEX_LIBRARY CPLEX_INCLUDE_DIR CPLEX_ILOCPLEX_LIBRARY CPLEX_CONCERT_INCLUDE_DIR CPLEX_CONCERT_LIBRARY)