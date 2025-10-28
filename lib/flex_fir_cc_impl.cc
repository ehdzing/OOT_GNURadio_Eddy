#include "flex_fir_cc_impl.h"
#include "flex_fir_kernel.cc"
#include <gnuradio/gr_complex.h>

namespace gr { namespace howto {

int flex_fir_cc_impl::work(int noutput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
{
  const gr_complex* in  = static_cast<const gr_complex*>(input_items[0]);
  gr_complex*       out = static_cast<gr_complex*>(output_items[0]);

  std::vector<float> taps;
  int m; float fs,f1,f2,w,g;
  snapshot_params_(m,fs,f1,f2,w,g,taps);

  return flex_fir_work_body<std::complex<float>, std::complex<float>>(noutput_items, in, out, d_hist, taps);
}

}} // namespace

