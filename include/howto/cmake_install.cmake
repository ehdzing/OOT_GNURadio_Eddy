# Install script for directory: /media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/howto" TYPE FILE FILES
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/api.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/square_ff.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/gain_ff.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/moving_avg_ff.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/moving_avg_history_ff.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/iq_mag_cf.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/iq_select_cf.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/flex_fir_cc.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/flex_fir_cf.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/flex_fir_ff.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/downsample_cc.h"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/include/howto/decimate_fir_cc.h"
    )
endif()

