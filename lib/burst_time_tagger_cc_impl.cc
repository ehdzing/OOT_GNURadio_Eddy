/* -*- c++ -*- */
/* ... misma cabecera ... */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <pmt/pmt.h>
#include <cstring>
#include <cmath>
#include <algorithm>

#include "burst_time_tagger_cc_impl.h"

namespace gr {
  namespace howto {

    // ===== Factories =====

    burst_time_tagger_cc::sptr
    burst_time_tagger_cc::make(double samp_rate,
                               double period_s,
                               int    burst_len,
                               double t0_usrp,
                               const std::vector<double> &offsets_s)
    {
      // Legacy factory: gap_s = 0.0
      return gnuradio::get_initial_sptr(
          new burst_time_tagger_cc_impl(samp_rate,
                                        period_s,
                                        burst_len,
                                        t0_usrp,
                                        offsets_s,
                                        0.0));
    }

    burst_time_tagger_cc::sptr
    burst_time_tagger_cc::make(double samp_rate,
                               double period_s,
                               double gap_s,
                               int    burst_len,
                               double t0_usrp,
                               const std::vector<double> &offsets_s)
    {
      return gnuradio::get_initial_sptr(
          new burst_time_tagger_cc_impl(samp_rate,
                                        period_s,
                                        burst_len,
                                        t0_usrp,
                                        offsets_s,
                                        gap_s));
    }

    // ===== Impl =====

    burst_time_tagger_cc_impl::burst_time_tagger_cc_impl(double samp_rate,
                                                         double period_s,
                                                         int    burst_len,
                                                         double t0_usrp,
                                                         const std::vector<double> &offsets_s,
                                                         double gap_s)
      : gr::sync_block("burst_time_tagger_cc",
                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                       gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_samp_rate(samp_rate),
        d_period_s(period_s),
        d_gap_s(gap_s),
        d_burst_len(burst_len),
        d_t0_usrp(t0_usrp),
        d_offsets_s(offsets_s),
        d_period_samps(0),
        d_offsets_samps(),
        d_sample_idx(0)
    {
      if(d_samp_rate <= 0.0)
        throw std::runtime_error("burst_time_tagger_cc: samp_rate must be > 0");
      if(d_burst_len <= 0)
        throw std::runtime_error("burst_time_tagger_cc: burst_len must be > 0");
      if(d_period_s <= 0.0)
        throw std::runtime_error("burst_time_tagger_cc: period_s must be > 0");
      if(d_gap_s < 0.0)
        throw std::runtime_error("burst_time_tagger_cc: gap_s must be >= 0");

      // If no offsets provided, default to [0.0] -> one burst per active period
      if(d_offsets_s.empty()) {
        d_offsets_s.push_back(0.0);
      }

      {
        boost::mutex::scoped_lock lock(d_mutex);
        recompute_period_and_offsets_nolock();
      }

      set_output_multiple(d_burst_len);
    }

    burst_time_tagger_cc_impl::~burst_time_tagger_cc_impl()
    {
    }

    void
    burst_time_tagger_cc_impl::recompute_period_and_offsets_nolock()
    {
      // d_mutex must already be locked

      const double effective_period_s = d_period_s + d_gap_s;
      double period_samps = effective_period_s * d_samp_rate;

      d_period_samps = static_cast<long long>(std::llround(period_samps));
      if(d_period_samps <= 0) {
        d_period_samps = d_burst_len;
      }

      d_offsets_samps.clear();
      d_offsets_samps.reserve(d_offsets_s.size());

      for(std::size_t i = 0; i < d_offsets_s.size(); ++i) {
        const double off_samps = d_offsets_s[i] * d_samp_rate;
        long long off_i = static_cast<long long>(std::llround(off_samps));
        if(off_i < 0) {
          off_i = 0;
        }
        d_offsets_samps.push_back(off_i);
      }

      if(d_offsets_samps.empty()) {
        d_offsets_samps.push_back(0);
      }
    }

    // ===== Setters =====

    void
    burst_time_tagger_cc_impl::set_samp_rate(double samp_rate)
    {
      if(samp_rate <= 0.0)
        return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_samp_rate = samp_rate;
      recompute_period_and_offsets_nolock();
    }

    void
    burst_time_tagger_cc_impl::set_period(double period_s)
    {
      if(period_s <= 0.0)
        return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_period_s = period_s;
      recompute_period_and_offsets_nolock();
    }

    void
    burst_time_tagger_cc_impl::set_gap(double gap_s)
    {
      if(gap_s < 0.0)
        return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_gap_s = gap_s;
      recompute_period_and_offsets_nolock();
    }

    void
    burst_time_tagger_cc_impl::set_burst_len(int burst_len)
    {
      if(burst_len <= 0)
        return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_burst_len = burst_len;
      // period_samps and offsets_samps do not depend directly on burst_len.
    }

    void
    burst_time_tagger_cc_impl::set_t0_usrp(double t0_usrp)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_t0_usrp = t0_usrp;
    }

