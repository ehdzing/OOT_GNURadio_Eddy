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


#ifndef INCLUDED_HOWTO_GATE_FF_H
#define INCLUDED_HOWTO_GATE_FF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <boost/shared_ptr.hpp>

namespace gr { 
  
  namespace howto {

  /*!
  * \brief Message-controlled gate for float streams.
  * 
  * Control:
  *  - Message in port "in_ctrl": dict {event: START|STOP}
  *  - Optional alignment using 'event' stream tags found in the input range
  * Behavior:
  *  - OPEN → pass samples through
  *  - CLOSED → output zeros
  */
  class HOWTO_API gate_ff : virtual public gr::sync_block
  {
  public:
    typedef boost::shared_ptr<gate_ff> sptr;

    //! Factory: create gate with initial state (true=open, false=closed)
    static sptr make(bool initially_open);

    //! Manually set gate state
    virtual void set_open(bool open_now) = 0;

    //! Read current gate state
    virtual bool is_open() const = 0;
  };

}} // namespace gr::howto
#endif /* INCLUDED_HOWTO_GATE_FF_H */


