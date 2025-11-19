#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Analizador de sincronismo TX/RX con modos de detección configurables.

detection_mode:
  0 = Detección simple por umbral (rising edges).
  1 = Detección por energía integrada (media móvil) + umbral.
  2 = Primer edge > threshold después de un silencio largo (>= gap_min_s).

Además:
  - Selección de la cola del archivo (últimas N muestras o suficientes para M ráfagas).
  - Cálculo de D_k y agrupación por ráfaga k (una muestra representativa por burst).
  - Gráficas con trazas diezmadAS para no matar el PC.
"""

import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
import os

# ==========================
# 1) Parámetros físicos
# ==========================

fs = 30.72e6            # sample rate [Hz]
t_rx0 =  0.195769    # tiempo de referencia RX (rx_time del sample 0) [s]
t0_tx = 0.6             # tiempo ideal de la primera ráfaga TX [s]
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
# Ventana de integración en segundos.
# Cuanto mayor sea, más filtras ruido pero más "desdibujas" el borde.
# Para 30.72 MHz, 0.1 ms = 3072 muestras, 0.2 ms = 6144 muestras.
integration_window_s = 0.0002      # 0.2 ms, buen compromiso para coseno ON/OFF
# El umbral se aplicará sobre la potencia integrada (media móvil).

# --- Modo 0 y 1: separación mínima entre detecciones ---
# Esto evita varios edges muy juntos dentro de la misma ráfaga.
min_separation_s = 0.003          # 3 ms >> variaciones rápidas, << 15 ms del periodo
# Con 10 ms ON / 5 ms OFF, 3 ms asegura normalmente 1 detección por ráfaga.
# Ajustable según lo que veas en los plots.

# --- Modo 2: primer edge después de silencio largo ---
gap_min_s = 0.005                 # silencio mínimo antes del burst (ej. 5 ms)
# Como tu patrón es 10 ms ON + 5 ms OFF, un gap de 5 ms encaja bien:
# ignoras cualquier basura interna y sólo aceptas el primer edge
# después de un silencio continuo >= 5 ms.

# ==========================
# 3) Configuración de análisis
# ==========================

# Muestras a analizar:
#   - Si manual_last_samples != None -> siempre las últimas N muestras.
#   - Si es None -> se calculan según target_num_bursts.
manual_last_samples = None        # p.ej. 100000 para "últimas 100k muestras"
target_num_bursts = 200            # ráfagas objetivo a cubrir

max_samples_cap = 20_000_000      # límite duro de muestras a analizar

# Ventanas para superposición de ráfagas (para plots)
pre_window_s = 1e-3               # tiempo antes del flanco de detección [s]
post_window_s = 12e-3             # tiempo después del flanco [s]
max_bursts_for_overlay = 20       # número máx de ráfagas a dibujar

base_dir = os.path.dirname(os.path.abspath(__file__))
power_filename = os.path.join(base_dir, 'power_rx.dat')

#power_filename = "power_rx.dat"

# ==========================
# 4) Parámetros gráficos (diezmado para no morir)
# ==========================

# Queremos como máximo ~100k puntos en cada gráfica grande
max_points_full_plot = 100_000
# Y unas ~2k muestras por ráfaga en la overlay
max_points_overlay = 2000

# ==========================
# 5) Funciones auxiliares
# ==========================

def decimate_for_plot(x, y, max_points):
    """
    Devuelve versiones diezmadAS de x, y para graficar rápido.
    No afecta a los cálculos del detector; sólo a las figuras.

    Si len(y) <= max_points -> no hace nada.
    Si no, toma un punto cada 'step'.
    """
    n = len(y)
    if n <= max_points:
        return x, y
    step = max(1, n // max_points)
    return x[::step], y[::step]


def detect_edges_threshold(sig, threshold, fs, min_separation_s=None):
    """
    Detección simple por umbral:
      - Encuentra flancos de subida (debajo -> encima del umbral).
      - Opcionalmente impone una separación mínima entre detecciones.

    Devuelve: array de índices de detección.
    """
    above = sig > threshold
    raw_edges = np.where(np.logical_and(above[1:], ~above[:-1]))[0] + 1

    if min_separation_s is None or len(raw_edges) == 0:
        return raw_edges

    min_sep_samples = int(round(min_separation_s * fs))
    filtered = []
    last_e = -1e12
    for e in raw_edges:
        if e - last_e >= min_sep_samples:
            filtered.append(e)
            last_e = e
    return np.array(filtered, dtype=int)


def detect_edges_energy(sig, fs, threshold, win_s, min_separation_s=None):
    """
    Detección por energía integrada (media móvil) + umbral.
    1) Se hace media móvil en ventana de win_s segundos.
    2) Se busca flancos de subida de esa señal filtrada.
    """
    win_samples = int(round(win_s * fs))
    if win_samples < 1:
        raise ValueError("integration_window_s demasiado pequeño; win_samples < 1")

    # Filtro caja: convolución con vector de unos / win_samples
    kernel = np.ones(win_samples, dtype=np.float32) / float(win_samples)
    smooth = np.convolve(sig, kernel, mode='same')

    edges = detect_edges_threshold(smooth, threshold, fs, min_separation_s)
    return edges, smooth


def detect_edges_after_silence(sig, fs, threshold, gap_min_s):
    """
    Primer edge > threshold después de un silencio largo (>= gap_min_s).

    Implementación:
      - Recorremos la señal.
      - Contamos cuántas muestras consecutivas ha estado por debajo del umbral.
      - Cuando vemos flanco de subida y el contador de silencio >= gap_min_samples,
        registramos ese índice como detección.
    """
    gap_min_samples = int(round(gap_min_s * fs))

    edges = []
    silence_count = 0

    for i in range(1, len(sig)):
        # Actualizamos contador de silencio
        if sig[i-1] <= threshold:
            silence_count += 1
        else:
            silence_count = 0

        # Flanco de subida
        if sig[i-1] <= threshold and sig[i] > threshold:
            if silence_count >= gap_min_samples:
                edges.append(i)
                # Después de detectar, reseteamos el silencio
                # para no disparar varios edges dentro del mismo ON
                silence_count = 0

    return np.array(edges, dtype=int)

# ==========================
# 6) Carga de datos
# ==========================

power = np.fromfile(power_filename, dtype=np.float32)
total_samples = len(power)
print("Leí", total_samples, "muestras de potencia de", power_filename)

if total_samples == 0:
    raise RuntimeError("El archivo de potencia está vacío. Nada que analizar.")

# ==========================
# 7) Selección de la cola del archivo
# ==========================

samples_per_period = int(round(T_period * fs))

if manual_last_samples is not None:
    n_analyze = min(total_samples, int(manual_last_samples))
    reason = "manual_last_samples"
else:
    n_needed = target_num_bursts * samples_per_period
    n_analyze = min(total_samples, n_needed, max_samples_cap)
    reason = "target_num_bursts"

if n_analyze <= 0:
    raise RuntimeError("n_analyze <= 0, revisa configuración.")

start_index = total_samples - n_analyze
power_seg = power[start_index:]

print("Analizando", n_analyze, "muestras (modo:", reason, ", start_index =", start_index, ")")

# ==========================
# 8) Detección según detection_mode
# ==========================

if detection_mode == 0:
    print("Modo de detección 0: simple por umbral.")
    edges = detect_edges_threshold(
        sig=power_seg,
        threshold=power_threshold,
        fs=fs,
        min_separation_s=min_separation_s
    )
    smooth_for_plot = None  # no hay señal suavizada

elif detection_mode == 1:
    print("Modo de detección 1: energía integrada (media móvil) + umbral.")
    edges, smooth_for_plot = detect_edges_energy(
        sig=power_seg,
        fs=fs,
        threshold=power_threshold,
        win_s=integration_window_s,
        min_separation_s=min_separation_s
    )

elif detection_mode == 2:
    print("Modo de detección 2: primer edge > threshold tras silencio >= {:.3f} ms".format(gap_min_s * 1e3))
    edges = detect_edges_after_silence(
        sig=power_seg,
        fs=fs,
        threshold=power_threshold,
        gap_min_s=gap_min_s
    )
    smooth_for_plot = None

else:
    raise ValueError("detection_mode inválido. Usa 0, 1 o 2.")

print("Detectadas", len(edges), "ráfagas (edges) en el segmento analizado")

if len(edges) == 0:
    print("No se detectaron ráfagas. Ajusta umbral o parámetros del detector.")
    # Dibujamos sólo potencia para inspección
    t_seg = np.arange(n_analyze) / fs
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
# 9) Cálculo de tiempos RX por edge
# ==========================

t_bursts_rx = t_rx0 + (start_index + edges) / fs

# ==========================
# 10) Cálculo de D_k por tag y agrupación por ráfaga k
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

    print("tag i={} k={} t_rx={:.9f} t_tx_ideal={:.9f} D_k={:.9e}".format(
        i, k, t_rx, t_tx_ideal, D_k))

D = np.array(D_list)

# Estadísticas globales con todas las etiquetas
if len(D) > 0:
    D_mean_all = D.mean()
    D_std_all = D.std()
    print("\n[GLOBAL - TODAS LAS ETIQUETAS]")
    print("Offset medio D̄_all     = {:.3e} s".format(D_mean_all))
    print("Jitter (std) σ_all      = {:.3e} s".format(D_std_all))

# Agrupar por k
by_k = defaultdict(list)
for k, D_k in zip(k_list, D_list):
    by_k[k].append(D_k)

D_burst_list = []

print("\n[POR RÁFAGA - UNA MUESTRA REPRESENTATIVA POR k]")
for k in sorted(by_k.keys()):
    values = by_k[k]

    # ESTRATEGIA DE SELECCIÓN:
    # Aquí seguimos usando "el D_k más cercano a cero" para robustez.
    # Si quisieras forzar "el primero" podrías usar: D_rep = values[0]
    D_rep = min(values, key=lambda x: abs(x))
    D_burst_list.append(D_rep)

    print("k={} -> num_tags={}  D_k_rep={:.9e} s".format(
        k, len(values), D_rep))

D_burst = np.array(D_burst_list)

if len(D_burst) > 0:
    D_mean_burst = D_burst.mean()
    D_std_burst = D_burst.std()
    print("\n[RESUMEN POR RÁFAGA - UNA ETIQUETA POR BURST]")
    print("Offset medio D̄_burst   = {:.3e} s".format(D_mean_burst))
    print("Jitter (std) σ_burst    = {:.3e} s".format(D_std_burst))
    print("Número de ráfagas (k únicos) =", len(D_burst))

# ==========================
# 11) Gráfica 1: potencia segmentada + detecciones
# ==========================

t_seg = np.arange(n_analyze) / fs
t_seg_ms = t_seg * 1e3
t_seg_ms_d, power_seg_d = decimate_for_plot(t_seg_ms, power_seg, max_points_full_plot)

plt.figure()
plt.plot(t_seg_ms_d, power_seg_d, label="Potencia")

# Marcar edges (sin diezmar, pero son pocos)
for idx, e in enumerate(edges):
    t_e_ms = (e / fs) * 1e3
    # Sólo lo pintamos si cae dentro del rango diezmado
    if t_e_ms >= t_seg_ms_d[0] and t_e_ms <= t_seg_ms_d[-1]:
        label = "Detección ráfaga" if idx == 0 else None
        plt.axvline(t_e_ms, linestyle='--', linewidth=0.8, label=label)

# Si en modo 1 tenemos señal suavizada, opcionalmente la ploteamos (diezmada)
if smooth_for_plot is not None:
    _, smooth_d = decimate_for_plot(t_seg_ms, smooth_for_plot, max_points_full_plot)
    plt.plot(t_seg_ms_d, smooth_d, label="Potencia integrada (media móvil)", alpha=0.7)

plt.xlabel("Tiempo en segmento [ms]")
plt.ylabel("Potencia [u.a.]")
plt.title("Potencia segmentada con detecciones (modo {}, umbral = {:.3f})".format(
    detection_mode, power_threshold))
plt.grid(True)
plt.legend(loc="best")
plt.tight_layout()

# ==========================
# 12) Gráfica 2: superposición de ráfagas
# ==========================

pre_win_samples = int(round(pre_window_s * fs))
post_win_samples = int(round(post_window_s * fs))
win_len = pre_win_samples + post_win_samples

segments = []
for e in edges:
    s = e - pre_win_samples
    en = e + post_win_samples
    if s < 0 or en > n_analyze:
        continue
    seg = power_seg[s:en]
    if len(seg) == win_len:
        segments.append(seg)

if len(segments) > 0:
    segments = np.array(segments)
    n_plot = min(max_bursts_for_overlay, segments.shape[0])

    # Diezmamos en eje de muestras para no dibujar 400k puntos por ráfaga
    step_overlay = max(1, win_len // max_points_overlay)
    t_rel = (np.arange(0, win_len, step_overlay) - pre_win_samples) / fs * 1e3

    plt.figure()
    for i in range(n_plot):
        seg_d = segments[i, ::step_overlay]
        plt.plot(t_rel, seg_d, alpha=0.5)
    plt.axvline(0.0, linestyle='--', linewidth=1.0, label="Punto de detección")

    plt.xlabel("Tiempo relativo a la detección [ms]")
    plt.ylabel("Potencia [u.a.]")
    plt.title("Superposición de ráfagas alineadas ({} mostradas, modo {})".format(
        n_plot, detection_mode))
    plt.grid(True)
    plt.legend(loc="best")
    plt.tight_layout()
else:
    print("No se pudieron construir ventanas para superposición; ajusta pre/post_window_s.")

# ==========================
# 13) Mostrar figuras
# ==========================

# plt.show() es BLOQUEANTE: el script no termina hasta que cierres las ventanas.
# Eso es normal. Si quisieras que no bloquee, puedes usar plt.show(block=False),
# pero entonces el script podría terminar antes de que veas los gráficos.
plt.show()
