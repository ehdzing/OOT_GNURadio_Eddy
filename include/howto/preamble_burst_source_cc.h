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

#ifndef INCLUDED_HOWTO_PREAMBLE_BURST_SOURCE_CC_H
#define INCLUDED_HOWTO_PREAMBLE_BURST_SOURCE_CC_H

#include <howto/api.h>
#include <gnuradio/sync_block.h>
#include <vector>

namespace gr {
namespace howto {

/*!
 * \brief Burst source that can output:
 *  - A single vector "burst" as-is (e.g., a tone from Python), OR
 *  - A selectable sum of 4 concatenated preambles [P0|P1|P2|P3].
 *
 * If use_preamble_split=true:
 *   - burst must have length 4*N
 *   - indices selects which Pi to sum (0..3)
 *
 * If use_preamble_split=false:
 *   - burst is used directly as the emitted preamble (indices ignored)
 *
 * The block repeats:
 *   [preamble/burst samples] then [silence_samples zeros] then repeats.
 */
class HOWTO_API preamble_burst_source_cc : public gr::sync_block
{
public:
  typedef boost::shared_ptr<preamble_burst_source_cc> sptr;

  static sptr make(const std::vector<gr_complex>& burst,
                   bool use_preamble_split,
                   const std::vector<int>& indices,
                   int samples_per_burst,
                   int silence_samples,
                   bool enable_tags);

  virtual void set_burst(const std::vector<gr_complex>& burst) = 0;
  virtual void set_use_preamble_split(bool enable) = 0;
  virtual void set_indices(const std::vector<int>& indices) = 0;
  virtual void set_silence_samples(int n) = 0;

protected:
  preamble_burst_source_cc(const std::string& name,
                           gr::io_signature::sptr in_sig,
                           gr::io_signature::sptr out_sig)
    : gr::sync_block(name, in_sig, out_sig) {}
};

} // namespace howto
} // namespace gr

#endif // INCLUDED_HOWTO_PREAMBLE_BURST_SOURCE_CC_H
