/* -*- c++ -*- */
/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef INCLUDED_HOWTO_IQ_SELECT_CF_IMPL_H
#define INCLUDED_HOWTO_IQ_SELECT_CF_IMPL_H

#include <howto/iq_select_cf.h>
#include <boost/thread/mutex.hpp>

namespace gr { 
  namespace howto {

    class iq_select_cf_impl final : public iq_select_cf
    {
    public:
        iq_select_cf_impl(float scale, int mode);
        ~iq_select_cf_impl() noexcept override {}

        // runtime control
        void  set_scale(float scale) noexcept override;
        float scale() const noexcept override;

        void  set_mode(int mode) noexcept override;
        int   mode() const noexcept override;

        // work
        int work(int noutput_items,
                 gr_vector_const_void_star& input_items,
                 gr_vector_void_star& output_items) override;

    private:
        mutable boost::mutex d_mutex;
        float d_scale;
        int   d_mode;
    };

}} // namespace gr::howto

#endif /* INCLUDED_HOWTO_IQ_SELECT_CF_IMPL_H */

