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
#include "sim_tx_modulator_cc_impl.h"
#include <algorithm>
#include <vector>
#include <cstring>
#include <iostream>


namespace gr {
  namespace howto {

    sim_tx_modulator_cc::sptr
    sim_tx_modulator_cc::make()
    {
      return gnuradio::get_initial_sptr
        (new sim_tx_modulator_cc_impl());
    }

    /*
     * The private constructor
     */
    sim_tx_modulator_cc_impl::sim_tx_modulator_cc_impl()
      : gr::sync_block("sim_tx_modulator_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_current_gain(1.0f),
        d_k_apply(pmt::intern("apply_cfg"))
    {
      // puerto de control por mensajes
      message_port_register_in(pmt::mp("tx_ctrl"));
      set_msg_handler(pmt::mp("tx_ctrl"),
                      boost::bind(&sim_tx_modulator_cc_impl::on_ctrl, this, _1));

      set_tag_propagation_policy(gr::block::TPP_ALL_TO_ALL);
    }

    /*
     * Our virtual destructor.
     */
    sim_tx_modulator_cc_impl::~sim_tx_modulator_cc_impl() noexcept {}
    
    float
    sim_tx_modulator_cc_impl::gain_from_mcs(const std::string &mcs) const noexcept
    {
      // mapeo tonto ejemplo
      if (mcs == "mcs_64qam"){
        //std::cout<<"Seleccionar la gnanacia a 3"<< std::endl;
        return 3.0f;
      } 
      if (mcs == "mcs_16qam"){
        //std::cout<<"Seleccionar la gnanacia a 2"<< std::endl;
        return 2.0f;
      } 
      if (mcs == "mcs_qpsk"){
        //std::cout<<"Seleccionar la gnanacia a 1"<< std::endl;
        return 1.0f;
      } 
      // desconocido
      return 1.0f;
    }

    float
    sim_tx_modulator_cc_impl::gain_from_dict(pmt::pmt_t dict) const noexcept
    {
      // dict se espera que tenga {"mcs": <symbol>} o algo parecido
      pmt::pmt_t mcs = pmt::dict_ref(dict, pmt::intern("mcs"), pmt::PMT_NIL);
      if (pmt::is_symbol(mcs)) {
        //std::cout<< "MSC es un simbol y va a seleccionar la gnanacia"<< std::endl;
        return gain_from_mcs(pmt::symbol_to_string(mcs));
      }
      return 1.0f;
    }

    void
    sim_tx_modulator_cc_impl::on_ctrl(pmt::pmt_t msg) noexcept
    {
      //std::cout<< "Entro un msg"<< std::endl;
      // mensaje externo. puede tener "apply_at" o no.
      if (!pmt::is_dict(msg))
        return;

      //std::cout<< "El msg que entro es un dict"<< std::endl;

      uint64_t apply_at = 0;
      bool has_apply_at = false;

      pmt::pmt_t p_apply = pmt::dict_ref(msg, pmt::intern("apply_at"), pmt::PMT_NIL);
      if (pmt::is_uint64(p_apply)) {
        apply_at = pmt::to_uint64(p_apply);
        has_apply_at = true;
        //std::cout<< "Llego msg y hay datos"<< std::endl;
      }

      float g = gain_from_dict(msg);

      pending_change ev;
      ev.apply_at = has_apply_at ? apply_at : 0; // si no viene, lo aplicamos ya en work
      ev.gain     = g;

      {
        boost::lock_guard<boost::mutex> lk(d_mtx);
        d_queue.push_back(ev);
        /*const auto& last = d_queue.back();
        std::cout << "apply_at=" << last.apply_at
              << "  gain=" << last.gain << "\n";
              */
      }
    }


    int
    sim_tx_modulator_cc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *)input_items[0];
      gr_complex *out = (gr_complex *)output_items[0];

      const uint64_t base = nitems_read(0);
      const uint64_t end_abs = base + (uint64_t)noutput_items;

