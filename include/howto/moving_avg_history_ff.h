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


#ifndef INCLUDED_HOWTO_MOVING_AVG_HISTORY_FF_H
#define INCLUDED_HOWTO_MOVING_AVG_HISTORY_FF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
//#include <boost/shared_ptr.hpp>

namespace gr {
  namespace howto {

    /*!
     * \brief <+description of block+>
     * \ingroup howto
     *
     */
    class HOWTO_API moving_avg_history_ff : virtual public gr::sync_block
    {
	public:
	  typedef boost::shared_ptr<moving_avg_history_ff> sptr;

	  static sptr make(int length, float scale = 1.0f);

	  // Runtime control
	  virtual void  set_length(int length) = 0; // N
	  //virtual int   length() const = 0;

	  virtual void  set_scale(float scale) = 0; // factor
	  //virtual float scale() const = 0;

	protected:
	  moving_avg_history_ff() {}
	};

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_MOVING_AVG_HISTORY_FF_H */

