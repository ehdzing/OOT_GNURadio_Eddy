#!/usr/bin/env python
# -*- coding: utf-8 -*-

from gnuradio import gr, gr_unittest, blocks
import howto_swig as howto

def moving_avg_ref_blocklike(xs, N, scale):
    """Emula EXACTAMENTE el bloque:
       - primeras N-1 salidas = 0.0
       - luego: scale * (promedio sobre la ventana de N)
    """
    out = []
    L = len(xs)
    acc = 0.0
    # ventana deslizante O(1)
    for i in range(L):
        acc += xs[i]
        if i >= N:
            acc -= xs[i - N]
        if i < N - 1:
            out.append(0.0)
        else:
            out.append(scale * (acc / float(N)))
    return out

class qa_moving_avg_ff(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_001_simple(self):
        N = 4
        # ahora el bloque divide por N internamente, así que scale es “ganancia” pura
        scale = 1.0

        x = [1.0, -2.0, 3.0, -4.0, 5.0]  # sin padding artificial
        y_ref = moving_avg_ref_blocklike(x, N, scale)

        src = blocks.vector_source_f(x, repeat=False)
        dut = howto.moving_avg_ff(N, scale)
        snk = blocks.vector_sink_f()

        self.tb.connect(src, dut, snk)
        self.tb.run()

        y = list(snk.data())

        # Compara salida completa (mismo largo que la entrada)
        self.assertEqual(len(y), len(y_ref))
        self.assertFloatTuplesAlmostEqual(y, y_ref, 6)

        # Cambios en runtime: no validamos valores aquí, solo que no crashee
        dut.set_length(3)
        dut.set_scale(0.5)

if __name__ == '__main__':
    gr_unittest.run(qa_moving_avg_ff, "qa_moving_avg_ff.xml")

