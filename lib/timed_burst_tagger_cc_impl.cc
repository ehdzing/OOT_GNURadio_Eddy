/* -*- c++ -*- */
/* Copyright 2025 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <howto/timed_burst_tagger_cc.h>
#include "timed_burst_tagger_cc_impl.h"

#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

using namespace std;

namespace gr {
  namespace howto {

    /******************** Tag helpers ********************/
    inline void
    timed_burst_tagger_cc_impl::add_out_tag_(const pmt::pmt_t& key,
                                             const pmt::pmt_t& val,
                                             uint64_t abs_out_idx)
    {
      // use this-> to avoid ADL weirdness in older compilers
      this->add_item_tag(0, abs_out_idx, key, val, pmt::intern(this->name()));
    }

    void
    timed_burst_tagger_cc_impl::forward_input_tags_(uint64_t in_abs_base, int in_offset0, int ncopy,
                                                    uint64_t out_abs_base, int out_offset0)
    {
      const uint64_t in_start = in_abs_base + (uint64_t)in_offset0;
      const uint64_t in_end   = in_start + (uint64_t)ncopy; // exclusive

      std::vector<tag_t> tags;
      this->get_tags_in_range(tags, 0, in_start, in_end);

      for (const auto& t : tags) {
        const int rel = (int)(t.offset - in_start);
        if (rel < 0 || rel >= ncopy) continue;
        const uint64_t out_abs = out_abs_base + (uint64_t)out_offset0 + (uint64_t)rel;
        this->add_item_tag(0, out_abs, t.key, t.value, t.srcid);
      }
    }

    /******************** Factory ********************/
    timed_burst_tagger_cc::sptr
    timed_burst_tagger_cc::make(double rate,
                                double burst_len_s,
                                double period_s,
                                std::vector<double> offsets_s,
                                float amplitude,
                                double t0_hw,
                                bool eob_en)
    {
      return timed_burst_tagger_cc::sptr(
        new timed_burst_tagger_cc_impl(rate, burst_len_s, period_s, offsets_s, amplitude, t0_hw, eob_en));
    }

    /******************** Constructor ********************/
    timed_burst_tagger_cc_impl::timed_burst_tagger_cc_impl(double rate,
                                                           double burst_len_s,
                                                           double period_s,
                                                           std::vector<double> offsets_s,
                                                           float amplitude,
                                                           double t0_hw,
                                                           bool eob_en)
      : gr::block("timed_burst_tagger_cc",
                  gr::io_signature::make(1, 1, sizeof(gr_complex)),
                  gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_rate(rate),
        d_burst_len_s(burst_len_s),
        d_period_s(period_s),
        d_offsets_s(offsets_s),
        d_amplitude(amplitude),
        d_t0_hw(t0_hw),
        d_eob_en(eob_en),
        d_drop_outside(false),
        d_burst_len_n(std::max(1, (int)std::llround(burst_len_s * rate))),
        d_next_event(0),
        d_base_period(0.0)
    {
      set_output_multiple(std::max(1, d_burst_len_n));
      set_tag_propagation_policy(TPP_DONT);

      // NEW: crear USRP para leer tiempo HW; opcionalmente toma devargs de env.
      //      Si defines HOWTO_TIMED_BURST_DEVARGS="type=b200,serial=XXXX", se usará eso.
      //      Si no, "" abre el primer B210 encontrado.
      try { // NEW
        const char* dev = std::getenv("HOWTO_TIMED_BURST_DEVARGS");        // NEW
        const std::string devargs = dev ? std::string(dev) : std::string(); // NEW
        d_usrp = uhd::usrp::multi_usrp::make(devargs);                     // NEW
      } catch (const std::exception& e) {                                  // NEW
        throw std::runtime_error(std::string("[timed_burst] cannot init hw time reader: ") + e.what()); // NEW
      } // NEW

    }

    /******************** Forecast ********************/
    void
    timed_burst_tagger_cc_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      int n_burst;
      {
        boost::mutex::scoped_lock lk(d_mtx);
        n_burst = std::max(1, (int)std::llround(d_burst_len_s * d_rate));
      }
      ninput_items_required[0] = n_burst;
    }

    /******************** general_work ********************/
    int
    timed_burst_tagger_cc_impl::general_work(int noutput_items,
                                             gr_vector_int &ninput_items,
                                             gr_vector_const_void_star &input_items,
                                             gr_vector_void_star &output_items)
    {
      const gr_complex* in  = reinterpret_cast<const gr_complex*>(input_items[0]);
      gr_complex*       out = reinterpret_cast<gr_complex*>(output_items[0]);

      // Snapshot under mutex
      double rate, burst_len_s, period_s, t0_hw; bool eob_en, drop_outside; float amp;
      std::vector<double> offsets_s;
      {
        boost::mutex::scoped_lock lk(d_mtx);
        rate         = d_rate;
        burst_len_s  = d_burst_len_s;
        period_s     = d_period_s;
        offsets_s    = d_offsets_s;
        amp          = d_amplitude;
        t0_hw        = d_t0_hw;
        eob_en       = d_eob_en;
        drop_outside = d_drop_outside;
        d_burst_len_n = std::max(1, (int)std::llround(burst_len_s * rate));
      }

      const int n_burst = d_burst_len_n;
      const int have_in = ninput_items[0];
      
      //cout<< "Entro al work" << endl;;

      if (offsets_s.empty() || n_burst <= 0) {
        consume_each(0);
        cout<< "Consumo 0" << endl;;
        return 0;
      }

      if (have_in < n_burst || noutput_items < n_burst) {
        if (drop_outside && have_in > 0) consume_each(have_in);
        else consume_each(0);
        cout<< "Entre aqui: no hay muestras suficientes en el buffer del work" << endl;
        return 0;
      }

      //cout<< "Doy salida" << endl;;

      // Compute absolute hw time for this burst
      const double t_abs  = t0_hw + d_base_period + offsets_s[d_next_event];
      const double full_d = std::floor(t_abs);
      const uint64_t full = (uint64_t)full_d;
      const double frac   = t_abs - full_d;

      // NEW: pacing determinista: si vas muy adelantado, duerme hasta acercarte al objetivo
      //      NOTA: no se cambia t_abs; solo se “espera” para no adelantar al HW.
      //NEW [Deadline]: decide si este slot está "tarde" más allá del deadline. si queires 
      //desactivar el deadline, ponlo a flase dentro del if, para que no se ejecute
      bool do_dtx = false;     
      {
        const double t_hw_now = d_usrp->get_time_now().get_real_secs();  // NEW
        const double lead     = t_abs - t_hw_now;                         // NEW
        if (lead < -d_deadline_s) {                                                // NEW [Deadline]
          do_dtx = true; // slot perdido: vamos a emitir silencio (DTX)            // NEW [Deadline]
        } else if (lead > d_lead_target) {                                         // sigue la lógica de pacing existente
          sleep_until_lead(t_abs); // vas adelantado: ajusta con sleep troceado
        }
        // Si vas tarde (-d_deadline_s <= lead <= d_lead_target), no dormimos, envía tal cual, sin dormir // NEW
      } // NEW

      // Absolute output index for SOB
      const uint64_t sob_abs_out = nitems_written(0);

      // UHD timing tags
      pmt::pmt_t when = pmt::make_tuple(pmt::from_uint64(full), pmt::from_double(frac));
      add_out_tag_(pmt::intern("tx_time"), when, sob_abs_out);
      add_out_tag_(pmt::intern("tx_sob"),  pmt::PMT_T, sob_abs_out);
      if (eob_en) {
        add_out_tag_(pmt::intern("tx_eob"), pmt::PMT_T, sob_abs_out + (n_burst - 1));
      }


      // NEW [Deadline]: marca DTX para inspección (opcional pero útil)
      // Copy and scale burst (o DTX si vas tarde)
      if (do_dtx) {                                                                // NEW [Deadline]
        add_out_tag_(pmt::intern("dtx"), pmt::PMT_T, sob_abs_out);  
        // Emitimos silencio para este slot manteniendo el calendario
        std::memset(out, 0, sizeof(gr_complex) * (size_t)n_burst);                 // NEW [Deadline]
        // consumimos igualmente la entrada para no acumular retraso
      } else if (amp == 1.0f) {
        std::memcpy(out, in, sizeof(gr_complex) * (size_t)n_burst);
      } else {
        for (int i = 0; i < n_burst; ++i) {
          out[i] = in[i] * amp;
        }
      }

      // Forward input tags within the copied region
      const uint64_t in_abs_base = nitems_read(0);
      forward_input_tags_(in_abs_base, 0, n_burst, sob_abs_out, 0);

      // Consume exactly what we copied
      consume_each(n_burst);

      // Advance schedule
      d_next_event++;
      if (d_next_event >= offsets_s.size()) {
        d_next_event = 0;
        d_base_period += period_s;
      }

      /*
      //DEBUG (visualizar valor de los offset y de d_next_event)
      {
        std::ostringstream ss;
        ss << "DBG Offsets(size=" << d_offsets_s.size() << "): [";
        for (size_t i = 0; i < d_offsets_s.size(); ++i) {
            ss << std::fixed << std::setprecision(6) << d_offsets_s[i];
            if (i + 1 < d_offsets_s.size()) ss << ", ";
        }
        ss << "]  d_next_event=" << d_next_event;
        std::cout << ss.str() << std::endl;
      }
      */

      return n_burst;
    }

    /******************** Setters ********************/
    void timed_burst_tagger_cc_impl::set_rate(double r) {
      boost::mutex::scoped_lock lk(d_mtx); d_rate = r;
      d_burst_len_n = std::max(1, (int)std::llround(d_burst_len_s * d_rate));
      set_output_multiple(d_burst_len_n);
    }
    void timed_burst_tagger_cc_impl::set_burst_len(double s) {
      boost::mutex::scoped_lock lk(d_mtx); d_burst_len_s = s;
      d_burst_len_n = std::max(1, (int)std::llround(d_burst_len_s * d_rate));
      set_output_multiple(d_burst_len_n);
    }
    void timed_burst_tagger_cc_impl::set_period(double s) {
      boost::mutex::scoped_lock lk(d_mtx); d_period_s = s;
    }
    void timed_burst_tagger_cc_impl::set_offsets(const std::vector<double>& v) {
        boost::mutex::scoped_lock lk(d_mtx);
        d_offsets_s = v;
        std::sort(d_offsets_s.begin(), d_offsets_s.end());
        d_offsets_s.erase(std::unique(d_offsets_s.begin(), d_offsets_s.end()), d_offsets_s.end());
        if (d_next_event >= d_offsets_s.size()) d_next_event = 0;

        std::ostringstream dbg;
        dbg << "[timed_burst] set_offsets (vector) -> ";
        for (size_t i = 0; i < d_offsets_s.size(); ++i) dbg << (i?", ":"") << std::setprecision(10) << d_offsets_s[i];
        std::cerr << dbg.str() << " (next_event=" << d_next_event << ")\n";
    }
    void timed_burst_tagger_cc_impl::set_amplitude(float a) {
      boost::mutex::scoped_lock lk(d_mtx); d_amplitude = a;
    }
    void timed_burst_tagger_cc_impl::set_t0_hw(double t0) {
      boost::mutex::scoped_lock lk(d_mtx); d_t0_hw = t0;
    }
    void timed_burst_tagger_cc_impl::set_eob_en(bool en) {
      boost::mutex::scoped_lock lk(d_mtx); d_eob_en = en;
    }
    void timed_burst_tagger_cc_impl::set_drop_outside(bool en) {
      boost::mutex::scoped_lock lk(d_mtx); d_drop_outside = en;
    }

    /******************** Getters ********************/
    double timed_burst_tagger_cc_impl::rate() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_rate;
    }
    double timed_burst_tagger_cc_impl::burst_len() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_burst_len_s;
    }
    double timed_burst_tagger_cc_impl::period() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_period_s;
    }
    std::vector<double> timed_burst_tagger_cc_impl::get_offsets() const {
      boost::mutex::scoped_lock lk(d_mtx);
      return d_offsets_s;  // copia; segura y sencilla
    }
    float timed_burst_tagger_cc_impl::amplitude() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_amplitude;
    }
    double timed_burst_tagger_cc_impl::t0_hw() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_t0_hw;
    }
    bool timed_burst_tagger_cc_impl::eob_en() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_eob_en;
    }
    bool timed_burst_tagger_cc_impl::drop_outside() const {
      boost::mutex::scoped_lock lk(d_mtx); return d_drop_outside;
    }

  } // namespace howto
} // namespace gr
