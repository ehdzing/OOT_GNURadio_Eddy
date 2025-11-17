#include "slot_timing_monitor_cc_impl.h"

#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/tags.h>
#include <pmt/pmt.h>

#include <boost/make_shared.hpp>

#include <cmath>
#include <iostream>
#include <cstring>   // std::memcpy

namespace gr {
namespace howto {

slot_timing_monitor_cc::sptr
slot_timing_monitor_cc::make(double samp_rate,
                             double t0,
                             double period_s,
                             double threshold,
                             int dt_window_len)
{
    return boost::make_shared<slot_timing_monitor_cc_impl>(
        samp_rate, t0, period_s, threshold, dt_window_len);
}

slot_timing_monitor_cc::~slot_timing_monitor_cc()
{
    // nothing
}

// --------------------------------------------------------------------

slot_timing_monitor_cc_impl::slot_timing_monitor_cc_impl(double samp_rate,
                                                         double t0,
                                                         double period_s,
                                                         double threshold,
                                                         int dt_window_len)
    : gr::sync_block("slot_timing_monitor_cc",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_samp_rate(samp_rate),
      d_t0(t0),
      d_period_s(period_s),
      d_threshold_power(threshold),
      d_dt_window_len(dt_window_len),
      d_have_rx_time(false),
      d_rx_time0(0.0),
      d_rx_time_offset(0),
      d_abs_sample_index(0),
      d_prev_above(false),
      d_last_offset_s(0.0),
      d_last_jitter_s(0.0)
{
    if (d_dt_window_len < 1) {
        d_dt_window_len = 1;
    }

    std::cout << "[slot_timing_monitor_cc] created with:"
              << " samp_rate=" << d_samp_rate
              << " t0=" << d_t0
              << " period_s=" << d_period_s
              << " threshold_power=" << d_threshold_power
              << " dt_window_len=" << d_dt_window_len
              << std::endl;
}

slot_timing_monitor_cc_impl::~slot_timing_monitor_cc_impl() noexcept
{
    // nothing
}

// --------------------------------------------------------------------
// Setters / getters (thread-safe)
// --------------------------------------------------------------------

void
slot_timing_monitor_cc_impl::set_samp_rate(double samp_rate)
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    d_samp_rate = samp_rate;
    std::cout << "[slot_timing_monitor_cc] set_samp_rate: "
              << d_samp_rate << std::endl;
}

void
slot_timing_monitor_cc_impl::set_t0(double t0)
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    d_t0 = t0;
    std::cout << "[slot_timing_monitor_cc] set_t0: "
              << d_t0 << std::endl;
}

void
slot_timing_monitor_cc_impl::set_period_s(double period_s)
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    d_period_s = period_s;
    std::cout << "[slot_timing_monitor_cc] set_period_s: "
              << d_period_s << std::endl;
}

void
slot_timing_monitor_cc_impl::set_threshold(double threshold)
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    d_threshold_power = threshold;
    std::cout << "[slot_timing_monitor_cc] set_threshold_power: "
              << d_threshold_power << std::endl;
}

void
slot_timing_monitor_cc_impl::set_dt_window_len(int len)
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    if (len < 1) {
        len = 1;
    }
    d_dt_window_len = len;
    if (static_cast<int>(d_dt_window.size()) > d_dt_window_len) {
        while (static_cast<int>(d_dt_window.size()) > d_dt_window_len) {
            d_dt_window.pop_front();
        }
    }
    std::cout << "[slot_timing_monitor_cc] set_dt_window_len: "
              << d_dt_window_len << std::endl;
}

double
slot_timing_monitor_cc_impl::last_offset_s() const
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    return d_last_offset_s;
}

double
slot_timing_monitor_cc_impl::last_jitter_s() const
{
    boost::lock_guard<boost::mutex> lock(d_mutex);
    return d_last_jitter_s;
}

// --------------------------------------------------------------------
// Internal helpers
// --------------------------------------------------------------------

