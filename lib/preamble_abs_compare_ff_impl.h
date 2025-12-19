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

#ifndef INCLUDED_HOWTO_PREAMBLE_ABS_COMPARE_FF_IMPL_H
#define INCLUDED_HOWTO_PREAMBLE_ABS_COMPARE_FF_IMPL_H

#include <howto/preamble_abs_compare_ff.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <vector>
#include <string>

namespace gr {
namespace howto {

class preamble_abs_compare_ff_impl : public preamble_abs_compare_ff
{
public:
  preamble_abs_compare_ff_impl(const std::vector<gr_complex>& preambles,
                               int preamble_layout,
                               const std::string& preamble_select);

  ~preamble_abs_compare_ff_impl();

  void forecast(int noutput_items, gr_vector_int& ninput_items_required);

  int general_work(int noutput_items,
                   gr_vector_int& ninput_items,
                   gr_vector_const_void_star& input_items,
                   gr_vector_void_star& output_items);

  void set_preamble_layout(int layout);
  void set_preamble_select(const std::string& sel);

private:
  static std::string trim_copy(const std::string& s);
  static int parse_preamble_select_mask_4(const std::string& sel);

  void build_references_from_param_locked(const std::vector<gr_complex>& preambles);
  void build_ref0_from_4_locked(const std::vector<gr_complex>& preambles);

  boost::mutex d_mutex;

  int d_preamble_layout;      // 0=AUTO, 1=SINGLE, 4=CONCAT4
  std::string d_preamble_select;
  int d_preamble_mask;

  int d_N;
  std::vector<gr_complex> d_ref0;
  std::vector<gr_complex> d_ref1;

  std::vector<gr_complex> d_preambles_param;
};

} // namespace howto
} // namespace gr

#endif
