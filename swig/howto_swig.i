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
#include "howto/slot_clock_cc.h"
#include "howto/metric_estimator_cc.h"
#include "howto/scheduler_ctrl.h"
#include "howto/sim_tx_modulator_cc.h"
#include "howto/timed_burst_tagger_cc.h"
#include "howto/burst_time_tagger_cc.h"
#include "howto/slot_guard_cc.h"
#include "howto/frame_gate_cc.h"
#include "howto/slot_timing_monitor_cc.h"
#include "howto/time_tagger_cc.h"
#include "howto/rx_time_recorder_cc.h"
#include "howto/preamble_burst_source_cc.h"
#include "howto/cfo_injector_cc.h"
#include "howto/frame_burst_source_cc.h"
#include "howto/preamble_abs_compare_ff.h"
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
%include "howto/slot_clock_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, slot_clock_cc);
%include "howto/metric_estimator_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, metric_estimator_cc);
%include "howto/scheduler_ctrl.h"
GR_SWIG_BLOCK_MAGIC2(howto, scheduler_ctrl);
%include "howto/sim_tx_modulator_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, sim_tx_modulator_cc);
%include "howto/timed_burst_tagger_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, timed_burst_tagger_cc);
%include "howto/burst_time_tagger_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, burst_time_tagger_cc);
%include "howto/slot_guard_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, slot_guard_cc);
%include "howto/frame_gate_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, frame_gate_cc);
%include "howto/slot_timing_monitor_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, slot_timing_monitor_cc);
%include "howto/time_tagger_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, time_tagger_cc);
%include "howto/rx_time_recorder_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, rx_time_recorder_cc);
%include "howto/preamble_burst_source_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, preamble_burst_source_cc);
%include "howto/cfo_injector_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, cfo_injector_cc);
%include "howto/frame_burst_source_cc.h"
GR_SWIG_BLOCK_MAGIC2(howto, frame_burst_source_cc);
%include "howto/preamble_abs_compare_ff.h"
GR_SWIG_BLOCK_MAGIC2(howto, preamble_abs_compare_ff);
