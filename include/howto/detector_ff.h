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


#ifndef INCLUDED_HOWTO_DETECTOR_FF_H
#define INCLUDED_HOWTO_DETECTOR_FF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <boost/shared_ptr.hpp>

namespace gr { 
  namespace howto {
  /*!
  * \brief Energy detector with hysteresis that publishes PMT messages and inserts stream tags.
  * 
  * Input: float stream (usually power |x|^2). Output: float stream (passthrough).
  * Compares a moving average against two thresholds:
  *  - High threshold: signal present → emit START
  *  - Low threshold:  signal absent  → emit STOP
  * On state transitions it:
  *  - Publishes a PMT dict on message port "out": {event: START|STOP, level: double, count: uint64}
  *  - Inserts a stream tag at the exact sample offset: key="event", value=START/STOP
  */
    class HOWTO_API detector_ff : virtual public gr::sync_block
    {
    public:
      typedef boost::shared_ptr<detector_ff> sptr;

      //! Factory: create detector with thresholds and window length
      static sptr make(float thr_high, float thr_low, int win);

      //! Runtime setters
      virtual void set_thresholds(float thr_high, float thr_low) = 0; // detection thresholds
      virtual void set_window(int win) = 0;                           // moving-average window length

      //! Accessors
      virtual float thr_high() const = 0;  // high detection threshold
      virtual float thr_low()  const = 0;  // low detection threshold
      virtual int   window()   const = 0;  // window length in samples
    };

}} // namespace gr::howto
#endif /* INCLUDED_HOWTO_DETECTOR_FF_H */



