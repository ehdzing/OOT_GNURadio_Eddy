/* -*- c++ -*- */

#define HOWTO_API
%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "howto_swig_doc.i"

%{
#include "howto/square_ff.h"
#include "howto/gain_ff.h"
#include "howto/moving_avg_ff.h"
%}


%include "howto/square_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, square_ff);
%include "howto/gain_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, gain_ff);
%include "howto/moving_avg_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, moving_avg_ff);
