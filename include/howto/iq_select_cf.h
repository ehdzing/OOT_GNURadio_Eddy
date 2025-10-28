/* -*- c++ -*- */
/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef INCLUDED_HOWTO_IQ_SELECT_CF_H
#define INCLUDED_HOWTO_IQ_SELECT_CF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>

namespace gr { 
  namespace howto {

  /*!
   * \brief I/Q selectable processor (complex -> float)
   *
   * Modes (int):
   *   0: |x|         (magnitude)
   *   1: |x|^2       (power)
   *   2: arg(x)      (phase)
   *   3: Re{x}       (real)
   *   4: Im{x}       (imag)
   *   5: |arg(x)|    (abs phase)
   */
  class HOWTO_API iq_select_cf : virtual public gr::sync_block
    {
    public:
        typedef boost::shared_ptr<iq_select_cf> sptr;

        /*!
         * \brief Factory
         * \param scale multiplicative scale
         * \param mode  processing mode (0..5)
         */
        static sptr make(float scale, int mode);

        virtual void  set_scale(float scale)  = 0;
        virtual float scale() const  = 0;

        virtual void  set_mode(int mode)  = 0;
        virtual int   mode() const  = 0;

        // interface dtor must be public or protected, virtual
        virtual ~iq_select_cf() {}
    };

  }} // namespace gr::howto

#endif /* INCLUDED_HOWTO_IQ_SELECT_CF_H */

