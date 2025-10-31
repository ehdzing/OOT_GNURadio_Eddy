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


#ifndef INCLUDED_HOWTO_DETECTOR_EXP_FF_H
#define INCLUDED_HOWTO_DETECTOR_EXP_FF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace howto {

    /*!
    * \brief Exponential energy detector with hysteresis.
    * Streams: out (passthrough), env (exponential envelope).
    * Message: state_msg (START/STOP events).
    * Tags on 'out': "state" and "env" at event sample offsets.
    */
    class HOWTO_API detector_exp_ff : virtual public gr::sync_block
    {
    public:
      typedef boost::shared_ptr<detector_exp_ff> sptr;

      /*!
      * \param length moving window size kept for priming/compatibility
      */
      static sptr make(int length);

      // Runtime controls
      virtual void set_length(int n) noexcept = 0;
      virtual void set_Ton(float t) noexcept = 0;
      virtual void set_Toff(float t) noexcept = 0;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_DETECTOR_EXP_FF_H */
