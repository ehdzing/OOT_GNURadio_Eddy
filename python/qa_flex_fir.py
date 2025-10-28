# -*- coding: utf-8 -*-
from __future__ import print_function
import numpy as np
from gnuradio import gr, blocks
from howto import howto_swig as howto
import unittest

class qa_flex_fir(unittest.TestCase):
    def _run_and_get(self, x, blk):
        tb = gr.top_block()

        if np.iscomplexobj(x):
            src = blocks.vector_source_c(x.astype(np.complex64).tolist(), False)
        else:
            x = np.asarray(x, dtype=np.float32)
            src = blocks.vector_source_f(x.tolist(), False)

        szi = blk.output_signature().sizeof_stream_item(0)
        if szi == gr.sizeof_gr_complex:
            snk = blocks.vector_sink_c()
        elif szi == gr.sizeof_float:
            snk = blocks.vector_sink_f()
        else:
            raise RuntimeError("Tipo de salida no soportado: {} bytes".format(szi))

        tb.connect(src, blk)
        tb.connect(blk, snk)
        tb.run()

        if szi == gr.sizeof_gr_complex:
            return np.array(snk.data(), dtype=np.complex64)
        return np.array(snk.data(), dtype=np.float32)

    def test_cc_passthrough_tone(self):
        fs = 32000.0
        n = 4096
        t = np.arange(n, dtype=np.float32) / fs
        tone = np.exp(1j * 2*np.pi*1000.0 * t).astype(np.complex64)
        g = 1.0
        blk = howto.flex_fir_cc(3, fs, 0.0, 0.0, 200.0, g)  # mode 3: bypass CC
        y = self._run_and_get(tone, blk)

        self.assertEqual(len(y), n)
        # RMS error vs g * tone
        err = np.sqrt(np.mean(np.abs(y - g*tone)**2))
        self.assertLess(err, 1e-3)

    def test_cf_basic(self):
        fs = 48000.0
        n = 4096
        x = (np.random.randn(n) + 1j*np.random.randn(n)).astype(np.complex64)
        blk = howto.flex_fir_cf(0, fs, 3000.0, 0.0, 300.0, 1.0)  # lowpass CF
        y = self._run_and_get(x, blk)
        self.assertEqual(y.dtype, np.float32)
        self.assertEqual(len(y), n)
        self.assertTrue(np.isfinite(y).all())

    def test_ff_lowpass_impulse(self):
        fs = 32000.0
        n = 257  # longitud suficiente
        imp = np.zeros(n, dtype=np.float32); imp[0] = 1.0
        blk = howto.flex_fir_ff(0, fs, 2000.0, 0.0, 300.0, 1.0)  # lowpass FF
        y = self._run_and_get(imp, blk)

        self.assertEqual(len(y), n)
        self.assertTrue(np.isfinite(y).all())
        # low-pass: energía total ~ 1 (si normalizas a ganancia 1)
        self.assertGreater(np.sum(np.abs(y)), 0.1)         # no todo ceros
        self.assertGreater(np.max(np.abs(y)), 1e-2)        # algún tap significativo

