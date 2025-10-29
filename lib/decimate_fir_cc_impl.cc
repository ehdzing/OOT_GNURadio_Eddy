#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "decimate_fir_cc_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <stdexcept>
#include <algorithm>

using gr::filter::firdes;
using std::size_t;

namespace gr { 
    namespace howto {

	decimate_fir_cc::sptr
	decimate_fir_cc::make(int decim,
		              double fs,
		              double cutoff,
		              double trans,
		              int window,
		              double beta)
	{
	  return decimate_fir_cc::sptr(
	    new decimate_fir_cc_impl(decim, fs, cutoff, trans, static_cast<firdes::win_type>(window), beta)
	  );
	}

	// -------- impl --------

	decimate_fir_cc_impl::decimate_fir_cc_impl(int decim,
		                                   double fs,
		                                   double cutoff,
		                                   double trans,
		                                   firdes::win_type window,
		                                   double beta)
	: gr::block("decimate_fir_cc",
		    gr::io_signature::make(1, 1, sizeof(gr_complex)),
		    gr::io_signature::make(1, 1, sizeof(gr_complex))),
	  d_decim(decim),
	  d_fs(fs),
	  d_cutoff(cutoff),
	  d_trans(trans),
	  d_window(window),
	  d_beta(beta),
	  d_dirty(true),
	  d_L(0)
	{
	  if (d_decim < 2)     throw std::invalid_argument("decim must be >= 2");
	  if (d_fs <= 0.0)     throw std::invalid_argument("samp_rate must be > 0");
	  if (d_cutoff <= 0.0) throw std::invalid_argument("cutoff must be > 0");
	  if (d_trans <= 0.0)  throw std::invalid_argument("transition must be > 0");

	  // Initial taps + scheduler alignment
	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    design_taps_locked();
	    apply_new_config_locked();
	  }
	}

	void decimate_fir_cc_impl::design_taps_locked()
	{
	  // Gain=1.0. firdes::low_pass expects absolute Hz when fs provided.
	  // If window is Kaiser, beta must be set; otherwise beta is ignored.
	  d_taps = firdes::low_pass(1.0f,            // gain
		                    static_cast<float>(d_fs),
		                    static_cast<float>(d_cutoff),
		                    static_cast<float>(d_trans),
		                    d_window,
		                    static_cast<float>(d_beta));
	  if (d_taps.empty())
	    throw std::runtime_error("firdes::low_pass returned empty taps");

	  d_L = d_taps.size();
	}

	void decimate_fir_cc_impl::apply_new_config_locked()
	{
	  // History = L to access last L samples per output
	  this->set_history(static_cast<int>(d_L));
	  // Relative rate is 1/D
	  this->set_relative_rate(1.0 / static_cast<double>(d_decim));
	  d_dirty = false;
	}

	// ---- setters: cheap, just mark dirty ----
	void decimate_fir_cc_impl::set_decimation(int decim)
	{
	  if (decim < 2) throw std::invalid_argument("decim must be >= 2");
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  if (decim != d_decim) { d_decim = decim; d_dirty = true; }
	}

	void decimate_fir_cc_impl::set_samp_rate(double fs)
	{
	  if (fs <= 0.0) throw std::invalid_argument("samp_rate must be > 0");
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  if (fs != d_fs) { d_fs = fs; d_dirty = true; }
	}

	void decimate_fir_cc_impl::set_cutoff(double fc)
	{
	  if (fc <= 0.0) throw std::invalid_argument("cutoff must be > 0");
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  if (fc != d_cutoff) { d_cutoff = fc; d_dirty = true; }
	}

	void decimate_fir_cc_impl::set_transition(double tw)
	{
	  if (tw <= 0.0) throw std::invalid_argument("transition must be > 0");
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  if (tw != d_trans) { d_trans = tw; d_dirty = true; }
	}
	
	static bool is_valid_window(int w) {
	  switch(static_cast<firdes::win_type>(w)) {
	    case firdes::WIN_RECTANGULAR:
	    case firdes::WIN_HAMMING:
	    case firdes::WIN_HANN:
	    case firdes::WIN_BLACKMAN:
	    case firdes::WIN_KAISER:
	    /* ... otros que soporte tu 3.7 ... */
	      return true;
	    default:
	      return false;
	  }
	}

	void decimate_fir_cc_impl::set_window(int w)
	{
	  boost::lock_guard<boost::mutex> lk(d_mutex);
          if (!is_valid_window(w)) throw std::invalid_argument("invalid firdes window id");
	  firdes::win_type neww = static_cast<firdes::win_type>(w);
	  if (neww != d_window) { d_window = neww; d_dirty = true; }
	}

	void decimate_fir_cc_impl::set_kaiser_beta(double beta)
	{
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  if (beta != d_beta) { d_beta = beta; d_dirty = true; }
	}

	// ---- scheduler functions ----

	void decimate_fir_cc_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
	{
	  // Need D samples per output plus history(L)-1 already accounted by scheduler.
	  int D = 0;
	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    D = d_decim;
	  }
	  ninput_items_required[0] = std::max(D * noutput_items, D);
	}

	int decimate_fir_cc_impl::general_work(int noutput_items,
		                               gr_vector_int &ninput_items,
		                               gr_vector_const_void_star &input_items,
		                               gr_vector_void_star &output_items)
	{
	  const gr_complex* in  = static_cast<const gr_complex*>(input_items[0]);
	  gr_complex*       out = static_cast<gr_complex*>(output_items[0]);

	  // Snapshot config without holding the lock during the hot loop
	  int    D;      // decimation
	  size_t L;      // taps length
	  std::vector<float> taps;
	  bool need_realign = false;

	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    if (d_dirty) {
	      // Recompute taps and sync scheduler contract
	      design_taps_locked();
	      apply_new_config_locked();
	      // Force scheduler to realign buffers with new history/rate
	      return 0;
	    }
	    D = d_decim;
	    L = d_L;
	    taps = d_taps; // local copy to avoid shared access in loop
	  }

	  // With set_history(L), the input pointer includes L-1 prior samples.
	  // Valid outputs limited by available inputs:
	  // usable_inputs = ninput_items[0]; but first (L-1) are history.
	  // We can produce at most floor((ninput_items[0] - (L - 1)) / D)
	  const int max_out = static_cast<int>((ninput_items[0] - static_cast<int>(L) + 1) / D);
	  const int nout = std::max(0, std::min(noutput_items, max_out));
	  if (nout <= 0) {
	    // Ask for more input
	    return 0;
	  }

	  // Base points to the "current" sample including history.
	  // For each output j, align to p = in + (L-1) + j*D, then convolve backwards.
	  const gr_complex* base = in + (L - 1);
	  for (int j = 0; j < nout; ++j) {
	    const gr_complex* p = base + j * D;
	    gr_complex acc(0.0f, 0.0f);
	    // Manual dot-product: y[n] = sum_{k=0..L-1} taps[k] * x[n - k]
	    for (size_t k = 0; k < L; ++k) {
	      acc += static_cast<float>(taps[k]) * p[-static_cast<int>(k)];
	    }
	    out[j] = acc;
	  }

	  // Consume exactly D per output produced
	  consume_each(nout * D);
	  return nout;
	}

}} // namespace gr::howto


