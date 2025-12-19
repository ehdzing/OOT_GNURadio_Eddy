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
/*
 * Copyright 2025 <+YOU OR YOUR COMPANY+>.
 * GPLv3-or-later.
 */

#ifndef INCLUDED_HOWTO_PREAMBLE_ABS_COMPARE_FF_H
#define INCLUDED_HOWTO_PREAMBLE_ABS_COMPARE_FF_H

#include <howto/api.h>
#include <gnuradio/block.h>
#include <vector>
#include <string>

namespace gr {
namespace howto {

/*!
 * \brief Debug block: outputs RX and REF I/Q components and their differences per preamble window.
 * \ingroup howto
 *
 * NOTE:
 *   Block name kept as preamble_abs_compare_ff for backward compatibility,
 *   but it now outputs I/Q components instead of magnitudes.
 */
class HOWTO_API preamble_abs_compare_ff : public gr::block
{
public:
  typedef boost::shared_ptr<preamble_abs_compare_ff> sptr;

  static sptr make(const std::vector<gr_complex>& preambles,
                   int preamble_layout,
                   const std::string& preamble_select);

  virtual void set_preamble_layout(int layout) = 0;
  virtual void set_preamble_select(const std::string& sel) = 0;

protected:
  preamble_abs_compare_ff(const std::string& name,
                          gr::io_signature::sptr input_signature,
                          gr::io_signature::sptr output_signature)
    : gr::block(name, input_signature, output_signature) {}
};

} // namespace howto
} // namespace gr

#endif
