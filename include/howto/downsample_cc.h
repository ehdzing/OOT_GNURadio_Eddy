#ifndef INCLUDED_HOWTO_DOWNSAMPLE_CC_H
#define INCLUDED_HOWTO_DOWNSAMPLE_CC_H

#include <howto/api.h>
#include <gnuradio/sync_decimator.h>
#include <boost/shared_ptr.hpp>

namespace gr { namespace howto {

/*!
 * \brief Complex-to-complex decimator that simply picks 1 of every D samples.
 * \details No filtering, no phase control. Expect aliasing.
 */
class HOWTO_API downsample_cc : virtual public gr::sync_decimator
{
public:
  typedef boost::shared_ptr<downsample_cc> sptr;

  /*!
   * \param decim Decimation factor (D >= 2)
   */
  static sptr make(int decim);

  // Convenience accessor
  virtual int decimation() const = 0;
};

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_DOWNSAMPLE_CC_H */

