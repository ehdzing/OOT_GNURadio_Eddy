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

#include <howto/frame_burst_source_cc.h>
#include "frame_burst_source_cc_impl.h"

#include <gnuradio/io_signature.h>
#include <cstring>
#include <algorithm>

namespace gr {
namespace howto {

frame_burst_source_cc::sptr
frame_burst_source_cc::make(const std::vector<gr_complex>& preamble,
                            const std::vector<gr_complex>& payload,
                            size_t silence_len,
                            float scale,
                            bool enable_tags)
{
  return gnuradio::get_initial_sptr(
    new frame_burst_source_cc_impl(preamble, payload, silence_len, scale, enable_tags));
}

frame_burst_source_cc_impl::frame_burst_source_cc_impl(
    const std::vector<gr_complex>& preamble,
    const std::vector<gr_complex>& payload,
    size_t silence_len,
    float scale,
    bool enable_tags)
  : frame_burst_source_cc("frame_burst_source_cc",
        gr::io_signature::make(0, 0, 0),
        gr::io_signature::make(1, 1, sizeof(gr_complex))),
    d_preamble(preamble),
    d_payload(payload),
    d_silence_len(silence_len),
    d_scale(scale),
    d_enable_tags(enable_tags),
    d_state(ST_PREAMBLE),
    d_idx(0),
    k_burst_start(pmt::intern("burst_start")),
    k_payload_start(pmt::intern("payload_start"))
{
  rebuild_scaled_unlocked();
}

frame_burst_source_cc_impl::~frame_burst_source_cc_impl() {}

void frame_burst_source_cc_impl::rebuild_scaled_unlocked()
{
  d_preamble_scaled.resize(d_preamble.size());
  d_payload_scaled.resize(d_payload.size());

  for (size_t i = 0; i < d_preamble.size(); i++)
    d_preamble_scaled[i] = d_preamble[i] * d_scale;

  for (size_t i = 0; i < d_payload.size(); i++)
    d_payload_scaled[i] = d_payload[i] * d_scale;
}

// -------------------- Setters/Getters --------------------

void frame_burst_source_cc_impl::set_scale(float scale)
{
  boost::mutex::scoped_lock lock(d_mutex);
  d_scale = scale;
  rebuild_scaled_unlocked();
}

float frame_burst_source_cc_impl::scale() const
{
  boost::mutex::scoped_lock lock(d_mutex);
  return d_scale;
}

void frame_burst_source_cc_impl::set_silence_len(size_t silence_len)
{
  boost::mutex::scoped_lock lock(d_mutex);
  d_silence_len = silence_len;
}

size_t frame_burst_source_cc_impl::silence_len() const
{
  boost::mutex::scoped_lock lock(d_mutex);
  return d_silence_len;
}

void frame_burst_source_cc_impl::set_enable_tags(bool en)
{
  boost::mutex::scoped_lock lock(d_mutex);
  d_enable_tags = en;
}

bool frame_burst_source_cc_impl::enable_tags() const
{
  boost::mutex::scoped_lock lock(d_mutex);
  return d_enable_tags;
}

// -------------------- work() --------------------

int frame_burst_source_cc_impl::work(int noutput_items,
                                     gr_vector_const_void_star&,
                                     gr_vector_void_star& output_items)
{
  gr_complex* out = (gr_complex*)output_items[0];

  // Snapshot of runtime params (avoid locking inside the while)
  size_t silence_len_local;
  bool enable_tags_local;

  {
    boost::mutex::scoped_lock lock(d_mutex);
    silence_len_local = d_silence_len;
    enable_tags_local = d_enable_tags;
  }

  int produced = 0;

  while (produced < noutput_items) {
    int space = noutput_items - produced;

    if (d_state == ST_PREAMBLE) {
      const size_t total = d_preamble_scaled.size();
      if (total == 0) {
        d_state = ST_PAYLOAD;
        d_idx = 0;
        continue;
      }

      if (d_idx == 0 && enable_tags_local)
        add_item_tag(0, nitems_written(0) + produced, k_burst_start, pmt::PMT_T);

      size_t remain = total - d_idx;
      size_t take = std::min((size_t)space, remain);

      std::memcpy(out + produced, &d_preamble_scaled[d_idx], take * sizeof(gr_complex));

      d_idx += take;
      produced += (int)take;

      if (d_idx >= total) {
        d_state = ST_PAYLOAD;
        d_idx = 0;
      }

    } else if (d_state == ST_PAYLOAD) {
      const size_t total = d_payload_scaled.size();
      if (total == 0) {
        d_state = ST_SILENCE;
        d_idx = 0;
        continue;
      }

      if (d_idx == 0 && enable_tags_local)
        add_item_tag(0, nitems_written(0) + produced, k_payload_start, pmt::PMT_T);

      size_t remain = total - d_idx;
      size_t take = std::min((size_t)space, remain);

      std::memcpy(out + produced, &d_payload_scaled[d_idx], take * sizeof(gr_complex));

      d_idx += take;
      produced += (int)take;

      if (d_idx >= total) {
        d_state = ST_SILENCE;
        d_idx = 0;
      }

    } else { // ST_SILENCE
      if (silence_len_local == 0) {
        d_state = ST_PREAMBLE;
        d_idx = 0;
        continue;
      }

      size_t remain = silence_len_local - d_idx;
      size_t take = std::min((size_t)space, remain);

      std::memset(out + produced, 0, take * sizeof(gr_complex));

      d_idx += take;
      produced += (int)take;

      if (d_idx >= silence_len_local) {
        d_state = ST_PREAMBLE;
        d_idx = 0;
      }
    }
  }

  return noutput_items;
}

} // namespace howto
} // namespace gr
