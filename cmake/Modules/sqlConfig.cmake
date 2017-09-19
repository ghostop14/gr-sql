INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SQL sql)

FIND_PATH(
    SQL_INCLUDE_DIRS
    NAMES sql/api.h
    HINTS $ENV{SQL_DIR}/include
        ${PC_SQL_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SQL_LIBRARIES
    NAMES gnuradio-sql
    HINTS $ENV{SQL_DIR}/lib
        ${PC_SQL_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQL DEFAULT_MSG SQL_LIBRARIES SQL_INCLUDE_DIRS)
MARK_AS_ADVANCED(SQL_LIBRARIES SQL_INCLUDE_DIRS)

