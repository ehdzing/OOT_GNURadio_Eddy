/* -*- c++ -*- */
/*
 * rx_time_recorder_cc_impl.h
 */

#ifndef INCLUDED_HOWTO_RX_TIME_RECORDER_CC_IMPL_H
#define INCLUDED_HOWTO_RX_TIME_RECORDER_CC_IMPL_H

#include <howto/rx_time_recorder_cc.h>
#include <boost/thread/mutex.hpp>
#include <fstream>

namespace gr {
  namespace howto {

    class rx_time_recorder_cc_impl : public rx_time_recorder_cc
    {
    private:
      double        d_samp_rate;
      int           d_decim;
      bool          d_verbose;

      std::string   d_filename;
      std::ofstream d_file;

      // Reference from first rx_time tag
      bool          d_have_ref_time;
      uint64_t      d_sample_ref;  // absolute sample index of the tag
      double        d_t_ref;       // absolute time (seconds) at d_sample_ref

      boost::mutex  d_mutex;

    public:
      rx_time_recorder_cc_impl(double samp_rate,
                               const std::string& filename,
                               int decim);
      ~rx_time_recorder_cc_impl();

      void set_verbose(bool verbose);

      // gr::block interface
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      int general_work (int noutput_items,
                        gr_vector_int &ninput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items);

    private:
      double parse_rx_time_tag_(const pmt::pmt_t& value) const;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_RX_TIME_RECORDER_CC_IMPL_H */
