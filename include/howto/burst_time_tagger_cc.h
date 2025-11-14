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
#ifndef INCLUDED_HOWTO_BURST_TIME_TAGGER_CC_H
#define INCLUDED_HOWTO_BURST_TIME_TAGGER_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <vector>

namespace gr {
  namespace howto {

    /*!
     * \brief Burst time tagger block for UHD-based transmission (with offsets).
     *
     * This block:
     *  - passes input complex samples to the output (1:1),
     *  - inserts UHD-compatible tags:
     *        "tx_time" at the start of each burst,
     *        "tx_sob"  at the start of each burst,
     *        "tx_eob"  at the end  of each burst.
     *
     * Bursts are defined by:
     *   - sample rate (samp_rate),
     *   - a base period in seconds (period_s),
     *   - an optional silent gap between periods (gap_s >= 0),
     *   - a list of offsets (offsets_s) inside each *active* period,
     *   - burst length in samples (burst_len),
     *   - USRP time reference t0_usrp.
     *
     * Effective repetition period is:
     *   T_effective = period_s + gap_s
     *
     * For each period k (k = 0, 1, 2, ...), and each offset o in offsets_s,
     * a burst starts at:
     *     t = t0_usrp + k*T_effective + o
     *
     * Special case:
     *   - If offsets_s is empty, the block behaves as:
     *       offsets_s = [0.0]
     *     i.e. one burst per active period starting at the beginning.
     */
    class HOWTO_API burst_time_tagger_cc : public virtual gr::sync_block
    {
     public:
      typedef boost::shared_ptr<burst_time_tagger_cc> sptr;

      /*!
       * Legacy factory:
       * \param samp_rate   sample rate in Hz.
       * \param period_s    active part of the period in seconds.
       * \param burst_len   number of samples per burst.
       * \param t0_usrp     USRP absolute time at sample index 0 (seconds).
       * \param offsets_s   list of offsets inside each active period (in seconds).
       * \note gap_s is assumed to be 0.0.
       */
      static sptr make(double samp_rate,
                       double period_s,
                       int    burst_len,
                       double t0_usrp,
                       const std::vector<double> &offsets_s);

      /*!
       * Extended factory with explicit gap between periods.
       *
       * \param samp_rate   sample rate in Hz.
       * \param period_s    active part of the period in seconds (e.g. frame length).
       * \param gap_s       silent gap after the active part (seconds). Must be >= 0.
       * \param burst_len   number of samples per burst.
       * \param t0_usrp     USRP absolute time at sample index 0 (seconds).
       * \param offsets_s   list of offsets inside each active period (in seconds).
       */
      static sptr make(double samp_rate,
                       double period_s,
                       double gap_s,
                       int    burst_len,
                       double t0_usrp,
                       const std::vector<double> &offsets_s);

      // Runtime setters
      virtual void set_samp_rate(double samp_rate) = 0;
      virtual void set_period(double period_s) = 0;
      virtual void set_gap(double gap_s) = 0;
      virtual void set_burst_len(int burst_len) = 0;
      virtual void set_t0_usrp(double t0_usrp) = 0;
      virtual void set_offsets(const std::vector<double> &offsets_s) = 0;

      // Runtime getters (for external inspection / MAC control logic)
      virtual double get_samp_rate() const = 0;
      virtual double get_period() const = 0;   //!< active part of the period
      virtual double get_gap() const = 0;      //!< silent gap after active part
      virtual int    get_burst_len() const = 0;
      virtual double get_t0_usrp() const = 0;
      virtual std::vector<double> get_offsets() const = 0;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_BURST_TIME_TAGGER_CC_H */
