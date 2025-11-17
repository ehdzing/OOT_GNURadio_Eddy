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
 * frame_gate_cc_impl.cc
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "frame_gate_cc_impl.h"

#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace gr {
  namespace howto {

    namespace {
      inline long long my_llround(double x)
      {
        return static_cast<long long>(std::llround(x));
      }
    } // anonymous namespace

    // Factory
    frame_gate_cc::sptr
    frame_gate_cc::make(double samp_rate,
                        double period_s,
                        double gap_s,
                        int    burst_len,
                        double t0_usrp)
    {
      return gnuradio::get_initial_sptr(
          new frame_gate_cc_impl(samp_rate,
                                 period_s,
                                 gap_s,
                                 burst_len,
                                 t0_usrp));
    }

    // Constructor
    frame_gate_cc_impl::frame_gate_cc_impl(double samp_rate,
                                           double period_s,
                                           double gap_s,
                                           int    burst_len,
                                           double t0_usrp)
      : gr::sync_block("frame_gate_cc",
                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                       gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_samp_rate(samp_rate),
        d_period_s(period_s),
        d_gap_s(gap_s),
        d_burst_len(burst_len),
        d_t0_usrp(t0_usrp),
        d_T_eff(0.0),
        d_sample_idx(0),
        d_mutex()
    {
      if (d_samp_rate <= 0.0) {
        throw std::runtime_error("frame_gate_cc: samp_rate must be > 0");
      }
      if (d_period_s <= 0.0) {
        throw std::runtime_error("frame_gate_cc: period_s must be > 0");
      }
      if (d_gap_s < 0.0) {
        throw std::runtime_error("frame_gate_cc: gap_s must be >= 0");
      }

      {
        boost::mutex::scoped_lock lock(d_mutex);
        recompute_derived_nolock();
      }

      // No specific output_multiple required:
      // we gate sample by sample according to time.
      set_output_multiple(1);
    }

    frame_gate_cc_impl::~frame_gate_cc_impl() noexcept
    {
    }

    void
    frame_gate_cc_impl::recompute_derived_nolock()
    {
      d_T_eff = d_period_s + d_gap_s;
      if (d_T_eff <= 0.0) {
        d_T_eff = d_period_s;
      }

      if (d_burst_len <= 0) {
        const double active_samps = d_period_s * d_samp_rate;
        d_burst_len = static_cast<int>(my_llround(active_samps));
        if (d_burst_len <= 0) {
          d_burst_len = 1;
        }
      }
    }

    // Runtime setters
    void
    frame_gate_cc_impl::set_samp_rate(double samp_rate)
    {
      if (samp_rate <= 0.0) {
        return;
      }

      boost::mutex::scoped_lock lock(d_mutex);
      d_samp_rate = samp_rate;
      recompute_derived_nolock();
    }

    void
    frame_gate_cc_impl::set_period(double period_s)
    {
      if (period_s <= 0.0) {
        return;
      }

      boost::mutex::scoped_lock lock(d_mutex);
      d_period_s = period_s;
      recompute_derived_nolock();
    }

    void
    frame_gate_cc_impl::set_gap(double gap_s)
    {
      if (gap_s < 0.0) {
        return;
      }

      boost::mutex::scoped_lock lock(d_mutex);
      d_gap_s = gap_s;
      recompute_derived_nolock();
    }

    void
    frame_gate_cc_impl::set_burst_len(int burst_len)
    {
      if (burst_len <= 0) {
        return;
      }

      boost::mutex::scoped_lock lock(d_mutex);
      d_burst_len = burst_len;
      // d_T_eff does not depend directly on burst_len.
    }

    void
    frame_gate_cc_impl::set_t0_usrp(double t0_usrp)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_t0_usrp = t0_usrp;
    }

    // Monitoring
    double
    frame_gate_cc_impl::get_samp_rate() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_samp_rate;
    }

    double
    frame_gate_cc_impl::get_period() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_period_s;
    }

    double
    frame_gate_cc_impl::get_gap() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_gap_s;
    }

    int
    frame_gate_cc_impl::get_burst_len() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_burst_len;
    }

    double
    frame_gate_cc_impl::get_t0_usrp() const
    {
      boost::mutex::scoped_lock lock(d_mutex);
      return d_t0_usrp;
    }

    int
    frame_gate_cc_impl::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items)
    {
      const gr_complex* in =
          static_cast<const gr_complex*>(input_items[0]);
      gr_complex* out =
          static_cast<gr_complex*>(output_items[0]);

      // Snapshot under mutex
      double samp_rate;
      double period_s;
      double gap_s;
      double t0_usrp;
      double T_eff;

      {
        boost::mutex::scoped_lock lock(d_mutex);
        samp_rate = d_samp_rate;
        period_s  = d_period_s;
        gap_s     = d_gap_s;
        t0_usrp   = d_t0_usrp;
        T_eff     = d_T_eff;
      }

      (void)gap_s;

      if (samp_rate <= 0.0 || period_s <= 0.0 || T_eff <= 0.0) {
        // Degenerate config: simple pass-through
        std::memcpy(out, in, noutput_items * sizeof(gr_complex));
        d_sample_idx += static_cast<long long>(noutput_items);
        consume_each(noutput_items);
        return noutput_items;
      }

      for (int i = 0; i < noutput_items; ++i) {
        const long long n = d_sample_idx + static_cast<long long>(i);

        // Absolute time for sample n
        const double t = t0_usrp + static_cast<double>(n) / samp_rate;
        const double delta = t - t0_usrp;

        if (delta < 0.0) {
          // Before t0_usrp: treat as gap (mute)
          out[i] = gr_complex(0.0f, 0.0f);
          continue;
        }

        const double k_real = delta / T_eff;
        const long long k = (k_real >= 0.0)
                                ? static_cast<long long>(std::floor(k_real))
                                : 0;

        const double t_frame_start = t0_usrp + static_cast<double>(k) * T_eff;
        const double t_frame_active_end = t_frame_start + period_s;

        if (t >= t_frame_start && t < t_frame_active_end) {
          // Active region: pass signal
          out[i] = in[i];
        } else {
          // Gap region: emit zeros (DTX)
          out[i] = gr_complex(0.0f, 0.0f);
        }
      }

      d_sample_idx += static_cast<long long>(noutput_items);
      consume_each(noutput_items);
      return noutput_items;
    }

  } // namespace howto
} // namespace gr
