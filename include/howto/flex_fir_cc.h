#ifndef INCLUDED_HOWTO_FLEX_FIR_CC_H
#define INCLUDED_HOWTO_FLEX_FIR_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <vector>
#include <complex>

namespace gr { namespace howto {

class HOWTO_API flex_fir_cc : virtual public gr::sync_block
{
public:
  typedef boost::shared_ptr<flex_fir_cc> sptr;

  static sptr make(int mode, float samp_rate,
                   float f1, float f2, float width, float gain);

  virtual ~flex_fir_cc() {}

  virtual void  set_mode(int mode) noexcept = 0;
  virtual int   mode() const noexcept = 0;

  virtual void  set_samp_rate(float fs) noexcept = 0;
  virtual float samp_rate() const noexcept = 0;

  virtual void  set_f1(float f) noexcept = 0;
  virtual float f1() const noexcept = 0;

  virtual void  set_f2(float f) noexcept = 0;
  virtual float f2() const noexcept = 0;

  virtual void  set_width(float w) noexcept = 0;
  virtual float width() const noexcept = 0;

  virtual void  set_gain(float g) noexcept = 0;
  virtual float gain() const noexcept = 0;

  virtual std::vector<float> taps() const = 0;
};

}} // namespace
#endif

