/* -*- c++ -*- */
/* Copyright 2025 */
#ifndef INCLUDED_GR_HOWTO_TIMED_BURST_TAGGER_CC_H
#define INCLUDED_GR_HOWTO_TIMED_BURST_TAGGER_CC_H

#include <howto/api.h>
#include <gnuradio/block.h>
#include <string>

namespace gr {
  namespace howto {

    /*!
     * \brief Forwards input only during scheduled windows and stamps UHD timing tags.
     * \ingroup howto
     */
    class HOWTO_API timed_burst_tagger_cc : virtual public gr::block
    {
    public:
      typedef boost::shared_ptr<timed_burst_tagger_cc> sptr;

      static sptr make(double rate,
                       double burst_len_s,
                       double period_s,
                       std::vector<double> offsets_s,
                       float amplitude,
                       double t0_hw,
                       bool eob_en);

      // Dynamic setters
      virtual void set_rate(double r) = 0;
      virtual void set_burst_len(double s) = 0;
      virtual void set_period(double s) = 0;
      virtual void set_offsets(const std::vector<double>& v) = 0;
      virtual void set_amplitude(float a) = 0;
      virtual void set_t0_hw(double t0) = 0;
      virtual void set_eob_en(bool en) = 0;
      virtual void set_drop_outside(bool en) = 0;

      // Getters
      virtual double rate() const = 0;
      virtual double burst_len() const = 0;
      virtual double period() const = 0;
      virtual std::vector<double> get_offsets() const = 0;
      virtual float  amplitude() const = 0;
      virtual double t0_hw() const = 0;
      virtual bool   eob_en() const = 0;
      virtual bool   drop_outside() const = 0;
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_GR_HOWTO_TIMED_BURST_TAGGER_CC_H */
