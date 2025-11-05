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

#ifndef INCLUDED_HOWTO_SIM_TX_MODULATOR_CC_IMPL_H
#define INCLUDED_HOWTO_SIM_TX_MODULATOR_CC_IMPL_H

#include <howto/sim_tx_modulator_cc.h>
#include <boost/thread/mutex.hpp>
#include <deque>
#include <pmt/pmt.h>

namespace gr {
  namespace howto {

    class sim_tx_modulator_cc_impl : public sim_tx_modulator_cc
    {
     private:
      struct pending_change {
        uint64_t apply_at;   // índice absoluto de muestra donde aplicar
        float    gain;       // ganancia a usar a partir de ahí
      };

      boost::mutex      d_mtx;
      std::deque<pending_change> d_queue;
      float             d_current_gain;

      pmt::pmt_t        d_k_apply;   // "apply_cfg"

      void on_ctrl(pmt::pmt_t msg) noexcept;
      float gain_from_mcs(const std::string &mcs) const noexcept;
      float gain_from_dict(pmt::pmt_t dict) const noexcept;

     public:
      sim_tx_modulator_cc_impl();
      ~sim_tx_modulator_cc_impl() noexcept override;

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_SIM_TX_MODULATOR_CC_IMPL_H */

