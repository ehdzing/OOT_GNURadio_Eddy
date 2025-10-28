/* -*- c++ -*- */
/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "iq_select_cf_impl.h"
#include <gnuradio/io_signature.h>
#include <cmath>

namespace gr { namespace howto {

// Factory definition (must match public header exactly)
iq_select_cf::sptr iq_select_cf::make(float scale, int mode)
{
    return gnuradio::get_initial_sptr(new iq_select_cf_impl(scale, mode));
}

static gr::io_signature::sptr sig_in()
{
    return gr::io_signature::make(1, 1, sizeof(gr_complex));
}
static gr::io_signature::sptr sig_out()
{
    return gr::io_signature::make(1, 1, sizeof(float));
}

iq_select_cf_impl::iq_select_cf_impl(float scale, int mode)
: gr::sync_block("iq_select_cf", sig_in(), sig_out()),
  d_scale(scale), d_mode(mode)
{
}

void iq_select_cf_impl::set_scale(float s) noexcept
{
    boost::lock_guard<boost::mutex> lk(d_mutex);
    d_scale = s;
}

float iq_select_cf_impl::scale() const noexcept
{
    boost::lock_guard<boost::mutex> lk(d_mutex);
    return d_scale;
}

void iq_select_cf_impl::set_mode(int m) noexcept
{
    boost::lock_guard<boost::mutex> lk(d_mutex);
    d_mode = m;
}

int iq_select_cf_impl::mode() const noexcept
{
    boost::lock_guard<boost::mutex> lk(d_mutex);
    return d_mode;
}

int iq_select_cf_impl::work(int noutput_items,
                            gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items)
{
    const gr_complex* in  = static_cast<const gr_complex*>(input_items[0]);
    float*            out = static_cast<float*>(output_items[0]);

    // Snapshot de parámetros fuera del bucle
    float sc; int m;
    {
        boost::lock_guard<boost::mutex> lk(d_mutex);
        sc = d_scale;
        m  = d_mode;
    }

    // Procesamiento sin ramas especiales para sc==1.0
    switch (m) {
        case 0: // |x|
            for (int i = 0; i < noutput_items; ++i) {
                const float re = in[i].real();
                const float im = in[i].imag();
                out[i] = sc * std::sqrt(re*re + im*im);
            }
            break;

        case 1: // |x|^2
            for (int i = 0; i < noutput_items; ++i) {
                const float re = in[i].real();
                const float im = in[i].imag();
                out[i] = sc * (re*re + im*im);
            }
            break;

        case 2: // arg(x)
            for (int i = 0; i < noutput_items; ++i) {
                out[i] = sc * std::atan2(in[i].imag(), in[i].real());
            }
            break;

        case 3: // Re{x}
            for (int i = 0; i < noutput_items; ++i) {
                out[i] = sc * in[i].real();
            }
            break;

        case 4: // Im{x}
            for (int i = 0; i < noutput_items; ++i) {
                out[i] = sc * in[i].imag();
            }
            break;

        case 5: // |arg(x)|
            for (int i = 0; i < noutput_items; ++i) {
                const float ph = std::atan2(in[i].imag(), in[i].real());
                out[i] = sc * std::fabs(ph);
            }
            break;

        default: // modo inválido -> cero
            for (int i = 0; i < noutput_items; ++i) {
                out[i] = 0.0f;
            }
            break;
    }

    return noutput_items;
}

}} // namespace gr::howto

