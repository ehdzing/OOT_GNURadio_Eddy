#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from gnuradio import gr, blocks
import howto
import numpy as np
import unittest

class qa_dual_decimate_ff(gr.top_block, unittest.TestCase):
    def setUp(self): gr.top_block.__init__(self)
    def tearDown(self): self.tb = None

    def test_001_independent_ports(self):
        n0, n1 = 240, 315
        x0 = np.arange(n0).astype(np.float32)
        x1 = (10 + 0.5*np.arange(n1)).astype(np.float32)
        D0, D1 = 3, 5

        src0 = blocks.vector_source_f(x0.tolist(), False)
        src1 = blocks.vector_source_f(x1.tolist(), False)
        blk  = howto.dual_decimate_ff(D0, D1)
        snk0 = blocks.vector_sink_f()
        snk1 = blocks.vector_sink_f()

        self.connect(src0, (blk, 0))
        self.connect(src1, (blk, 1))
        self.connect((blk, 0), snk0)
        self.connect((blk, 1), snk1)
        self.run()

        y0 = np.array(snk0.data(), dtype=np.float32)
        y1 = np.array(snk1.data(), dtype=np.float32)

        self.assertEqual(len(y0), n0//D0)
        self.assertEqual(len(y1), n1//D1)

        def avg_blocks(x, D, cnt):
            out = []
            for i in range(cnt):
                out.append(np.mean(x[i*D:(i+1)*D]))
            return np.array(out, dtype=np.float32)

        np.testing.assert_allclose(y0, avg_blocks(x0, D0, n0//D0), rtol=1e-6, atol=1e-6)
        np.testing.assert_allclose(y1, avg_blocks(x1, D1, n1//D1), rtol=1e-6, atol=1e-6)

    def test_002_runtime_change_with_flag(self):
        n = 400
        x = np.ones(n, dtype=np.float32)
        src0 = blocks.vector_source_f(x.tolist(), False)
        src1 = blocks.vector_source_f(x.tolist(), False)
        blk  = howto.dual_decimate_ff(2, 3)
        snk0 = blocks.vector_sink_f()
        snk1 = blocks.vector_sink_f()

        self.connect(src0, (blk, 0)); self.connect(src1, (blk, 1))
        self.connect((blk, 0), snk0); self.connect((blk, 1), snk1)

        blk.set_D0(4)
        blk.set_D1(5)

        self.run()

        y0 = np.array(snk0.data(), dtype=np.float32)
        y1 = np.array(snk1.data(), dtype=np.float32)

        self.assertEqual(len(y0), n//4)
        self.assertEqual(len(y1), n//5)
        self.assertTrue(np.allclose(y0, 1.0))
        self.assertTrue(np.allclose(y1, 1.0))

if __name__ == '__main__':
    unittest.main()

