/* -*- c++ -*- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <pmt/pmt.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include "time_tagger_cc_impl.h"

namespace gr {
  namespace howto {

    // ===== Factory única =====

    time_tagger_cc::sptr
    time_tagger_cc::make(double samp_rate,
                         double period_s,
                         double gap_s,
                         int    burst_len,
                         double t0_usrp,
                         const std::vector<double> &offsets_s,
                         double lead_s,
                         int tx_time_interval)
    {
      return gnuradio::get_initial_sptr(
          new time_tagger_cc_impl(samp_rate,
                                  period_s,
                                  gap_s,
                                  burst_len,
                                  t0_usrp,
                                  offsets_s,
                                  lead_s,
                                  tx_time_interval));
    }

    // ===== Impl =====

    time_tagger_cc_impl::time_tagger_cc_impl(double samp_rate,
                                             double period_s,
                                             double gap_s,
                                             int    burst_len,
                                             double t0_usrp,
                                             const std::vector<double> &offsets_s,
                                             double lead_s,
                                             int tx_time_interval)
      : gr::sync_block("time_tagger_cc",
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
        d_lead_s((lead_s >= 0.0) ? lead_s : 0.0),
        d_sample_idx(0),
        d_ctrl_ready(true),
        tx_time(true),
        d_tx_time_interval(tx_time_interval),
        d_burst_counter(0)
    {
      if(d_samp_rate <= 0.0)
        throw std::runtime_error("time_tagger_cc: samp_rate must be > 0");
      if(d_burst_len <= 0)
        throw std::runtime_error("time_tagger_cc: burst_len must be > 0");
      if(d_period_s <= 0.0)
        throw std::runtime_error("time_tagger_cc: period_s must be > 0");
      if(d_gap_s < 0.0)
        throw std::runtime_error("time_tagger_cc: gap_s must be >= 0");

      if(d_offsets_s.empty()) {
        d_offsets_s.push_back(0.0);
      }

      {
        boost::mutex::scoped_lock lock(d_mutex);
        recompute_period_and_offsets_nolock();
      }

      set_output_multiple(d_burst_len);

      message_port_register_in(pmt::mp("ctrl"));
      set_msg_handler(pmt::mp("ctrl"),
                      boost::bind(&time_tagger_cc_impl::handle_ctrl_msg,
                                  this, _1));
    }

    time_tagger_cc_impl::~time_tagger_cc_impl()
    {
    }

    // ============================================================
    void
    time_tagger_cc_impl::recompute_period_and_offsets_nolock()
    {
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

    // ============================================================
    void
    time_tagger_cc_impl::handle_ctrl_msg(const pmt::pmt_t& msg)
    {
      /*
      // Si quieres usar mensajes de control, descomenta esto.

      if(!pmt::is_dict(msg)) {
        return;
      }

      const pmt::pmt_t key_t0   = pmt::intern("t0_usrp");
      const pmt::pmt_t key_lead = pmt::intern("lead_s");

      bool updated = false;

      if(pmt::dict_has_key(msg, key_t0)) {
        pmt::pmt_t v = pmt::dict_ref(msg, key_t0, pmt::PMT_NIL);
        if(pmt::is_number(v)) {
          double t0 = pmt::to_double(v);
          set_t0_usrp(t0);
          updated = true;
        }
      }

      if(pmt::dict_has_key(msg, key_lead)) {
        pmt::pmt_t v = pmt::dict_ref(msg, key_lead, pmt::PMT_NIL);
        if(pmt::is_number(v)) {
          double lead = pmt::to_double(v);
          set_lead(lead);
          updated = true;
        }
      }

      if(updated) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_ctrl_ready = true;
        std::cout << "[burst] t0_usrp=" << d_t0_usrp
                  << " lead_s=" << d_lead_s
                  << " (ctrl_ready=1)" << std::endl;
      }
      */
    }

    // ============================================================
    // SETTERS
    // ============================================================

    void time_tagger_cc_impl::set_samp_rate(double v)
    {
      if(v <= 0.0) return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_samp_rate = v;
      recompute_period_and_offsets_nolock();
    }

    void time_tagger_cc_impl::set_period(double v)
    {
      if(v <= 0.0) return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_period_s = v;
      recompute_period_and_offsets_nolock();
    }

    void time_tagger_cc_impl::set_gap(double v)
    {
      if(v < 0.0) return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_gap_s = v;
      recompute_period_and_offsets_nolock();
    }

    void time_tagger_cc_impl::set_burst_len(int v)
    {
      if(v <= 0) return;
      boost::mutex::scoped_lock lock(d_mutex);
      d_burst_len = v;
    }

    void time_tagger_cc_impl::set_t0_usrp(double v)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_t0_usrp = v;
    }

    void time_tagger_cc_impl::set_offsets(const std::vector<double> &o)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_offsets_s = o;
      if(d_offsets_s.empty()) d_offsets_s.push_back(0.0);
      recompute_period_and_offsets_nolock();
    }

    void time_tagger_cc_impl::set_lead(double v)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_lead_s = (v >= 0.0 ? v : 0.0);
    }

    void time_tagger_cc_impl::set_tx_time_interval(int n)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_tx_time_interval = (n < 0 ? 0 : n);
    }

    // ============================================================
    // GETTERS
    // ============================================================

    double time_tagger_cc_impl::get_samp_rate() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_samp_rate;
    }

    double time_tagger_cc_impl::get_period() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_period_s;
    }

    double time_tagger_cc_impl::get_gap() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_gap_s;
    }

    int time_tagger_cc_impl::get_burst_len() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_burst_len;
    }

    double time_tagger_cc_impl::get_t0_usrp() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_t0_usrp;
    }

    std::vector<double> time_tagger_cc_impl::get_offsets() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_offsets_s;
    }

    double time_tagger_cc_impl::get_lead() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_lead_s;
    }

    // ============================================================
    // WORK
    // ============================================================

    int
    time_tagger_cc_impl::work(int noutput_items,
                              gr_vector_const_void_star &input_items,
                              gr_vector_void_star &output_items)
    {
      const gr_complex *in  = (const gr_complex *) input_items[0];
      gr_complex *out       = (gr_complex *) output_items[0];

      memcpy(out, in, noutput_items * sizeof(gr_complex));

      const long long block_start = d_sample_idx;
      const long long block_end   = d_sample_idx + noutput_items - 1;

      double    samp_rate;
      double    period_s;
      double    gap_s;
      int       burst_len;
      double    t0_usrp;
      long long period_samps;
      std::vector<long long> offsets_samps;
      double    lead_s;
      bool      ctrl_ready_local;
      int       tx_time_interval_local;

      {
        boost::mutex::scoped_lock lock(d_mutex);
        samp_rate        = d_samp_rate;
        period_s         = d_period_s;
        gap_s            = d_gap_s;
        burst_len        = d_burst_len;
        t0_usrp          = d_t0_usrp;
        period_samps     = d_period_samps;
        offsets_samps    = d_offsets_samps;
        lead_s           = d_lead_s;
        ctrl_ready_local = d_ctrl_ready;
        tx_time_interval_local = d_tx_time_interval;
      }

      if(period_samps <= 0 || offsets_samps.empty()) {
        d_sample_idx += noutput_items;
        return noutput_items;
      }

      static const pmt::pmt_t key_tx_time = pmt::string_to_symbol("tx_time");
      static const pmt::pmt_t key_tx_sob  = pmt::string_to_symbol("tx_sob");
      static const pmt::pmt_t key_tx_eob  = pmt::string_to_symbol("tx_eob");

      long long max_off = 0;
      for(size_t i = 0; i < offsets_samps.size(); ++i)
        if(offsets_samps[i] > max_off)
          max_off = offsets_samps[i];

      long long k_min = (block_start > max_off ?
                         (block_start - max_off) / period_samps : 0);

      long long k_max = (block_end / period_samps) + 1;

      for(long long k = k_min; k <= k_max; ++k) {

        const long long base = k * period_samps;

        for(size_t i = 0; i < offsets_samps.size(); ++i) {

          const long long burst_start = base + offsets_samps[i];
          const long long burst_end   = burst_start + burst_len - 1;

          if(burst_start >= block_start && burst_start <= block_end) {

            d_burst_counter++;

            bool periodic_tx =
              (tx_time_interval_local > 0 &&
               (d_burst_counter % tx_time_interval_local) == 0);

            bool put_tx = periodic_tx || tx_time;
            
            const int sob_ofs = burst_start - block_start;

            if(put_tx) {
              const double t = t0_usrp
                             + (double)burst_start / samp_rate
                             + lead_s;

              long long sec = floor(t);
              double frac = t - sec;

              pmt::pmt_t tuple =
                pmt::make_tuple(pmt::from_uint64(sec),
                                pmt::from_double(frac));

              add_item_tag(0, d_sample_idx + sob_ofs,
                           key_tx_time, tuple);

              tx_time = false;
            }

            add_item_tag(0, d_sample_idx + sob_ofs,
                         key_tx_sob, pmt::PMT_T);
          }

          if(burst_end >= block_start && burst_end <= block_end) {

            const int eob_ofs = burst_end - block_start;

            add_item_tag(0, d_sample_idx + eob_ofs,
                         key_tx_eob, pmt::PMT_T);
          }
        }
      }

      d_sample_idx += noutput_items;
      return noutput_items;
    }

  } /* namespace howto */
} /* namespace gr */
