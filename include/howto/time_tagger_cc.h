/* -*- c++ -*- */
/*
 * time_tagger_cc.h
 */

#ifndef INCLUDED_HOWTO_TIME_TAGGER_CC_H
#define INCLUDED_HOWTO_TIME_TAGGER_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace gr {
  namespace howto {

    class HOWTO_API time_tagger_cc : public virtual gr::sync_block
    {
     public:
      typedef boost::shared_ptr<time_tagger_cc> sptr;

      static sptr make(double samp_rate,
                       double period_s,
                       double gap_s,
                       int    burst_len,
                       double t0_usrp,
                       const std::vector<double> &offsets_s,
                       double lead_s,
                       int tx_time_interval);

      virtual void set_samp_rate(double samp_rate) = 0;
      virtual void set_period(double period_s) = 0;
      virtual void set_gap(double gap_s) = 0;
      virtual void set_burst_len(int burst_len) = 0;
      virtual void set_t0_usrp(double t0_usrp) = 0;
      virtual void set_offsets(const std::vector<double> &offsets_s) = 0;
      virtual void set_lead(double lead_s) = 0;
      virtual void set_tx_time_interval(int tx_time_interval) = 0;

      virtual double get_samp_rate() const = 0;
      virtual double get_period() const = 0;
      virtual double get_gap() const = 0;
      virtual int    get_burst_len() const = 0;
      virtual double get_t0_usrp() const = 0;
      virtual std::vector<double> get_offsets() const = 0;
      virtual double get_lead() const = 0;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_TIME_TAGGER_CC_H */
