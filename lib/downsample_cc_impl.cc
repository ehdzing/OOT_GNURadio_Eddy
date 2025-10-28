#include "downsample_cc_impl.h"
#include <gnuradio/gr_complex.h>
#include <stdexcept>

namespace gr { namespace howto {

downsample_cc::sptr downsample_cc::make(int decim)
{
  if (decim < 2)
    throw std::invalid_argument("decimation must be >= 2");

  return downsample_cc::sptr(new downsample_cc_impl(decim));
}

downsample_cc_impl::downsample_cc_impl(int decim)
: gr::sync_decimator("downsample_cc",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     decim),
  d_decim(decim)
{
  // No filtering, no history. Pure pick-every-D behavior.
}

int downsample_cc_impl::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items)
{
  const gr_complex *in = static_cast<const gr_complex*>(input_items[0]);
  gr_complex *out = static_cast<gr_complex*>(output_items[0]);

  const int D = d_decim;
  for (int i = 0; i < noutput_items; ++i)
    out[i] = in[i * D];

  return noutput_items;
}

}} // namespace gr::howto

