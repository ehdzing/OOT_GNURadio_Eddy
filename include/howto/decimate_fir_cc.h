#ifndef INCLUDED_HOWTO_DECIMATE_FIR_CC_H
#define INCLUDED_HOWTO_DECIMATE_FIR_CC_H

#include <howto/api.h>
#include <gnuradio/block.h>
#include <boost/shared_ptr.hpp>

namespace gr { namespace howto {

/*!
 * \brief Complex FIR decimator with runtime-changeable decimation and taps.
 * \details Anti-alias low-pass via firdes. Window selectable (incl. Kaiser).
 *          Uses set_history(L) and general_block to support D changes at runtime.
 */
class HOWTO_API decimate_fir_cc : virtual public gr::block
{
public:
  typedef boost::shared_ptr<decimate_fir_cc> sptr;

  /*!
   * \param decim       Initial decimation factor (>= 2)
   * \param samp_rate   Sampling rate [Hz]
   * \param cutoff      Low-pass cutoff [Hz]
   * \param transition  Transition width [Hz]
   * \param window      firdes window id (e.g., firdes::WIN_HAMMING)
   * \param kaizer_beta Kaiser beta (only used if window == WIN_KAISER)
   */
  static sptr make(int decim,
                   double samp_rate,
                   double cutoff,
                   double transition,
                   int window,
                   double kaiser_beta);

  // Read-only query helpers
  virtual int    decimation()  const noexcept = 0;
  virtual double samp_rate()   const noexcept = 0;
  virtual double cutoff()      const noexcept = 0;
  virtual double transition()  const noexcept = 0;
  virtual int    window()      const noexcept = 0;
  virtual double kaiser_beta() const noexcept = 0;

  // Runtime setters (thread-safe)
  virtual void set_decimation(int decim) = 0;
  virtual void set_samp_rate(double fs) = 0;
  virtual void set_cutoff(double fc) = 0;
  virtual void set_transition(double tw) = 0;
  virtual void set_window(int w) = 0;
  virtual void set_kaiser_beta(double beta) = 0;
};

}} // namespace gr::howto
#endif

