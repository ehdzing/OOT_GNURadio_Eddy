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

 /* -*- c++ -*- */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cfo_injector_cc_impl.h"
#include <gnuradio/io_signature.h>
#include <cmath>

namespace gr {
namespace howto {

cfo_injector_cc::sptr
cfo_injector_cc::make(double samp_rate, double cfo_hz)
{
  return gnuradio::get_initial_sptr(
      new cfo_injector_cc_impl(samp_rate, cfo_hz));
}

cfo_injector_cc_impl::cfo_injector_cc_impl(double samp_rate, double cfo_hz)
  : cfo_injector_cc("cfo_injector_cc",
        gr::io_signature::make2(2, 2, sizeof(gr_complex), sizeof(gr_complex)),
        gr::io_signature::make2(2, 2, sizeof(gr_complex), sizeof(gr_complex))),
    d_fs(samp_rate),
    d_cfo_hz(cfo_hz),
    d_phase(1.0f, 0.0f)
{
  update_phase_inc_locked();
}

cfo_injector_cc_impl::~cfo_injector_cc_impl() {}

void
cfo_injector_cc_impl::update_phase_inc_locked()
{
  const double w = 2.0 * M_PI * (d_cfo_hz / d_fs);
  d_phase_inc = gr_complex(std::cos(w), std::sin(w));
}

void
cfo_injector_cc_impl::set_cfo_hz(double hz)
{
  boost::lock_guard<boost::mutex> lock(d_mutex);
  d_cfo_hz = hz;
  update_phase_inc_locked();
}

int
cfo_injector_cc_impl::work(int noutput_items,
                           gr_vector_const_void_star& input_items,
                           gr_vector_void_star& output_items)
{
  const gr_complex* in0 = (const gr_complex*)input_items[0];
  const gr_complex* in1 = (const gr_complex*)input_items[1];
  gr_complex* out0 = (gr_complex*)output_items[0];
  gr_complex* out1 = (gr_complex*)output_items[1];

  boost::lock_guard<boost::mutex> lock(d_mutex);

  volk_32fc_s32fc_x2_rotator_32fc(
      out0, in0, d_phase_inc, &d_phase, noutput_items);

  volk_32fc_s32fc_x2_rotator_32fc(
      out1, in1, d_phase_inc, &d_phase, noutput_items);

  return noutput_items;
}

} // namespace howto
} // namespace gr
