#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Python 2.7 / GNU Radio 3.7.11 compatible helpers for preamble + payload generation.

from __future__ import division

import numpy as np

# ---------------------------------------------------------------------
# Small helpers / compatibility
# ---------------------------------------------------------------------

def struct(data):
    """Simple struct-like object (legacy helper)."""
    return type('Struct', (object,), data)()


# ---------------------------------------------------------------------
# Preamble generation (your existing code, kept as-is)
# ---------------------------------------------------------------------

def get_zadoffChu(length, family):
    M = family
    N = int(length)
    k = np.arange(N)
    zc = np.exp(-1j * ((M * np.pi * k * (k + k.size % 2)) / float(N)))
    return zc


def get_preamble():
    """
    Generates the synchronization reference signal for all 4 6MHz TV channels.
    Returns: list(complex) with channels concatenated in Fortran order {0 1 2 3}
    """

    preamble_length = 2048
    activebw = 1584
    pad = (preamble_length - activebw) / 2.0  # keep float for intermediate ops

    ratio = 0.75
    root = {0: 3, 1: 5, 2: 7, 3: 11}

    chlen = activebw / 4.0
    zclen = int(2 * round(0.5 * chlen * ratio))
    guard = (chlen - zclen) / 2.0

    flatlen = int(2 * round(0.5 * preamble_length * 0.75))
    wlen = (preamble_length - flatlen) / 2.0

    time_window = np.concatenate((
        0.5 + (0.5 * np.cos(np.linspace(-np.pi, 0, int(wlen)))),
        np.ones(flatlen),
        0.5 + (0.5 * np.cos(np.linspace(0, np.pi, int(wlen)))),
    ))

    pre = np.zeros((preamble_length, 4), dtype=np.complex64)

    np.random.seed(0)
    for channel in [0, 1, 2, 3]:
        g = int(guard + pad)
        pre[g:g + zclen, channel] = 0.5 * np.sqrt(2) * (
            np.random.randn(zclen) + 1j * np.random.randn(zclen)
        )

        # Alternative ZC (disabled as in your original)
        # pre[g:g + zclen, channel] = get_zadoffChu(zclen, root[channel])

        pre[:, channel] = np.roll(pre[:, channel], int(channel * chlen - preamble_length / 2))
        pre[:, channel] = np.fft.ifft(pre[:, channel])
        pre[:, channel] = (pre[:, channel] * time_window) / (
            2 * np.std(pre[:, channel]) * (preamble_length / float(activebw))
        )

    pre = np.ravel(pre, order='F').tolist()
    return pre*0.7

def get_tone(freq_hz,
             amp=0.7,
             samp_rate=30.72e6,
             length=2048):
    """
    Returns a complex sinusoidal tone.

    freq_hz   : tone frequency in Hz
    amp       : amplitude (< 1.0)
    samp_rate : sampling rate in Hz
    length    : number of samples

    return : list(complex)
    """
    N = int(length)
    if N <= 0:
        return []

    k = np.arange(N, dtype=np.float64)
    x = amp * np.exp(1j * 2.0 * np.pi * float(freq_hz) * (k / float(samp_rate)))
    return x.astype(np.complex64).tolist()


def get_syncref(ant=0):
    pre = np.array(get_preamble())
    pre = pre.reshape((pre.size / 4, 4), order='F')
    sum_pre = np.sum(pre, axis=1)

    if int(ant) == 0:
        ref = sum_pre
    else:
        ref = np.flipud(-np.conj(sum_pre))

    return ref


# ---------------------------------------------------------------------
# Payload marker (deterministic)
# ---------------------------------------------------------------------

