/* -*- c++ -*- */
/* 
 * Copyright 2025 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* -*- c++ -*- */
/*
 * frame_gate_cc_impl.h
 */

#ifndef INCLUDED_HOWTO_FRAME_GATE_CC_IMPL_H
#define INCLUDED_HOWTO_FRAME_GATE_CC_IMPL_H

#include <howto/frame_gate_cc.h>
#include <boost/thread/mutex.hpp>

namespace gr {
  namespace howto {

    class frame_gate_cc_impl final : public frame_gate_cc
    {
     public:
      frame_gate_cc_impl(double samp_rate,
                         double period_s,
                         double gap_s,
                         int    burst_len,
                         double t0_usrp);

      ~frame_gate_cc_impl() noexcept override;

      // Runtime setters
      void set_samp_rate(double samp_rate) override;
      void set_period(double period_s) override;
      void set_gap(double gap_s) override;
      void set_burst_len(int burst_len) override;
      void set_t0_usrp(double t0_usrp) override;

      // Monitoring
      double get_samp_rate() const override;
      double get_period() const override;
      double get_gap() const override;
      int    get_burst_len() const override;
      double get_t0_usrp() const override;

      // gr::sync_block
      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items) override;

     private:
      // Parameters (protected by d_mutex when modified)
      double d_samp_rate;
      double d_period_s;   // active part (seconds)
      double d_gap_s;      // gap after active part (seconds)
      int    d_burst_len;  // active samples (if <=0, derived from period_s)
      double d_t0_usrp;    // absolute time for sample index 0 (seconds)

      double d_T_eff;      // period_s + gap_s

      // Global sample index (counting from 0)
      long long d_sample_idx;

      mutable boost::mutex d_mutex;

      void recompute_derived_nolock();
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_FRAME_GATE_CC_IMPL_H */
