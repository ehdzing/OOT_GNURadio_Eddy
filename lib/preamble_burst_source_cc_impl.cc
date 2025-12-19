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

#include "preamble_burst_source_cc_impl.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <stdexcept>
#include <algorithm>

namespace gr {
namespace howto {

preamble_burst_source_cc::sptr
preamble_burst_source_cc::make(const std::vector<gr_complex>& burst,
                               bool use_preamble_split,
                               const std::vector<int>& indices,
                               int samples_per_burst,
                               int silence_samples,
                               bool enable_tags)
{
  return gnuradio::get_initial_sptr(
      new preamble_burst_source_cc_impl(burst,
                                        use_preamble_split,
                                        indices,
                                        samples_per_burst,
                                        silence_samples,
                                        enable_tags));
}

preamble_burst_source_cc_impl::preamble_burst_source_cc_impl(
    const std::vector<gr_complex>& burst,
    bool use_preamble_split,
    const std::vector<int>& indices,
    int samples_per_burst,
    int silence_samples,
    bool enable_tags)
  : preamble_burst_source_cc("preamble_burst_source_cc",
                             gr::io_signature::make(0, 0, 0),
                             gr::io_signature::make(1, 1, sizeof(gr_complex))),
    d_burst(burst),
    d_use_preamble_split(use_preamble_split),
    d_preamble_len(0),
    d_indices(indices),
    d_samples_per_burst(samples_per_burst),
    d_silence_samples(silence_samples),
    d_enable_tags(enable_tags),
    d_state(0),
    d_pos(0)
{
  if (d_samples_per_burst <= 0) {
    throw std::runtime_error("[preamble_burst_source_cc] samples_per_burst must be > 0.");
  }
  if (d_silence_samples < 0) {
    throw std::runtime_error("[preamble_burst_source_cc] silence_samples must be >= 0.");
  }
  if (d_burst.empty()) {
    throw std::runtime_error("[preamble_burst_source_cc] burst vector must not be empty.");
  }

  // Initial build
  boost::lock_guard<boost::mutex> lock(d_mutex);
  rebuild_active_locked();
}

preamble_burst_source_cc_impl::~preamble_burst_source_cc_impl() {}

void
preamble_burst_source_cc_impl::validate_indices_locked() const
{
  if (!d_use_preamble_split) {
    return;
  }
  if (d_indices.empty()) {
    throw std::runtime_error("[preamble_burst_source_cc] indices must not be empty when use_preamble_split=true.");
  }
  for (size_t i = 0; i < d_indices.size(); i++) {
    if (d_indices[i] < 0 || d_indices[i] > 3) {
      throw std::runtime_error("[preamble_burst_source_cc] indices must be in [0..3].");
    }
  }
}

void
preamble_burst_source_cc_impl::rebuild_active_locked()
{
  if (!d_use_preamble_split) {
    // Use burst as-is (tone, custom waveform, etc.)
    d_active = d_burst;
    d_preamble_len = (int)d_active.size();
    return;
  }

  // Split into 4 preambles
  if ((d_burst.size() % 4) != 0) {
    throw std::runtime_error("[preamble_burst_source_cc] With use_preamble_split=true, burst length must be 4*N.");
  }

  d_preamble_len = (int)(d_burst.size() / 4);
  d_preambles4.resize(4);

  for (int i = 0; i < 4; i++) {
    d_preambles4[i].assign(d_burst.begin() + i * d_preamble_len,
                           d_burst.begin() + (i + 1) * d_preamble_len);
  }

  validate_indices_locked();

  // Sum selected indices
  d_active.assign(d_preamble_len, gr_complex(0, 0));
  for (size_t k = 0; k < d_indices.size(); k++) {
    const int idx = d_indices[k];
    for (int n = 0; n < d_preamble_len; n++) {
      d_active[n] += d_preambles4[idx][n];
    }
  }
}

void
preamble_burst_source_cc_impl::set_burst(const std::vector<gr_complex>& burst)
{
  if (burst.empty()) {
    return;
  }
  boost::lock_guard<boost::mutex> lock(d_mutex);
  d_burst = burst;
  rebuild_active_locked();
  d_state = 0;
  d_pos = 0;
}

void
preamble_burst_source_cc_impl::set_use_preamble_split(bool enable)
{
  boost::lock_guard<boost::mutex> lock(d_mutex);
  d_use_preamble_split = enable;
  rebuild_active_locked();
  d_state = 0;
  d_pos = 0;
}

void
preamble_burst_source_cc_impl::set_indices(const std::vector<int>& indices)
{
  boost::lock_guard<boost::mutex> lock(d_mutex);
  d_indices = indices;
  rebuild_active_locked();
  d_state = 0;
  d_pos = 0;
}

void
preamble_burst_source_cc_impl::set_silence_samples(int n)
{
  if (n < 0) {
    return;
  }
  boost::lock_guard<boost::mutex> lock(d_mutex);
  d_silence_samples = n;
  // no need to reset state
}

int
preamble_burst_source_cc_impl::work(int noutput_items,
                                    gr_vector_const_void_star&,
                                    gr_vector_void_star& output_items)
{
  gr_complex* out = (gr_complex*)output_items[0];

  // Snapshot under mutex (so we don't lock during the whole loop)
  std::vector<gr_complex> active_local;
  int pre_len = 0;
  int spb = 0;
  int silence = 0;
  bool tags = false;

  int state = 0;
  int pos = 0;

  {
    boost::lock_guard<boost::mutex> lock(d_mutex);
    active_local = d_active;
    pre_len = (int)d_active.size();
    spb = d_samples_per_burst;
    silence = d_silence_samples;
    tags = d_enable_tags;

    state = d_state;
    pos = d_pos;
  }

  int produced = 0;

  while (produced < noutput_items) {

    if (state == 0) {
      // Emit burst samples: we emit "spb" samples, where:
      // - first pre_len are the waveform
      // - remaining (if spb > pre_len) are zeros (padding)
      if (pos == 0 && tags) {
        // Optional tags if you want later (left disabled for now)
        // add_item_tag(0, nitems_written(0) + produced, pmt::string_to_symbol("tx_sob"), pmt::PMT_T);
      }

      if (pos >= spb) {
        state = 1;
        pos = 0;
        continue;
      }

      if (pos < pre_len) {
        out[produced++] = active_local[pos++];
      } else {
        out[produced++] = gr_complex(0, 0);
        pos++;
      }

      continue;
    }

    // state == 1: silence
    if (pos >= silence) {
      state = 0;
      pos = 0;
      continue;
    }
    out[produced++] = gr_complex(0, 0);
    pos++;
  }

  // Write back state
  {
    boost::lock_guard<boost::mutex> lock(d_mutex);
    d_state = state;
    d_pos = pos;
  }

  return produced;
}

} // namespace howto
} // namespace gr
