#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function
import numpy as np
import time
import unittest

from gnuradio import gr, blocks
import howto


def avg_blocks(x, D, cnt):
    """Block-wise average with stride D for the first cnt windows."""
    x = np.asarray(x, dtype=np.float32)
    D = int(D)
    cnt = int(cnt)
    out = np.empty(cnt, dtype=np.float32)
    for i in range(cnt):
        s = i * D
        out[i] = float(np.mean(x[s:s + D]))
    return out


class qa_dual_decimate_ff(unittest.TestCase):
    """
    Tests for howto.dual_decimate_ff (2-in / 2-out) under GNU Radio 3.7.
    We assert scheduler-coupled behavior: both outputs advance in lockstep
    and final lengths equal min(n0//D0, n1//D1).
    """

    def test_001_coupled_min_count(self):
        # Inputs of different lengths and decimations
        n0, n1 = 240, 315
        D0, D1 = 3, 5

        x0 = np.linspace(-1.0, 1.0, n0).astype(np.float32)
        x1 = (0.5 * np.sin(2 * np.pi * 0.03 * np.arange(n1)) + 0.5).astype(np.float32)

        tb = gr.top_block()
        src0 = blocks.vector_source_f(x0.tolist(), repeat=False)
        src1 = blocks.vector_source_f(x1.tolist(), repeat=False)
        blk = howto.dual_decimate_ff(D0, D1)
        snk0 = blocks.vector_sink_f()
        snk1 = blocks.vector_sink_f()

        tb.connect(src0, (blk, 0))
        tb.connect(src1, (blk, 1))
        tb.connect((blk, 0), snk0)
        tb.connect((blk, 1), snk1)

        # Use start()+wait() without wrapper-specific args to avoid SWIG issues.
        tb.start()
        tb.wait()

        y0 = np.array(snk0.data(), dtype=np.float32)
        y1 = np.array(snk1.data(), dtype=np.float32)

        cnt0 = n0 // D0
        cnt1 = n1 // D1
        cnt = min(cnt0, cnt1)

        # Both outputs must have the same length = min(...)
        self.assertEqual(len(y0), cnt)
        self.assertEqual(len(y1), cnt)

        ref0 = avg_blocks(x0, D0, cnt)
        ref1 = avg_blocks(x1, D1, cnt)

        np.testing.assert_allclose(y0, ref0, rtol=1e-6, atol=1e-6)
        np.testing.assert_allclose(y1, ref1, rtol=1e-6, atol=1e-6)

    def test_002_runtime_change_with_flag(self):
        """
        After changing D0/D1 at runtime, the block realigns once (returns 0)
        and continues. Outputs remain coupled: final lengths are equal.

        Important: total output count can exceed n // D_new because part of the
        stream was produced with the initial (possibly smaller) decimation.
        We therefore bound by n // min(D0_init, D1_init).
        """
        n = 100000
        D0_init, D1_init = 3, 3
        D0_new, D1_new = 4, 5

        x0 = np.ones(n, dtype=np.float32)
        x1 = np.ones(n, dtype=np.float32)

        tb = gr.top_block()
        src0 = blocks.vector_source_f(x0.tolist(), repeat=False)
        src1 = blocks.vector_source_f(x1.tolist(), repeat=False)
        blk = howto.dual_decimate_ff(D0_init, D1_init)
        snk0 = blocks.vector_sink_f()
        snk1 = blocks.vector_sink_f()

        tb.connect(src0, (blk, 0))
        tb.connect(src1, (blk, 1))
        tb.connect((blk, 0), snk0)
        tb.connect((blk, 1), snk1)

        tb.start()
        # Let forecast/work run at least once at initial D
        time.sleep(0.02)
        blk.set_D0(int(D0_new))
        blk.set_D1(int(D1_new))
        tb.wait()

        y0 = np.array(snk0.data(), dtype=np.float32)
        y1 = np.array(snk1.data(), dtype=np.float32)

        # Numeric value stays 1.0 regardless of D
        self.assertTrue(np.allclose(y0, 1.0))
        self.assertTrue(np.allclose(y1, 1.0))

        # Coupled property: same final length
        self.assertEqual(len(y0), len(y1))

        # Safe upper bound: cannot exceed production at the fastest initial decimation
        max_before_change = n // min(int(D0_init), int(D1_init))
        self.assertLessEqual(len(y0), max_before_change)
        self.assertLessEqual(len(y1), max_before_change)

        # Sanity: must produce something nonzero
        self.assertGreater(len(y0), 0)


if __name__ == '__main__':
    unittest.main()

