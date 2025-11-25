/* -*- c++ -*- */
/*
 * rx_time_recorder_cc.h
 */

#ifndef INCLUDED_HOWTO_RX_TIME_RECORDER_CC_H
#define INCLUDED_HOWTO_RX_TIME_RECORDER_CC_H

#include <howto/api.h>
#include <gnuradio/block.h>
#include <boost/shared_ptr.hpp>
#include <string>

namespace gr {
  namespace howto {

    /*!
     * \brief Record decimated RX power and USRP-based RX time into a single file.
     *
     * This block:
     *  - receives complex samples from an USRP source,
     *  - uses the UHD "rx_time" tag as absolute time reference,
     *  - computes per-sample power p = |x|^2,
     *  - keeps 1 sample every \p decim input samples,
     *  - writes one binary record per kept sample.
     *
     * File format (binary, little-endian):
     *
     *   [double t_rx_0][float p_0][double t_rx_1][float p_1] ...
     *
     * where t_rx_n is the absolute RX time (seconds) and p_n is the power.
     *
     * \param samp_rate  Physical sample rate at the USRP (Hz).
     * \param filename   Path of the binary output file.
     * \param decim      Decimation factor (>= 1). Keep 1 every \p decim samples.
     */
    class HOWTO_API rx_time_recorder_cc : virtual public gr::block
    {
    public:
      typedef boost::shared_ptr<rx_time_recorder_cc> sptr;

      static sptr make(double samp_rate,
                       const std::string& filename,
                       int decim = 1);
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_RX_TIME_RECORDER_CC_H */
