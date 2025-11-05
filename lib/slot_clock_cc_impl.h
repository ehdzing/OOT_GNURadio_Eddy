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

#ifndef INCLUDED_HOWTO_SLOT_CLOCK_CC_IMPL_H
#define INCLUDED_HOWTO_SLOT_CLOCK_CC_IMPL_H

#include <howto/slot_clock_cc.h>
#include <boost/thread/mutex.hpp>
#include <pmt/pmt.h>

namespace gr {
  namespace howto {

    class slot_clock_cc_impl : public slot_clock_cc
    {
     private:
      int d_sps;
      boost::mutex d_mtx;
      pmt::pmt_t d_key;
      pmt::pmt_t d_src;

     public:
      slot_clock_cc_impl(int sps,const std::string& srcid) noexcept;
      ~slot_clock_cc_impl() noexcept override;
      
      void set_samples_per_slot(int samples_per_slot) noexcept override;
      
      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_SLOT_CLOCK_CC_IMPL_H */

