/* -*- c++ -*- */
/* 
 * slot_guard_cc.h
 */

#ifndef INCLUDED_HOWTO_SLOT_GUARD_CC_H
#define INCLUDED_HOWTO_SLOT_GUARD_CC_H

#include <howto/api.h>
#include <gnuradio/block.h>
#include <uhd/usrp/multi_usrp.hpp>
#include <chrono>

namespace gr {
  namespace howto {

    //! Slot-level decision codes
    enum decision_t_cc {
      DECISION_PASS = 0,
      DECISION_DTX_ZEROS = 1,
      DECISION_DONT_CONSUME = 2
    };

    /*!
     * \brief Slot-level guard for USRP/host time drift.
     *
     * This block:
     *  - splits the stream into slots of N samples,
     *  - measures USRP vs host time for each slot,
     *  - tracks jitter on a sliding window,
     *  - decides per-slot:
     *       PASS / DTX_ZEROS / DONT_CONSUME
     *    using two sets of thresholds:
     *       - PASS thresholds (offset_thr_pass_s, jitter_thr_pass_s)
     *       - DONT_CONSUME thresholds (offset_thr_dont_s, jitter_thr_dont_s)
     *
     * Between those two regions, DTX_ZEROS is used.
     *
     * The block also publishes:
     *   - a PMT dictionary on message port "stats" for each slot,
     *   - a control PMT dictionary on message port "ctrl" with:
     *       "t0_usrp" and "lead_s" (MOD) used to configure the burst tagger.
     */
    class HOWTO_API slot_guard_cc : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<slot_guard_cc> sptr;

      static sptr make(double sample_rate,
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
                       double guard_initial_s,   // MOD
                       double lead_s);           // MOD

      // Monitoring
      virtual double last_offset_seconds() const = 0;
      virtual double last_jitter_seconds() const = 0;
      virtual int    last_decision_code() const = 0;
      virtual size_t samples_per_slot() const = 0;

      virtual double offset_thr_pass_seconds() const = 0;
      virtual double jitter_thr_pass_seconds() const = 0;
      virtual double offset_thr_dont_seconds() const = 0;
      virtual double jitter_thr_dont_seconds() const = 0;

      // MOD: monitoring of guard / lead configuration
      virtual double guard_initial_seconds() const = 0;
      virtual double lead_seconds() const = 0;

      // Runtime setters (thread-safe)
      virtual void set_sample_rate(double sample_rate) = 0;
      virtual void set_numerology_id(int numerology_id) = 0;
      virtual void set_samples_per_slot(size_t samples_per_slot) = 0;
      virtual void set_use_pps(bool use_pps) = 0;

      virtual void set_offset_thr_pass(double v) = 0;
      virtual void set_jitter_thr_pass(double v) = 0;
      virtual void set_offset_thr_dont(double v) = 0;
      virtual void set_jitter_thr_dont(double v) = 0;

      virtual void set_hysteresis_slots(int hysteresis_slots) = 0;
      virtual void set_dtx_consumes_input(bool dtx_consumes_input) = 0;
      virtual void set_allow_dont_consume(bool allow_dont_consume) = 0;

      // MOD: runtime setters for guard / lead
      virtual void set_guard_initial(double v) = 0;
      virtual void set_lead(double v) = 0;
    };

  } // namespace howto
} // namespace gr

#endif // INCLUDED_HOWTO_SLOT_GUARD_CC_H
