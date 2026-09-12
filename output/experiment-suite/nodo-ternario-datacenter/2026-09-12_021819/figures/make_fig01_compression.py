"""Fig1: Comparación de ratios de compresión — MEDIDO."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({"font.size": 10, "axes.grid": True, "grid.alpha": 0.3,
                     "svg.fonttype": "none"})

# Datos de results.json
methods = ["Binario\n(raw)", "gzip", "LZ4", "Ternario\n(raw)", "Ternario\n residual", "Ternario\n+ Quipu", "Ternario\n+ ZSTD"]
ratios = [1.0, 0.133, 1.8, 8.0, 1.996, 1.108, 2.196]
colors = ["#999999", "#ff7f0e", "#2ca02c", "#d62728", "#1f77b4", "#9467bd", "#8c564b"]

fig, ax = plt.subplots(figsize=(8, 4.5))
x = np.arange(len(methods))
bars = ax.bar(x, ratios, color=colors, edgecolor="black", linewidth=0.6)
ax.set_xticks(x, methods)
ax.set_ylabel("Ratio de compresión (bits originales / bits comprimidos)")
ax.set_title("(a) Compresión: ternario raw supera a todos los métodos (medido)")
ax.axhline(1.0, color="black", linewidth=0.8, linestyle="--", alpha=0.5)

for i, v in enumerate(ratios):
    ax.text(i, v + 0.1, f"{v:.2f}x", ha="center", fontsize=9, fontweight="bold")

fig.tight_layout()
fig.savefig("fig01_compression_ratio.pdf")
fig.savefig("fig01_compression_ratio.svg")
print("fig01 OK", ratios)
