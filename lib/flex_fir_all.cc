//#include <howto/flex_fir_ff.h>
//#include <howto/flex_fir_cc.h>
//#include <howto/flex_fir_cf.h>
#include <gnuradio/io_signature.h>
#include "flex_fir_ff_impl.h"
#include "flex_fir_cc_impl.h"
#include "flex_fir_cf_impl.h"

namespace gr { namespace howto {

flex_fir_ff::sptr flex_fir_ff::make(int mode, float fs, float f1, float f2, float w, float g)
{
  return gnuradio::get_initial_sptr(new flex_fir_ff_impl(mode, fs, f1, f2, w, g));
}

flex_fir_cc::sptr flex_fir_cc::make(int mode, float fs, float f1, float f2, float w, float g)
{
  return gnuradio::get_initial_sptr(new flex_fir_cc_impl(mode, fs, f1, f2, w, g));
}

flex_fir_cf::sptr flex_fir_cf::make(int mode, float fs, float f1, float f2, float w, float g)
{
  return gnuradio::get_initial_sptr(new flex_fir_cf_impl(mode, fs, f1, f2, w, g));
}

}} // namespace

