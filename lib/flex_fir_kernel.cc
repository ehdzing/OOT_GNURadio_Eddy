#ifndef INCLUDED_HOWTO_FLEX_FIR_KERNEL_TCC
#define INCLUDED_HOWTO_FLEX_FIR_KERNEL_TCC

#include <vector>
#include <complex>
#include <cstring>

namespace gr { namespace howto {

template<typename Tin, typename Tout>
static inline void write_sample_(Tout& dst, const Tin& x) { dst = static_cast<Tout>(x); }

// especialización: Tin=complex<float>, Tout=float -> por defecto paso parte real
template<>
inline void write_sample_<std::complex<float>, float>(float& dst, const std::complex<float>& x)
{
  dst = x.real();
}

template<typename Tin, typename Tout>
int flex_fir_work_body(int noutput_items,
                       const Tin* in, Tout* out,
                       std::vector<Tin>& hist,
                       const std::vector<float>& taps)
{
  const int T = static_cast<int>(taps.size());
  if (T <= 0) {
    std::memset(out, 0, sizeof(Tout)*noutput_items);
    return noutput_items;
  }

  // Concatena historial + entrada
  std::vector<Tin> buf;
  buf.reserve(hist.size() + noutput_items);
  buf.insert(buf.end(), hist.begin(), hist.end());
  buf.insert(buf.end(), in, in + noutput_items);

  // Convolución directa (sí, O(NT); esto es referencia simple)
  for (int n = 0; n < noutput_items; ++n) {
    // acumula en tipo intermedio
    typename std::conditional<std::is_same<Tin, std::complex<float>>::value,
                              std::complex<float>, float>::type acc(0);
    for (int k = 0; k < T; ++k) {
      acc += buf[n + T - 1 - k] * taps[k];
    }
    write_sample_<decltype(acc), Tout>(out[n], acc);
  }

  // actualiza historial
  if (T > 1) {
    hist.assign(buf.end() - (T - 1), buf.end());
  }

  return noutput_items;
}

}} // namespace
#endif

