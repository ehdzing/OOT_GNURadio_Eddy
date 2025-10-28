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

#include "iq_mag_cf_impl.h"
#include <gnuradio/io_signature.h>
#include <cmath>  // std::sqrtf

namespace gr {
  namespace howto {

    iq_mag_cf::sptr
    iq_mag_cf::make(float scale)
    {
      return gnuradio::get_initial_sptr(new iq_mag_cf_impl(scale));
    }

    /*
     * The private constructor
     */
    iq_mag_cf_impl::iq_mag_cf_impl(float scale)
      : gr::sync_block("iq_mag_cf",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(float))),
	d_scale(scale)
    {}

    int iq_mag_cf_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex* in = static_cast<const gr_complex*>(input_items[0]);
      float* out = static_cast<float*>(output_items[0]);

      float sc;
      {
        boost::lock_guard<boost::mutex> lock(d_mutex);
        sc = d_scale;
      }

      for (int i = 0; i < noutput_items; ++i) {
        const float re = in[i].real();
        const float im = in[i].imag();
        out[i] = sc * std::sqrt(re * re + im * im);
      }

      return noutput_items;
    }

    void iq_mag_cf_impl::set_scale(float scale) noexcept
    {
      boost::lock_guard<boost::mutex> lock(d_mutex);
      d_scale = scale;
    }

    float iq_mag_cf_impl::scale() const noexcept
    {
      boost::lock_guard<boost::mutex> lock(d_mutex);
      return d_scale;
    }

  } /* namespace howto */
} /* namespace gr */

