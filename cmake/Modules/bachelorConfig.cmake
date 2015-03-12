INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_BACHELOR bachelor)

FIND_PATH(
    BACHELOR_INCLUDE_DIRS
    NAMES bachelor/api.h
    HINTS $ENV{BACHELOR_DIR}/include
        ${PC_BACHELOR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    BACHELOR_LIBRARIES
    NAMES gnuradio-bachelor
    HINTS $ENV{BACHELOR_DIR}/lib
        ${PC_BACHELOR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BACHELOR DEFAULT_MSG BACHELOR_LIBRARIES BACHELOR_INCLUDE_DIRS)
MARK_AS_ADVANCED(BACHELOR_LIBRARIES BACHELOR_INCLUDE_DIRS)