      // 1) copiar entrada→salida luego vamos machacando por tramos
      // (podríamos ir directo pero así queda más claro)
      //std::memcpy(out, in, sizeof(gr_complex) * noutput_items);

      // 2) recolectar cambios pendientes desde mensajes que caen en esta ventana
      std::vector<pending_change> local_changes;
      {
        boost::lock_guard<boost::mutex> lk(d_mtx);

        // primero, aplicar los que quedaron "en el pasado" (apply_at < base)
        // esos cambian la ganancia actual antes de procesar esta ventana
        while (!d_queue.empty()) {
          const pending_change &ev = d_queue.front();
          if (ev.apply_at == 0 || ev.apply_at < base) {
            // aplicar ya
            //std::cout<< "Msg antes del rango"<< std::endl;
            d_current_gain = ev.gain;
            d_queue.pop_front();
          } else if (ev.apply_at >= base && ev.apply_at < end_abs) {
            // este sí cae dentro de la ventana
            //std::cout<< "Msg dentro del rango"<< std::endl;
            local_changes.push_back(ev);
            d_queue.pop_front();
          } else {
            //std::cout<< "Msg despues del rango"<< std::endl;
            // este y los que siguen son del futuro, dejamos de mirar
            break;
          }
        }
      }

      // 3) recolectar tags "apply_cfg" que vengan pegados a este tramo
      std::vector<tag_t> tags;
      get_tags_in_range(tags, 0, base, end_abs, d_k_apply);

      for (size_t i = 0; i < tags.size(); ++i) {
        //std::cout<< "Vio algun tag"<< std::endl;
        const tag_t &tg = tags[i];
        // offset absoluto del tag
        const uint64_t toff = tg.offset;
        if (toff < base || toff >= end_abs)
          continue;

        // el valor del tag se espera que sea un dict con "mcs" u otra cosa
        float g = 1.0f;
        if (pmt::is_dict(tg.value)) {
          g = gain_from_dict(tg.value);
          //std::cout<< "El tag es un dict y la ganancia es"<< g << std::endl;
        } else if (pmt::is_symbol(tg.value)) {
          g = gain_from_mcs(pmt::symbol_to_string(tg.value));
          //std::cout<< "El tag es un symbol y la ganancia es"<< g << std::endl;
        }

        pending_change ev;
        ev.apply_at = toff;
        ev.gain     = g;
        local_changes.push_back(ev);
      }

      // 4) si no hay cambios dentro del rango, aplicar todo con la ganancia actual
      if (local_changes.empty()) {
        const float g_now = d_current_gain;
        for (int i = 0; i < noutput_items; ++i)
          out[i] = in[i] * g_now;
        return noutput_items;
      }

      // 5) ordenar los cambios por offset absoluto
      std::sort(local_changes.begin(),
                local_changes.end(),
                [](const pending_change &a, const pending_change &b) {
                  return a.apply_at < b.apply_at;
                });

      // 6) recorrer y aplicar por tramos
      float gain_now = d_current_gain;
      int cursor = 0;

      for (size_t k = 0; k < local_changes.size(); ++k) {
        const pending_change &ev = local_changes[k];
        const int rel = (int)(ev.apply_at - base);
        
        // evento más allá del bloque actual: paramos y dejamos que quede en cola
        if (rel >= noutput_items) {
            break;
        }
    
        // si el cambio cayó antes del cursor, solo actualizamos la ganancia
        if (rel <= cursor) {
          gain_now = ev.gain;
          continue;
        }

        // tramo [cursor, rel)
        for (int i = cursor; i < rel; ++i)
          out[i] = in[i] * gain_now;

        cursor = rel;
        gain_now = ev.gain;
      }

      // 7) tramo final
      for (int i = cursor; i < noutput_items; ++i)
        out[i] = in[i] * gain_now;

      // 8) guardar la última ganancia como estado para la próxima llamada
      {
        boost::lock_guard<boost::mutex> lk(d_mtx);
        d_current_gain = gain_now;
      }

      return noutput_items;
    }

  } /* namespace howto */
} /* namespace gr */

