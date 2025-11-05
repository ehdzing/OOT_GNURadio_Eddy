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
#include "scheduler_ctrl_impl.h"

namespace gr {
  namespace howto {

    scheduler_ctrl::sptr
    scheduler_ctrl::make(int samples_per_slot)
    {
      return gnuradio::get_initial_sptr
        (new scheduler_ctrl_impl(samples_per_slot));
    }

    /*
     * The private constructor
     */
    scheduler_ctrl_impl::scheduler_ctrl_impl(int samples_per_slot)
      : gr::sync_block("scheduler_ctrl",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0)),
        d_sps(samples_per_slot)
    {
      message_port_register_in(pmt::mp("cqi_in"));
      set_msg_handler(pmt::mp("cqi_in"),
                      boost::bind(&scheduler_ctrl_impl::on_cqi, this, _1));

      message_port_register_out(pmt::mp("tx_ctrl"));
    }
    /*
     * Our virtual destructor.
     */
    scheduler_ctrl_impl::~scheduler_ctrl_impl() noexcept {}
    
    void
    scheduler_ctrl_impl::set_samples_per_slot(int sps) noexcept
    {
      boost::lock_guard<boost::mutex> g(d_mtx);
      d_sps = (sps > 0) ? sps : 1;
    }

    pmt::pmt_t
    scheduler_ctrl_impl::select_mcs(pmt::pmt_t cqi) const noexcept
    {
      if (!pmt::is_symbol(cqi))
        return pmt::PMT_NIL;;

      const std::string s = pmt::symbol_to_string(cqi);
      if (s == "cqi_high"){
        //std::cout<< "Selecciona mcs_64qam"<< std::endl;
        return pmt::intern("mcs_64qam");
      }
      if (s == "cqi_med"){
        //std::cout<< "Selecciona mcs_16qam"<< std::endl;
        return pmt::intern("mcs_16qam");
      }
      if (s == "cqi_low"){
        //std::cout<< "Selecciona mcs_qpsk"<< std::endl;
        return pmt::intern("mcs_qpsk");
      }

      //std::cout<< "Selecciona tipo de mcs pmt::PMT_NIL"<< std::endl;
      // cqi desconocido → no emitir
      return pmt::PMT_NIL;
    }

    void
    scheduler_ctrl_impl::on_cqi(pmt::pmt_t msg) noexcept
    { 
      //std::cout<< "Scheduler recibio un msg"<< std::endl;
      if (!pmt::is_dict(msg))
        return;

      //std::cout<< "ES un dict"<< std::endl;

      pmt::pmt_t cqi      = pmt::dict_ref(msg, pmt::intern("cqi"), pmt::PMT_NIL);
      pmt::pmt_t slot_abs = pmt::dict_ref(msg, pmt::intern("slot_abs"), pmt::PMT_NIL);

      if (!pmt::is_uint64(slot_abs))
        return;

      //std::cout<< "El slot_abs es un uint64"<< std::endl;

      // decidir MCS
      pmt::pmt_t mcs = select_mcs(cqi);
      if (pmt::eq(mcs, pmt::PMT_NIL)) {
        // no hay MCS válido, no molestamos al TX
        return;
      }

      const uint64_t slot_val = pmt::to_uint64(slot_abs);
      
      //int sps_local;
      //{
      //  boost::lock_guard<boost::mutex> g(d_mtx);
      //  sps_local = d_sps;
      //}
      
      //const uint64_t apply_at = slot_val + (uint64_t)sps_local;
      const uint64_t apply_at = slot_val;
      
      
  
      pmt::pmt_t out = pmt::make_dict();
      out = pmt::dict_add(out, pmt::intern("apply_at"), pmt::from_uint64(apply_at));
      out = pmt::dict_add(out, pmt::intern("mcs"), mcs);
      // opcional, reenviamos cqi original
      out = pmt::dict_add(out, pmt::intern("cqi"), cqi);

      message_port_pub(pmt::mp("tx_ctrl"), out);
      //std::cout<< "Envia el sms por el puerto de salida"<< std::endl;
    }
    
    int
    scheduler_ctrl_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      return 0;
    }

  } /* namespace howto */
} /* namespace gr */

