#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from gnuradio import gr, blocks, gr_unittest
import howto
import numpy as np

def avg_blocks(x, D, cnt):
    out = []
    for i in range(cnt):
        out.append(np.mean(x[i*D:(i+1)*D]))
    return np.array(out, dtype=np.float32)

class qa_dual_decimate_ff(gr_unittest.TestCase):
    """
    QA for howto.dual_decimate_ff
    - Puerto 0 diezmado por D0
    - Puerto 1 diezmado por D1
    Cambios en runtime: set_D0/set_D1 marcan realineo; la primera work() devuelve 0.
    """

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_001_independent_ports(self):
        n0, n1 = 240, 315
        x0 = np.arange(n0).astype(np.float32)
        x1 = (10.0 + 0.5*np.arange(n1)).astype(np.float32)
        D0, D1 = 3, 5

        src0 = blocks.vector_source_f(x0.tolist(), False)
        src1 = blocks.vector_source_f(x1.tolist(), False)
        blk  = howto.dual_decimate_ff(D0, D1)
        snk0 = blocks.vector_sink_f()
        snk1 = blocks.vector_sink_f()

        self.tb.connect(src0, (blk, 0))
        self.tb.connect(src1, (blk, 1))
        self.tb.connect((blk, 0), snk0)
        self.tb.connect((blk, 1), snk1)
        self.tb.run()

        y0 = np.array(snk0.data(), dtype=np.float32)
        y1 = np.array(snk1.data(), dtype=np.float32)

        self.assertEqual(len(y0), n0//D0)
        self.assertEqual(len(y1), n1//D1)

        np.testing.assert_allclose(y0, avg_blocks(x0, D0, n0//D0), rtol=1e-6, atol=1e-6)
        np.testing.assert_allclose(y1, avg_blocks(x1, D1, n1//D1), rtol=1e-6, atol=1e-6)

    def test_002_runtime_change_with_flag(self):
        # Señal constante para que el promedio por bloque sea 1.0 siempre
        n = 400
        x = np.ones(n, dtype=np.float32)

        src0 = blocks.vector_source_f(x.tolist(), False)
        src1 = blocks.vector_source_f(x.tolist(), False)
        blk  = howto.dual_decimate_ff(2, 3)
        snk0 = blocks.vector_sink_f()
        snk1 = blocks.vector_sink_f()

        self.tb.connect(src0, (blk, 0))
        self.tb.connect(src1, (blk, 1))
        self.tb.connect((blk, 0), snk0)
        self.tb.connect((blk, 1), snk1)

        # Cambios en runtime ANTES de run():
        # los setters solo marcan el flag; la primera work() aplicará:
        # set_history()/set_relative_rate()/set_output_multiple() y return 0.
        blk.set_D0(4)
        blk.set_D1(5)

        self.tb.run()

        y0 = np.array(snk0.data(), dtype=np.float32)
        y1 = np.array(snk1.data(), dtype=np.float32)

        self.assertEqual(len(y0), n//4)
        self.assertEqual(len(y1), n//5)
        self.assertTrue(np.allclose(y0, 1.0))
        self.assertTrue(np.allclose(y1, 1.0))


if __name__ == '__main__':
    gr_unittest.run(qa_dual_decimate_ff, "qa_dual_decimate_ff.xml")

