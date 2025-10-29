#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import time
import numpy as np

from gnuradio import gr, blocks, gr_unittest
from gnuradio.filter import firdes
import howto

# Parámetros del “run corto”
TIMEOUT_SEC = 5.0        # tiempo máximo de captura
WARMUP_SEC  = 0.05       # darle un respiro para inicializar
TARGET_SAMPLES = 8000    # cuántas muestras queremos recolectar como mínimo

class qa_decimate_fir_cc(gr_unittest.TestCase):
    """
    QA no-bloqueante para howto.decimate_fir_cc:
      - Arranca, espera hasta acumular suficientes muestras y corta.
      - Verifica que produce “lo suficiente” y que el alias cerca de Nyquist de salida está atenuado.
    """

    def setUp(self):
        self.tb = gr.top_block("qa_decimate_fir_cc")

    def tearDown(self):
        try:
            self.tb.stop()
        except Exception:
            pass
        try:
            self.tb.wait()
        except Exception:
            pass
        self.tb = None

    def test_length_and_alias_attenuation(self):
        # Parámetros
        fs_in   = 1.0e6
        D       = 5
        fs_out  = fs_in / D
        nyq_out = fs_out / 2.0

        # Especificación que tu bloque dice aceptar (según XML)
        cutoff     = 0.45 * nyq_out     # 45 kHz
        transition = 0.10 * nyq_out     # 10 kHz
        window     = firdes.WIN_HAMMING
        beta       = 6.76               # ignorado para Hamming

        # Estímulo: dos tonos
        n_in = 200000
        t  = np.arange(n_in) / fs_in
        x1 = np.exp(1j * 2 * np.pi * 50e3  * t)   # en banda
        x2 = np.exp(1j * 2 * np.pi * 300e3 * t)   # fuera de banda -> alias ~100 kHz
        x  = (0.8 * x1 + 0.2 * x2).astype(np.complex64)

        src = blocks.vector_source_c(x.tolist(), repeat=False)

        # Bloque bajo prueba (firma: decim, samp_rate, cutoff, transition, window, kaiser_beta)
        dec = howto.decimate_fir_cc(int(D), float(fs_in),
                                    float(cutoff), float(transition),
                                    int(window), float(beta))

        # Sink de captura
        snk = blocks.vector_sink_c()

        # Conexión en streaming (SIN head abajo): dejamos correr y cortamos nosotros
        self.tb.connect(src, dec, snk)

        # Arranca la FG
        self.tb.start()
        time.sleep(WARMUP_SEC)

        # Espera activa acotada: juntamos TARGET_SAMPLES o expiramos
        start = time.time()
        last_len = 0
        while True:
            ylen = len(snk.data())
            # si llegamos a la cuota, bien
            if ylen >= TARGET_SAMPLES:
                break
            # si no crece nada durante un ratito, igual seguimos hasta el timeout global
            now = time.time()
            if now - start > TIMEOUT_SEC:
                break
            # micro-sueño para no quemar CPU
            time.sleep(0.01)
            last_len = ylen

        # Cortamos la FG explícitamente
        self.tb.stop()
        self.tb.wait()

        y = np.array(snk.data(), dtype=np.complex64)

        # 1) Debe haber producido “suficiente”
        # Con D=5, 8000 muestras de salida equivalen a 40k de entrada, así que si no llegaste
        # ni a eso con n_in=200k, el bloque está dormido o pidiendo de más en forecast.
        self.assertGreaterEqual(len(y), TARGET_SAMPLES,
                                msg="El bloque produjo demasiado poco antes del timeout: got %d" % len(y))

        # 2) Chequeo simple de alias con FFT
        Y = np.fft.rfft(y)
        freqs = np.fft.rfftfreq(len(y), d=1.0/fs_out)

        def peak_in_band(f0, bw_hz):
            lo = max(0.0, f0 - bw_hz)
            hi = min(nyq_out, f0 + bw_hz)
            mask = (freqs >= lo) & (freqs <= hi)
            if not np.any(mask):
                return -300.0
            mag = np.abs(Y[mask])
            pwr = np.max(mag) + 1e-20
            return 20.0 * np.log10(pwr)

        p_in  = peak_in_band(50e3, 2e3)
        p_ali = peak_in_band(100e3, 3e3)
        margin_db = p_in - p_ali

        # Umbral conservador
        self.assertGreater(margin_db, 20.0,
                           msg="Atenuación de alias insuficiente: %.1f dB (need > 20 dB)" % margin_db)

        # Logs para ctest -V
        print("Captured len(y)=%d (fs_out=%.0f)" % (len(y), fs_out))
        print("Pico 50 kHz = %.1f dB; Pico ~100 kHz (alias) = %.1f dB; Margen = %.1f dB"
              % (p_in, p_ali, margin_db))


if __name__ == '__main__':
    # Ejecuta con el runner estándar de GNU Radio 3.7
    gr_unittest.run(qa_decimate_fir_cc, "qa_decimate_fir_cc.xml")

