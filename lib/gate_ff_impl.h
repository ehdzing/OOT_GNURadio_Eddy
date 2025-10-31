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

#ifndef INCLUDED_HOWTO_GATE_FF_IMPL_H
#define INCLUDED_HOWTO_GATE_FF_IMPL_H

#include <howto/gate_ff.h>
//#include <pmt/pmt.h> 
#include <boost/thread/mutex.hpp>

namespace gr { 
  namespace howto {

    /*!
    * \brief Implementation of a message-controlled gate for float streams.
    *
    * Core members:
    *  - d_open: current gate state (true=open, false=closed)
    *  - k_event/v_START/v_STOP: PMT atoms for control payload and tags
    * Behavior:
    *  - On message {event: START} → set_open(true)
    *  - On message {event: STOP}  → set_open(false)
    *  - If a tag 'event' appears in current work-range, align switching at its offset
    */
    class gate_ff_impl : public gate_ff
    {
      private:
        mutable boost::mutex d_mtx; // protect d_open
        bool  d_open;               // gate state: true=open (pass), false=closed (zeros)

        // PMT symbols for control and tag matching
        pmt::pmt_t k_event;         // key: "event"
        pmt::pmt_t v_START;         // value: "START"
        pmt::pmt_t v_STOP;          // value: "STOP"

        // Message handler (in_ctrl)
        void on_msg(pmt::pmt_t msg);

      public:
        gate_ff_impl(bool initially_open);
        ~gate_ff_impl() override;

        // GNU Radio
        int work(int noutput_items,
                gr_vector_const_void_star &input_items,
                gr_vector_void_star &output_items) override;

        // API
        void set_open(bool open_now) override;
        bool is_open() const override;
    };

  }
} // namespace gr::howto

#endif /* INCLUDED_HOWTO_GATE_FF_IMPL_H */
