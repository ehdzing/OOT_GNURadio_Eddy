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

/* -*- c++ -*- */
#ifndef INCLUDED_HOWTO_CFO_INJECTOR_CC_H
#define INCLUDED_HOWTO_CFO_INJECTOR_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace howto {

/*!
 * \brief Injects a known CFO (Hz) by complex rotation.
 * 2 inputs / 2 outputs.
 */
class HOWTO_API cfo_injector_cc : public gr::sync_block
{
public:
  typedef boost::shared_ptr<cfo_injector_cc> sptr;

  static sptr make(double samp_rate, double cfo_hz);

  virtual void set_cfo_hz(double hz) = 0;

protected:
  cfo_injector_cc(const std::string& name,
                  gr::io_signature::sptr in_sig,
                  gr::io_signature::sptr out_sig)
    : gr::sync_block(name, in_sig, out_sig) {}
};

} // namespace howto
} // namespace gr

#endif
