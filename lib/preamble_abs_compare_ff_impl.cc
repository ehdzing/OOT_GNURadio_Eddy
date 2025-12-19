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

#include "preamble_abs_compare_ff_impl.h"

#include <gnuradio/io_signature.h>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdlib>

namespace gr {
namespace howto {

std::string preamble_abs_compare_ff_impl::trim_copy(const std::string& s)
{
  size_t a = 0;
  while (a < s.size() && std::isspace((unsigned char)s[a])) a++;
  size_t b = s.size();
  while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
  return s.substr(a, b - a);
}

int preamble_abs_compare_ff_impl::parse_preamble_select_mask_4(const std::string& sel)
{
  std::string s = trim_copy(sel);
  if (s.empty()) return 15;

  std::string low = s;
  std::transform(low.begin(), low.end(), low.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  if (low == "all" || low == "[all]") return 15;

  if (s.size() >= 2 && s.front() == '[' && s.back() == ']') {
    s = trim_copy(s.substr(1, s.size() - 2));
  }
  if (s.empty()) return 15;

  std::stringstream ss(s);
  std::string tok;
  int mask = 0;
  bool saw_any = false;

  while (std::getline(ss, tok, ',')) {
    tok = trim_copy(tok);
    if (tok.empty()) continue;

    char* endp = 0;
    long v = std::strtol(tok.c_str(), &endp, 10);
    if (endp == tok.c_str() || *endp != '\0') continue;

    if (v >= 0 && v <= 3) {
      mask |= (1 << (int)v);
      saw_any = true;
    }
  }

  mask &= 0x0F;
  if (!saw_any || mask == 0) return 15;
  return mask;
}

preamble_abs_compare_ff::sptr
preamble_abs_compare_ff::make(const std::vector<gr_complex>& preambles,
                              int preamble_layout,
                              const std::string& preamble_select)
{
  return gnuradio::get_initial_sptr(
      new preamble_abs_compare_ff_impl(preambles, preamble_layout, preamble_select));
}

preamble_abs_compare_ff_impl::preamble_abs_compare_ff_impl(const std::vector<gr_complex>& preambles,
                                                           int preamble_layout,
                                                           const std::string& preamble_select)
  : preamble_abs_compare_ff("preamble_abs_compare_ff",
                            gr::io_signature::make(2, 2, sizeof(gr_complex)),
                            gr::io_signature::make(12, 12, sizeof(float))),
    d_preamble_layout(preamble_layout),
    d_preamble_select(preamble_select),
    d_preamble_mask(parse_preamble_select_mask_4(preamble_select)),
    d_N(0),
    d_preambles_param(preambles)
{
  if (d_preambles_param.empty()) {
    throw std::runtime_error("[preamble_abs_compare_ff] preambles vector is empty.");
  }

  boost::lock_guard<boost::mutex> lock(d_mutex);
  build_references_from_param_locked(d_preambles_param);
}

preamble_abs_compare_ff_impl::~preamble_abs_compare_ff_impl() {}

void preamble_abs_compare_ff_impl::set_preamble_layout(int layout)
{
  boost::lock_guard<boost::mutex> lock(d_mutex);
  if (!(layout == 0 || layout == 1 || layout == 4)) return;
  if (layout == d_preamble_layout) return;

  d_preamble_layout = layout;
  build_references_from_param_locked(d_preambles_param);
}

void preamble_abs_compare_ff_impl::set_preamble_select(const std::string& sel)
{
  boost::lock_guard<boost::mutex> lock(d_mutex);
  const int new_mask = parse_preamble_select_mask_4(sel);
  if (sel == d_preamble_select && new_mask == d_preamble_mask) return;

  d_preamble_select = sel;
  d_preamble_mask = new_mask;
  build_references_from_param_locked(d_preambles_param);
}

void preamble_abs_compare_ff_impl::build_ref0_from_4_locked(const std::vector<gr_complex>& preambles)
{
  const int N = (int)(preambles.size() / 4);
  d_N = N;
  d_ref0.assign(d_N, gr_complex(0, 0));

  const int mask = (d_preamble_mask & 0x0F) ? (d_preamble_mask & 0x0F) : 15;

  for (int k = 0; k < 4; k++) {
    if (((mask >> k) & 0x1) == 0) continue;
    const gr_complex* pk = &preambles[(size_t)k * d_N];
    for (int n = 0; n < d_N; n++) {
      d_ref0[n] += pk[n];
    }
  }
}

void preamble_abs_compare_ff_impl::build_references_from_param_locked(const std::vector<gr_complex>& preambles)
{
  const size_t L = preambles.size();
  const bool auto_mode = (d_preamble_layout == 0);
  const bool force_single = (d_preamble_layout == 1);
  const bool force_concat4 = (d_preamble_layout == 4);

  if (force_concat4 || (auto_mode && ((L % 4) == 0) && !force_single)) {
    build_ref0_from_4_locked(preambles);
  } else {
    d_N = (int)L;
    d_ref0 = preambles;
  }

  d_ref1.resize(d_N);
  for (int i = 0; i < d_N; i++) {
    d_ref1[i] = -std::conj(d_ref0[d_N - i - 1]);
  }
}

void preamble_abs_compare_ff_impl::forecast(int, gr_vector_int& ninput_items_required)
{
  ninput_items_required[0] = d_N;
  ninput_items_required[1] = d_N;
}

int preamble_abs_compare_ff_impl::general_work(int noutput_items,
                                               gr_vector_int& ninput_items,
                                               gr_vector_const_void_star& input_items,
                                               gr_vector_void_star& output_items)
{
  const gr_complex* in0 = (const gr_complex*)input_items[0];
  const gr_complex* in1 = (const gr_complex*)input_items[1];

  float* o0 = (float*)output_items[0];
  float* o1 = (float*)output_items[1];
  float* o2 = (float*)output_items[2];
  float* o3 = (float*)output_items[3];
  float* o4 = (float*)output_items[4];
  float* o5 = (float*)output_items[5];
  float* o6 = (float*)output_items[6];
  float* o7 = (float*)output_items[7];
  float* o8 = (float*)output_items[8];
  float* o9 = (float*)output_items[9];
  float* o10 = (float*)output_items[10];
  float* o11 = (float*)output_items[11];

  int N = d_N;
  if (N <= 0) return 0;
  if (ninput_items[0] < N || ninput_items[1] < N) return 0;
  if (noutput_items < N) return 0;

  for (int i = 0; i < N; i++) {
    o0[i]  = in0[i].real();
    o1[i]  = in0[i].imag();
    o2[i]  = d_ref0[i].real();
    o3[i]  = d_ref0[i].imag();
    o4[i]  = in0[i].real() - d_ref0[i].real();
    o5[i]  = in0[i].imag() - d_ref0[i].imag();

    o6[i]  = in1[i].real();
    o7[i]  = in1[i].imag();
    o8[i]  = d_ref1[i].real();
    o9[i]  = d_ref1[i].imag();
    o10[i] = in1[i].real() - d_ref1[i].real();
    o11[i] = in1[i].imag() - d_ref1[i].imag();
  }

  consume_each(N);
  return N;
}

} // namespace howto
} // namespace gr
