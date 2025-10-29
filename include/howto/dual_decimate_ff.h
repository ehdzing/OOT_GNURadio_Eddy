/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_DUAL_DECIMATE_FF_H
#define INCLUDED_HOWTO_DUAL_DECIMATE_FF_H

#include <howto/api.h>
#include <gnuradio/block.h>
#include <boost/shared_ptr.hpp>

namespace gr {
namespace howto {

/*!
 * \brief Dual decimator with two inputs and two outputs.
 * Each output i emits the average over non-overlapping windows of Di from input i.
 * Runtime setters mark a realign flag used by general_work().
 */
class HOWTO_API dual_decimate_ff : virtual public gr::block
{
public:
  typedef boost::shared_ptr<dual_decimate_ff> sptr;
  static sptr make(int D0, int D1);

  // Destructor can be noexcept
  virtual ~dual_decimate_ff() noexcept {}

  // Our own virtuals can be noexcept safely
  virtual void set_D0(int D0) noexcept = 0;
  virtual void set_D1(int D1) noexcept = 0;

  virtual int D0() const noexcept = 0;
  virtual int D1() const noexcept = 0;
};

} // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_DUAL_DECIMATE_FF_H */

