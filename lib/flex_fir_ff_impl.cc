#include "flex_fir_ff_impl.h"
#include "flex_fir_kernel.cc"

namespace gr { namespace howto {

int flex_fir_ff_impl::work(int noutput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
{
  const float* in  = static_cast<const float*>(input_items[0]);
  float*       out = static_cast<float*>(output_items[0]);

  std::vector<float> taps;
  int m; float fs,f1,f2,w,g;
  snapshot_params_(m,fs,f1,f2,w,g,taps);

  return flex_fir_work_body<float,float>(noutput_items, in, out, d_hist, taps);
}

}} // namespace

