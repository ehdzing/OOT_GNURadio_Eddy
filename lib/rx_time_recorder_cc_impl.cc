/* -*- c++ -*- */
/*
 * rx_time_recorder_cc_impl.cc
 */

#include "rx_time_recorder_cc_impl.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <stdexcept>
#include <iostream>

namespace gr {
  namespace howto {

    // Factory
    rx_time_recorder_cc::sptr
    rx_time_recorder_cc::make(double samp_rate,
                              const std::string& filename,
                              int decim)
    {
      return gnuradio::get_initial_sptr(
          new rx_time_recorder_cc_impl(samp_rate, filename, decim));
    }

    // Constructor
    rx_time_recorder_cc_impl::rx_time_recorder_cc_impl(double samp_rate,
                                                       const std::string& filename,
                                                       int decim)
      : gr::block("rx_time_recorder_cc",
                  gr::io_signature::make(1, 1, sizeof(gr_complex)),
                  gr::io_signature::make(0, 0, 0)),
        d_samp_rate(samp_rate),
        d_decim((decim <= 0) ? 1 : decim),
        d_verbose(false),
        d_filename(filename),
        d_have_ref_time(false),
        d_sample_ref(0),
        d_t_ref(0.0)
    {
      if (d_samp_rate <= 0.0)
        throw std::runtime_error("rx_time_recorder_cc: samp_rate must be > 0");

      d_file.open(d_filename.c_str(),
                  std::ios::binary | std::ios::out | std::ios::trunc);

      if (!d_file.is_open())
        throw std::runtime_error("rx_time_recorder_cc: cannot open output file: " + d_filename);

      if (d_verbose) {
        std::cout << "[rx_time_recorder_cc] samp_rate = "
                  << d_samp_rate << " Hz, decim = "
                  << d_decim << ", file = " << d_filename << std::endl;
      }
    }

    // Destructor
    rx_time_recorder_cc_impl::~rx_time_recorder_cc_impl()
    {
      boost::mutex::scoped_lock lock(d_mutex);
      if (d_file.is_open())
        d_file.close();
    }

    void
    rx_time_recorder_cc_impl::set_verbose(bool verbose)
    {
      boost::mutex::scoped_lock lock(d_mutex);
      d_verbose = verbose;
    }

    void
    rx_time_recorder_cc_impl::forecast (int noutput_items,
                                        gr_vector_int &ninput_items_required)
    {
      // We are a pure sink: require some input, ignore noutput_items.
      const int min_in = (noutput_items > 0) ? noutput_items : 1024;
      ninput_items_required[0] = min_in;
    }

    // Parse UHD rx_time tag:
    //  - most common: tuple (uint64 secs, double frac_sec)
    //  - defensive fallback for other formats
    double
    rx_time_recorder_cc_impl::parse_rx_time_tag_(const pmt::pmt_t& value) const
    {
      if (pmt::is_tuple(value)) {
        pmt::pmt_t v0 = pmt::tuple_ref(value, 0);
        pmt::pmt_t v1 = pmt::tuple_ref(value, 1);

        const double secs  = pmt::to_uint64(v0);
        const double frac  = pmt::to_double(v1);
        return secs + frac;
      }

      if (pmt::is_real(value))
        return pmt::to_double(value);

      // Fallback: try to_string and give something deterministic
      std::cerr << "[rx_time_recorder_cc] WARNING: unsupported rx_time tag format, "
                   "falling back to 0.0" << std::endl;
      return 0.0;
    }

    int
    rx_time_recorder_cc_impl::general_work (int noutput_items,
                                            gr_vector_int &ninput_items,
                                            gr_vector_const_void_star &input_items,
                                            gr_vector_void_star &output_items)
    {
      (void)noutput_items;
      (void)output_items;

      const int n_in = ninput_items[0];
      if (n_in <= 0)
        return 0;

      const gr_complex *in = reinterpret_cast<const gr_complex*>(input_items[0]);

      const uint64_t abs_start = nitems_read(0);
      const uint64_t abs_end   = abs_start + static_cast<uint64_t>(n_in);

      boost::mutex::scoped_lock lock(d_mutex);

      if (!d_file.is_open()) {
        // Nothing to do; consume input and bail out.
        consume_each(n_in);
        return 0;
      }

      // 1) Look for rx_time tags in this region to get an absolute reference.
      std::vector<tag_t> rx_tags;
      const pmt::pmt_t key = pmt::string_to_symbol("rx_time");
      get_tags_in_range(rx_tags, 0, abs_start, abs_end, key);

      if (!rx_tags.empty()) {
        // Use the first rx_time tag as reference.
        const tag_t &tag = rx_tags.front();
        const uint64_t tag_offset = tag.offset;
        const double t0 = parse_rx_time_tag_(tag.value);

        d_sample_ref = tag_offset;
        d_t_ref      = t0;
        d_have_ref_time = true;

        if (d_verbose) {
          std::cout << "[rx_time_recorder_cc] got rx_time tag:"
                    << " sample_ref = " << d_sample_ref
                    << " t_ref = " << d_t_ref << " s"
                    << std::endl;
        }
      }

      if (d_have_ref_time) {
        // 2) For each input sample, compute time and optionally record.
        for (int i = 0; i < n_in; ++i) {
          const uint64_t idx = abs_start + static_cast<uint64_t>(i);

          // Apply decimation in the physical sample domain.
          if (d_decim > 1 && (idx % static_cast<uint64_t>(d_decim)) != 0)
            continue;

          // Time of this physical sample
          const int64_t delta_samples = static_cast<int64_t>(idx)
                                      - static_cast<int64_t>(d_sample_ref);
          const double t_sample = d_t_ref
                                + static_cast<double>(delta_samples) / d_samp_rate;

          const float power = in[i].real() * in[i].real()
                            + in[i].imag() * in[i].imag();

          // Write [double t][float p] without padding
          d_file.write(reinterpret_cast<const char*>(&t_sample), sizeof(double));
          d_file.write(reinterpret_cast<const char*>(&power),   sizeof(float));
        }
      }

      // We are a sink: consume everything and produce nothing.
      consume_each(n_in);
      return 0;
    }

  } /* namespace howto */
} /* namespace gr */
