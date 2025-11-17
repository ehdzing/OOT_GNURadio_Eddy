#ifndef INCLUDED_HOWTO_SLOT_TIMING_MONITOR_CC_H
#define INCLUDED_HOWTO_SLOT_TIMING_MONITOR_CC_H

#include <gnuradio/sync_block.h>
#include <howto/api.h>
#include <boost/shared_ptr.hpp>

namespace gr {
namespace howto {

/*!
 * \brief Slot timing monitor block (complex in/out).
 *
 * This block:
 *  - Passes the input complex stream to the output unchanged.
 *  - Detects burst starts based on a power threshold.
 *  - Uses the initial rx_time tag from the USRP source to map
 *    sample indices to absolute time.
 *  - For each detected burst, it:
 *      * estimates the burst index k,
 *      * compares the actual burst time with the ideal time:
 *          T_tx(k) = t0 + k * period_s,
 *      * computes the timing error D_k = T_rx(k) - T_tx(k),
 *      * maintains a sliding window of D_k to estimate jitter.
 *
 * Parameters:
 *  - samp_rate      : sampling rate in Hz.
 *  - t0             : expected time of the first burst (seconds).
 *  - period_s       : burst period (seconds), e.g. 0.010 for 10 ms.
 *  - threshold      : power threshold for burst detection
 *                     (applied to |x[n]|^2).
 *  - dt_window_len  : number of last D_k values used to compute jitter.
 *
 * The block prints timing info to stdout and exposes:
 *  - last_offset_s() : last D_k (seconds).
 *  - last_jitter_s() : estimated jitter (seconds).
 */
class HOWTO_API slot_timing_monitor_cc : virtual public gr::sync_block
{
public:
    typedef boost::shared_ptr<slot_timing_monitor_cc> sptr;

    static sptr make(double samp_rate,
                     double t0,
                     double period_s,
                     double threshold,
                     int dt_window_len);

    virtual void set_samp_rate(double samp_rate) = 0;
    virtual void set_t0(double t0) = 0;
    virtual void set_period_s(double period_s) = 0;
    virtual void set_threshold(double threshold) = 0;
    virtual void set_dt_window_len(int len) = 0;

    virtual double last_offset_s() const = 0;
    virtual double last_jitter_s() const = 0;

    virtual ~slot_timing_monitor_cc();
};

} // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_SLOT_TIMING_MONITOR_CC_H */
