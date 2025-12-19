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
#ifndef INCLUDED_HOWTO_FRAME_BURST_SOURCE_CC_H
#define INCLUDED_HOWTO_FRAME_BURST_SOURCE_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <vector>

namespace gr {
namespace howto {

/*!
 * \brief Source block that generates:
 * PREAMBLE -> PAYLOAD -> SILENCE -> repeat
 */
class HOWTO_API frame_burst_source_cc : public gr::sync_block
{
public:
  typedef boost::shared_ptr<frame_burst_source_cc> sptr;

  static sptr make(const std::vector<gr_complex>& preamble,
                   const std::vector<gr_complex>& payload,
                   size_t silence_len,
                   float scale,
                   bool enable_tags);

  // Setters
  virtual void set_scale(float scale) = 0;
  virtual void set_silence_len(size_t silence_len) = 0;
  virtual void set_enable_tags(bool en) = 0;

  // Getters (SWIG expects these if you expose them)
  virtual float scale() const = 0;
  virtual size_t silence_len() const = 0;
  virtual bool enable_tags() const = 0;

protected:
  frame_burst_source_cc(const std::string& name,
                        gr::io_signature::sptr in_sig,
                        gr::io_signature::sptr out_sig)
    : gr::sync_block(name, in_sig, out_sig)
  {}
};

} // namespace howto
} // namespace gr

#endif /* INCLUDED_HOWTO_FRAME_BURST_SOURCE_CC_H */
