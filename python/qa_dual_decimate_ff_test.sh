#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/python
export PATH=/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/python:$PATH
export LD_LIBRARY_PATH=/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/lib:$LD_LIBRARY_PATH
export PYTHONPATH=/media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/swig:$PYTHONPATH
/usr/bin/python2 /media/inatel-crr/Dados/Eddy/ModulosGNU/OOT_GNURadio_Eddy/python/qa_dual_decimate_ff.py 
