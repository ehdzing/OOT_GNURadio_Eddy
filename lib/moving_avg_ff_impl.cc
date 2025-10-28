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


#include <howto/moving_avg_ff.h>
#include "moving_avg_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <algorithm>
#include <vector>

namespace gr { namespace howto {

  moving_avg_ff_impl::moving_avg_ff_impl(int length, float scale)
    : gr::sync_block("moving_avg_ff",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float))),
      d_N(clamp_len(length)),
      d_scale(scale),
      d_buf(d_N, 0.0f),
      d_head(0),
      d_filled(0),
      d_sum(0.0f)
    {}
        
    inline void moving_avg_ff_impl::reset_state_(int newN)
    {
      d_N      = clamp_len(newN);
      d_buf.assign(d_N, 0.0f);
      d_head   = 0;
      d_filled = 0;
      d_sum    = 0.0f;
    }   

    void moving_avg_ff_impl::set_length(int length) {
      // Reinicia estado: nuevo N implica nueva ventana
      reset_state_(length);
    }

    void moving_avg_ff_impl::set_scale(float scale) { d_scale = scale; }

    int moving_avg_ff_impl::work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) 
    {
      const float* in  = static_cast<const float*>(input_items[0]);
      float*       out = static_cast<float*>(output_items[0]);

      const int   N  = d_N;
      const float sc = d_scale;

      for (int i = 0; i < noutput_items; ++i) {
        // quitar el valor viejo de la suma
        d_sum -= d_buf[d_head];

        // escribir el nuevo sample y sumarlo
        d_buf[d_head] = in[i];
        d_sum += d_buf[d_head];

        // avanzar índice circular
        if (++d_head == N) d_head = 0;

        // aumentar contador hasta N
        if (d_filled < N) d_filled++;

        // salida: 0.0 hasta llenar ventana; luego promedio
        out[i] = (d_filled == N) ? (d_sum * sc / N) : 0.0f;
      }

      // sync_block: consume == produce == noutput_items
      return noutput_items;
    }
   
   // fábrica de la BASE, a nivel de namespace
   moving_avg_ff::sptr moving_avg_ff::make(int length, float scale) {
      return gnuradio::get_initial_sptr(new moving_avg_ff_impl(length, scale));
    }
    

}} // namespace gr::howto


