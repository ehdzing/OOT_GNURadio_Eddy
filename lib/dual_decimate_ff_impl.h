/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_DUAL_DECIMATE_FF_IMPL_H
#define INCLUDED_HOWTO_DUAL_DECIMATE_FF_IMPL_H

#include <howto/dual_decimate_ff.h>
#include <boost/thread/mutex.hpp>

namespace gr {
    
    namespace howto {

	class dual_decimate_ff_impl final : public dual_decimate_ff
	{
	private:
	  mutable boost::mutex d_mutex;
	  int  d_D0;
	  int  d_D1;
	  bool d_need_realign; // flagged by setters; consumed in general_work

	  static inline float mean_window(const float* ptr, int len) noexcept
	  {
	    float acc = 0.0f;
	    for (int i = 0; i < len; ++i) acc += ptr[i];
	    return acc / (float)len;
	  }

	public:
	  explicit dual_decimate_ff_impl(int D0, int D1);
	  ~dual_decimate_ff_impl() noexcept override {}

	  // Do NOT add noexcept here: they override gr::block virtuals (ABI compatibility)
	  void forecast (int noutput_items, gr_vector_int &ninput_items_required) override;

	  int general_work(int noutput_items,
		           gr_vector_int &ninput_items,
		           gr_vector_const_void_star &input_items,
		           gr_vector_void_star &output_items) override;

	  // Our API: noexcept + override
	  void set_D0(int D0) noexcept override;
	  void set_D1(int D1) noexcept override;

	  int D0() const noexcept override;
	  int D1() const noexcept override;
	};

	} // namespace howto
	} // namespace gr

#endif /* INCLUDED_HOWTO_DUAL_DECIMATE_FF_IMPL_H */

