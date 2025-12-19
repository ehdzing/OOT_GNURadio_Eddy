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
#ifndef INCLUDED_HOWTO_FRAME_BURST_SOURCE_CC_IMPL_H
#define INCLUDED_HOWTO_FRAME_BURST_SOURCE_CC_IMPL_H

#include <howto/frame_burst_source_cc.h>
#include <pmt/pmt.h>
#include <boost/thread/mutex.hpp>

namespace gr {
namespace howto {

class frame_burst_source_cc_impl : public frame_burst_source_cc
{
private:
  enum state_t { ST_PREAMBLE, ST_PAYLOAD, ST_SILENCE };

  std::vector<gr_complex> d_preamble;
  std::vector<gr_complex> d_payload;

  std::vector<gr_complex> d_preamble_scaled;
  std::vector<gr_complex> d_payload_scaled;

  size_t d_silence_len;
  float d_scale;
  bool d_enable_tags;

  state_t d_state;
  size_t d_idx;

  pmt::pmt_t k_burst_start;
  pmt::pmt_t k_payload_start;

  mutable boost::mutex d_mutex;

  void rebuild_scaled_unlocked();

public:
  frame_burst_source_cc_impl(const std::vector<gr_complex>& preamble,
                             const std::vector<gr_complex>& payload,
                             size_t silence_len,
                             float scale,
                             bool enable_tags);

  ~frame_burst_source_cc_impl() override;

  int work(int noutput_items,
           gr_vector_const_void_star&,
           gr_vector_void_star& output_items) override;

  // Setters
  void set_scale(float scale) override;
  void set_silence_len(size_t silence_len) override;
  void set_enable_tags(bool en) override;

  // Getters
  float scale() const override;
  size_t silence_len() const override;
  bool enable_tags() const override;
};

} // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_FRAME_BURST_SOURCE_CC_IMPL_H */
