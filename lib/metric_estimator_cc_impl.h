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

#ifndef INCLUDED_HOWTO_METRIC_ESTIMATOR_CC_IMPL_H
#define INCLUDED_HOWTO_METRIC_ESTIMATOR_CC_IMPL_H

#include <howto/metric_estimator_cc.h>
#include <boost/thread/mutex.hpp>
#include <pmt/pmt.h>

namespace gr {
  namespace howto {

    class metric_estimator_cc_impl : public metric_estimator_cc
    {
     private:
      int d_sps;
      double d_thrL;
      double d_thrH;
      boost::mutex d_mtx;

      pmt::pmt_t d_k_slot;   // "slot_start"
      pmt::pmt_t d_k_apply;  // "apply_cfg"
      pmt::pmt_t d_port;     // "cqi_out"

      pmt::pmt_t cqi_from_power(double p);

     public:
      metric_estimator_cc_impl(int sps, double thrL, double thrH) noexcept;
      ~metric_estimator_cc_impl() noexcept override;;
  
      void set_thresholds(double thr_low, double thr_high) noexcept override;
      void set_samples_per_slot(int samples_per_slot) noexcept override;
      pmt::pmt_t select_mcs(pmt::pmt_t cqi) const noexcept;
  
      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_METRIC_ESTIMATOR_CC_IMPL_H */

