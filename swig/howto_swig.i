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

