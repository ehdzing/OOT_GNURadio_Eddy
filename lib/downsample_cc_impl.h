#ifndef INCLUDED_HOWTO_DOWNSAMPLE_CC_IMPL_H
#define INCLUDED_HOWTO_DOWNSAMPLE_CC_IMPL_H

#include <howto/downsample_cc.h>

namespace gr { namespace howto {

/*!
 * \brief Implementation of the trivial complex decimator.
 * \note D is fixed at construction time.
 */
class downsample_cc_impl final : public downsample_cc
{
private:
  const int d_decim;

public:
  explicit downsample_cc_impl(int decim);
  ~downsample_cc_impl() override = default;

  int decimation() const override { return d_decim; }

  // Never noexcept in your template
  int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;
};

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_DOWNSAMPLE_CC_IMPL_H */

