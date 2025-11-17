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
 * frame_gate_cc.h
 *
 * Periodic frame gate: passes signal during active frame window
 * and emits zeros during the configured gap.
 */

#ifndef INCLUDED_HOWTO_FRAME_GATE_CC_H
#define INCLUDED_HOWTO_FRAME_GATE_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace howto {

    /*!
     * \brief Periodic frame gate: passes signal during active frame,
     *        emits zeros during gap.
     *
     * Time model:
     *
     *   - samp_rate (Hz)
     *   - active period:  period_s  (e.g. 10 ms)
     *   - gap after:      gap_s     (e.g. 5 ms)
     *   - effective period: T_eff = period_s + gap_s
     *   - t0_usrp: absolute time (seconds) for global sample index 0
     *
     * For each integer k = 0,1,2,...:
     *
     *   frame k starts at:       t_k = t0_usrp + k * T_eff
     *   active region:           [t_k, t_k + period_s)
     *   gap region:              [t_k + period_s, t_k + T_eff)
     *
     * Mapping:
     *
     *   t = t0_usrp + n / samp_rate
     *
     * If t is inside active region => out[n] = in[n]
     * If t is inside gap           => out[n] = 0
     *
     * This block does not insert UHD tags; it only shapes the baseband.
     */
    class HOWTO_API frame_gate_cc : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<frame_gate_cc> sptr;

      static sptr make(double samp_rate,
                       double period_s,
                       double gap_s,
                       int    burst_len,
                       double t0_usrp);

      // Runtime setters (thread-safe)
      virtual void set_samp_rate(double samp_rate) = 0;
      virtual void set_period(double period_s) = 0;
      virtual void set_gap(double gap_s) = 0;
      virtual void set_burst_len(int burst_len) = 0;
      virtual void set_t0_usrp(double t0_usrp) = 0;

      // Monitoring
      virtual double get_samp_rate() const = 0;
      virtual double get_period() const = 0;
      virtual double get_gap() const = 0;
      virtual int    get_burst_len() const = 0;
      virtual double get_t0_usrp() const = 0;

     protected:
      frame_gate_cc() = default;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_FRAME_GATE_CC_H */