def get_payload_marker(marker_len=128, amp=0.7):
    """
    Returns the deterministic marker used by get_payload(mode='marker_qpsk').
    This is the "ground truth" reference for alignment validation in RX.
    """
    M = int(marker_len)
    if M <= 0:
        return []

    amp = float(amp)
    out = np.zeros(M, dtype=np.complex64)

    for i in range(M):
        a = (i % 32) / 31.0
        b = ((i // 32) % 32) / 31.0
        s = -1.0 if (i % 2) else 1.0
        out[i] = np.complex64(
            amp * (s * (0.15 + 0.85 * a) + 1j * (0.15 + 0.85 * b))
        )

    return out.tolist()


# ---------------------------------------------------------------------
# Payload generation (controlled)
# ---------------------------------------------------------------------

def get_payload(payload_len,
                mode='marker_qpsk',
                marker_len=128,
                seed=123,
                amp=0.7,
                tone_hz=3e6,
                samp_rate=30.72e6):
    """
    Controlled payload generator.

    payload_len : int
        Total number of complex samples.
    mode : str
        'marker_qpsk' : deterministic marker + reproducible QPSK
        'tone'        : complex tone (debug CFO, spectrum)
        'ramp'        : deterministic complex ramp (debug alignment)
    marker_len : int
        Number of deterministic marker samples at payload start (only marker_qpsk).
    seed : int
        RNG seed for reproducible QPSK.
    amp : float
        Output amplitude (keep < 1.0).
    tone_hz : float
        Tone frequency if mode='tone'.
    samp_rate : float
        Sample rate for tone generation.

    return : list(complex)
        Python list of complex numbers (SWIG -> std::vector<gr_complex>)
    """

    N = int(payload_len)
    if N <= 0:
        return []

    amp = float(amp)
    mode = str(mode)

    # -----------------------------
    # Mode: complex tone
    # -----------------------------
    if mode == 'tone':
        k = np.arange(N, dtype=np.float64)
        x = amp * np.exp(1j * 2.0 * np.pi * float(tone_hz) * (k / float(samp_rate)))
        return x.astype(np.complex64).tolist()

    # -----------------------------
    # Mode: deterministic ramp
    # -----------------------------
    if mode == 'ramp':
        k = np.arange(N, dtype=np.float64)
        a = (k % 256) / 255.0
        b = ((k // 256) % 256) / 255.0
        x = amp * (a + 1j * b)
        return x.astype(np.complex64).tolist()

    # -----------------------------
    # Default: marker + reproducible QPSK
    # -----------------------------
    M = int(marker_len)
    if M < 0:
        M = 0
    if M > N:
        M = N

    out = np.zeros(N, dtype=np.complex64)

    # Marker (same as get_payload_marker)
    if M > 0:
        marker = np.array(get_payload_marker(M, amp), dtype=np.complex64)
        out[:M] = marker

    # QPSK tail
    if M < N:
        rng = np.random.RandomState(int(seed))
        sym = rng.randint(0, 4, size=(N - M)).astype(np.int32)

        lut = np.array([1+1j, 1-1j, -1+1j, -1-1j],
                       dtype=np.complex64) / np.sqrt(2.0)

        out[M:] = np.complex64(amp) * lut[sym]

    return out.tolist()



# ---------------------------------------------------------------------
# Numerology config (your existing code, unchanged structure)
# ---------------------------------------------------------------------

def get_numerology_config(numid):
    # Definition of 5G range values see lib5grange.h
    if numid == 0:
        p = struct({
            'k': 16384, 'm': 4, 'ncp': 4352, 'ncs': 768, 'nw': 512,
            'kon': 12672, 'koff': 3712, 'a': 0,
            'subcarriers_per_rb': 96,
            'symbols_per_subframe': 2,
            'pilot_dt': 2, 'pilot_df': 4,
            'num_pilot_sc': 3168,
            'num_dci_sc': 32, 'num_dci_qam': 224,
        })
    elif numid == 1:
        p = struct({
            'k': 8192, 'm': 4, 'ncp': 2176, 'ncs': 384, 'nw': 256,
            'kon': 6336, 'koff': 1856, 'a': 0,
            'subcarriers_per_rb': 48,
            'symbols_per_subframe': 4,
            'pilot_dt': 4, 'pilot_df': 4,
            'num_pilot_sc': 1584,
            'num_dci_sc': 16, 'num_dci_qam': 240,
        })
    elif numid == 2:
        p = struct({
            'k': 4096, 'm': 4, 'ncp': 1088, 'ncs': 192, 'nw': 128,
            'kon': 3168, 'koff': 928, 'a': 0,
            'subcarriers_per_rb': 24,
            'symbols_per_subframe': 8,
            'pilot_dt': 4, 'pilot_df': 4,
            'num_pilot_sc': 792,
            'num_dci_sc': 8, 'num_dci_qam': 240,
        })
    elif numid == 3:
        p = struct({
            'k': 2048, 'm': 4, 'ncp': 544, 'ncs': 96, 'nw': 64,
            'kon': 1584, 'koff': 464, 'a': 0,
            'subcarriers_per_rb': 12,
            'symbols_per_subframe': 16,
            'pilot_dt': 4, 'pilot_df': 4,
            'num_pilot_sc': 396,
            'num_dci_sc': 4, 'num_dci_qam': 240,
        })
    elif numid == 4:
        p = struct({
            'k': 1024, 'm': 2, 'ncp': 136, 'ncs': 24, 'nw': 16,
            'kon': 792, 'koff': 232, 'a': 0,
            'subcarriers_per_rb': 6,
            'symbols_per_subframe': 64,
            'pilot_dt': 4, 'pilot_df': 6,
            'num_pilot_sc': 132,
            'num_dci_sc': 2, 'num_dci_qam': 224,
        })
    else:
        raise ValueError("Unknown numerology id: %s" % str(numid))

    return p
