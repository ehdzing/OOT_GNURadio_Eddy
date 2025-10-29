from gnuradio import gr, blocks
from gnuradio.filter import firdes
import howto
import numpy as np

class qa_decimate_fir_cc(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        fs = 1.0e6
        D  = 5

        # Two-tone: one in passband, one near/out-of-band (should be attenuated after decimation)
        n  = 200000
        t  = np.arange(n)/fs
        x1 = np.exp(1j*2*np.pi*50e3*t)    # in-band
        x2 = np.exp(1j*2*np.pi*300e3*t)   # should be suppressed by the LPF before decimation
        x  = (0.8*x1 + 0.2*x2).astype(np.complex64)

        src = blocks.vector_source_c(x.tolist(), repeat=False)

        # Design edges relative to post-decimation Nyquist
        fs_out = fs / D
        f_pass = 0.45 * (fs_out/2.0)
        f_stop = 0.55 * (fs_out/2.0)
        win    = firdes.WIN_HAMMING
        beta   = 6.76  # ignored for Hamming, fine for Kaiser otherwise

        dec = howto.decimate_fir_cc(D, fs, f_pass, f_stop, win, beta)
        snk = blocks.vector_sink_c()

        self.connect(src, dec, snk)
        self.run()

        y = np.array(snk.data(), dtype=np.complex64)

        # Basic sanity: length shrinks ~1/D
        assert abs(len(y) - len(x)//D) < 10, "Unexpected output length after decimation"

if __name__ == "__main__":
    qa_decimate_fir_cc()
