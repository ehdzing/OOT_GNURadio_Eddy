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

#ifndef INCLUDED_HOWTO_PREAMBLE_BURST_SOURCE_CC_IMPL_H
#define INCLUDED_HOWTO_PREAMBLE_BURST_SOURCE_CC_IMPL_H

#include <howto/preamble_burst_source_cc.h>
#include <boost/thread/mutex.hpp>
#include <vector>

namespace gr {
namespace howto {

class preamble_burst_source_cc_impl : public preamble_burst_source_cc
{
private:
  boost::mutex d_mutex;

  // Raw input vector from Python (either 4*N concatenated, or a single burst)
  std::vector<gr_complex> d_burst;

  // Mode control
  bool d_use_preamble_split;

  // When split enabled:
  int d_preamble_len; // N (derived from burst size / 4)
  std::vector<std::vector<gr_complex> > d_preambles4;
  std::vector<int> d_indices;

  // Active emitted burst (either sum of selected preambles, or direct burst)
  std::vector<gr_complex> d_active;

  // Burst/silence pacing
  int d_samples_per_burst; // usually equals preamble length, but can be >=
  int d_silence_samples;
  bool d_enable_tags;

  // State machine
  // 0 = emit active burst, 1 = emit silence
  int d_state;
  int d_pos;

  // Helpers
  void rebuild_active_locked();
  void validate_indices_locked() const;

public:
  preamble_burst_source_cc_impl(const std::vector<gr_complex>& burst,
                                bool use_preamble_split,
                                const std::vector<int>& indices,
                                int samples_per_burst,
                                int silence_samples,
                                bool enable_tags);

  ~preamble_burst_source_cc_impl() override;

  void set_burst(const std::vector<gr_complex>& burst) override;
  void set_use_preamble_split(bool enable) override;
  void set_indices(const std::vector<int>& indices) override;
  void set_silence_samples(int n) override;

  int work(int noutput_items,
           gr_vector_const_void_star& input_items,
           gr_vector_void_star& output_items) override;
};

} // namespace howto
} // namespace gr

#endif // INCLUDED_HOWTO_PREAMBLE_BURST_SOURCE_CC_IMPL_H