void
slot_timing_monitor_cc_impl::handle_rx_time_tags(
    const std::vector<gr::tag_t> &tags)
{
    // Only care about the first rx_time tag to anchor the time axis.
    if (d_have_rx_time) {
        return;
    }

    for (size_t i = 0; i < tags.size(); ++i) {
        const gr::tag_t &t = tags[i];
        if (pmt::symbol_to_string(t.key) == "rx_time") {
            const pmt::pmt_t value = t.value;
            if (!pmt::is_tuple(value)) {
                continue;
            }
            const double full_secs =
                static_cast<double>(pmt::to_uint64(pmt::tuple_ref(value, 0)));
            const double frac_secs =
                pmt::to_double(pmt::tuple_ref(value, 1));
            d_rx_time0 = full_secs + frac_secs;
            d_rx_time_offset = static_cast<long long>(t.offset);
            d_have_rx_time = true;

            std::cout << "[slot_timing_monitor_cc] rx_time anchor: t0="
                      << d_rx_time0
                      << " at offset=" << d_rx_time_offset
                      << std::endl;
            break;
        }
    }
}

void
slot_timing_monitor_cc_impl::process_burst(double t_burst,
                                           double samp_rate,
                                           double t0,
                                           double period_s,
                                           int    dt_window_len)
{
    (void)samp_rate; // reserved for future extensions

    if (period_s <= 0.0) {
        return;
    }

    // Estimate burst index k (nearest slot)
    const double k_real = (t_burst - t0) / period_s;
    const long long k =
        static_cast<long long>(std::floor(k_real + 0.5));
    const double t_ideal = t0 + static_cast<double>(k) * period_s;
    const double dt = t_burst - t_ideal;

    double jitter_local = 0.0;
    std::size_t win_size = 0;

    {
        boost::lock_guard<boost::mutex> lock(d_mutex);

        d_last_offset_s = dt;

        d_dt_window.push_back(dt);
        if (static_cast<int>(d_dt_window.size()) > dt_window_len) {
            d_dt_window.pop_front();
        }

        // Compute jitter = stddev of dt window
        double mean = 0.0;
        win_size = d_dt_window.size();
        if (win_size > 0) {
            for (std::size_t i = 0; i < win_size; ++i) {
                mean += d_dt_window[i];
            }
            mean /= static_cast<double>(win_size);
        }

        double var = 0.0;
        for (std::size_t i = 0; i < win_size; ++i) {
            const double diff = d_dt_window[i] - mean;
            var += diff * diff;
        }
        if (win_size > 0) {
            var /= static_cast<double>(win_size);
        }
        d_last_jitter_s = std::sqrt(var);
        jitter_local = d_last_jitter_s;
    }

    std::cout << "[slot_timing_monitor_cc] burst k=" << k
              << " t_burst=" << t_burst
              << " dt=" << dt
              << " jitter=" << jitter_local
              << " (N=" << win_size << ")"
              << std::endl;
}

// --------------------------------------------------------------------
// work()
// --------------------------------------------------------------------

int
slot_timing_monitor_cc_impl::work(int noutput_items,
                                  gr_vector_const_void_star &input_items,
                                  gr_vector_void_star &output_items) noexcept
{
    const gr_complex *in =
        static_cast<const gr_complex *>(input_items[0]);
    gr_complex *out =
        static_cast<gr_complex *>(output_items[0]);

    // Pass-through
    std::memcpy(out, in, noutput_items * sizeof(gr_complex));

    // Snapshot config under mutex
    double samp_rate;
    double t0;
    double period_s;
    double threshold_power;
    int dt_window_len;

    {
        boost::lock_guard<boost::mutex> lock(d_mutex);
        samp_rate = d_samp_rate;
        t0 = d_t0;
        period_s = d_period_s;
        threshold_power = d_threshold_power;
        dt_window_len = d_dt_window_len;
    }

    // Handle rx_time tags on this chunk
    const uint64_t start = this->nitems_read(0);
    const uint64_t end = start + static_cast<uint64_t>(noutput_items);

    std::vector<gr::tag_t> tags;
    this->get_tags_in_range(tags, 0, start, end);
    handle_rx_time_tags(tags);

    if (!d_have_rx_time || samp_rate <= 0.0) {
        d_abs_sample_index += noutput_items;
        return noutput_items;
    }

    // Threshold-based burst detection
    for (int i = 0; i < noutput_items; ++i) {
        const float power = std::norm(in[i]);
        const bool above = (power > threshold_power);

        if (above && !d_prev_above) {
            // Rising edge: burst start candidate
            const long long abs_idx = d_abs_sample_index + i;
            const long long rel_idx = abs_idx - d_rx_time_offset;
            const double t_burst =
                d_rx_time0 + static_cast<double>(rel_idx) / samp_rate;

            process_burst(t_burst, samp_rate, t0, period_s, dt_window_len);
        }

        d_prev_above = above;
    }

    d_abs_sample_index += noutput_items;
    return noutput_items;
}

} // namespace howto
} // namespace gr
