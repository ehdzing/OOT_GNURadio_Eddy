#ifndef INCLUDED_HOWTO_DECIMATE_FIR_CC_IMPL_H
#define INCLUDED_HOWTO_DECIMATE_FIR_CC_IMPL_H

#include <howto/decimate_fir_cc.h>
#include <boost/thread/mutex.hpp>
#include <vector>

#include <gnuradio/filter/firdes.h>
using gr::filter::firdes;

namespace gr { 
  
   namespace howto {

	class decimate_fir_cc_impl final : public decimate_fir_cc
	{
	private:
	  // Parameters guarded by d_mutex
	  mutable boost::mutex d_mutex;
	  int    d_decim;
	  double d_fs;
	  double d_cutoff;
	  double d_trans;
	  firdes::win_type d_window;
	  double d_beta;

	  // FIR taps (float for firdes), copied locally when updated
	  std::vector<float> d_taps;
	  bool   d_dirty;    // taps or decim changed
	  size_t d_L;        // taps length snapshot

	  // Internal helpers
	  void   design_taps_locked();   // assumes d_mutex locked
	  void   apply_new_config_locked(); // set_history + set_relative_rate + L cache

	public:
	  // Ctor/Dtor
	  decimate_fir_cc_impl(int decim,
		               double fs,
		               double cutoff,
		               double trans,
		               firdes::win_type d_window,   
		               double beta); /* may throw */

	  ~decimate_fir_cc_impl() override = default;

	  // Queries
	  int    decimation()  const noexcept override { return d_decim; }
	  double samp_rate()   const noexcept override { return d_fs; }
	  double cutoff()      const noexcept override { return d_cutoff; }
	  double transition()  const noexcept override { return d_trans; }
	  int    window()      const noexcept override { return static_cast<int>(d_window); } // getter sigue devolviendo int
	  double kaiser_beta() const noexcept override { return d_beta; }

	  // Setters (mark dirty, cheap)
	  void set_decimation(int decim) override;
	  void set_samp_rate(double fs) override;
	  void set_cutoff(double fc) override;
	  void set_transition(double tw) override;
	  void set_window(int w) override;
	  void set_kaiser_beta(double beta) override;

	  // GNURadio API
	  void forecast (int noutput_items, gr_vector_int &ninput_items_required) override;

	  // Work is NEVER noexcept in your template
	  int general_work(int noutput_items,
		           gr_vector_int &ninput_items,
		           gr_vector_const_void_star &input_items,
		           gr_vector_void_star &output_items) override;
	};

}} // namespace gr::howto
#endif

