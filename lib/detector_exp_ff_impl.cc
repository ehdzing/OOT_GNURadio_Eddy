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

#include "detector_exp_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <algorithm>
#include <cmath>

namespace gr {
  namespace howto {

    // Factory
    detector_exp_ff::sptr detector_exp_ff::make(int length)
    {
      return gnuradio::get_initial_sptr(new detector_exp_ff_impl(length));
    }

    // Constructor
    detector_exp_ff_impl::detector_exp_ff_impl(int length)
      : gr::sync_block("detector_exp_ff",
          gr::io_signature::make(1, 1, sizeof(float)),   // in
          gr::io_signature::make(2, 2, sizeof(float)))   // out, env
      , d_length(std::max(1, length))
      , d_env(0.0f)
      , d_alpha(0.95f)
      , d_Ton(0.20f)
      , d_Toff(0.10f)
      , d_state(false)
    {
      // One message port for START/STOP
      message_port_register_out(pmt::mp("state_msg"));

      // If strict alignment via history is desired, enable:
      // set_history(d_length);
    }

    // Destructor
    detector_exp_ff_impl::~detector_exp_ff_impl() = default;

    // Controls
    void detector_exp_ff_impl::set_length(int n) noexcept
    {
      boost::mutex::scoped_lock lk(d_mtx);
      d_length = std::max(1, n);
      // If using history: consider realignment when length changes.
      // if (d_length != history()) { set_history(d_length); }
    }

    void detector_exp_ff_impl::set_Ton(float t) noexcept
    {
      boost::mutex::scoped_lock lk(d_mtx);
      d_Ton = t;
    }

    void detector_exp_ff_impl::set_Toff(float t) noexcept
    {
      boost::mutex::scoped_lock lk(d_mtx);
      d_Toff = t;
    }

    // Publish PMT event on 'state_msg'
    void detector_exp_ff_impl::publish_event(const char* ev,
                                            uint64_t idx,
                                            float env) noexcept
    {
      pmt::pmt_t d = pmt::make_dict();
      d = pmt::dict_add(d, pmt::intern("event"), pmt::intern(ev));
      d = pmt::dict_add(d, pmt::intern("idx"),   pmt::from_uint64(idx));
      d = pmt::dict_add(d, pmt::intern("env"),   pmt::from_double(env));
      message_port_pub(pmt::mp("state_msg"), d);
    }

    // Stamp stream tags on 'out' (port 0)
    void detector_exp_ff_impl::tag_event(int out_port,
                                        uint64_t abs_off,
                                        const char* ev,
                                        float env) noexcept
    {
      const pmt::pmt_t key_state = pmt::intern("state");
      const pmt::pmt_t val_state = pmt::intern(ev);
      const pmt::pmt_t key_env   = pmt::intern("env");
      const pmt::pmt_t val_env   = pmt::from_double(env);

      add_item_tag(out_port, abs_off, key_state, val_state);
      add_item_tag(out_port, abs_off, key_env,   val_env);
    }

    // Propagate input tags to 'out' (port 0) preserving offsets
    void detector_exp_ff_impl::passthrough_tags(uint64_t abs_read,
                                                uint64_t abs_write,
                                                int noutput_items) noexcept
    {
      std::vector<tag_t> tags;
      const uint64_t beg = abs_read;
      const uint64_t end = abs_read + static_cast<uint64_t>(noutput_items);
      get_tags_in_range(tags, 0, beg, end);

      for (size_t k = 0; k < tags.size(); ++k) {
        const uint64_t rel = tags[k].offset - beg;
        const uint64_t dst = abs_write + rel;
        add_item_tag(0, dst, tags[k].key, tags[k].value);
      }
    }

    // work()
    int detector_exp_ff_impl::work(int noutput_items,
                                  gr_vector_const_void_star &input_items,
                                  gr_vector_void_star &output_items)
    {
      const float* in  = static_cast<const float*>(input_items[0]);
      float* out_sig   = static_cast<float*>(output_items[0]); // passthrough
      float* out_env   = static_cast<float*>(output_items[1]); // d_env

      // Snapshot params
      float Ton, Toff, alpha;
      {
        boost::mutex::scoped_lock lk(d_mtx);
        Ton   = d_Ton;
        Toff  = d_Toff;
        alpha = d_alpha;
      }

      const uint64_t abs_read  = nitems_read(0);
      const uint64_t abs_write = nitems_written(0);

      // Tag passthrough from input to 'out'
      passthrough_tags(abs_read, abs_write, noutput_items);

      for (int i = 0; i < noutput_items; ++i) {
        // 1) Passthrough
        const float x = in[i];
        out_sig[i] = x;

        // 2) Exponential energy envelope
        const float x2 = x * x;
        d_env = alpha * d_env + (1.0f - alpha) * x2;
        out_env[i] = d_env;

        // 3) Hysteresis events and annotations at sample offset
        const uint64_t abs_off = abs_write + static_cast<uint64_t>(i);
        if (!d_state && d_env >= Ton) {
          d_state = true;
          publish_event("START", abs_off, d_env);
          tag_event(0, abs_off, "START", d_env); // tag on 'out'
        } else if (d_state && d_env <= Toff) {
          d_state = false;
          publish_event("STOP", abs_off, d_env);
          tag_event(0, abs_off, "STOP", d_env); // tag on 'out'
        }
      }

      return noutput_items;
    }

  } // namespace howto
} // namespace gr
