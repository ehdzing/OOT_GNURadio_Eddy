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
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <cctype>

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

    static const double DEFAULT_GUARD_INITIAL_S = 0.5;   // 500 ms
    static const double DEFAULT_LEAD_S          = 2e-2;  // 20 ms

    // Helpers
    static inline void trim_str(std::string &s)
    {
      s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](unsigned char ch){ return !std::isspace(ch); }));
      s.erase(std::find_if(s.rbegin(), s.rend(),
            [](unsigned char ch){ return !std::isspace(ch); }).base(), s.end());
    }

    static inline std::string to_lower_copy(const std::string &s)
    {
      std::string out(s);
      std::transform(out.begin(), out.end(), out.begin(),
                     [](unsigned char c){ return std::tolower(c); });
      return out;
    }

    // Factory
    slot_guard_cc::sptr
    slot_guard_cc::make(double sample_rate,
                        int    numerology_id,
                        size_t samples_per_slot,
                        const std::string &use_pps,
                        double offset_thr_pass_s,
                        double jitter_thr_pass_s,
                        double offset_thr_dont_s,
                        double jitter_thr_dont_s,
                        int    hysteresis_slots,
                        bool   dtx_consumes_input,
                        bool   allow_dont_consume,
                        double guard_initial_s,
                        double lead_s,
                        const std::string &device_addrs)
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
                                 lead_s,
                                 device_addrs));
    }

    // Constructor
    slot_guard_cc_impl::slot_guard_cc_impl(double sample_rate,
                                           int    numerology_id,
                                           size_t samples_per_slot,
                                           const std::string &use_pps,
                                           double offset_thr_pass_s,
                                           double jitter_thr_pass_s,
                                           double offset_thr_dont_s,
                                           double jitter_thr_dont_s,
                                           int    hysteresis_slots,
                                           bool   dtx_consumes_input,
                                           bool   allow_dont_consume,
                                           double guard_initial_s,
                                           double lead_s,
                                           const std::string &device_addrs)
      : gr::block("slot_guard_cc",
                  gr::io_signature::make(1, 1, sizeof(gr_complex)),
                  gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_fs(sample_rate),
        d_mu(numerology_id),
        d_samples_per_slot(samples_per_slot),
        d_offset_thr_pass_s(offset_thr_pass_s),
        d_jitter_thr_pass_s(jitter_thr_pass_s),
        d_offset_thr_dont_s(offset_thr_dont_s),
        d_jitter_thr_dont_s(jitter_thr_dont_s),
        d_hysteresis_slots(std::max(1, hysteresis_slots)),
        d_dtx_consumes_input(dtx_consumes_input),
        d_allow_dont_consume(allow_dont_consume),
        d_guard_initial_s(guard_initial_s > 0.0 ? guard_initial_s
                                                : DEFAULT_GUARD_INITIAL_S),
        d_lead_s(lead_s >= 0.0 ? lead_s : DEFAULT_LEAD_S),
        d_use_pps_flags(),
        d_usrps(),
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

      if (d_offset_thr_pass_s <= 0.0)
        d_offset_thr_pass_s = DEFAULT_OFFSET_THR_PASS;
      if (d_jitter_thr_pass_s <= 0.0)
        d_jitter_thr_pass_s = DEFAULT_JITTER_THR_PASS;
      if (d_offset_thr_dont_s <= 0.0)
        d_offset_thr_dont_s = DEFAULT_OFFSET_THR_DONT;
      if (d_jitter_thr_dont_s <= 0.0)
        d_jitter_thr_dont_s = DEFAULT_JITTER_THR_DONT;

      // Parse per-USRP PPS flags from string
      std::vector<bool> parsed_flags;
      parse_use_pps_list_(use_pps, parsed_flags);

      // Create USRPs
      init_usrps_(device_addrs);

      // *** Give PLL time to lock if using GPSDO ***
      std::this_thread::sleep_for(std::chrono::seconds(3));

      // Build flags aligned with number of USRPs
      build_use_pps_flags_(parsed_flags);

      // Initialize time (set_time_next_pps / set_time_now)
      init_time_();

      // Slot size
      if (d_samples_per_slot == 0) {
        compute_slot_params_from_mu_();
      }
      if (d_samples_per_slot == 0) {
        throw std::runtime_error(
            "slot_guard_cc: samples_per_slot not set and numerology invalid");
      }

      message_port_register_out(pmt::mp("stats"));
      message_port_register_out(pmt::mp("ctrl"));

      set_output_multiple(static_cast<int>(d_samples_per_slot));
      set_relative_rate(1.0);

      publish_ctrl_config_();
    }

    slot_guard_cc_impl::~slot_guard_cc_impl() noexcept
    {
    }

    void slot_guard_cc_impl::parse_use_pps_list_(const std::string &use_pps_str,
                                                 std::vector<bool> &parsed_flags)
    {
      parsed_flags.clear();
      std::string s = use_pps_str;
      trim_str(s);

      if (s.empty()) {
        return;
      }

      // Remove brackets if present
      if (s.front() == '[' && s.back() == ']') {
        s = s.substr(1, s.size() - 2);
        trim_str(s);
        if (s.empty()) {
          return;
        }
      }

      std::stringstream ss(s);
      std::string token;
      while (std::getline(ss, token, ',')) {
        trim_str(token);
        if (token.empty())
          continue;

        // Strip quotes
        if (token.size() >= 2 &&
           ((token.front() == '"' && token.back() == '"') ||
            (token.front() == '\'' && token.back() == '\'')))
        {
          token = token.substr(1, token.size() - 2);
          trim_str(token);
          if (token.empty())
            continue;
        }

        std::string low = to_lower_copy(token);

        bool value_set = false;
        bool value = false;

        if (low == "true" || low == "t" || low == "1" ||
            low == "yes"  || low == "y")
        {
          value_set = true;
          value = true;
        }
        else if (low == "false" || low == "f" || low == "0" ||
                 low == "no"    || low == "n")
        {
          value_set = true;
          value = false;
        }

        if (value_set) {
          parsed_flags.push_back(value);
        }
      }

      if (!parsed_flags.empty()) {
        std::cout << "[slot_guard_cc] parse_use_pps_list_: parsed "
                  << parsed_flags.size()
                  << " PPS flags from string" << std::endl;
      }
    }

    void slot_guard_cc_impl::build_use_pps_flags_(const std::vector<bool> &parsed_flags)
    {
      const size_t N = d_usrps.size();
      d_use_pps_flags.clear();

      if (N == 0) {
        return;
      }

      if (parsed_flags.empty()) {
        // No PPS requested explicitly: all false
        std::cout << "[slot_guard_cc] build_use_pps_flags_: no PPS requested; "
                  << "all USRPs use set_time_now(0.0)" << std::endl;
        return;
      }

      d_use_pps_flags.assign(N, false);

      const size_t M = parsed_flags.size();
      const size_t K = (M < N) ? M : N;

      for (size_t i = 0; i < K; ++i) {
        d_use_pps_flags[i] = parsed_flags[i];
      }

      if (M < N) {
        std::cout << "[slot_guard_cc] build_use_pps_flags_: PPS list shorter than "
                  << "number of USRPs (" << M << " < " << N
                  << "); remaining devices default to False"
                  << std::endl;
      }

      std::cout << "[slot_guard_cc] build_use_pps_flags_: effective PPS flags: ";
      for (size_t i = 0; i < N; ++i) {
        bool flag = d_use_pps_flags[i];
        std::cout << (flag ? "1" : "0");
        if (i + 1 < N) std::cout << ",";
      }
      std::cout << std::endl;
    }

    void slot_guard_cc_impl::init_usrps_(const std::string &device_addrs)
    {
      d_usrps.clear();

      std::string s = device_addrs;
      trim_str(s);

      if (!s.empty() && s.front() == '[' && s.back() == ']') {
        s = s.substr(1, s.size() - 2);
      }

      std::vector<std::string> specs;
      if (!s.empty()) {
        std::stringstream ss(s);
        std::string token;
        while (std::getline(ss, token, ',')) {
          trim_str(token);
          if (token.empty())
            continue;

          if (token.size() >= 2 &&
             ((token.front() == '"' && token.back() == '"') ||
              (token.front() == '\'' && token.back() == '\'')))
          {
            token = token.substr(1, token.size() - 2);
            trim_str(token);
          }

          if (token.empty())
            continue;

          // If no '=', assume bare serial
          if (token.find('=') == std::string::npos) {
            token = std::string("serial=") + token;
          }

          specs.push_back(token);
        }
      }

      if (specs.empty()) {
        try {
          uhd::device_addr_t dev_addr;
          d_usrps.push_back(uhd::usrp::multi_usrp::make(dev_addr));
          std::cout << "[slot_guard_cc] init_usrps_: using default device discovery"
                    << std::endl;
        } catch (const std::exception &e) {
          std::cerr << "[slot_guard_cc] ERROR: cannot create default USRP: "
                    << e.what() << std::endl;
          throw;
        }
      } else {
        for (size_t i = 0; i < specs.size(); ++i) {
          try {
            uhd::device_addr_t dev_addr(specs[i]);
            uhd::usrp::multi_usrp::sptr usrp =
                uhd::usrp::multi_usrp::make(dev_addr);
            d_usrps.push_back(usrp);
            std::cout << "[slot_guard_cc] init_usrps_: created USRP["
                      << i << "] with addr \"" << specs[i] << "\""
                      << std::endl;
          } catch (const std::exception &e) {
            std::cerr << "[slot_guard_cc] ERROR: cannot create USRP for addr \""
                      << specs[i] << "\": " << e.what() << std::endl;
            throw;
          }
        }
      }

      // Try to use gpsdo as clock/time source; failures are just warnings.
      for (size_t i = 0; i < d_usrps.size(); ++i) {
        uhd::usrp::multi_usrp::sptr usrp = d_usrps[i];

        try {
          usrp->set_clock_source("gpsdo");
          if (i == 0) {
            std::cout << "[slot_guard_cc] clock_source set to 'gpsdo'" << std::endl;
          }
        } catch (const std::exception &e) {
          std::cerr << "[slot_guard_cc] WARNING: set_clock_source('gpsdo') failed on USRP["
                    << i << "]: " << e.what() << std::endl;
        }

        try {
          usrp->set_time_source("gpsdo");
          if (i == 0) {
            std::cout << "[slot_guard_cc] time_source = '"
                      << usrp->get_time_source(0) << "'" << std::endl;
            std::cout << "[slot_guard_cc] clock_source (query) = '"
                      << usrp->get_clock_source(0) << "'" << std::endl;
          }
        } catch (const std::exception &e) {
          std::cerr << "[slot_guard_cc] WARNING: setting time_source('gpsdo') failed on USRP["
                    << i << "]: " << e.what() << std::endl;
        }
      }
    }

    uhd::usrp::multi_usrp::sptr slot_guard_cc_impl::primary_usrp_() const
    {
      if (d_usrps.empty()) {
        return uhd::usrp::multi_usrp::sptr();
      }
      return d_usrps.front(); // USRP[0] = referencia (TX)
    }

    void slot_guard_cc_impl::init_time_()
    {
      d_t0_host = std::chrono::steady_clock::now();

      const size_t N = d_usrps.size();
      if (N == 0) {
        std::cerr << "[slot_guard_cc] ERROR: init_time_() called with no USRPs"
                  << std::endl;
        return;
      }

      std::vector<bool> use_pps_i(N, false);
      if (!d_use_pps_flags.empty()) {
        const size_t K = (d_use_pps_flags.size() < N) ? d_use_pps_flags.size() : N;
        for (size_t i = 0; i < K; ++i) {
          use_pps_i[i] = d_use_pps_flags[i];
        }
      }

      std::vector<bool> pps_ok(N, false);
      bool any_pps_requested = false;
      bool any_pps_ok = false;

      // Check PPS only where requested
      for (size_t i = 0; i < N; ++i) {
        if (!use_pps_i[i])
          continue;

        any_pps_requested = true;

        try {
          std::vector<std::string> sns = d_usrps[i]->get_mboard_sensor_names(0);
          for (size_t k = 0; k < sns.size(); ++k) {
            const std::string &name = sns[k];

            if (name == "gps_locked" ||
                name == "gpsdo_locked" ||
                name == "pps_locked")
            {
              uhd::sensor_value_t sv = d_usrps[i]->get_mboard_sensor(name, 0);
              std::cout << "[slot_guard_cc] USRP[" << i << "] sensor " << name
                        << " = " << sv.to_pp_string() << std::endl;

              if (sv.to_bool()) {
                pps_ok[i] = true;
                any_pps_ok = true;
              }
            }
            else if (name == "ref_locked" || name == "10mhz_locked") {
              uhd::sensor_value_t sv = d_usrps[i]->get_mboard_sensor(name, 0);
              std::cout << "[slot_guard_cc] USRP[" << i << "] sensor " << name
                        << " = " << sv.to_pp_string() << std::endl;
            }
          }
        } catch (const std::exception &e) {
          std::cerr << "[slot_guard_cc] WARNING: sensor check for PPS failed on USRP["
                    << i << "]: " << e.what() << std::endl;
        }
      }

      if (any_pps_ok) {
        std::cout << "[slot_guard_cc] init_time_: using set_time_next_pps(0.0) "
                  << "on USRPs with valid PPS lock, and set_time_now(0.0) "
                  << "on remaining devices after PPS edge"
                  << std::endl;

        // Program next PPS reset on PPS-locked devices
        for (size_t i = 0; i < N; ++i) {
          if (!pps_ok[i])
            continue;
          try {
            d_usrps[i]->set_time_next_pps(uhd::time_spec_t(0.0));
          } catch (const std::exception &e) {
            std::cerr << "[slot_guard_cc] WARNING: set_time_next_pps(0.0) failed on USRP["
                      << i << "]: " << e.what() << std::endl;
          }
        }

        // Wait PPS edge
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        d_t0_host = std::chrono::steady_clock::now();

        // Devices sin PPS lock o sin PPS solicitado: set_time_now(0.0)
        for (size_t i = 0; i < N; ++i) {
          if (pps_ok[i])
            continue;
          try {
            d_usrps[i]->set_time_now(uhd::time_spec_t(0.0));
          } catch (const std::exception &e) {
            std::cerr << "[slot_guard_cc] WARNING: set_time_now(0.0) failed on USRP["
                      << i << "]: " << e.what() << std::endl;
          }
        }
      } else {
        if (any_pps_requested) {
          std::cerr << "[slot_guard_cc] WARNING: PPS requested but no USRP has "
                       "valid PPS/GPSDO lock; using set_time_now(0.0) on all"
                    << std::endl;
        } else {
          std::cout << "[slot_guard_cc] init_time_: PPS not requested; using "
                    << "set_time_now(0.0) on all devices"
                    << std::endl;
        }

        for (size_t i = 0; i < N; ++i) {
          try {
            d_usrps[i]->set_time_now(uhd::time_spec_t(0.0));
          } catch (const std::exception &e) {
            std::cerr << "[slot_guard_cc] WARNING: set_time_now(0.0) failed on USRP["
                      << i << "]: " << e.what() << std::endl;
          }
        }
      }

      // Clear command time
      for (size_t i = 0; i < N; ++i) {
        try {
          d_usrps[i]->clear_command_time();
        } catch (const std::exception &e) {
          std::cerr << "[slot_guard_cc] WARNING: clear_command_time() failed on USRP["
                    << i << "]: " << e.what() << std::endl;
        }
      }

      double t_host0 = now_host_seconds_();
      double t_usrp0 = usrp_now_seconds_();
      d_dt0_bias = t_usrp0 - t_host0;

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
      uhd::usrp::multi_usrp::sptr usrp0 = primary_usrp_();
      if (!usrp0) {
        return 0.0;
      }
      uhd::time_spec_t t = usrp0->get_time_now();
      return t.get_real_secs();
    }

    void slot_guard_cc_impl::compute_slot_params_from_mu_()
    {
      static const double sym_ext_us[6] =
          {2150.0, 1075.0, 575.0, 287.0, 147.75, 71.9};

      int idx = (d_mu >= 0 && d_mu < 6) ? d_mu : 0;
      double sym_us = sym_ext_us[idx];
      double samples_per_symbol = d_fs * (sym_us * 1e-6);
      d_samples_per_slot = round_to_size(samples_per_symbol * 14.0);
    }

    void slot_guard_cc_impl::update_stats_(double dt_raw)
    {
      // Smoothing factor for long-term bias between host and USRP clocks.
      // For ~1 ms slots and 10 ppm drift, alpha ~1e-3 keeps the residual
      // offset below ~10 us while reacting on a ~1 s time scale.
      static const double alpha = 1e-3;

      d_dt0_bias = (1.0 - alpha) * d_dt0_bias + alpha * dt_raw;

      const double dt = dt_raw - d_dt0_bias;

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

    void slot_guard_cc_impl::publish_ctrl_config_()
    {
      double guard_s;
      double lead_s;
      {
        boost::mutex::scoped_lock lock(d_mutex);
        guard_s = d_guard_initial_s;
        lead_s  = d_lead_s;
      }

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

        double dt_raw = t_usrp - t_host;

        update_stats_(dt_raw);
        decision_t_cc d = decide_(d_last_offset_s, d_last_jitter_s);

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
            s = slots_to_process;
            break;
          }
        }

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

    // ==== Runtime setters ====

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

    void slot_guard_cc_impl::set_use_pps(const std::string &use_pps)
    {
      std::vector<bool> parsed_flags;
      parse_use_pps_list_(use_pps, parsed_flags);
      build_use_pps_flags_(parsed_flags);
      // Nota: no re-llamamos init_time_() en runtime; para cambios
      // profundos de sincronismo, recrear el bloque es lo sensato.
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
      publish_ctrl_config_();
    }

    void slot_guard_cc_impl::set_lead(double v)
    {
      {
        boost::mutex::scoped_lock lock(d_mutex);
        d_lead_s = (v >= 0.0) ? v : DEFAULT_LEAD_S;
      }
      publish_ctrl_config_();
    }

  } // namespace howto
} // namespace gr
