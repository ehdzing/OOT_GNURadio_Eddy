/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_MOVING_AVG_FF_IMPL_H
#define INCLUDED_HOWTO_MOVING_AVG_FF_IMPL_H

#include <howto/moving_avg_history_ff.h>
#include <boost/thread/mutex.hpp>

namespace gr { namespace howto {

  /*!
   * Implementación simple de promedio móvil:
   *  - sync_block (1:1 entradas/salidas)
   *  - mantiene un buffer circular y la suma actual
   *  - N y scale se pueden cambiar en runtime con set_length() y set_scale()
   */
  class moving_avg_history_ff_impl final : public moving_avg_history_ff
  {
  private:
    // Parameters guarded by d_mutex
    int   d_length;     // window length (N)
    float d_scale;      // multiplicative scale
    boost::mutex d_mutex;
    
  public:
    moving_avg_history_ff_impl(int length, float scale);
    ~moving_avg_history_ff_impl() override {}

    // Métodos públicos expuestos por la API
    void  set_length(int length) override;
    //int   length() const override {}

    void  set_scale(float scale) override;
    //float scale() const override {}

    // Procesamiento principal
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;
  };

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_MOVING_AVG_FF_IMPL_H */

