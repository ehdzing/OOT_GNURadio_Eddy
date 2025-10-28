/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_MOVING_AVG_FF_H
#define INCLUDED_HOWTO_MOVING_AVG_FF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>

namespace gr { namespace howto {

/*!
 * Promedio móvil sencillo:
 *  - 1 salida por cada entrada (sync_block).
 *  - Las primeras (N-1) salidas son 0.0 hasta llenar ventana.
 *  - N y scale configurables y ajustables en runtime vía setters.
 */
class HOWTO_API moving_avg_ff : virtual public gr::sync_block
{
public:
  typedef boost::shared_ptr<moving_avg_ff> sptr;

  static sptr make(int length, float scale = 1.0f);

  // Runtime control
  virtual void  set_length(int length) = 0; // N
  virtual int   length() const = 0;

  virtual void  set_scale(float scale) = 0; // factor
  virtual float scale() const = 0;

protected:
  moving_avg_ff() {}
};

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_MOVING_AVG_FF_H */

