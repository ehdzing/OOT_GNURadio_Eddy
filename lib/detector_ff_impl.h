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

#ifndef INCLUDED_HOWTO_DETECTOR_FF_IMPL_H
#define INCLUDED_HOWTO_DETECTOR_FF_IMPL_H

#include <howto/detector_ff.h>
#include <boost/thread/mutex.hpp>
#include <vector>

namespace gr {
   namespace howto {

  /*!
  * \brief Implementation of detector_ff with O(1) rolling average and hysteresis.
  *
  * Key members (documented inline):
  *  - Thresholds: d_thr_high (high detect), d_thr_low (low detect)
  *  - Window: d_win (moving-average length)
  *  - Rolling state: d_buf (circular), d_head (index), d_sum (sum), d_primed (window filled)
  *  - Counters: d_count (total processed samples)
  *  - FSM: d_state (IDLE/ACTIVE)
  *  - PMT keys/symbols: k_event, k_level, v_START, v_STOP
  */
    class detector_ff_impl : public detector_ff
    {
      private:
        // -------- Parameter protection --------
        mutable boost::mutex d_mtx; // protects concurrent access to thresholds/window

        // -------- Detection parameters --------
        float d_thr_high;           // high detection threshold (START when avg > d_thr_high)
        float d_thr_low;            // low detection threshold (STOP  when avg < d_thr_low)
        int   d_win;                // moving-average window length (samples)

        // -------- Rolling average state --------
        std::vector<float> d_buf;   // circular buffer storing the last d_win samples
        int   d_head;               // circular buffer write index (0..d_win-1)
        bool  d_primed;             // true once at least d_win samples have been seen
        double d_sum;               // running sum of samples inside the window
        uint64_t d_count;           // total processed samples since start (monotonic)

        // -------- Finite state machine --------
        enum state_t { IDLE = 0, ACTIVE = 1 };
        state_t d_state;            // current detector state

        // -------- Cached PMT atoms --------
        pmt::pmt_t k_event;         // PMT key symbol: "event"
        pmt::pmt_t k_level;         // PMT key symbol: "level"
        pmt::pmt_t v_START;         // PMT value symbol: "START"
        pmt::pmt_t v_STOP;          // PMT value symbol: "STOP"

        // -------- Helpers --------
        void publish_event(bool start, double level);          // publish PMT dict event on port "out"
        void add_event_tag(uint64_t abs_off, bool start);      // insert stream tag at absolute offset

      public:
        detector_ff_impl(float thr_high, float thr_low, int win);
        ~detector_ff_impl() override;

        // -------- GNU Radio --------
        int work(int noutput_items,
                gr_vector_const_void_star &input_items,
                gr_vector_void_star &output_items) override;

        // -------- API --------
        void set_thresholds(float thr_high, float thr_low) override;
        void set_window(int win) override;
        float thr_high() const override;
        float thr_low()  const override;
        int   window()   const override;
    };

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_DETECTOR_FF_IMPL_H */

 /* INCLUDED_HOWTO_DETECTOR_FF_IMPL_H */

