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

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

find_package(Gnuradio REQUIRED COMPONENTS runtime filter blocks fft) #lo anhadi, busca los componentes de gnuradio y en las lineas de abajo los incluyo, ahi los boost de c++ viene incluidos ya de dentro de gnuradio, por eso mas abajo los comente, seria redundadnte

include_directories(${GNURADIO_ALL_INCLUDE_DIRS})   #lo anhadi, para incluir lo que busque arriba
link_directories(${GNURADIO_ALL_LIBRARY_DIRS})       #lo anhadi, para incluir lo que busque arriba 

#include_directories(${Boost_INCLUDE_DIR})
#link_directories(${Boost_LIBRARY_DIRS})

list(APPEND howto_sources
    square_ff_impl.cc
    gain_ff_impl.cc
    moving_avg_ff_impl.cc
    moving_avg_history_ff_impl.cc
    iq_mag_cf_impl.cc
    iq_select_cf_impl.cc
    flex_fir_ff_impl.cc
    flex_fir_cc_impl.cc
    flex_fir_cf_impl.cc
    flex_fir_all.cc
    downsample_cc_impl.cc
    decimate_fir_cc_impl.cc
    dual_decimate_ff_impl.cc )

set(howto_sources "${howto_sources}" PARENT_SCOPE)
if(NOT howto_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT howto_sources)


add_library(gnuradio-howto SHARED ${howto_sources})

#target_link_libraries(gnuradio-howto ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${GNURADIO_FILTER_LIBRARIES})  por alguna razon aqui no esta incluyendo las librerias de filder para filtrado de gnuradio, tal ves esa variable (GNURADIO_FILTER_LIBRARIES) no este definida en GNURadio 3.7.11

target_link_libraries(gnuradio-howto gnuradio-runtime gnuradio-filter gnuradio-blocks gnuradio-fft ${Boost_LIBRARIES})  #lo anhadi para que agarre los filtros de gnuradio filder

set_target_properties(gnuradio-howto PROPERTIES DEFINE_SYMBOL "gnuradio_howto_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-howto PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-howto RUNTIME_COMPONENT "howto_runtime" DEVEL_COMPONENT "howto_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_howto_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_howto.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_howto.cc
)

add_executable(test-howto ${test_howto_sources})

target_link_libraries(
  test-howto
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-howto
)

GR_ADD_TEST(test_howto test-howto)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

