# Copyright 2011,2012,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

set (CMAKE_CXX_STANDARD 11)

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND sql_sources
    sqlsource_impl.cc
)

set(sql_sources "${sql_sources}" PARENT_SCOPE)
if(NOT sql_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT sql_sources)

add_library(gnuradio-sql SHARED ${sql_sources})
target_link_libraries(gnuradio-sql ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES})
set_target_properties(gnuradio-sql PROPERTIES DEFINE_SYMBOL "gnuradio_sql_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-sql PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-sql RUNTIME_COMPONENT "sql_runtime" DEVEL_COMPONENT "sql_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_sql_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sql.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_sql.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_sqlsource.cc
)

add_executable(test-sql ${test_sql_sources})

target_link_libraries(
  test-sql
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-sql
)

GR_ADD_TEST(test_sql test-sql)

########################################################################
# Build and register grsql commandline
########################################################################
list(APPEND grsql_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/grsql.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlsource_impl.cc
)

add_executable(grsql ${grsql_sources})

target_link_libraries(
  grsql
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  volk
  gnuradio-blocks
)

install(TARGETS grsql DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" RUNTIME)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

