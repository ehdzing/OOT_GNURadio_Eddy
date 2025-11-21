#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Analizador de sincronismo TX/RX con modos de detección configurables
y filtrado de ráfagas incompletas.

detection_mode:
  0 = Detección simple por umbral (rising edges).
  1 = Detección por energía integrada (media móvil) + umbral.
  2 = Primer edge > threshold después de un silencio largo (>= gap_min_s).

Además:
  - Selección flexible del segmento a analizar (archivo completo / últimas N muestras / últimas N ráfagas).
  - Cálculo de D_k y agrupación por ráfaga k (una muestra representativa por burst).
  - Cálculo de error e_k respecto al retardo medio.
  - Gráficas con trazas diezmadas para no matar la CPU.
  - Gráfica de muestras iniciales RX (potencia) con tiempo ABSOLUTO.
  - Soporte para diezmado previo en RX: rx_decim.
"""

import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
import os

# ==========================
# 1) Parámetros físicos
# ==========================

fs = 30.72e6            # sample rate físico del sistema [Hz]
rx_decim = 20           # factor de diezmado aplicado ANTES de guardar power_rx.dat (1 si no hubo)
fs_eff = fs / rx_decim  # sample rate efectivo de las muestras guardadas [Hz]

# Cómo debe mostrarse el eje X en la gráfica inicial:
# 0 = tiempo ABSOLUTO (comienza en t_rx0)
# 1 = tiempo RELATIVO (comienza en 0 ms)
align_x_mode = 0


#t_rx0 = 0.0977908    # tiempo de referencia RX (rx_time del sample 0, en segundos)   para tx_time solo al comienzo
t_rx0 = 0.068225    # tiempo de referencia RX (rx_time del sample 0, en segundos)   para tx_time en cada rafaga
#t_rx0 = 0.0488185    # tiempo de referencia RX (rx_time del sample 0, en segundos)

t0_tx = 0.7             # tiempo ideal de la primera ráfaga TX [s]
T_period = 0.015        # periodo entre ráfagas (10 ms ON + 5 ms OFF) [s]

# ==========================
# 2) Configuración del detector
# ==========================

# MODO DE DETECCIÓN:
#   0 = simple por umbral
#   1 = energía integrada (media móvil) + umbral
#   2 = primer edge > threshold luego de silencio largo
detection_mode = 0

# Umbral base de potencia (se usa en todos los modos)
power_threshold = 0.01

# --- Modo 1: energía integrada / media móvil ---
integration_window_s = 0.0002      # 0.2 ms

# --- Modo 0 y 1: separación mínima entre detecciones ---
min_separation_s = 0.003           # 3 ms

# --- Modo 2: primer edge después de silencio largo ---
gap_min_s = 0.005                  # 5 ms

# ==========================
# 3) Configuración de análisis (segmento del archivo)
# ==========================

# segment_mode:
#   0 = usar TODO el archivo
#   1 = usar SIEMPRE las últimas N muestras
#   2 = usar las últimas N ráfagas (aprox) según T_period
#   3 = usar SIEMPRE las primeras N muestras
#   4 = usar las primeras N ráfagas (aprox) según T_period
segment_mode = 3

manual_last_samples = 10_000_000    # usado solo si segment_mode == 1 ó 3
target_num_bursts   = 500          # usado solo si segment_mode == 2 ó 4

max_samples_cap = 20_000_000_000   # límite duro de muestras a analizar

# Ventanas para superposición de ráfagas (para plots)
pre_window_s = 1e-3                # antes del flanco
post_window_s = 12e-3              # después del flanco
max_bursts_for_overlay = 20

base_dir = os.path.dirname(os.path.abspath(__file__))
power_filename = os.path.join(base_dir, 'power_rx1.dat')

# ==========================
# 4) Parámetros gráficos globales
# ==========================

max_points_full_plot = 158000
max_points_overlay   = 2000

# ==========================
# 4bis) Gráfica de muestras iniciales RX
# ==========================

# Activar/desactivar gráfica de las primeras muestras recibidas
plot_initial_enabled   = True      # pon False si no quieres esta gráfica

# Número de muestras iniciales a inspeccionar (ANTES de segmentar)
plot_initial_nsamples  = 10_000_000   # muestras de power_rx.dat

# Factor de diezmado para esa gráfica (1 = sin diezmado)
plot_initial_decim     = 2000

# ==========================
# 4ter) Ventana temporal manual (opcional)
# ==========================

plot_time_window_enabled = False    # pon True si quieres ver una ventana concreta

# Define tiempos absolutos en segundos
t_win_start = 0.500      # inicio [s]
t_win_end   = 0.530      # fin [s]

# ==========================
# 5) Parámetros de ráfaga / estadística
# ==========================

# Número "esperado" de edges por ráfaga (ajústalo según tu patrón)
expected_edges_per_burst = 1

# Umbral mínimo para considerar que una ráfaga es "completa" para estadística
min_edges_for_valid_burst = 1

# ¿Qué representante usar por ráfaga?
#   False -> D_k más cercano a 0 (robusto)
#   True  -> primera detección del k (values[0])
use_first_edge_per_k = False

# ==========================
# 6) Funciones auxiliares
# ==========================

def decimate_for_plot(x, y, max_points):
    n = len(y)
    if n <= max_points:
        return x, y
    step = max(1, n // max_points)
    return x[::step], y[::step]


def detect_edges_threshold(sig, threshold, fs_local, min_separation_s=None):
    """
    fs_local debe ser fs_eff (frecuencia de muestreo de lo que tienes en 'sig').
    """
    above = sig > threshold
    raw_edges = np.where(np.logical_and(above[1:], ~above[:-1]))[0] + 1

    if min_separation_s is None or len(raw_edges) == 0:
        return raw_edges

    min_sep_samples = int(round(min_separation_s * fs_local))
    filtered = []
    last_e = -1e12
    for e in raw_edges:
        if e - last_e >= min_sep_samples:
            filtered.append(e)
            last_e = e
    return np.array(filtered, dtype=int)


def detect_edges_energy(sig, fs_local, threshold, win_s, min_separation_s=None):
    """
    fs_local = fs_eff (frecuencia de muestreo efectiva).
    """
    win_samples = int(round(win_s * fs_local))
    if win_samples < 1:
        raise ValueError("integration_window_s demasiado pequeño; win_samples < 1")

    kernel = np.ones(win_samples, dtype=np.float32) / float(win_samples)
    smooth = np.convolve(sig, kernel, mode='same')

    edges = detect_edges_threshold(smooth, threshold, fs_local, min_separation_s)
    return edges, smooth


def detect_edges_after_silence(sig, fs_local, threshold, gap_min_s):
    """
    Detección por silencio largo: fs_local = fs_eff.
    """
    gap_min_samples = int(round(gap_min_s * fs_local))

    edges = []
    silence_count = 0

    for i in range(1, len(sig)):
        if sig[i-1] <= threshold:
            silence_count += 1
        else:
            silence_count = 0

        if sig[i-1] <= threshold and sig[i] > threshold:
            if silence_count >= gap_min_samples:
                edges.append(i)
                silence_count = 0

    return np.array(edges, dtype=int)

# ==========================
# 7) Carga de datos
# ==========================

power = np.fromfile(power_filename, dtype=np.float32)
total_samples = len(power)
print("Leí", total_samples, "muestras de potencia de", power_filename)
print("fs    (físico)   = {:.3f} MHz".format(fs / 1e6))
print("rx_decim         = {}".format(rx_decim))
print("fs_eff (archivo) = {:.3f} MHz".format(fs_eff / 1e6))

if total_samples == 0:
    raise RuntimeError("El archivo de potencia está vacío. Nada que analizar.")

# ==========================
# 7bis) Gráfica de muestras iniciales RX (antes de segmentar)
# ==========================

if plot_initial_enabled:
    n_init = min(total_samples, int(plot_initial_nsamples))
    if n_init <= 0:
        print("plot_initial_enabled=True pero n_init <= 0, nada que graficar.")
    else:
        decim = max(1, int(plot_initial_decim))
        sig_init = power[:n_init]
        sig_init_decim = sig_init[::decim]

        # Índices de las muestras diezmadas
        idxs = np.arange(len(sig_init_decim)) * decim
        
        # Tiempo ABSOLUTO: cada muestra del archivo equivale a rx_decim muestras físicas
        # pero ya hemos metido eso en fs_eff, así que usamos fs_eff:
        t_abs = t_rx0 + idxs / fs_eff        # segundos
        
        # Alineación del eje X
        if align_x_mode == 0:
            # Mostrar tiempo absoluto
            x_axis_ms = t_abs * 1e3
        elif align_x_mode == 1:
            # Tiempo relativo (empezar en 0)
            x_axis_ms = (t_abs - t_abs[0]) * 1e3
        else:
            raise ValueError("align_x_mode debe ser 0 o 1")
        

        plt.figure()
        plt.plot(x_axis_ms, sig_init_decim, label="Potencia RX (inicial, diezmada)")
        plt.xlabel("Tiempo absoluto [ms]")
        plt.ylabel("Potencia [u.a.]")
        plt.title("Muestras iniciales RX ({} muestras, decim = {})".format(n_init, decim))
        plt.grid(True)
        plt.legend(loc="best")
        plt.tight_layout()

# ==========================
# 7ter) Ventana temporal manual (opcional)
# ==========================

if plot_time_window_enabled:
    t_all = t_rx0 + np.arange(total_samples) / fs_eff

    mask = np.logical_and(t_all >= t_win_start, t_all <= t_win_end)
    if np.any(mask):
        t_sel = t_all[mask]
        power_sel = power[mask]

        decim = max(1, int(len(power_sel) // 100_000))  # hasta 100k puntos
        t_sel_d = t_sel[::decim]
        power_sel_d = power_sel[::decim]

        plt.figure()
        plt.plot(t_sel_d * 1e3, power_sel_d)
        plt.xlabel("Tiempo absoluto [ms]")
        plt.ylabel("Potencia [u.a.]")
        plt.title("Ventana temporal seleccionada {:.3f} s → {:.3f} s".format(
            t_win_start, t_win_end))
        plt.grid(True)
        plt.tight_layout()
    else:
        print("La ventana temporal seleccionada no coincide con ninguna muestra del archivo.")

# ==========================
# 8) Selección del segmento para análisis
# ==========================

samples_per_period = int(round(T_period * fs_eff))  # en muestras del archivo (ya diezmado)

if segment_mode == 0:
    # Todo el archivo
    n_analyze = min(total_samples, max_samples_cap)
    start_index = total_samples - n_analyze
    reason = "full_file"

elif segment_mode == 1:
    # Últimas N muestras
    n_analyze = min(total_samples, int(manual_last_samples), max_samples_cap)
    start_index = total_samples - n_analyze
    reason = "last_N_samples"

elif segment_mode == 2:
    # Últimas N ráfagas (aprox) usando fs_eff
    n_needed = target_num_bursts * samples_per_period
    n_analyze = min(total_samples, n_needed, max_samples_cap)
    start_index = total_samples - n_analyze
    reason = "last_N_bursts"

elif segment_mode == 3:
    # Primeras N muestras
    n_analyze = min(total_samples, int(manual_last_samples), max_samples_cap)
    start_index = 0
    reason = "first_N_samples"

elif segment_mode == 4:
    # Primeras N ráfagas (aprox) usando fs_eff
    n_needed = target_num_bursts * samples_per_period
    n_analyze = min(total_samples, n_needed, max_samples_cap)
    start_index = 0
    reason = "first_N_bursts"

else:
    raise ValueError("segment_mode inválido. Usa 0, 1, 2, 3 o 4.")

if n_analyze <= 0:
    raise RuntimeError("n_analyze <= 0, revisa configuración.")

power_seg = power[start_index:]
print("Analizando", n_analyze, "muestras (modo_segmento:", reason,
      ", start_index =", start_index, ")")

# ==========================
# 9) Detección según detection_mode
# ==========================

smooth_for_plot = None

if detection_mode == 0:
    print("Modo de detección 0: simple por umbral.")
    edges = detect_edges_threshold(
        sig=power_seg,
        threshold=power_threshold,
        fs_local=fs_eff,
        min_separation_s=min_separation_s
    )

elif detection_mode == 1:
    print("Modo de detección 1: energía integrada (media móvil) + umbral.")
    edges, smooth_for_plot = detect_edges_energy(
        sig=power_seg,
        fs_local=fs_eff,
        threshold=power_threshold,
        win_s=integration_window_s,
        min_separation_s=min_separation_s
    )

elif detection_mode == 2:
    print("Modo de detección 2: primer edge > threshold tras silencio >= {:.3f} ms".format(gap_min_s * 1e3))
    edges = detect_edges_after_silence(
        sig=power_seg,
        fs_local=fs_eff,
        threshold=power_threshold,
        gap_min_s=gap_min_s
    )
else:
    raise ValueError("detection_mode inválido. Usa 0, 1 o 2.")

print("Detectadas", len(edges), "ráfagas (edges) en el segmento analizado")

if len(edges) == 0:
    print("No se detectaron ráfagas. Ajusta umbral o parámetros del detector.")
    t_seg = np.arange(n_analyze) / fs_eff
    t_seg_ms = t_seg * 1e3
    t_seg_ms_d, power_seg_d = decimate_for_plot(t_seg_ms, power_seg, max_points_full_plot)

    plt.figure()
    plt.plot(t_seg_ms_d, power_seg_d, label="Potencia")
    plt.xlabel("Tiempo en segmento [ms]")
    plt.ylabel("Potencia [u.a.]")
    plt.title("Potencia segmentada (sin detecciones)")
    plt.grid(True)
    plt.legend(loc="best")
    plt.tight_layout()
    plt.show()
    raise SystemExit

# ==========================
# 10) Tiempos RX por edge (ABSOLUTOS)
# ==========================

# Cada índice del archivo equivale a 1/fs_eff segundos
t_bursts_rx = t_rx0 + (start_index + edges) / fs_eff

# ==========================
# 11) Cálculo de D_k por tag
# ==========================

D_list = []
k_list = []

for i, t_rx in enumerate(t_bursts_rx):
    k_real = (t_rx - t0_tx) / T_period
    k = int(np.round(k_real))
    t_tx_ideal = t0_tx + k * T_period
    D_k = t_rx - t_tx_ideal

    D_list.append(D_k)
    k_list.append(k)

    # Expresamos D_k en muestras FÍSICAS, usando fs original
    D_samples = D_k * fs
    print("tag i={} k={} t_rx={:.9f} t_tx_ideal={:.9f} D_k={:.9e} s ({:.3f} muestras)".format(
        i, k, t_rx, t_tx_ideal, D_k, D_samples))

D = np.array(D_list)

if len(D) > 0:
    D_mean_all = D.mean()
    D_std_all = D.std()
    print("\n[GLOBAL - TODAS LAS ETIQUETAS]")
    print("Offset medio D̄_all     = {:.3e} s ({:.3f} muestras)".format(
        D_mean_all, D_mean_all * fs))
    print("Jitter (std) σ_all      = {:.3e} s ({:.3f} muestras)".format(
        D_std_all, D_std_all * fs))

# ==========================
# 12) Agrupar por ráfaga k
# ==========================

by_k = defaultdict(list)
for k, D_k in zip(k_list, D_list):
    by_k[k].append(D_k)

print("\n[POR RÁFAGA - TODAS LAS ETIQUETAS POR k]")
for k in sorted(by_k.keys()):
    values = by_k[k]
    print("k={} -> num_tags={}  D_k = {}".format(
        k, len(values), ["{:.3e}".format(v) for v in values]))

# Representante por ráfaga (y filtrado de ráfagas incompletas)
D_burst_list = []
k_valid_list = []

print("\n[POR RÁFAGA - UNA MUESTRA REPRESENTATIVA POR k]")
for k in sorted(by_k.keys()):
    values = by_k[k]
    num_tags = len(values)

    # Info básica
    print("  k={} -> num_tags={}".format(k, num_tags), end='')

    # Comprobamos si la ráfaga es "completa" o no
    if num_tags < min_edges_for_valid_burst:
        print("  -> IGNORADA en estadísticas (burst incompleto)")
        continue

    # Elegimos representante
    if use_first_edge_per_k:
        D_rep = values[0]
    else:
        # Más cercano a 0 (robusto a outliers dentro de la ráfaga)
        D_rep = min(values, key=lambda x: abs(x))

    D_burst_list.append(D_rep)
    k_valid_list.append(k)

    D_rep_samples = D_rep * fs
    print("  -> D_k_rep={:.9e} s ({:.3f} muestras)".format(D_rep, D_rep_samples))

D_burst = np.array(D_burst_list)

if len(D_burst) == 0:
    print("\nNo quedó ninguna ráfaga válida tras el filtrado (min_edges_for_valid_burst = {}).".format(
        min_edges_for_valid_burst))
else:
    D_mean_burst = D_burst.mean()
    D_std_burst = D_burst.std()
    print("\n[RESUMEN POR RÁFAGA - UNA ETIQUETA POR BURST]")
    print("Offset medio D̄_burst   = {:.3e} s ({:.3f} muestras)".format(
        D_mean_burst, D_mean_burst * fs))
    print("Jitter (std) σ_burst    = {:.3e} s ({:.3f} muestras)".format(
        D_std_burst, D_std_burst * fs))
    print("Número de ráfagas válidas (k únicos) =", len(D_burst))

    # ==========================
    # 13) Cálculo de e_k y jitter "puro"
    # ==========================

    print("\n[ERROR POR RÁFAGA RESPECTO AL RETARDO MEDIO]")
    e_list = []
    for k, D_rep in zip(k_valid_list, D_burst):
        e_k = D_rep - D_mean_burst
        e_samples = e_k * fs
        print("k={} -> D_k_rep={:.9e} s ({:.3f} muestras), e_k={:.9e} s ({:.3f} muestras)".format(
            k, D_rep, D_rep * fs, e_k, e_samples))
        e_list.append(e_k)

    e = np.array(e_list)
    e_std = e.std()
    print("\n[RESUMEN JITTER]")
    print("Retardo medio τ_est      = {:.3e} s ({:.3f} muestras)".format(
        D_mean_burst, D_mean_burst * fs))
    print("Jitter (std e_k)         = {:.3e} s ({:.3f} muestras)".format(
        e_std, e_std * fs))

# ==========================
# 12bis) Contabilidad de ráfagas esperadas vs recibidas (con t0_tx)
# ==========================

# Ojo: t_start_seg y t_end_seg en tiempo ABSOLUTO usando fs_eff
t_start_seg = t_rx0 + start_index / fs_eff
t_end_seg   = t_rx0 + (start_index + n_analyze - 1) / fs_eff

# Segmento vs inicio de TX
t_ref_start = max(t_start_seg, t0_tx)

if t_end_seg < t0_tx:
    # Segmento completamente antes del inicio de TX: no debería haber ráfagas
    expected_ks = np.array([], dtype=int)
    k_start_exp = 0
    k_end_exp   = -1
else:
    k_start_exp = int(np.ceil((t_ref_start - t0_tx) / T_period))
    k_start_exp = max(k_start_exp, 0)   # por seguridad
    k_end_exp   = int(np.floor((t_end_seg - t0_tx) / T_period))

    if k_end_exp >= k_start_exp:
        expected_ks = np.arange(k_start_exp, k_end_exp + 1)
    else:
        expected_ks = np.array([], dtype=int)

# k detectados (todos)
received_ks_all = sorted(set(k_list))
expected_ks_set = set(expected_ks)

# Solo los k recibidos dentro del rango esperado
received_ks = sorted(expected_ks_set.intersection(received_ks_all))

expected_count = len(expected_ks)
received_count = len(received_ks)

missing_ks = sorted(expected_ks_set - set(received_ks))

print("\n[CONTABILIDAD DE RÁFAGAS ESPERADAS VS RECIBIDAS]")
print("t_start_seg       = {:.9f} s".format(t_start_seg))
print("t_end_seg         = {:.9f} s".format(t_end_seg))
print("t0_tx (inicio TX) = {:.9f} s".format(t0_tx))
print("k_start_exp       = {}".format(k_start_exp))
print("k_end_exp         = {}".format(k_end_exp))
print("Ráfagas esperadas = {}".format(expected_count))
print("Ráfagas recibidas = {}".format(received_count))
print("Ráfagas perdidas  = {}".format(len(missing_ks)))

# También mostramos qué k caen fuera del rango esperado
extra_ks_before = sorted(k for k in received_ks_all if k < k_start_exp)
extra_ks_after  = sorted(k for k in received_ks_all if k > k_end_exp)
#print("Ráfagas detectadas fuera de la ventana esperada:")
#print("  Antes de k_start_exp (k < {}): {}".format(k_start_exp, extra_ks_before))
#print("  Después de k_end_exp (k > {}): {}".format(k_end_exp, extra_ks_after))

if len(missing_ks) > 0:
    # Función auxiliar para comprimir índices en rangos
    def compress_indices(indices):
        ranges = []
        for k in sorted(indices):
            if not ranges or k > ranges[-1][1] + 1:
                ranges.append([k, k])
            else:
                ranges[-1][1] = k
        return ranges

    ranges = compress_indices(missing_ks)

    #print("Lista de ráfagas perdidas (por índice k):")
    #print("k_miss =", missing_ks)

    #print("Rangos compactados de k perdidos:")
    #for r0, r1 in ranges:
        #if r0 == r1:
            #print("  k = {}".format(r0))
        #else:
            #print("  k = {}..{}".format(r0, r1))

    #print("\nEjemplos de tiempos ideales de ráfagas perdidas (primeros 10):")
    #for k in missing_ks[:10]:
    #    t_tx_ideal = t0_tx + k * T_period
    #    print("  k={} -> t_tx_ideal={:.9f} s".format(k, t_tx_ideal))
else:
    print("No se detectaron ráfagas perdidas en el segmento (todas las k esperadas tienen detección).")

## ==========================
## 14) Gráfica 1: potencia segmentada + detecciones
## ==========================
#
#t_seg = np.arange(n_analyze) / fs_eff
#t_seg_ms = t_seg * 1e3
#t_seg_ms_d, power_seg_d = decimate_for_plot(t_seg_ms, power_seg, max_points_full_plot)
#
#plt.figure()
#plt.plot(t_seg_ms_d, power_seg_d, label="Potencia")
#
#for idx, e in enumerate(edges):
#    t_e_ms = (e / fs_eff) * 1e3
#    if t_e_ms >= t_seg_ms_d[0] and t_e_ms <= t_seg_ms_d[-1]:
#        label = "Detección ráfaga" if idx == 0 else None
#        plt.axvline(t_e_ms, linestyle='--', linewidth=0.8, label=label)
#
#if smooth_for_plot is not None:
#    _, smooth_d = decimate_for_plot(t_seg_ms, smooth_for_plot, max_points_full_plot)
#    plt.plot(t_seg_ms_d, smooth_d, label="Potencia integrada (media móvil)", alpha=0.7)
#
#plt.xlabel("Tiempo en segmento [ms]")
#plt.ylabel("Potencia [u.a.]")
#plt.title("Potencia segmentada con detecciones (modo {}, umbral = {:.3f})".format(
#    detection_mode, power_threshold))
#plt.grid(True)
#plt.legend(loc="best")
#plt.tight_layout()
#
## ==========================
## 15) Gráfica 2: superposición de ráfagas
## ==========================
#
#pre_win_samples = int(round(pre_window_s * fs_eff))
#post_win_samples = int(round(post_window_s * fs_eff))
#win_len = pre_win_samples + post_window_s
#
#segments = []
#for e in edges:
#    s = e - pre_win_samples
#    en = e + post_win_samples
#    if s < 0 or en > n_analyze:
#        continue
#    seg = power_seg[s:en]
#    if len(seg) == win_len:
#        segments.append(seg)
#
#if len(segments) > 0:
#    segments = np.array(segments)
#    n_plot = min(max_bursts_for_overlay, segments.shape[0])
#
#    step_overlay = max(1, win_len // max_points_overlay)
#    t_rel = (np.arange(0, win_len, step_overlay) - pre_win_samples) / fs_eff * 1e3
#
#    plt.figure()
#    for i in range(n_plot):
#        seg_d = segments[i, ::step_overlay]
#        plt.plot(t_rel, seg_d, alpha=0.5)
#    plt.axvline(0.0, linestyle='--', linewidth=1.0, label="Punto de detección")
#
#    plt.xlabel("Tiempo relativo a la detección [ms]")
#    plt.ylabel("Potencia [u.a.]")
#    plt.title("Superposición de ráfagas alineadas ({} mostradas, modo {})".format(
#        n_plot, detection_mode))
#    plt.grid(True)
#    plt.legend(loc="best")
#    plt.tight_layout()
#else:
#   print("No se pudieron construir ventanas para superposición; ajusta pre/post_window_s.")

# ==========================
# 16) Mostrar figuras
# ==========================

plt.show()
