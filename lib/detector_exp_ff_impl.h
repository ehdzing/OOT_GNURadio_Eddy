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

#ifndef INCLUDED_HOWTO_DETECTOR_EXP_FF_IMPL_H
#define INCLUDED_HOWTO_DETECTOR_EXP_FF_IMPL_H

#include <howto/detector_exp_ff.h>
#include <boost/thread/mutex.hpp>

namespace gr {
  namespace howto {

    /*!
    * \brief Implementation of detector_exp_ff.
    * Computes exponential energy envelope and hysteresis events.
    */
    class detector_exp_ff_impl final : public detector_exp_ff
    {
    private:
      int   d_length;   //!< kept for priming/compatibility
      float d_env;      //!< exponential envelope
      float d_alpha;    //!< smoothing factor (0<alpha<1)
      float d_Ton;      //!< threshold ON
      float d_Toff;     //!< threshold OFF
      bool  d_state;    //!< current state (true = active)
      boost::mutex d_mtx;

      void publish_event(const char* ev, uint64_t idx, float env) noexcept;
      void tag_event(int out_port, uint64_t abs_off,
                    const char* ev, float env) noexcept;
      void passthrough_tags(uint64_t abs_read,
                            uint64_t abs_write,
                            int noutput_items) noexcept;

    public:
      explicit detector_exp_ff_impl(int length);
      ~detector_exp_ff_impl() override;

      void set_length(int n) noexcept override;
      void set_Ton(float t) noexcept override;
      void set_Toff(float t) noexcept override;

      int work(int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items) override;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_DETECTOR_EXP_FF_IMPL_H */
