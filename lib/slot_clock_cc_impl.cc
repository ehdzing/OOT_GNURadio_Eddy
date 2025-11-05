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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "slot_clock_cc_impl.h"
#include <cstring>

namespace gr {
  namespace howto {

    slot_clock_cc::sptr
    slot_clock_cc::make(int samples_per_slot,const std::string& srcid)
    {
      return gnuradio::get_initial_sptr
        (new slot_clock_cc_impl(samples_per_slot,srcid));
    }

    /*
     * The private constructor
     */
    slot_clock_cc_impl::slot_clock_cc_impl(int sps, const std::string& srcid) noexcept
      : gr::sync_block("slot_clock_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_sps(sps),
        d_key(pmt::intern("slot_start")),
        d_src(pmt::string_to_symbol(srcid))
    {
      //set_tag_propagation_policy(gr::TPP_ALL);
      set_tag_propagation_policy(gr::block::TPP_ALL_TO_ALL);
    }

    /*
     * Our virtual destructor.
     */
    slot_clock_cc_impl::~slot_clock_cc_impl() noexcept {}
    
    void
    slot_clock_cc_impl::set_samples_per_slot(int samples_per_slot) noexcept
    {
      boost::lock_guard<boost::mutex> g(d_mtx);
      d_sps = samples_per_slot > 0 ? samples_per_slot : 1;
    }

    int slot_clock_cc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *)input_items[0];
      gr_complex *out = (gr_complex *)output_items[0];

      // passthrough
      std::memcpy(out, in, sizeof(gr_complex) * noutput_items);
      
      int sps_local;
      {
        boost::lock_guard<boost::mutex> g(d_mtx);
        sps_local = d_sps;
      }
  
      const uint64_t base = nitems_written(0);

      for (int i = 0; i < noutput_items; ++i) {
          const uint64_t abs_off = base + i;
          if ((abs_off % (uint64_t)d_sps) == 0) {
              // igual que antes: tag en el offset absoluto que GNURadio ya entiende
              add_item_tag(0, abs_off, d_key, pmt::from_uint64(abs_off / (uint64_t)d_sps), d_src);
          }
      }
      return noutput_items;
   }

  } /* namespace howto */
} /* namespace gr */

