import numpy as np

# Parámetros que tú sabes:
fs = 30.72e6            # sample rate
t_rx0 = 0.0891389       # de Tag Debug: rx_time {0 0.0451556}
t0_tx = 3          # tiempo ideal de la primera ráfaga en TX (ejemplo)
T_period = 0.015        # periodo entre ráfagas (10 ms on + 5 ms off, ej.)
power_threshold = 0.01  # umbral de potencia para detectar ráfagas

# Carga de datos
power = np.fromfile("power_rx.dat", dtype=np.float32)
print("Leí", len(power), "muestras de potencia")

# Detección de ráfagas: rising edges
above = power > power_threshold
edges = np.where(np.logical_and(above[1:], ~above[:-1]))[0] + 1  # índices de subidas

print("Detectadas", len(edges), "posibles ráfagas")

# Calcula tiempo real de cada ráfaga en RX
t_bursts_rx = t_rx0 + edges / fs

# Para cada ráfaga, calcula índice k y error D_k
D_list = []
for i, t_rx in enumerate(t_bursts_rx):
    # índice de slot k (redondeo al más cercano)
    k_real = (t_rx - t0_tx) / T_period
    k = int(np.round(k_real))
    t_tx_ideal = t0_tx + k * T_period
    D_k = t_rx - t_tx_ideal
    D_list.append(D_k)
    print("burst i={} k={} t_rx={:.9f} t_tx_ideal={:.9f} D_k={:.9e}".format(
        i, k, t_rx, t_tx_ideal, D_k))

D = np.array(D_list)
if len(D) > 0:
    print("Offset medio D̄ = {:.3e} s".format(D.mean()))
    print("Jitter (desv. estándar) = {:.3e} s".format(D.std()))

