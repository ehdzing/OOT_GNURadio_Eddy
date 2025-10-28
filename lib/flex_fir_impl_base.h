#ifndef INCLUDED_HOWTO_FLEX_FIR_IMPL_BASE_H
#define INCLUDED_HOWTO_FLEX_FIR_IMPL_BASE_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>

namespace gr { namespace howto {

enum { FLEX_FIR_LP = 0, FLEX_FIR_HP = 1, FLEX_FIR_BP = 2 };

template<typename Tin, typename Tout>
class flex_fir_impl_base
{
protected:
  mutable boost::mutex d_mutex;
  int   d_mode;
  float d_fs, d_f1, d_f2, d_width, d_gain;
  std::vector<float> d_taps;
  std::vector<Tin>   d_hist;
  bool  d_dirty;

  static inline float sinc_(float x) { return x == 0.0f ? 1.0f : std::sin(M_PI*x)/(M_PI*x); }

  static std::vector<float> hamming_(size_t N)
  {
    std::vector<float> w(N);
    for(size_t n=0;n<N;++n) w[n] = 0.54f - 0.46f*std::cos(2.0f*M_PI*float(n)/(N-1));
    return w;
  }

  void design_taps_()
  {
    // Diseño rápido estilo firdes “casero”: ventana Hamming + sinc
    // width = ancho de transición (Hz). Convertimos a N aproximado:
    const float tw = std::max(1.0f, d_width);
    size_t N = static_cast<size_t>(std::ceil(4.0f * d_fs / tw)); // regla simple
    N |= 1; // impar

    std::vector<float> h(N, 0.0f);
    const int M = (int)N/2;
    const float fc1 = d_f1 / d_fs; // normalizadas
    const float fc2 = d_f2 / d_fs;

    for(int n=-M; n<=M; ++n) {
      float w = hamming_(N)[n+M];
      float val = 0.0f;
      if(d_mode == FLEX_FIR_LP) {
        val = 2.0f*fc1*sinc_(2.0f*fc1*n);
      } else if(d_mode == FLEX_FIR_HP) {
        if(n==0) val = 1.0f - 2.0f*fc1;
        else     val = -2.0f*fc1*sinc_(2.0f*fc1*n);
      } else { // BP
        val = 2.0f*fc2*sinc_(2.0f*fc2*n) - 2.0f*fc1*sinc_(2.0f*fc1*n);
      }
      h[n+M] = w * val;
    }

    // Ganancia
    for(size_t i=0;i<N;++i) h[i] *= d_gain;

    d_taps.swap(h);
    d_hist.clear();
    d_hist.resize(N-1, Tin());
  }

  void snapshot_params_(int& mode, float& fs, float& f1, float& f2, float& width, float& gain,
                        std::vector<float>& taps)
  {
    boost::lock_guard<boost::mutex> g(d_mutex);
    if(d_dirty) { design_taps_(); d_dirty = false; }
    mode  = d_mode;  fs = d_fs; f1 = d_f1; f2 = d_f2; width = d_width; gain = d_gain;
    taps  = d_taps;
  }

public:
  flex_fir_impl_base(int mode, float fs, float f1, float f2, float width, float gain)
  : d_mode(mode), d_fs(fs), d_f1(f1), d_f2(f2), d_width(width), d_gain(gain), d_dirty(true)
  {}

  void set_mode(int m) noexcept { boost::lock_guard<boost::mutex> lck(d_mutex); d_mode = m; d_dirty = true; }
  int  mode() const noexcept    { boost::lock_guard<boost::mutex> lck(d_mutex); return d_mode; }

  void set_samp_rate(float fs) noexcept { boost::lock_guard<boost::mutex> lck(d_mutex); d_fs = fs; d_dirty = true; }
  float samp_rate() const noexcept      { boost::lock_guard<boost::mutex> lck(d_mutex); return d_fs; }

  void set_f1(float f) noexcept { boost::lock_guard<boost::mutex> lck(d_mutex); d_f1 = f; d_dirty = true; }
  float f1() const noexcept     { boost::lock_guard<boost::mutex> glck(d_mutex); return d_f1; }

  void set_f2(float f) noexcept { boost::lock_guard<boost::mutex> lck(d_mutex); d_f2 = f; d_dirty = true; }
  float f2() const noexcept     { boost::lock_guard<boost::mutex> lck(d_mutex); return d_f2; }

  void set_width(float w) noexcept { boost::lock_guard<boost::mutex> lck(d_mutex); d_width = w; d_dirty = true; }
  float width() const noexcept     { boost::lock_guard<boost::mutex> lck(d_mutex); return d_width; }

  void set_gain(float g) noexcept { boost::lock_guard<boost::mutex> lck(d_mutex); d_gain = g; d_dirty = true; }
  float gain() const noexcept     { boost::lock_guard<boost::mutex> lck(d_mutex); return d_gain; }

  std::vector<float> taps() const { boost::lock_guard<boost::mutex> lck(d_mutex); return d_taps; }
};

}} // namespace
#endif

