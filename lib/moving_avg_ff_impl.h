/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_MOVING_AVG_FF_IMPL_H
#define INCLUDED_HOWTO_MOVING_AVG_FF_IMPL_H

#include <howto/moving_avg_ff.h>
#include <vector>

namespace gr { namespace howto {

  /*!
   * Implementación simple de promedio móvil:
   *  - sync_block (1:1 entradas/salidas)
   *  - mantiene un buffer circular y la suma actual
   *  - N y scale se pueden cambiar en runtime con set_length() y set_scale()
   */
  class moving_avg_ff_impl final : public moving_avg_ff
  {
  private:
    int               d_N;       // tamaño de ventana
    float             d_scale;   // factor de escala
    std::vector<float> d_buf;    // buffer circular
    int               d_head;    // índice de escritura
    int               d_filled;  // cuántas posiciones válidas (<= N)
    float             d_sum;     // suma de la ventana

    static inline int clamp_len(int N) { return std::max(1, N); }
    inline void reset_state_(int newN);

  public:
    moving_avg_ff_impl(int length, float scale);
    ~moving_avg_ff_impl() override {}

    // Métodos públicos expuestos por la API
    void  set_length(int length) override;
    int   length() const override { return d_N; }

    void  set_scale(float scale) override;
    float scale() const override { return d_scale; }

    // Procesamiento principal
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;
  };

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_MOVING_AVG_FF_IMPL_H */

