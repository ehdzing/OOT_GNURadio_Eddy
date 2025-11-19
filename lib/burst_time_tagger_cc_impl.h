/* -*- c++ -*- */
/*
 * burst_time_tagger_cc_impl.h
 */

#ifndef INCLUDED_HOWTO_BURST_TIME_TAGGER_CC_IMPL_H
#define INCLUDED_HOWTO_BURST_TIME_TAGGER_CC_IMPL_H

#include <howto/burst_time_tagger_cc.h>
#include <boost/thread/mutex.hpp>
#include <vector>

namespace gr {
  namespace howto {

    class burst_time_tagger_cc_impl final : public burst_time_tagger_cc
    {
     private:
      mutable boost::mutex d_mutex;

      // Parameters (protected by d_mutex)
      double                 d_samp_rate;
      double                 d_period_s;       // active part
      double                 d_gap_s;          // silent gap after active part
      int                    d_burst_len;
      double                 d_t0_usrp;
      std::vector<double>    d_offsets_s;       // offsets in seconds (per active period)
      long long              d_period_samps;    // (period_s + gap_s) * samp_rate (rounded)
      std::vector<long long> d_offsets_samps;   // offsets_s * samp_rate (rounded)

      // additional per-burst lead time (seconds)
      double                 d_lead_s;

      // Internal global sample index (not under mutex, only used in work())
      long long d_sample_idx;

      // If you want to gate tag generation until a control message arrives
      bool d_ctrl_ready;

      void recompute_period_and_offsets_nolock();

      // handler for control messages (t0_usrp / lead_s) - currently commented
      void handle_ctrl_msg(const pmt::pmt_t& msg);

     public:
      burst_time_tagger_cc_impl(double samp_rate,
                                double period_s,
                                double gap_s,
                                int    burst_len,
                                double t0_usrp,
                                const std::vector<double> &offsets_s,
                                double lead_s);

      ~burst_time_tagger_cc_impl() override;

      // Setters
      void set_samp_rate(double samp_rate) override;
      void set_period(double period_s) override;
      void set_gap(double gap_s) override;
      void set_burst_len(int burst_len) override;
      void set_t0_usrp(double t0_usrp) override;
      void set_offsets(const std::vector<double> &offsets_s) override;
      void set_lead(double lead_s) override;

      // Getters
      double get_samp_rate() const override;
      double get_period() const override;
      double get_gap() const override;
      int    get_burst_len() const override;
      double get_t0_usrp() const override;
      std::vector<double> get_offsets() const override;
      double get_lead() const override;

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items) override;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_BURST_TIME_TAGGER_CC_IMPL_H */