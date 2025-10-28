#ifndef INCLUDED_HOWTO_FLEX_FIR_FF_IMPL_H
#define INCLUDED_HOWTO_FLEX_FIR_FF_IMPL_H

#include <howto/flex_fir_ff.h>
#include "flex_fir_impl_base.h"

namespace gr { namespace howto {

class flex_fir_ff_impl final : public flex_fir_ff,
                               public flex_fir_impl_base<float,float>
{
public:
  flex_fir_ff_impl(int mode, float fs, float f1, float f2, float w, float g)
  : gr::sync_block("flex_fir_ff",
        gr::io_signature::make(1,1,sizeof(float)),
        gr::io_signature::make(1,1,sizeof(float))),
    flex_fir_impl_base<float,float>(mode,fs,f1,f2,w,g)
  {}

  ~flex_fir_ff_impl() override {}

  // iface
  void  set_mode(int m) noexcept override { flex_fir_impl_base::set_mode(m); }
  int   mode() const noexcept override    { return flex_fir_impl_base::mode(); }

  void  set_samp_rate(float fs) noexcept override { flex_fir_impl_base::set_samp_rate(fs); }
  float samp_rate() const noexcept override       { return flex_fir_impl_base::samp_rate(); }

  void  set_f1(float f) noexcept override { flex_fir_impl_base::set_f1(f); }
  float f1() const noexcept override      { return flex_fir_impl_base::f1(); }

  void  set_f2(float f) noexcept override { flex_fir_impl_base::set_f2(f); }
  float f2() const noexcept override      { return flex_fir_impl_base::f2(); }

  void  set_width(float w) noexcept override { flex_fir_impl_base::set_width(w); }
  float width() const noexcept override      { return flex_fir_impl_base::width(); }

  void  set_gain(float g) noexcept override { flex_fir_impl_base::set_gain(g); }
  float gain() const noexcept override      { return flex_fir_impl_base::gain(); }

  std::vector<float> taps() const override  { return flex_fir_impl_base::taps(); }

  int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items) override;
};

}} // namespace
#endif

