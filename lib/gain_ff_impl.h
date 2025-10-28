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

#ifndef INCLUDED_HOWTO_GAIN_FF_IMPL_H
#define INCLUDED_HOWTO_GAIN_FF_IMPL_H

#include <howto/gain_ff.h>

namespace gr {
  namespace howto {

    class gain_ff_impl : public gain_ff
    {
     private:
      float d_gain;

     public:
      gain_ff_impl(float gain);
      ~gain_ff_impl();

      void set_gain(float g) {d_gain = g;} // permite modificar la ganancia en tiempo de ejecucion		
      float gain() const { return d_gain; }

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_GAIN_FF_IMPL_H */

