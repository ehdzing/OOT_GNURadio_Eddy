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

#include "gate_ff_impl.h"

#include <gnuradio/io_signature.h>
#include <gnuradio/tags.h> 
#include <pmt/pmt.h>

#include <algorithm>               // std::sort, std::min
#include <cstring>                 // std::memcpy, std::memset
#include <boost/bind.hpp>
#include <boost/thread/lock_guard.hpp>
//#include <boost/thread/mutex.hpp>

namespace gr {
  namespace howto {

    gate_ff::sptr gate_ff::make(bool initially_open)
    {
        return gnuradio::get_initial_sptr(new gate_ff_impl(initially_open));
    }

    gate_ff_impl::gate_ff_impl(bool initially_open)
      : gr::sync_block("gate_ff",
            gr::io_signature::make(1, 1, sizeof(float)),
            gr::io_signature::make(1, 1, sizeof(float))),
        d_open(false) // start closed
    {
        // Message port for control
        message_port_register_in(pmt::mp("in_ctrl"));
        set_msg_handler(pmt::mp("in_ctrl"),
            boost::bind(&gate_ff_impl::on_msg, this, _1));

        // Cached PMT atoms
        k_event = pmt::intern("event");
        v_START = pmt::intern("START");
        v_STOP  = pmt::intern("STOP");
    }

    gate_ff_impl::~gate_ff_impl() {}

    void gate_ff_impl::on_msg(pmt::pmt_t msg)
    {
        // Accept either dict {event: START|STOP} or raw symbol START/STOP
        boost::lock_guard<boost::mutex> lk(d_mtx);
        if (pmt::is_dict(msg)) {
            pmt::pmt_t ev = pmt::dict_ref(msg, k_event, pmt::PMT_NIL);
            if (pmt::eq(ev, v_START))      d_open = true;
            else if (pmt::eq(ev, v_STOP))  d_open = false;
        } else if (pmt::is_symbol(msg)) {
            if (pmt::eq(msg, v_START))      d_open = true;
            else if (pmt::eq(msg, v_STOP))  d_open = false;
        }
    }

    void gate_ff_impl::set_open(bool open_now)
    {
        boost::lock_guard<boost::mutex> lk(d_mtx);
        d_open = open_now;
    }

    bool gate_ff_impl::is_open() const {
        boost::mutex::scoped_lock lk(d_mtx);
        return d_open;
    }


    int gate_ff_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items)
    {
        const float* in  = static_cast<const float*>(input_items[0]);
        float*       out = static_cast<float*>(output_items[0]);

        // Snapshot current state without holding the mutex in the hot path
        bool open_now;
        {
            boost::lock_guard<boost::mutex> lk(d_mtx);
            open_now = d_open;
        }

        // Gather event tags (START/STOP) within this window
        std::vector<tag_t> tags;
        const uint64_t base = nitems_read(0);
        get_tags_in_range(tags, 0, base, base + noutput_items, k_event);

        if (tags.empty()) {
            // No tags → apply current state to the whole chunk
            if (open_now) {
                std::memcpy(out, in, sizeof(float) * noutput_items);
            } else {
                std::memset(out, 0, sizeof(float) * noutput_items);
            }
            return noutput_items;
        }

        // Sort tags by absolute offset to ensure deterministic ordering
        std::sort(tags.begin(), tags.end(),
                  [](const tag_t& a, const tag_t& b){ return a.offset < b.offset; });

        auto apply_run = [&](int begin, int end, bool open_flag)
        {
            if (begin >= end) return;
            const int count = end - begin;
            if (open_flag) {
                std::memcpy(out + begin, in + begin, sizeof(float) * count);
            } else {
                std::memset(out + begin, 0, sizeof(float) * count);
            }
        };

        int cursor = 0; // start of the yet-to-be-written segment [cursor, ...)
        for (const auto& tg : tags) {
            // Convert absolute tag offset to relative index inside this window
            // Note: guard against off-by-one or exotic upstream tag manipulations
            const int rel = static_cast<int>(tg.offset - base);
            if (rel <= cursor) {
                // Tag at or before current cursor:
                // just flip state (affects subsequent segments)
                if (pmt::eq(tg.value, v_START))      open_now = true;
                else if (pmt::eq(tg.value, v_STOP))  open_now = false;
                continue;
            }
            if (rel >= noutput_items) {
                // Tag at/after the end → finish current window entirely with current state
                break;
            }

            // 1) Emit the segment up to rel with the current state
            apply_run(cursor, rel, open_now);
            cursor = rel;

            // 2) Flip state exactly at rel according to the tag value
            if (pmt::eq(tg.value, v_START))      open_now = true;
            else if (pmt::eq(tg.value, v_STOP))  open_now = false;
        }

        // Tail segment after the last tag within this window
        apply_run(cursor, noutput_items, open_now);

        // Persist final state for the next call (thread-safe)
        {
            boost::lock_guard<boost::mutex> lk(d_mtx);
            d_open = open_now;
        }

        return noutput_items;
    }

  } // namespace howto
} // namespace gr


