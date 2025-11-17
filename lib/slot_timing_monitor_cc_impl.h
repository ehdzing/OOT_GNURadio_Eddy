#ifndef INCLUDED_HOWTO_SLOT_TIMING_MONITOR_CC_IMPL_H
#define INCLUDED_HOWTO_SLOT_TIMING_MONITOR_CC_IMPL_H

#include <howto/slot_timing_monitor_cc.h>
#include <boost/thread/mutex.hpp>
#include <deque>

namespace gr {
namespace howto {

class slot_timing_monitor_cc_impl final : public slot_timing_monitor_cc
{
private:
    mutable boost::mutex d_mutex;

    // Configuration parameters (guarded by d_mutex)
    double d_samp_rate;
    double d_t0;
    double d_period_s;
    double d_threshold_power;
    int    d_dt_window_len;

    // State for time mapping
    bool        d_have_rx_time;
    double      d_rx_time0;        // seconds
    long long   d_rx_time_offset;  // item offset where rx_time was seen

    // Absolute sample index since start
    long long d_abs_sample_index;

    // Burst detection state
    bool d_prev_above;

    // Timing error history
    std::deque<double> d_dt_window;
    double             d_last_offset_s;
    double             d_last_jitter_s;

    // Internal helpers (not thread-safe by themselves; use snapshots)
    void handle_rx_time_tags(const std::vector<gr::tag_t> &tags);
    void process_burst(double t_burst,
                       double samp_rate,
                       double t0,
                       double period_s,
                       int    dt_window_len);

public:
    slot_timing_monitor_cc_impl(double samp_rate,
                                double t0,
                                double period_s,
                                double threshold,
                                int dt_window_len);

    ~slot_timing_monitor_cc_impl() noexcept override;

    void set_samp_rate(double samp_rate) override;
    void set_t0(double t0) override;
    void set_period_s(double period_s) override;
    void set_threshold(double threshold) override;
    void set_dt_window_len(int len) override;

    double last_offset_s() const override;
    double last_jitter_s() const override;

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) noexcept override;
};

} // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_SLOT_TIMING_MONITOR_CC_IMPL_H */
