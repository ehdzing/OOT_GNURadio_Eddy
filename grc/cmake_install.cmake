# Install script for directory: /media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/gnuradio/grc/blocks" TYPE FILE FILES
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_square_ff.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_gain_ff.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_moving_avg_ff.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_moving_avg_history_ff.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_iq_mag_cf.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_iq_select_cf.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_flex_fir_ff.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_flex_fir_cc.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_flex_fir_cf.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_downsample_cc.xml"
    "/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/grc/howto_decimate_fir_cc.xml"
    )
endif()

