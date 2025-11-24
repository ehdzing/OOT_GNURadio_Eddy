/* -*- c++ -*- */
/* 
 * slot_guard_cc_impl.h
 */

#ifndef INCLUDED_HOWTO_SLOT_GUARD_CC_IMPL_H
#define INCLUDED_HOWTO_SLOT_GUARD_CC_IMPL_H

#include <howto/slot_guard_cc.h>
#include <deque>
#include <chrono>
#include <boost/thread/mutex.hpp>

namespace gr {
  namespace howto {

    class slot_guard_cc_impl final : public slot_guard_cc
    {
     public:
      slot_guard_cc_impl(double sample_rate,
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
                         double guard_initial_s,  // MOD
                         double lead_s);          // MOD

      ~slot_guard_cc_impl() noexcept override;

      // gr::block API
      void forecast(int noutput_items,
                    gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items) override;

      // Monitoring
      double last_offset_seconds() const override { return d_last_offset_s; }
      double last_jitter_seconds() const override { return d_last_jitter_s; }
      int    last_decision_code() const override { return static_cast<int>(d_last_decision); }
      size_t samples_per_slot() const override { return d_samples_per_slot; }

      double offset_thr_pass_seconds() const override { return d_offset_thr_pass_s; }
      double jitter_thr_pass_seconds() const override { return d_jitter_thr_pass_s; }
      double offset_thr_dont_seconds() const override { return d_offset_thr_dont_s; }
      double jitter_thr_dont_seconds() const override { return d_jitter_thr_dont_s; }

      double guard_initial_seconds() const override { return d_guard_initial_s; } // MOD
      double lead_seconds() const override { return d_lead_s; }                   // MOD

      // Runtime setters
      void set_sample_rate(double sample_rate) override;
      void set_numerology_id(int numerology_id) override;
      void set_samples_per_slot(size_t samples_per_slot) override;
      void set_use_pps(bool use_pps) override;

      void set_offset_thr_pass(double v) override;
      void set_jitter_thr_pass(double v) override;
      void set_offset_thr_dont(double v) override;
      void set_jitter_thr_dont(double v) override;

      void set_hysteresis_slots(int hysteresis_slots) override;
      void set_dtx_consumes_input(bool dtx_consumes_input) override;
      void set_allow_dont_consume(bool allow_dont_consume) override;

      void set_guard_initial(double v) override;  // MOD
      void set_lead(double v) override;          // MOD

     private:
      // Configuration (protected by d_mutex when modified)
      double d_fs;
      int    d_mu;
      size_t d_samples_per_slot;
      bool   d_use_pps;

      double d_offset_thr_pass_s;
      double d_jitter_thr_pass_s;
      double d_offset_thr_dont_s;
      double d_jitter_thr_dont_s;

      int    d_hysteresis_slots;
      bool   d_dtx_consumes_input;
      bool   d_allow_dont_consume;

      // MOD: configuration for initial guard and per-burst lead
      double d_guard_initial_s;
      double d_lead_s;

      // USRP / timing
      uhd::usrp::multi_usrp::sptr d_usrp;
      std::chrono::steady_clock::time_point d_t0_host;
      bool   d_time_init_done;

      // Initial host–USRP time bias (usrp_now - host_now at init)
      double d_dt0_bias;


      // State
      decision_t_cc  d_last_decision;
      int            d_stable_counter;

      std::deque<double> d_dt_window;
      size_t             d_dt_window_len;
      double             d_last_offset_s;
      double             d_last_jitter_s;

      mutable boost::mutex d_mutex;

      // Internal helpers
      void   init_time_();
      void   compute_slot_params_from_mu_();
      double now_host_seconds_() const;
      double usrp_now_seconds_() const;

      void   update_stats_(double dt_raw);
      decision_t_cc decide_(double dt, double jitter);

      size_t copy_pass_(const gr_complex* in, gr_complex* out, size_t n);
      size_t emit_zeros_(gr_complex* out, size_t n);

      // MOD: publish control dict (t0_usrp, lead_s) on "ctrl" port
      void   publish_ctrl_config_();
    };

  } // namespace howto
} // namespace gr

#endif // INCLUDED_HOWTO_SLOT_GUARD_CC_IMPL_H
