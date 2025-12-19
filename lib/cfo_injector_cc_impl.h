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
#ifndef INCLUDED_HOWTO_CFO_INJECTOR_CC_IMPL_H
#define INCLUDED_HOWTO_CFO_INJECTOR_CC_IMPL_H

#include <howto/cfo_injector_cc.h>
#include <boost/thread/mutex.hpp>
#include <volk/volk.h>

namespace gr {
namespace howto {

class cfo_injector_cc_impl : public cfo_injector_cc
{
private:
  boost::mutex d_mutex;

  double d_fs;
  double d_cfo_hz;

  gr_complex d_phase_inc;
  gr_complex d_phase;

  void update_phase_inc_locked();

public:
  cfo_injector_cc_impl(double samp_rate, double cfo_hz);
  ~cfo_injector_cc_impl() override;

  void set_cfo_hz(double hz) override;

  int work(int noutput_items,
           gr_vector_const_void_star& input_items,
           gr_vector_void_star& output_items) override;
};

} // namespace howto
} // namespace gr

#endif
