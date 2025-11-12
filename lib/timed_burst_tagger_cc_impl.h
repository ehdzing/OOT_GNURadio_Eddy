/* -*- c++ -*- */
/* Copyright 2025 */
#ifndef INCLUDED_HOWTO_TIMED_BURST_TAGGER_CC_IMPL_H
#define INCLUDED_HOWTO_TIMED_BURST_TAGGER_CC_IMPL_H

#include <howto/timed_burst_tagger_cc.h>
#include <boost/thread/mutex.hpp>
#include <vector>
#include <iostream>

// NEW: reloj HW del USRP y sleep troceado 
#include <uhd/usrp/multi_usrp.hpp>   // NEW
#include <boost/thread.hpp>          // NEW
#include <boost/chrono.hpp>          // NEW

using namespace std;
namespace gr {
  namespace howto {

    class timed_burst_tagger_cc_impl : public timed_burst_tagger_cc
    {
     private:
      // Thread-guarded parameters
      mutable boost::mutex d_mtx;
      double d_rate;                   // [S/s]
      double d_burst_len_s;            // [s]
      double d_period_s;               // [s]
      std::vector<double> d_offsets_s; // [s]
      float  d_amplitude;              // linear
      double d_t0_hw;                  // absolute hw time anchor [s]
      bool   d_eob_en;
      bool   d_drop_outside;

      // Derived
      int    d_burst_len_n;            // samples
      // Runtime
      size_t d_next_event;             // index in offsets
      double d_base_period;            // m * period_s

      // Helpers
      static std::vector<double> parse_offsets_csv(const std::string& s);
      inline void add_out_tag_(const pmt::pmt_t& key, const pmt::pmt_t& val, uint64_t abs_out_idx);
      void forward_input_tags_(uint64_t in_abs_base, int in_offset0, int ncopy,
                               uint64_t out_abs_base, int out_offset0);

      // NEW: USRP para leer tiempo HW (independiente del sink)
      uhd::usrp::multi_usrp::sptr d_usrp;   // NEW

      // NEW: parámetros de pacing determinista (sin mover t_tx)
      double d_lead_target = 0.050;   // 50 ms de colchón objetivo        // NEW
      double d_recheck     = 0.002;   // 2 ms porción de sleep para re-medición // NEW

      // NEW: dormir en trozos hasta acercarnos al lead objetivo
      inline void sleep_until_lead(double t_tx) { 
        std::cout << "EStoy durmiendo: Host adelantado a USRP"<< endl;                         // NEW
        while (true) {
          const double t_hw   = d_usrp->get_time_now().get_real_secs();
          const double lead_s = t_tx - t_hw;
          if (lead_s <= d_lead_target) break;      // suficiente colchón
          const double want  = lead_s - d_lead_target;
          const double chunk = (want < d_recheck) ? want : d_recheck;
          if (chunk <= 0) break;
          boost::this_thread::sleep_for( boost::chrono::duration<double>(chunk) );
        }
      } // NEW
      
      // NEW [Deadline]: umbral de “slot perdido” por llegar tarde
      double d_deadline_s = 0.015;  // 15 ms; si lead < -d_deadline_s => DTX  // NEW [Deadline]
      
     public:
      timed_burst_tagger_cc_impl(double rate,
                                 double burst_len_s,
                                 double period_s,
                                 std::vector<double> offsets_s,
                                 float amplitude,
                                 double t0_hw,
                                 bool eob_en);

      ~timed_burst_tagger_cc_impl() override {}

      // general block API
      void forecast (int noutput_items, gr_vector_int &ninput_items_required) override;

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items) override;

      // Setters
      void set_rate(double r) override;
      void set_burst_len(double s) override;
      void set_period(double s) override;
      void set_offsets(const std::vector<double>& v) override;
      void set_amplitude(float a) override;
      void set_t0_hw(double t0) override;
      void set_eob_en(bool en) override;
      void set_drop_outside(bool en) override;

      // Getters
      double rate() const override;
      double burst_len() const override;
      double period() const override;
      std::vector<double> get_offsets() const override;
      float  amplitude() const override;
      double t0_hw() const override;
      bool   eob_en() const override;
      bool   drop_outside() const override;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_TIMED_BURST_TAGGER_CC_IMPL_H */
