#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# QA for howto.iq_mag_cf
#



import howto_swig as howto
from gnuradio import gr, gr_unittest, blocks
import math

class qa_iq_mag_cf(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_001_mag_cf(self):
        # Datos de prueba: 5 muestras complejas
        data_in = [
            complex(0.0, 0.0),
            complex(1.0, 2.0),
            complex(-3.0, 4.0),
            complex(0.5, -0.5),
            complex(-2.0, -2.0)
        ]
        scale = 1.0

        # Resultado esperado
        expected = []
        for s in data_in:
            mag = math.sqrt(s.real * s.real + s.imag * s.imag)
            expected.append(scale * mag)

        # Construcción del flujo
        src = blocks.vector_source_c(data_in)
        blk = howto.iq_mag_cf(scale)
        snk = blocks.vector_sink_f()

        self.tb.connect(src, blk)
        self.tb.connect(blk, snk)
        self.tb.run()

        result = snk.data()

        # Verificación GNU Radio (sin numpy)
        self.assertFloatTuplesAlmostEqual(expected, result, 6)

if __name__ == '__main__':
    gr_unittest.run(qa_iq_mag_cf, "qa_iq_mag_cf.xml")

