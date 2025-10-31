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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "detector_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <boost/thread/lock_guard.hpp>
#include <algorithm>


namespace gr { 
  namespace howto {

    // ---------- Factory ----------
    detector_ff::sptr detector_ff::make(float thr_high, float thr_low, int win)
    {
      return gnuradio::get_initial_sptr(new detector_ff_impl(thr_high, thr_low, win));
    }

    // ---------- Constructor ----------
    detector_ff_impl::detector_ff_impl(float thr_high, float thr_low, int win)
    : gr::sync_block("detector_ff",
        gr::io_signature::make(1, 1, sizeof(float)), // 1 float input
        gr::io_signature::make(1, 1, sizeof(float))),// 1 float output (passthrough)
      d_thr_high(std::max(thr_high, thr_low)),        // ensure high ≥ low
      d_thr_low(std::min(thr_high, thr_low)),
      d_win(std::max(2, win)),                        // minimal window length is 2
      d_buf(std::max(2, win), 0.0f),                  // allocate circular buffer
      d_head(0),                                      // start at index 0
      d_primed(false),                                // not primed until d_win samples processed
      d_sum(0.0),                                     // running sum initialization
      d_count(0),                                     // global sample counter
      d_state(IDLE),                                  // initial state: no detection
      k_event(pmt::intern("event")),                  // PMT key: "event"
      k_level(pmt::intern("level")),                  // PMT key: "level"
      v_START(pmt::intern("START")),                  // PMT value: START
      v_STOP(pmt::intern("STOP"))                     // PMT value: STOP
    {
      // Register a message output port named "out" for control events
      message_port_register_out(pmt::mp("out_sms"));

      // Warm-up note:
      // The detector will not trigger until the rolling window is primed with d_win samples.
    }

    // ---------- Destructor ----------
    detector_ff_impl::~detector_ff_impl() {}

    // ---------- Publish PMT event ----------
    void detector_ff_impl::publish_event(bool start, double level)
    {
      // Build dictionary: {event: START/STOP, level: <avg>, count: <processed_samples>}
      pmt::pmt_t m = pmt::make_dict();
      m = pmt::dict_add(m, k_event, start ? v_START : v_STOP);
      m = pmt::dict_add(m, k_level, pmt::from_double(level));
      m = pmt::dict_add(m, pmt::intern("count"), pmt::from_uint64(d_count));

      // Publish the message on the "out" port
      message_port_pub(pmt::mp("out_sms"), m);
    }

    // ---------- Add 'event' stream tag at absolute output offset ----------
    void detector_ff_impl::add_event_tag(uint64_t abs_off, bool start)
    {
      // Port 0 (only output). Key "event": START or STOP as PMT symbol.
      add_item_tag(0, abs_off, k_event, start ? v_START : v_STOP,pmt::string_to_symbol(alias()));
    }

    // ---------- Main processing ----------
    int detector_ff_impl::work(int noutput_items,
                              gr_vector_const_void_star &input_items,
                              gr_vector_void_star &output_items)
    {
      const float *in = static_cast<const float*>(input_items[0]);   // input float pointer
      float *out = static_cast<float*>(output_items[0]);             // output float pointer

      // Snapshot parameters (avoid holding mutex in the main loop)
      float thrH, thrL;
      int win;
      {
        boost::mutex::scoped_lock lk(d_mtx);
        thrH = d_thr_high;  // high detection threshold (START when avg > thrH)
        thrL = d_thr_low;   // low detection threshold  (STOP  when avg < thrL)
        win  = d_win;       // moving-average window length
      }

      // If window length changed since last call, reinitialize rolling state
      if ((int)d_buf.size() != win) {
        d_buf.assign(win, 0.0f);
        d_head = 0;
        d_sum = 0.0;
        d_primed = false;
      }

      // Process samples
      for (int i = 0; i < noutput_items; ++i) {
        // Pass-through: copy input to output
        out[i] = in[i];

        // Rolling average update in O(1)
        float p = in[i];                // sample value (assumed power)
        float old = d_buf[d_head];      // value leaving the window
        d_sum += (double)p - (double)old;
        d_buf[d_head] = p;              // store new sample in circular buffer
        d_head = (d_head + 1) % win;    // advance head index

        // Warm-up: wait until window filled before any detection
        if (!d_primed) {
          if (d_count + 1 >= (uint64_t)win)
            d_primed = true;
          d_count++;
          continue; // skip detection until primed
        }

        // Compute mean power over the window
        double avg = d_sum / (double)win;

        // Hysteresis-based detection
        if (d_state == IDLE && avg > thrH) {
          // Transition to ACTIVE: signal detected (START)
          d_state = ACTIVE;
          uint64_t abs_off = nitems_written(0) + i; // absolute output item index
          add_event_tag(abs_off, true);             // tag: event=START at exact sample
          publish_event(true, avg);                 // PMT message: START with current avg
        }
        else if (d_state == ACTIVE && avg < thrL) {
          // Transition to IDLE: signal gone (STOP)
          d_state = IDLE;
          uint64_t abs_off = nitems_written(0) + i;
          add_event_tag(abs_off, false);            // tag: event=STOP
          publish_event(false, avg);                // PMT message: STOP with current avg
        }

        // Global processed sample counter (used in 'count' field)
        d_count++;
      }

      return noutput_items; // produced same number as requested (sync_block)
    }

    // ---------- Runtime setters / getters ----------
    void detector_ff_impl::set_thresholds(float thr_high, float thr_low)
    {
      boost::lock_guard<boost::mutex> lk(d_mtx);
      // Keep invariants: d_thr_high ≥ d_thr_low
      d_thr_high = std::max(thr_high, thr_low);
      d_thr_low  = std::min(thr_high, thr_low);
    }

    void detector_ff_impl::set_window(int win)
    {
      if (win < 2) win = 2;
      boost::lock_guard<boost::mutex> lk(d_mtx);
      d_win = win; // reallocation happens on next work() entry
    }

    float detector_ff_impl::thr_high() const {
      boost::lock_guard<boost::mutex> lk(d_mtx);
      return d_thr_high;
    }

    float detector_ff_impl::thr_low() const {
      boost::lock_guard<boost::mutex> lk(d_mtx);
      return d_thr_low;
    }

    int detector_ff_impl::window() const {
      boost::lock_guard<boost::mutex> lk(d_mtx);
      return d_win;
    }

}} // namespace gr::howto
