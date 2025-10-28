/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_FLEX_FIR_COMMON_H
#define INCLUDED_HOWTO_FLEX_FIR_COMMON_H

#include <vector>
#include <algorithm>
#include <gnuradio/filter/firdes.h>

namespace howto_flexfir_detail {

inline std::vector<float> design_taps_lpf(float g, float fs, float f1, float width) {
  using gr::filter::firdes;
  return firdes::low_pass(g, fs, std::max(0.0f, f1), std::max(width, fs*0.005f),
                          firdes::WIN_HAMMING, 6.76);
}
inline std::vector<float> design_taps_hpf(float g, float fs, float f1, float width) {
  using gr::filter::firdes;
  return firdes::high_pass(g, fs, std::max(0.0f, f1), std::max(width, fs*0.005f),
                           firdes::WIN_HAMMING, 6.76);
}
inline std::vector<float> design_taps_bpf(float g, float fs, float f1, float f2, float width) {
  using gr::filter::firdes;
  const float nyq = 0.5f*fs;
  float lo = std::max(0.0f, std::min(f1, f2));
  float hi = std::max(0.0f, std::max(f1, f2));
  if (hi <= lo) hi = std::min(nyq, lo + std::max(width, fs*0.01f));
  return firdes::band_pass(g, fs, lo, hi, std::max(width, fs*0.005f),
                           firdes::WIN_HAMMING, 6.76);
}

} // namespace howto_flexfir_detail

#endif

