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


#ifndef INCLUDED_HOWTO_IQ_MAG_CF_H
#define INCLUDED_HOWTO_IQ_MAG_CF_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace howto {

    /*!
     * \brief <+description of block+>
     * \ingroup howto
     *
     */
    class HOWTO_API iq_mag_cf : virtual public gr::sync_block
    {
     public:
       typedef boost::shared_ptr<iq_mag_cf> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of howto::iq_mag_cf.
       *
       * To avoid accidental use of raw pointers, howto::iq_mag_cf's
       * constructor is in a private implementation
       * class. howto::iq_mag_cf::make is the public interface for
       * creating new instances.
       */
       static sptr make(float scale);

       /*! \brief Runtime control (thread-safe) */
       virtual void  set_scale(float scale) = 0;
       virtual float scale() const = 0;

       virtual ~iq_mag_cf() {}
    };

  } // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_IQ_MAG_CF_H */

