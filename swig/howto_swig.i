/* -*- c++ -*- */

#define HOWTO_API
%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "howto_swig_doc.i"

%{
#include "howto/square_ff.h"
#include "howto/gain_ff.h"
#include "howto/moving_avg_ff.h"
#include "howto/moving_avg_history_ff.h"
#include "howto/iq_mag_cf.h"
#include "howto/iq_select_cf.h"
#include "howto/flex_fir_ff.h"
#include "howto/flex_fir_cc.h"
#include "howto/flex_fir_cf.h"
#include "howto/downsample_cc.h"
#include "howto/decimate_fir_cc.h"
#include "howto/dual_decimate_ff.h"
#include "howto/detector_ff.h"
#include "howto/gate_ff.h"
#include "howto/detector_exp_ff.h"
%}


%include "howto/square_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, square_ff);
%include "howto/gain_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, gain_ff);
%include "howto/moving_avg_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, moving_avg_ff);
%include "howto/moving_avg_history_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, moving_avg_history_ff);
%include "howto/iq_mag_cf.h"
GR_SWIG_BLOCK_MAGIC2(howto, iq_mag_cf);
%include "howto/iq_select_cf.h"
GR_SWIG_BLOCK_MAGIC2(howto, iq_select_cf);
%include "howto/flex_fir_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, flex_fir_ff);
%include "howto/flex_fir_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, flex_fir_cc);
%include "howto/flex_fir_cf.h"
GR_SWIG_BLOCK_MAGIC2(howto, flex_fir_cf);
%include "howto/downsample_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, downsample_cc);
%include "howto/decimate_fir_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, decimate_fir_cc);
%include "howto/dual_decimate_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, dual_decimate_ff);
%include "howto/gate_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, gate_ff);
%include "howto/detector_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, detector_ff);



%include "howto/detector_exp_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, detector_exp_ff);
