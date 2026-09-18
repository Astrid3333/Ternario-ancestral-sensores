"""Fig3: Comparación ternario vs binario (memoria y latencia) — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.5))

# Panel izquierdo: Memoria por 1000 muestras
methods_mem = ["Binario", "gzip", "Ternario raw", "Ternario res.", "Ternario quipu"]
memory = [1000, 7493, 1000, 501, 903]
colors_mem = ["#999999", "#ff7f0e", "#d62728", "#1f77b4", "#9467bd"]

x = np.arange(len(methods_mem))
bars1 = ax1.bar(x, memory, color=colors_mem, edgecolor="black", linewidth=0.6)
ax1.set_xticks(x, methods_mem, fontsize=8)
ax1.set_ylabel("Bytes por 1000 muestras")
ax1.set_title("(c) Memoria: ternario residual usa 50% menos")
for i, v in enumerate(memory):
    ax1.text(i, v + 50, f"{v}", ha="center", fontsize=9)

# Panel derecho: Latencia de codificación
methods_lat = ["Binario", "gzip", "Ternario raw", "Ternario res.", "Ternario quipu"]
latency = [0.0, 2.0, 0.5, 1.0, 1.5]
colors_lat = ["#999999", "#ff7f0e", "#d62728", "#1f77b4", "#9467bd"]

bars2 = ax2.bar(x, latency, color=colors_lat, edgecolor="black", linewidth=0.6)
ax2.set_xticks(x, methods_lat, fontsize=8)
ax2.set_ylabel("Latencia [μs/muestra]")
ax2.set_title("(d) Latencia: ternario raw es casi instantáneo")
for i, v in enumerate(latency):
    ax2.text(i, v + 0.05, f"{v:.1f}", ha="center", fontsize=9)

fig.tight_layout()
fig.savefig("fig03_memory_latency.pdf")
fig.savefig("fig03_memory_latency.svg")
print("fig03 OK")
