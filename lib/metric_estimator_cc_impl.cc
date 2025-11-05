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
#include "metric_estimator_cc_impl.h"
#include <cstring>
#include <cmath>

namespace gr {
  namespace howto {

    metric_estimator_cc::sptr
    metric_estimator_cc::make(int samples_per_slot, double thr_low, double thr_high)
    {
      return gnuradio::get_initial_sptr
        (new metric_estimator_cc_impl(samples_per_slot, thr_low, thr_high));
    }

    /*
     * The private constructor
     */
    metric_estimator_cc_impl::metric_estimator_cc_impl(int sps, double thrL, double thrH) noexcept
      : gr::sync_block("metric_estimator_cc",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_sps(sps),
        d_thrL(thrL),
        d_thrH(thrH),
        d_k_slot(pmt::intern("slot_start")),
        d_k_apply(pmt::intern("apply_cfg")),
        d_port(pmt::mp("cqi_out"))
    {
    // puerto de mensaje de salida
    message_port_register_out(d_port);

    // necesitamos ver el slot completo hacia atrás
    set_history(d_sps);

    // dejamos que pasen los tags
    //set_tag_propagation_policy(TPP_ALL);
    }

    /*
     * Our virtual destructor.
     */
    metric_estimator_cc_impl::~metric_estimator_cc_impl() noexcept {}
    
    void
    metric_estimator_cc_impl::set_thresholds(double thr_low, double thr_high) noexcept
    {
      boost::lock_guard<boost::mutex> g(d_mtx);
      d_thrL = thr_low;
      d_thrH = thr_high;
    }

    void
    metric_estimator_cc_impl::set_samples_per_slot(int samples_per_slot) noexcept
    {
      boost::lock_guard<boost::mutex> g(d_mtx);
      d_sps = (samples_per_slot > 0) ? samples_per_slot : 1;
    }

    pmt::pmt_t
    metric_estimator_cc_impl::cqi_from_power(double p) 
    {
      double L, H;
      {
        boost::lock_guard<boost::mutex> g(d_mtx);
        L = d_thrL;
        H = d_thrH;
      }

      if (p < L)  return pmt::intern("cqi_low");
      if (p > H)  return pmt::intern("cqi_high");
      return pmt::intern("cqi_med");
    }

    int
    metric_estimator_cc_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *)input_items[0];
      gr_complex *out = (gr_complex *)output_items[0];

      std::memcpy(out, in, sizeof(gr_complex) * noutput_items);

      int sps_local;
      {
        boost::lock_guard<boost::mutex> g(d_mtx);
        sps_local = d_sps;
      }

      if (history() != sps_local) {
        set_history(sps_local);
        return 0;
      }

      const uint64_t base = nitems_read(0);

      std::vector<tag_t> tags;
      get_tags_in_range(tags,
                        0,
                        base,
                        base + noutput_items,
                        d_k_slot);

      for (size_t t = 0; t < tags.size(); ++t) {
        const tag_t &tg = tags[t];
        const uint64_t off = tg.offset;
        const int rel = (int)(off - base);
        if (rel < 0 || rel >= noutput_items)
          continue;

        const int idx_center = (sps_local - 1) + rel;

        double sum = 0.0;
        for (int k = 0; k < sps_local; ++k) {
          const gr_complex s = in[idx_center - k];
          sum += (double)(s.real()*s.real() + s.imag()*s.imag());
        }
        const double avgp = sum / (double)sps_local;

        pmt::pmt_t cqi = cqi_from_power(avgp);

        pmt::pmt_t m = pmt::make_dict();
        m = pmt::dict_add(m, pmt::intern("slot_abs"), pmt::from_uint64(off));
        m = pmt::dict_add(m, pmt::intern("cqi"), cqi);
        m = pmt::dict_add(m, pmt::intern("pwr"), pmt::from_double(avgp));
        message_port_pub(d_port, m);

        //const uint64_t next_off = off + (uint64_t)sps_local;
        pmt::pmt_t tv = pmt::make_dict();
        tv = pmt::dict_add(tv, pmt::intern("mcs"), cqi);

        add_item_tag(0,
                     off,
                     d_k_apply,
                     tv,
                     pmt::string_to_symbol(alias()));
      }

      return noutput_items;
    }

  } /* namespace howto */
} /* namespace gr */

