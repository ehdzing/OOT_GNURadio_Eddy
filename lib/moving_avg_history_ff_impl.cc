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
#include "moving_avg_history_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <algorithm>

namespace gr {
  namespace howto {

    moving_avg_history_ff::sptr
    moving_avg_history_ff::make(int length, float scale)
    {
      return gnuradio::get_initial_sptr
        (new moving_avg_history_ff_impl(length, scale));
    }

    /*
     * The private constructor
     */
    moving_avg_history_ff_impl::moving_avg_history_ff_impl(int length, float scale)
      : gr::sync_block("moving_avg_history_ff",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
	d_length(std::max(1, length)),
        d_scale(scale) 
    {
	// Set initial history so the scheduler provides (N-1) past items
	set_history(std::max(1, d_length));
	// You may keep output multiple as 1; not needed to change
	// set_output_multiple(1);
    }

	
    /*
    Work strategy with history(N):
    - The input pointer 'in' includes (N-1) preceding items due to set_history(N).
    - We compute a sliding mean using an O(1) rolling sum per output item:
      sum_{i..i+N-1}  =  prev_sum + in[i+N-1] - in[i-1]
    - To keep processing lock-free, we copy 'd_length' and 'd_scale' to local consts
    under the mutex, then release the lock before looping.
    - If runtime N changed and does not match history(), we adjust history and return 0
    so the scheduler re-invokes work() with the new overlap.
    */

    int moving_avg_history_ff_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = static_cast<const float *>(input_items[0]);
      float *out = static_cast<float *>(output_items[0]);

      // Snapshot parameters under mutex
      int   n;
      float scale;
      {
        boost::lock_guard<boost::mutex> lk(d_mutex);
        n     = std::max(1, d_length);
        scale = d_scale;
      }

      // If length was changed via setter, align the block history lazily here
      if (n != static_cast<int>(history())) {
        set_history(n);
        // Tell the scheduler to come back with the correct overlap
        return 0;
      }

      // Trivial window
      if (n == 1) {
        for (int i = 0; i < noutput_items; ++i)
          out[i] = in[i] * scale;
        return noutput_items;
      }

      // We have (n-1) past items available at the beginning of 'in'.
      // Build initial sum over the first window: in[0 .. n-1]
      float sum = 0.0f;
      for (int k = 0; k < n; ++k) sum += in[k];
      out[0] = (sum * scale) / n;

      // Slide window across outputs:
      // For output i, the window is in[i .. i + (n-1)]
      // Transition i-1 -> i: add in[i + (n-1)], subtract in[i - 1]
      for (int i = 1; i < noutput_items; ++i) {
        sum += in[i + (n - 1)] - in[i - 1];
        out[i] = (sum * scale) / n;
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    void moving_avg_history_ff_impl::set_length(int length) {
      // Update parameter under mutex. We DO NOT call set_history() here.
      boost::lock_guard<boost::mutex> lk(d_mutex);
      d_length = std::max(1, length);
      // History reconciliation occurs lazily in work(), returning 0 once.
    }

    void moving_avg_history_ff_impl::set_scale(float scale) {
      boost::lock_guard<boost::mutex> lk(d_mutex);
      d_scale = scale;
    }
   
    /*Solo usarlas si quieres consultar estos valores desde la capa alta
    int moving_avg_history_ff_impl::length() const {
      boost::lock_guard<boost::mutex> lk(d_mutex);
      return d_length;
    }
    float moving_avg_history_ff_impl::scale() const {
      boost::lock_guard<boost::mutex> lk(d_mutex);
      return d_scale;
    }
    */

  } /* namespace howto */
} /* namespace gr */