    void
    burst_time_tagger_cc_impl::set_offsets(const std::vector<double> &offsets_s)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_offsets_s = offsets_s;
      if(d_offsets_s.empty()) {
        d_offsets_s.push_back(0.0);
      }
      recompute_period_and_offsets_nolock();
    }

    // ===== Getters =====

    double
    burst_time_tagger_cc_impl::get_samp_rate() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_samp_rate;
    }

    double
    burst_time_tagger_cc_impl::get_period() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_period_s;
    }

    double
    burst_time_tagger_cc_impl::get_gap() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_gap_s;
    }

    int
    burst_time_tagger_cc_impl::get_burst_len() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_burst_len;
    }

    double
    burst_time_tagger_cc_impl::get_t0_usrp() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_t0_usrp;
    }

    std::vector<double>
    burst_time_tagger_cc_impl::get_offsets() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_offsets_s;
    }

    // ===== work() =====

    int
    burst_time_tagger_cc_impl::work(int noutput_items,
                                    gr_vector_const_void_star &input_items,
                                    gr_vector_void_star &output_items)
    {
      const gr_complex *in  = static_cast<const gr_complex *>(input_items[0]);
      gr_complex *out       = static_cast<gr_complex *>(output_items[0]);

      // Snapshot parameters under mutex
      double    samp_rate;
      double    period_s;
      double    gap_s;
      int       burst_len;
      double    t0_usrp;
      long long period_samps;
      std::vector<long long> offsets_samps;

      {
        boost::mutex::scoped_lock lock(d_mutex);
        samp_rate     = d_samp_rate;
        period_s      = d_period_s;
        gap_s         = d_gap_s;
        burst_len     = d_burst_len;
        t0_usrp       = d_t0_usrp;
        period_samps  = d_period_samps;
        offsets_samps = d_offsets_samps;
      }

      (void)period_s;
      (void)gap_s;

      // Pass-through copy
      std::memcpy(out, in, noutput_items * sizeof(gr_complex));

      const long long block_start = d_sample_idx;
      const long long block_end   = d_sample_idx + static_cast<long long>(noutput_items) - 1;

      if(period_samps <= 0 || offsets_samps.empty()) {
        d_sample_idx += static_cast<long long>(noutput_items);
        return noutput_items;
      }

      // PMT symbols for UHD tags
      static const pmt::pmt_t key_tx_time = pmt::string_to_symbol("tx_time");
      static const pmt::pmt_t key_tx_sob  = pmt::string_to_symbol("tx_sob");
      static const pmt::pmt_t key_tx_eob  = pmt::string_to_symbol("tx_eob");

      // Compute max offset (in samples) to bound the search in k
      long long max_off = 0;
      for(std::size_t i = 0; i < offsets_samps.size(); ++i) {
        if(offsets_samps[i] > max_off) {
          max_off = offsets_samps[i];
        }
      }

      // Determine range of period indices that might intersect this block
      long long k_min;
      if(block_start > max_off) {
        k_min = (block_start - max_off) / period_samps;
      } else {
        k_min = 0;
      }

      long long k_max = (block_end / period_samps) + 1;

      if(k_min < 0) {
        k_min = 0;
      }
      if(k_max < 0) {
        k_max = 0;
      }

      for(long long k = k_min; k <= k_max; ++k) {
        const long long base = k * period_samps;

        for(std::size_t i = 0; i < offsets_samps.size(); ++i) {
          const long long burst_start = base + offsets_samps[i];
          const long long burst_end   = burst_start + static_cast<long long>(burst_len) - 1;

          // SOB inside this block?
          if(burst_start >= block_start && burst_start <= block_end) {
            const int sob_offset = static_cast<int>(burst_start - block_start);

            const double t_burst = t0_usrp + static_cast<double>(burst_start) / samp_rate;
            const long long full_sec = static_cast<long long>(std::floor(t_burst));
            const double frac_sec    = t_burst - static_cast<double>(full_sec);

            pmt::pmt_t time_tuple = pmt::make_tuple(
                pmt::from_uint64(static_cast<uint64_t>(full_sec)),
                pmt::from_double(frac_sec));

            add_item_tag(0,
                         d_sample_idx + sob_offset,
                         key_tx_time,
                         time_tuple);

            add_item_tag(0,
                         d_sample_idx + sob_offset,
                         key_tx_sob,
                         pmt::PMT_T);
          }

          // EOB inside this block?
          if(burst_end >= block_start && burst_end <= block_end) {
            const int eob_offset = static_cast<int>(burst_end - block_start);

            add_item_tag(0,
                         d_sample_idx + eob_offset,
                         key_tx_eob,
                         pmt::PMT_T);
          }
        }
      }

      d_sample_idx += static_cast<long long>(noutput_items);
      return noutput_items;
    }

  } /* namespace howto */
} /* namespace gr */
