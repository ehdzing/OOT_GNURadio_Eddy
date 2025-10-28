#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2025 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, gr_unittest
from gnuradio import blocks
import howto_swig as howto

class qa_gain_ff (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
	# Datos de entrada y ganancia de prueba
        src_data = [-3.0, 4.0, -5.5, 2.0, 3.0]
        gain = 2.0
        expected_result = [x * gain for x in src_data]

        # Flujo
        src = blocks.vector_source_f(src_data, repeat=False)
        dut = howto.gain_ff(gain)  # Firma SWIG en 3.7
        snk = blocks.vector_sink_f()

        self.tb.connect(src, dut)
        self.tb.connect(dut, snk)
        self.tb.run()

        # Verificación
        result = list(snk.data())
        self.assertFloatTuplesAlmostEqual(result, expected_result, 6)
	# set up fg
        self.tb.run ()
        # check data


if __name__ == '__main__':
    gr_unittest.run(qa_gain_ff, "qa_gain_ff.xml")
