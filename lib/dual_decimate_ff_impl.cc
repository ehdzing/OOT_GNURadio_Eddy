/* -*- c++ -*- */
#include "dual_decimate_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <stdexcept>
#include <algorithm>

namespace gr {

    namespace howto {

	dual_decimate_ff::sptr
	dual_decimate_ff::make(int D0, int D1)
	{
	  return dual_decimate_ff::sptr(new dual_decimate_ff_impl(D0, D1));
	}

	dual_decimate_ff_impl::dual_decimate_ff_impl(int D0, int D1)
	  : gr::block("dual_decimate_ff",
		      gr::io_signature::make2(2, 2, sizeof(float), sizeof(float)),
		      gr::io_signature::make2(2, 2, sizeof(float), sizeof(float))),
	    d_D0(D0),
	    d_D1(D1),
	    d_need_realign(false)
	{
	  if (D0 < 1 || D1 < 1)
	    throw std::invalid_argument("D0 and D1 must be >= 1");
	  // No set_relative_rate(): multi-port rates are governed by forecast/consume/produce.
	  // Optional: set_output_multiple(128);
	}

	void
	dual_decimate_ff_impl::forecast (int noutput_items, gr_vector_int &req)
	{
	  int D0s, D1s;
	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    D0s = d_D0; D1s = d_D1;
	  }
	  req.resize(2);
	  req[0] = D0s * noutput_items;
	  req[1] = D1s * noutput_items;
	}

	int
	dual_decimate_ff_impl::general_work(int noutput_items,
		                            gr_vector_int &ninput,
		                            gr_vector_const_void_star &input_items,
		                            gr_vector_void_star &output_items)
	{
	  // Realign if decimation changed since last call
	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    if (d_need_realign) {
	      d_need_realign = false;
	      return 0; // scheduler will re-query forecast with new D
	    }
	  }

	  const float* in0 = static_cast<const float*>(input_items[0]);
	  const float* in1 = static_cast<const float*>(input_items[1]);
	  float* out0 = static_cast<float*>(output_items[0]);
	  float* out1 = static_cast<float*>(output_items[1]);

	  int D0s, D1s;
	  { boost::lock_guard<boost::mutex> lk(d_mutex); D0s = d_D0; D1s = d_D1; }

	  const int max0 = std::min(noutput_items, ninput[0] / D0s);
	  const int max1 = std::min(noutput_items, ninput[1] / D1s);

	  for (int i = 0; i < max0; ++i) {
	    const float* w0 = &in0[i * D0s];
	    out0[i] = mean_window(w0, D0s);
	  }
	  for (int i = 0; i < max1; ++i) {
	    const float* w1 = &in1[i * D1s];
	    out1[i] = mean_window(w1, D1s);
	  }

	  consume(0, max0 * D0s);
	  consume(1, max1 * D1s);

	  produce(0, max0);
	  produce(1, max1);

	  return WORK_CALLED_PRODUCE;
	}

	void dual_decimate_ff_impl::set_D0(int D0) noexcept
	{
	  if (D0 < 1) return;
	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    if (D0 != d_D0) {
	      d_D0 = D0;
	      d_need_realign = true;
	    }
	  }
	}

	void dual_decimate_ff_impl::set_D1(int D1) noexcept
	{
	  if (D1 < 1) return;
	  {
	    boost::lock_guard<boost::mutex> lk(d_mutex);
	    if (D1 != d_D1) {
	      d_D1 = D1;
	      d_need_realign = true;
	    }
	  }
	}

	int dual_decimate_ff_impl::D0() const noexcept
	{
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  return d_D0;
	}

	int dual_decimate_ff_impl::D1() const noexcept
	{
	  boost::lock_guard<boost::mutex> lk(d_mutex);
	  return d_D1;
	}

} // namespace howto
} // namespace gr

