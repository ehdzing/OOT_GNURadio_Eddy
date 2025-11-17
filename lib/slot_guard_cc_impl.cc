/* -*- c++ -*- */
/* 
 * slot_guard_cc_impl.cc
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "slot_guard_cc_impl.h"

#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <uhd/types/time_spec.hpp>
#include <pmt/pmt.h>

#include <cmath>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <iostream>   // for prints

namespace gr {
  namespace howto {

    static inline size_t round_to_size(double x)
    {
      return (x < 0.0) ? 0 : static_cast<size_t>(std::llround(x));
    }

    // Recommended defaults (seconds)
    static const double DEFAULT_OFFSET_THR_PASS  = 1e-4; // 100 us
    static const double DEFAULT_JITTER_THR_PASS  = 5e-5; // 50 us
    static const double DEFAULT_OFFSET_THR_DONT  = 7e-4; // 700 us
    static const double DEFAULT_JITTER_THR_DONT  = 3e-4; // 300 us

    // MOD: recommended defaults for initial guard and lead
    static const double DEFAULT_GUARD_INITIAL_S = 0.5;   // 500 ms
    static const double DEFAULT_LEAD_S          = 2e-2;  // 20 ms

    // Factory
    slot_guard_cc::sptr
    slot_guard_cc::make(double sample_rate,
                        int    numerology_id,
                        size_t samples_per_slot,
                        bool   use_pps,
                        double offset_thr_pass_s,
                        double jitter_thr_pass_s,
                        double offset_thr_dont_s,
                        double jitter_thr_dont_s,
                        int    hysteresis_slots,
                        bool   dtx_consumes_input,
                        bool   allow_dont_consume,
                        double guard_initial_s,
                        double lead_s)
    {
      return gnuradio::get_initial_sptr(
          new slot_guard_cc_impl(sample_rate,
                                 numerology_id,
                                 samples_per_slot,
                                 use_pps,
                                 offset_thr_pass_s,
                                 jitter_thr_pass_s,
                                 offset_thr_dont_s,
                                 jitter_thr_dont_s,
                                 hysteresis_slots,
                                 dtx_consumes_input,
                                 allow_dont_consume,
                                 guard_initial_s,
                                 lead_s));
    }

    // Constructor
    slot_guard_cc_impl::slot_guard_cc_impl(double sample_rate,
                                           int    numerology_id,
                                           size_t samples_per_slot,
                                           bool   use_pps,
                                           double offset_thr_pass_s,
                                           double jitter_thr_pass_s,
                                           double offset_thr_dont_s,
                                           double jitter_thr_dont_s,
                                           int    hysteresis_slots,
                                           bool   dtx_consumes_input,
                                           bool   allow_dont_consume,
                                           double guard_initial_s,
                                           double lead_s)
      : gr::block("slot_guard_cc",
                  gr::io_signature::make(1, 1, sizeof(gr_complex)),
                  gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_fs(sample_rate),
        d_mu(numerology_id),
        d_samples_per_slot(samples_per_slot),
        d_use_pps(use_pps),
        d_offset_thr_pass_s(offset_thr_pass_s),
        d_jitter_thr_pass_s(jitter_thr_pass_s),
        d_offset_thr_dont_s(offset_thr_dont_s),
        d_jitter_thr_dont_s(jitter_thr_dont_s),
        d_hysteresis_slots(std::max(1, hysteresis_slots)),
        d_dtx_consumes_input(dtx_consumes_input),
        d_allow_dont_consume(allow_dont_consume),
        // MOD: initialize guard / lead with defaults if invalid
        d_guard_initial_s(guard_initial_s > 0.0 ? guard_initial_s
                                                : DEFAULT_GUARD_INITIAL_S),
        d_lead_s(lead_s >= 0.0 ? lead_s : DEFAULT_LEAD_S),
        d_usrp(),
        d_t0_host(),
        d_time_init_done(false),
        d_dt0_bias(0.0),
        d_last_decision(DECISION_PASS),
        d_stable_counter(0),
        d_dt_window(),
        d_dt_window_len(8),
        d_last_offset_s(0.0),
        d_last_jitter_s(0.0),
        d_mutex()
    {
      if (d_fs <= 0.0) {
        throw std::runtime_error("slot_guard_cc: sample_rate must be > 0");
      }

      // Fill recommended defaults if user passed non-positive values
      if (d_offset_thr_pass_s <= 0.0)
        d_offset_thr_pass_s = DEFAULT_OFFSET_THR_PASS;
      if (d_jitter_thr_pass_s <= 0.0)
        d_jitter_thr_pass_s = DEFAULT_JITTER_THR_PASS;
      if (d_offset_thr_dont_s <= 0.0)
        d_offset_thr_dont_s = DEFAULT_OFFSET_THR_DONT;
      if (d_jitter_thr_dont_s <= 0.0)
        d_jitter_thr_dont_s = DEFAULT_JITTER_THR_DONT;

      // USRP handle
      uhd::device_addr_t dev_addr;
      d_usrp = uhd::usrp::multi_usrp::make(dev_addr);
      init_time_();

      // Slot size
      if (d_samples_per_slot == 0) {
        compute_slot_params_from_mu_();
      }
      if (d_samples_per_slot == 0) {
        throw std::runtime_error(
            "slot_guard_cc: samples_per_slot not set and numerology invalid");
      }

      // Message ports
      message_port_register_out(pmt::mp("stats"));
      // MOD: control port for burst_time_tagger_cc (t0_usrp, lead_s)
      message_port_register_out(pmt::mp("ctrl"));

      set_output_multiple(static_cast<int>(d_samples_per_slot));
      set_relative_rate(1.0);

      // MOD: publish initial control config for the tagger
      publish_ctrl_config_();
    }

    slot_guard_cc_impl::~slot_guard_cc_impl() noexcept
    {
    }

    // Time initialization
    void slot_guard_cc_impl::init_time_()
    {
      // Define host reference time
      d_t0_host = std::chrono::steady_clock::now();

      // Align USRP time to 0 or next PPS
      if (d_use_pps) {
        d_usrp->set_time_next_pps(uhd::time_spec_t(0.0));
        // In a real PPS setup you would sleep until the PPS edge here.
      } else {
        d_usrp->set_time_now(uhd::time_spec_t(0.0));
      }
      d_usrp->clear_command_time();

      // Measure initial bias between USRP and host clocks
      double t_host0 = now_host_seconds_();
      double t_usrp0 = usrp_now_seconds_();
      d_dt0_bias = t_usrp0 - t_host0;

      // Print once so you can inspect initial misalignment
      std::cout << "[slot_guard_cc] initial time bias dt0 = "
                << d_dt0_bias
                << " s (usrp - host)" << std::endl;

      d_time_init_done = true;
    }

    double slot_guard_cc_impl::now_host_seconds_() const
    {
      auto now = std::chrono::steady_clock::now();
      return std::chrono::duration<double>(now - d_t0_host).count();
    }

    double slot_guard_cc_impl::usrp_now_seconds_() const
    {
      uhd::time_spec_t t = d_usrp->get_time_now();
      return t.get_real_secs();
    }

    // Approximate samples_per_slot from numerology and sample rate
    void slot_guard_cc_impl::compute_slot_params_from_mu_()
    {
      static const double sym_ext_us[6] =
          {2150.0, 1075.0, 575.0, 287.0, 147.75, 71.9};

      int idx = (d_mu >= 0 && d_mu < 6) ? d_mu : 0;
      double sym_us = sym_ext_us[idx];
      double samples_per_symbol = d_fs * (sym_us * 1e-6);
      d_samples_per_slot = round_to_size(samples_per_symbol * 14.0);
    }

    void slot_guard_cc_impl::update_stats_(double dt)
    {
      d_last_offset_s = dt;
      d_dt_window.push_back(dt);
      while (d_dt_window.size() > d_dt_window_len) {
        d_dt_window.pop_front();
      }

      double mean = 0.0;
      for (double x : d_dt_window) {
        mean += x;
      }
      if (!d_dt_window.empty()) {
        mean /= static_cast<double>(d_dt_window.size());
      }

      double var = 0.0;
      for (double x : d_dt_window) {
        double e = x - mean;
        var += e * e;
      }
      if (!d_dt_window.empty()) {
        var /= static_cast<double>(d_dt_window.size());
      }

      d_last_jitter_s = std::sqrt(var);
    }

    // Three-region decision:
    //  1) PASS                if both offset & jitter inside PASS thresholds
    //  2) DTX_ZEROS           if outside PASS but not severe
    //  3) DONT_CONSUME        if severe && host is ahead && allow_dont_consume
    decision_t_cc slot_guard_cc_impl::decide_(double dt, double jitter)
    {
      const double abs_dt = std::fabs(dt);

      const bool offset_pass = (abs_dt < d_offset_thr_pass_s);
      const bool jitter_pass = (jitter < d_jitter_thr_pass_s);

      const bool offset_severe = (abs_dt >= d_offset_thr_dont_s);
      const bool jitter_severe = (jitter >= d_jitter_thr_dont_s);

      decision_t_cc proposal = DECISION_PASS;

      if (offset_pass && jitter_pass) {
        proposal = DECISION_PASS;
      } else {
        // Outside PASS region
        const bool severe = offset_severe || jitter_severe;

        if (severe &&
            d_allow_dont_consume &&
            dt < -d_offset_thr_dont_s)
        {
          proposal = DECISION_DONT_CONSUME;
        } else {
          proposal = DECISION_DTX_ZEROS;
        }
      }

      // Hysteresis by slots
      if (proposal == d_last_decision) {
        d_stable_counter =
            std::min(d_stable_counter + 1, d_hysteresis_slots);
      } else {
        if (d_stable_counter >= d_hysteresis_slots) {
          d_last_decision = proposal;
          d_stable_counter = 1;
        } else {
          d_stable_counter++;
        }
      }

      return d_last_decision;
    }

    size_t slot_guard_cc_impl::copy_pass_(const gr_complex* in,
                                          gr_complex* out,
                                          size_t n)
    {
      std::memcpy(out, in, n * sizeof(gr_complex));
      return n;
    }

    size_t slot_guard_cc_impl::emit_zeros_(gr_complex* out, size_t n)
    {
      std::fill(out, out + n, gr_complex(0.0f, 0.0f));
      return n;
    }

    // MOD: publish control dict "ctrl" for burst_time_tagger_cc
    void slot_guard_cc_impl::publish_ctrl_config_()
    {
      double guard_s;
      double lead_s;
      {
        boost::mutex::scoped_lock lock(d_mutex);
        guard_s = d_guard_initial_s;
        lead_s  = d_lead_s;
      }

      // Take current USRP time as reference
      double t_usrp_now = usrp_now_seconds_();
      double t0_cfg = t_usrp_now + guard_s;

      pmt::pmt_t dict = pmt::make_dict();
      dict = pmt::dict_add(dict, pmt::intern("t0_usrp"),
                           pmt::from_double(t0_cfg));
      dict = pmt::dict_add(dict, pmt::intern("lead_s"),
                           pmt::from_double(lead_s));

      std::cout << "[slot_guard_cc] publish ctrl: t0_usrp=" << t0_cfg
                << " s, lead_s=" << lead_s << " s" << std::endl;

      message_port_pub(pmt::mp("ctrl"), dict);
    }

    void slot_guard_cc_impl::forecast(int noutput_items,
                                      gr_vector_int &ninput_items_required)
    {
      size_t Nslot = d_samples_per_slot;
      if (Nslot == 0) {
        ninput_items_required[0] = noutput_items;
        return;
      }

      int slots_needed =
          std::max(1, noutput_items / static_cast<int>(Nslot));
      ninput_items_required[0] =
          slots_needed * static_cast<int>(Nslot);
    }

    int slot_guard_cc_impl::general_work(int noutput_items,
                                         gr_vector_int &ninput_items,
                                         gr_vector_const_void_star &input_items,
                                         gr_vector_void_star &output_items)
    {
      const gr_complex* in =
          static_cast<const gr_complex*>(input_items[0]);
      gr_complex* out =
          static_cast<gr_complex*>(output_items[0]);

      size_t Nslot = d_samples_per_slot;
      if (Nslot == 0) {
        consume(0, 0);
        return 0;
      }

      size_t can_out_slots = noutput_items / Nslot;
      size_t can_in_slots  = ninput_items[0] / Nslot;
      size_t slots_to_process = std::min(can_out_slots, can_in_slots);

      size_t consumed = 0;
      size_t produced = 0;

      for (size_t s = 0; s < slots_to_process; ++s) {
        double t_host = now_host_seconds_();
        double t_usrp = usrp_now_seconds_();

        // Raw offset between clocks
        double dt_raw = t_usrp - t_host;
        // Corrected offset removing initial bias
        double dt = dt_raw - d_dt0_bias;

        update_stats_(dt);
        decision_t_cc d = decide_(dt, d_last_jitter_s);

        const gr_complex* in_slot = in + consumed;
        gr_complex*       out_slot = out + produced;

        switch (d) {
          case DECISION_PASS: {
            size_t n = copy_pass_(in_slot, out_slot, Nslot);
            consumed += Nslot;
            produced += n;
            break;
          }
          case DECISION_DTX_ZEROS: {
            size_t n = emit_zeros_(out_slot, Nslot);
            produced += n;
            if (d_dtx_consumes_input) {
              consumed += Nslot;
            }
            break;
          }
          case DECISION_DONT_CONSUME: {
            // Backpressure: no consume, no produce, exit loop
            s = slots_to_process;
            break;
          }
        }

        // Publish stats for this slot (offset already bias-corrected)
        pmt::pmt_t dict = pmt::make_dict();
        dict = pmt::dict_add(dict, pmt::intern("slot_idx"),
                             pmt::from_long(static_cast<long>(s)));
        dict = pmt::dict_add(dict, pmt::intern("offset_s"),
                             pmt::from_double(d_last_offset_s));
        dict = pmt::dict_add(dict, pmt::intern("jitter_s"),
                             pmt::from_double(d_last_jitter_s));
        dict = pmt::dict_add(dict, pmt::intern("decision"),
                             pmt::from_long(static_cast<long>(d)));
        dict = pmt::dict_add(dict, pmt::intern("host_s"),
                             pmt::from_double(t_host));
        dict = pmt::dict_add(dict, pmt::intern("usrp_s"),
                             pmt::from_double(t_usrp));

        message_port_pub(pmt::mp("stats"), dict);
      }

      if (consumed > 0) {
        consume(0, static_cast<int>(consumed));
      }
      return static_cast<int>(produced);
    }

    // ==== Runtime setters (with mutex) ====

    void slot_guard_cc_impl::set_sample_rate(double sample_rate)
    {
      if (sample_rate <= 0.0)
        return;

      boost::mutex::scoped_lock lock(d_mutex);
      d_fs = sample_rate;
      if (d_samples_per_slot == 0) {
        compute_slot_params_from_mu_();
      }
      if (d_samples_per_slot != 0) {
        set_output_multiple(static_cast<int>(d_samples_per_slot));
      }
    }

    void slot_guard_cc_impl::set_numerology_id(int numerology_id)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_mu = numerology_id;
      if (d_samples_per_slot == 0) {
        compute_slot_params_from_mu_();
      }
      if (d_samples_per_slot != 0) {
        set_output_multiple(static_cast<int>(d_samples_per_slot));
      }
    }

    void slot_guard_cc_impl::set_samples_per_slot(size_t samples_per_slot)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_samples_per_slot = samples_per_slot;
      if (d_samples_per_slot == 0) {
        compute_slot_params_from_mu_();
      }
      if (d_samples_per_slot != 0) {
        set_output_multiple(static_cast<int>(d_samples_per_slot));
      }
    }

    void slot_guard_cc_impl::set_use_pps(bool use_pps)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_use_pps = use_pps;
    }

    void slot_guard_cc_impl::set_offset_thr_pass(double v)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_offset_thr_pass_s = (v > 0.0) ? v : DEFAULT_OFFSET_THR_PASS;
    }

    void slot_guard_cc_impl::set_jitter_thr_pass(double v)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_jitter_thr_pass_s = (v > 0.0) ? v : DEFAULT_JITTER_THR_PASS;
    }

    void slot_guard_cc_impl::set_offset_thr_dont(double v)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_offset_thr_dont_s = (v > 0.0) ? v : DEFAULT_OFFSET_THR_DONT;
    }

    void slot_guard_cc_impl::set_jitter_thr_dont(double v)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_jitter_thr_dont_s = (v > 0.0) ? v : DEFAULT_JITTER_THR_DONT;
    }

    void slot_guard_cc_impl::set_hysteresis_slots(int hysteresis_slots)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_hysteresis_slots = std::max(1, hysteresis_slots);
    }

    void slot_guard_cc_impl::set_dtx_consumes_input(bool dtx_consumes_input)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_dtx_consumes_input = dtx_consumes_input;
    }

    void slot_guard_cc_impl::set_allow_dont_consume(bool allow_dont_consume)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_allow_dont_consume = allow_dont_consume;
    }

    void slot_guard_cc_impl::set_guard_initial(double v)
    {
      {
        boost::mutex::scoped_lock lock(d_mutex);
        d_guard_initial_s = (v > 0.0) ? v : DEFAULT_GUARD_INITIAL_S;
      }
      // MOD: re-publish control config with new guard
      publish_ctrl_config_();
    }

    void slot_guard_cc_impl::set_lead(double v)
    {
      {
        boost::mutex::scoped_lock lock(d_mutex);
        d_lead_s = (v >= 0.0) ? v : DEFAULT_LEAD_S;
      }
      // MOD: re-publish control config with new lead
      publish_ctrl_config_();
    }

  } // namespace howto
} // namespace gr
