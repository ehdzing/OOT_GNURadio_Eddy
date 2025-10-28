# -*- coding: utf-8 -*-
from gnuradio import gr, gr_unittest, blocks
import howto_swig as howto
import math

class qa_iq_select_cf(gr_unittest.TestCase):

    def _run_case(self, mode, scale, src_items, expected, tol=1e-6):
        tb = gr.top_block()

        src = blocks.vector_source_c(src_items, repeat=False)
        blk = howto.iq_select_cf(scale, mode)
        snk = blocks.vector_sink_f()

        tb.connect(src, blk)
        tb.connect(blk, snk)
        tb.run()

        out = snk.data()
        self.assertEqual(len(out), len(expected))
        for i, (a, b) in enumerate(zip(out, expected)):
            self.assertAlmostEqual(a, b, places=5, msg="idx {}".format(i))

    def test_000_real(self):
        src = [1+2j, -3+4j, 0-1j]
        exp = [1.0, -3.0, 0.0]
        self._run_case(mode=3, scale=1.0, src_items=src, expected=exp)

    def test_001_imag_scaled(self):
        src = [1+2j, -3+4j, 0-1j]
        exp = [2.0*0.5, 4.0*0.5, -1.0*0.5]
        self._run_case(mode=4, scale=0.5, src_items=src, expected=exp)

    def test_002_mag(self):
        src = [3+4j, 1+0j, 0+0j]
        exp = [5.0, 1.0, 0.0]
        self._run_case(mode=0, scale=1.0, src_items=src, expected=exp)

    def test_003_mag2_scaled(self):
        src = [3+4j, 1+0j, 0+0j]
        exp = [25.0*2.0, 1.0*2.0, 0.0]
        self._run_case(mode=1, scale=2.0, src_items=src, expected=exp)

    def test_004_phase(self):
        src = [1+0j, 0+1j, -1+0j, 0-1j]
        exp = [0.0, math.pi/2, math.pi, -math.pi/2]
        self._run_case(mode=2, scale=1.0, src_items=src, expected=exp)

    def test_005_absphase_scaled(self):
        src = [1+0j, 0+1j, -1+0j, 0-1j]
        exp = [abs(0.0)*0.3, abs(math.pi/2)*0.3, abs(math.pi)*0.3, abs(-math.pi/2)*0.3]
        self._run_case(mode=5, scale=0.3, src_items=src, expected=exp)

if __name__ == '__main__':
    gr_unittest.run(qa_iq_select_cf, "qa_iq_select_cf.xml")

